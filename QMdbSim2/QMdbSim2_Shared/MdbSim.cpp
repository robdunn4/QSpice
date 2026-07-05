//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
#include "MdbSim.h"
#include <iostream>

#ifdef _DEBUG
#define DBG_TXT " (DEBUG)"
#else
#define DBG_TXT ""
#endif
static const char *VersionInfo = "QMdbSim v0.7.0" DBG_TXT;

// dbg_ holds a reference to jvmHost_, so jvmHost_ must be constructed first.
// Declaration order in MdbSim.h guarantees this.
MdbSim::MdbSim() : dbg_(jvmHost_) {}

MdbSim::~MdbSim() {
  if (simState == Running || simState == Initialised) stopSim();
}

const char *MdbSim::getVerInfo() { return VersionInfo; }

void MdbSim::configure(const Config &cfg) { jvmConfig_ = cfg; }

// ── Individual initialisation steps ──────────────────────────────────────────

bool MdbSim::initSim(const char *deviceName) {
  if (simState != NotStarted) {
    if (simState != ErrState) setError("initSim(invalid state)");
    return false;
  }

  // Start the JVM with classpath and system properties
  JvmHost::Config hc;
  hc.jvmDllPath    = jvmConfig_.jvmDllPath;
  hc.mdbcsJar      = jvmConfig_.mdbcsJar;
  hc.mplabxRoot    = jvmConfig_.mplabxRoot;
  hc.packsFolder   = jvmConfig_.packsFolder;
  hc.thirdpartyLib = jvmConfig_.thirdpartyLib;

  if (!jvmHost_.start(hc)) {
    setError("initSim(JVM failed to start)");
    return false;
  }

  // Construct the MDBCS Debugger object
  if (!dbg_.construct(deviceName, jvmConfig_.tool, true)) {
    setError("initSim(Debugger construct failed)");
    return false;
  }

  // Retrieve the QMdbCS JAR version for runtime diagnostics.
  qmdbcsVersion_ = dbg_.getQMdbCSVersion();

  // Suppress "Stepping" and other simulator display noise.
  // Non-fatal if unsupported -- just warn.
  if (!dbg_.setConciseMode(true))
    std::cerr << "[WARN] initSim: setConciseMode failed (non-fatal)\n";

  simState = Initialised;
  return true;
}

bool MdbSim::setVDD(const char *pinName, double vdd) {
  if (simState == Running || simState == Stopped) {
    std::cerr << "[FAIL] MdbSim::setVDD: cannot be called after connectSim()"
                 " -- VDD is latched once immediately after program()\n";
    return false;
  }
  vddPinName_ = pinName ? pinName : "";
  vddV        = vdd;
  return true;
}

bool MdbSim::loadProgram(const char *pgmPath) {
  if (simState != Initialised) {
    if (simState != ErrState)
      setError("loadProgram(invalid state -- call initSim first)");
    return false;
  }

  if (!dbg_.loadFile(pgmPath)) {
    setError("loadProgram(loadFile failed)");
    return false;
  }

  return true;
}

bool MdbSim::connectSim() {
  if (simState != Initialised) {
    if (simState != ErrState)
      setError("connectSim(invalid state -- call loadProgram first)");
    return false;
  }

  if (vddPinName_.empty()) {
    setError("connectSim(setVDD() was not called -- vddPinName is required)");
    return false;
  }

  // Best-effort JNI setVDD -- may be a no-op on the simulator.
  if (!dbg_.setVDD(vddV)) {
    setError("connectSim(setVDD failed)");
    return false;
  }
  if (!dbg_.connect()) {
    setError("connectSim(connect failed)");
    return false;
  }
  if (!dbg_.program()) {
    setError("connectSim(program failed)");
    return false;
  }

  // Fetch the VDD pin and drive it immediately after program() --
  // this is the call that reliably sets the supply voltage on the simulator.
  pinVdd_ = dbg_.getPin(vddPinName_.c_str());
  if (!pinVdd_) {
    setError("connectSim(could not get VDD pin -- check vddPinName)");
    return false;
  }
  if (!dbg_.pinExternalSetVoltage(pinVdd_, vddV)) {
    setError("connectSim(pinExternalSetVoltage on VDD failed)");
    return false;
  }

  simState = Running;

  // Fetch and cache Pin objects for all pins registered via addPinPortMap().
  if (!fetchAndCachePins()) return false;

  return true;
}

bool MdbSim::reset() {
  if (!dbg_.reset()) return false;

  // Re-drive VDD immediately after reset -- same requirement as post-program().
  if (pinVdd_ && !dbg_.pinExternalSetVoltage(pinVdd_, vddV)) {
    setError("reset(pinExternalSetVoltage on VDD failed)");
    return false;
  }
  return true;
}

bool MdbSim::startSim(const char *deviceName, const char *pgmPath) {
  // setVDD() must have been called before startSim().
  return initSim(deviceName) && loadProgram(pgmPath) && connectSim();
}

// ── Simulator commands
// ────────────────────────────────────────────────────────

