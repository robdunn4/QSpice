//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// MdbSim.h -- MDBCS JNI simulator interface for QSpice C-Block components.
//
// Replaces QMdbSim.h/cpp (stdio-pipe + MDB.bat approach) with direct JNI
// calls to the MDBCS Java library.  The public API is intentionally compatible
// with QMdbSim so that existing QSpice C-Block code requires minimal changes.
//
// Key differences from QMdbSim:
//   - No child process, no pipes, no text parsing.
//   - configure() must be called before initSim() to supply JVM paths.
//   - Initialisation is split into explicit steps so the caller controls the
//     sequence:
//       1. initSim(deviceName)      -- start JVM, construct Debugger
//       2. setVDD(vdd)              -- call Debugger.setVDD(); before
//                                      connectSim()
//       3. loadProgram(pgmPath)     -- load ELF
//       4. connectSim()             -- connect, program, cache pins
//     startSim() is a convenience wrapper for all four steps.
//   - Two separate VDD methods are exposed:
//       setVDD(vdd)             -- calls Debugger.setVDD(); intended before
//                                  connectSim()
//       pinSetVDD(pinName, vdd) -- calls pinExternalSetVoltage()
//                                  on a named pin; pin name is device-specific
//                                  (e.g. "VDD")
//     Neither enforces state; both warn and proceed if state is unexpected.
//   - Pin objects are fetched once in connectSim() and cached for the lifetime
//     of the session.
//------------------------------------------------------------------------------
#pragma once
#include "JvmHost.h"
#include "MdbcsDebugger.h"
#include <jni.h>
#include <string>
#include <vector>

#include "PinState.h"

// maps a pin name to QSpice tri-state (or input-only) ports
class PinPortMap {
public:
  PinPortMap(const char *const pinName, double *const inPort,
             double *const outPort = nullptr, bool *const dirPort = nullptr)
      : pinName(pinName), inPort(inPort), outPort(outPort), dirPort(dirPort) {}

  const char *const pinName;
  double *const     inPort;  // points into QSpice data[]; written by QSpice
  double           *outPort; // points into QSpice data[]; read by QSpice
  bool             *dirPort; // points into QSpice data[]; read by QSpice
  PinState          pinState;
  // Last voltage written to the simulator via pinExternalSetVoltage().
  // Initialized to -1.0 as a sentinel so the first step always writes.
  // setInPins() skips the JNI call when *inPort matches this value.
  double lastInVoltage = -1.0;
};

typedef std::vector<PinPortMap> PinPortMapList;

enum SimState { NotStarted, Initialised, Running, Stopped, ErrState };

class MdbSim {
public:
  // JVM and MDBCS path configuration.  Must be filled and passed to
  // configure() before calling initSim().
  struct Config {
    std::string jvmDllPath;    // full path to jvm.dll
    std::string mdbcsJar;      // full path to qmdbcs.jar (fat JAR)
    std::string mplabxRoot;    // MPLAB X install root
    std::string packsFolder;   // -Dpackslib.packsfolder
    std::string thirdpartyLib; // -Dmplabx.thirdparty.lib.path
    std::string tool = "sim";  // simulator tool name

    // Supply voltage and VDD pin name are set via MdbSim::setVDD(), not here.
    // connectSim() will fail with a clear error if setVDD() has not been
    // called.
  };

  MdbSim();
  ~MdbSim();
  MdbSim(const MdbSim &)            = delete;
  MdbSim &operator=(const MdbSim &) = delete;

  // Must be called before initSim().
  void configure(const Config &cfg);

  SimState    getSimState() const { return simState; }
  bool        getErrState() const { return simState == ErrState; }
  const char *getLastErrMsg() const { return lastErrMsg.c_str(); }
  const char *getVerInfo();
  // Returns the QMdbCS JAR version string retrieved during initSim().
  // Empty string if initSim() has not yet succeeded.
  const std::string &getQMdbCSVersion() const { return qmdbcsVersion_; }

  // ── Individual initialisation steps ───────────────────────────────────
  // Call in order: initSim -> setVDD -> loadProgram -> connectSim.
  // setVDD() MUST be called after initSim() and BEFORE connectSim() for it
  // to have any effect on the simulator.

  // Step 1: start JVM and construct the MDBCS Debugger object.
  // State: NotStarted -> Initialised.
  bool initSim(const char *deviceName);

  // Step 2: set the supply voltage and the device-specific VDD pin name
  // (e.g. "VDD" for PIC16F15213).  May be called any time before connectSim();
  // returns false with a diagnostic if called after connectSim() since VDD is
  // latched exactly once (immediately after program()).
  // Updates vddV used for input-pin clamping in setPin().
  bool setVDD(const char *pinName, double vdd);

  // Step 3: load the ELF/HEX program file.
  bool loadProgram(const char *pgmPath);

  // Step 4: connect to simulator, program device, cache pin objects.
  // State: Initialised -> Running.
  bool connectSim();

  // ── Convenience wrapper ────────────────────────────────────────────────
  // Calls initSim -> loadProgram -> connectSim.
  // setVDD() must have been called before startSim().
  bool startSim(const char *deviceName, const char *pgmPath);

  // ── Simulator commands (mirrors QMdbSim public API) ───────────────────
  bool stopSim();
  bool reset(); // resets device; re-drives VDD immediately after
  bool stepInst();
  bool getPinState(const char *pinName, PinState &pinState);
  bool setPin(const char *pinName, double toVoltage);

  // ── Bulk port update helpers (mirrors QMdbSim public API) ─────────────
  void addPinPortMap(const char *const pinName, double *const inPort,
                     double *const outPort = nullptr,
                     bool *const   dirPort = nullptr);
  bool getPinStates();
  void setCtrlPorts();
  bool setInPins();
  void setOutPorts();

protected:
  SimState    simState = NotStarted;
  double      vddV = 5.0;  // updated by setVDD(); used for clamping in setPin()
  std::string vddPinName_; // device-specific VDD pin name, set by setVDD()
  std::string lastErrMsg = "No errors";
  std::string qmdbcsVersion_; // retrieved from QMdbCS.getVersion() in initSim()

  // JVM configuration set by configure()
  Config jvmConfig_;

  // JVM host and debugger -- jvmHost_ MUST be declared before dbg_ so that
  // dbg_(jvmHost_) is initialized with a fully constructed JvmHost.
  JvmHost       jvmHost_;
  MdbcsDebugger dbg_;

  // Pin port maps and their corresponding cached Java Pin global refs.
  // Indices are kept in sync: pinObjs_[i] is the Pin for ppmList_[i].
  // Refs are nullptr until fetchAndCachePins() is called in connectSim().
  PinPortMapList       ppmList_;
  std::vector<jobject> pinObjs_;

  // Cached VDD pin global ref, fetched in fetchAndCachePins().
  jobject pinVdd_ = nullptr;

  void    setError(const char *msg);
  jobject findCachedPin(const char *pinName);
  bool    fetchAndCachePins();
  void    releasePins();
};