bool MdbSim::stopSim() {
  if (simState == ErrState) return false;
  if (simState == NotStarted) {
    setError("stopSim(not started)");
    return false;
  }

  releasePins();

  if (simState == Running) {
    dbg_.disconnect();
    dbg_.destroy();
  }

  simState = Stopped;
  return true;
}

// ── Single-step
// ───────────────────────────────────────────────────────────────

bool MdbSim::stepInst() {
  if (simState == ErrState) return false;
  if (simState != Running) {
    setError("stepInst(not running)");
    return false;
  }

  // stepInstr() returns the new PC on success, -1 on failure.
  if (dbg_.stepInstr() < 0) {
    setError("stepInst(stepInstr failed)");
    return false;
  }
  return true;
}

// ── Pin state ────────────────────────────────────────────────────────────────

bool MdbSim::getPinState(const char *pinName, PinState &pinState) {
  if (simState == ErrState) return false;
  if (simState != Running) {
    setError("getPinState(not running)");
    return false;
  }

  jobject pin = findCachedPin(pinName);
  if (!pin) {
    setError(
        "getPinState(pin not found -- was it registered with addPinPortMap?)");
    return false;
  }

  // Single JNI call to QMdbCS.getPinState(Pin) -> packed long.
  // MdbcsDebugger unpacks voltage, ioState, and daState into pinState directly.
  if (!dbg_.getPinState(pin, pinState)) {
    setError("getPinState(QMdbCS.getPinState failed)");
    return false;
  }

  return true;
}

bool MdbSim::setPin(const char *pinName, double toVoltage) {
  if (simState == ErrState) return false;
  if (simState != Running) {
    setError("setPin(not running)");
    return false;
  }

  jobject pin = findCachedPin(pinName);
  if (!pin) {
    setError("setPin(pin not found -- was it registered with addPinPortMap?)");
    return false;
  }

  // Clamp to valid voltage range to absorb any QSpice input spikes.
  if (toVoltage > vddV) toVoltage = vddV;
  if (toVoltage < 0.0) toVoltage = 0.0;

  // externalSetVoltage() models an external agent driving the pin (e.g.
  // QSpice).
  if (!dbg_.pinExternalSetVoltage(pin, toVoltage)) {
    setError("setPin(externalSetVoltage failed)");
    return false;
  }

  return true;
}

// ── Bulk port update helpers
// ──────────────────────────────────────────────────

void MdbSim::addPinPortMap(const char *const pinName, double *const inPort,
                           double *const outPort, bool *const dirPort) {
  ppmList_.push_back(PinPortMap(pinName, inPort, outPort, dirPort));
  pinObjs_.push_back(nullptr); // placeholder; filled in fetchAndCachePins()
}

bool MdbSim::getPinStates() {
  for (PinPortMap &ppMap : ppmList_)
    if (!getPinState(ppMap.pinName, ppMap.pinState)) return false;
  return true;
}

void MdbSim::setCtrlPorts() {
  // QSpice tri-state control port: true = hi-Z (input), false = actively
  // driving (output).
  for (PinPortMap &ppMap : ppmList_)
    if (ppMap.dirPort) *ppMap.dirPort = ppMap.pinState.isInput();
}

bool MdbSim::setInPins() {
  for (PinPortMap &ppMap : ppmList_) {
    if (!ppMap.pinState.isInput() || !ppMap.inPort) continue;
    double v = *ppMap.inPort;
    if (v == ppMap.lastInVoltage) continue; // no change -- skip JNI call
    if (!setPin(ppMap.pinName, v)) return false;
    ppMap.lastInVoltage = v;
  }
  return true;
}

void MdbSim::setOutPorts() {
  for (PinPortMap &ppMap : ppmList_)
    if (ppMap.outPort) *ppMap.outPort = ppMap.pinState.voltage;
}
// ── Internal helpers
// ──────────────────────────────────────────────────────────

void MdbSim::setError(const char *msg) {
  if (simState == ErrState) return;
  simState   = ErrState;
  lastErrMsg = msg;
}

jobject MdbSim::findCachedPin(const char *pinName) {
  for (size_t i = 0; i < ppmList_.size(); ++i)
    if (strcmp(ppmList_[i].pinName, pinName) == 0) return pinObjs_[i];
  return nullptr;
}

bool MdbSim::fetchAndCachePins() {
  pinVdd_ = dbg_.getPin("VDD");
  if (!pinVdd_) std::cerr << "[WARN] MdbSim: could not get VDD pin\n";

  for (size_t i = 0; i < ppmList_.size(); ++i) {
    jobject pin = dbg_.getPin(ppmList_[i].pinName);
    if (!pin) {
      setError("fetchAndCachePins(getPin failed -- check pin name)");
      return false;
    }
    pinObjs_[i] = pin;
  }
  return true;
}

void MdbSim::releasePins() {
  if (pinVdd_) {
    dbg_.releasePin(pinVdd_);
    pinVdd_ = nullptr;
  }
  for (jobject pin : pinObjs_) dbg_.releasePin(pin);
  pinObjs_.assign(pinObjs_.size(), nullptr);
}
