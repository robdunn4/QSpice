//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
#pragma once
#include "JvmHost.h"
#include "PinState.h"
#include <functional>
#include <jni.h>
#include <string>

class MdbcsDebugger {
public:
  explicit MdbcsDebugger(JvmHost &host);
  ~MdbcsDebugger();
  MdbcsDebugger(const MdbcsDebugger &)            = delete;
  MdbcsDebugger &operator=(const MdbcsDebugger &) = delete;

  // ── QMdbCS API ───────────────────────────────────────────────────────────
  // Maps 1:1 to com.microchip.mdbcs.QMdbCS.  All method IDs are cached
  // during construct() using signatures verified with javap -s.

  // ── QMdbCS class/object construction ─────────────────────────────────────
  // Split into two steps so callers can install stdout/stderr capture in
  // between: resolveClass() finds the QMdbCS class and caches all method
  // IDs but does NOT create the Java-side object; instantiate() then calls
  // the Java constructor.  This matters because MDBCore emits a substantial
  // amount of its own startup chatter (pack-loading diagnostics,
  // "Peripheral Missing" warnings, etc.) directly from within the Java
  // Debugger constructor -- i.e. triggered by instantiate(), not by any
  // later call -- so capture must be installed before instantiate() runs,
  // not merely before getQMdbCSVersion()/setConciseMode().
  //
  // construct() is a convenience wrapper (resolveClass() + instantiate())
  // for callers that don't need capture installed before construction.

  bool resolveClass();
  bool instantiate(const std::string &device, const std::string &tool,
                   bool asDebugger);
  bool construct(const std::string &device, const std::string &tool,
                bool asDebugger);
  // Returns the QMdbCS JAR version string (calls QMdbCS.getVersion()).
  // Returns an empty string on failure.
  std::string getQMdbCSVersion();
  bool      loadFile(const std::string &elfPath);
  bool      connect();
  bool      program();
  long long stepInstr(); // returns new PC, -1 on error
  bool      reset();     // non-fatal: simulator may skip RESET state
  bool      setVDD(double vdd);
  bool      setConciseMode(bool concise = true); // simulatordisplay.concisemode
  bool      disconnect();
  bool      destroy();

  // ── Stdout/stderr capture ───────────────────────────────────────────────
  // Redirects the JVM's System.out and System.err (com.microchip.mdbcs.
  // QMdbCS.installStdoutCapture()) so every line Java would otherwise write
  // to either stream instead calls the corresponding handler synchronously
  // -- stdoutHandler for System.out lines, stderrHandler for System.err
  // lines. Call this immediately after construct() and before any other
  // JNI call that might produce output (getQMdbCSVersion(),
  // setConciseMode(), etc.) -- anything written before install() completes
  // bypasses the handlers and goes to the JVM's real stdout/stderr.
  //
  // Handlers run on whatever Java thread produced the line; no thread-attach
  // is needed since JNI supplies a valid JNIEnv* to the native callback
  // automatically. Handlers must not throw -- any exception is caught and
  // discarded at the JNI boundary, since an unwind across a JNI-called frame
  // is undefined behavior.
  //
  // Either handler may be empty (default-constructed std::function) if that
  // stream doesn't need handling; the corresponding native callback will
  // simply find no handler registered and return.
  //
  // Returns false if native registration or the install() call failed;
  // non-fatal to the caller (JVM stdout/stderr are simply left unredirected).
  using StdoutLineHandler = std::function<void(const std::string &)>;
  bool installStdoutCapture(StdoutLineHandler stdoutHandler,
                            StdoutLineHandler stderrHandler);

  // ── Pin access ───────────────────────────────────────────────────────────
  // getPin() calls QMdbCS.getPin(String) and returns a JNI global ref to
  // the com.microchip.mdbcs.Pin interface object.  The caller owns the ref
  // and must call releasePin() when done (typically at stopSim time).

  jobject getPin(const std::string &name);
  void    releasePin(jobject pin);

  // ── Aggregated pin state -- one JNI call per pin per step ────────────────
  // Calls QMdbCS.getPinState(Pin) -> packed long, then unpacks on the C++ side.
  // The Pin object is a cached global ref; no per-call name lookup.
  //
  // Packed long layout (matches QMdbCS.java):
  //   Bits 63..32  voltage as IEEE 754 float bits
  //   Bit  1       1 = digital output, 0 = input / hi-Z
  //   Bit  0       1 = analog, 0 = digital
  //
  // Voltage is packed as float (~7 sig. digits).  The float->double precision
  // loss was previously suspected as the cause of QSpice solver non-convergence
  // in 64-bit math mode; that was ruled out -- the actual cause was hardwired
  // RA4/RA5 pins registered via addPinPortMap() causing solver non-convergence.
  // Single-call design restored for performance.
  //
  // Returns false on JNI error; pinState is unmodified on failure.
  bool getPinState(jobject pin, PinState &pinState);

  // ── Pin write -- external stimulus side ──────────────────────────────────
  // Routes through QMdbCS.pinExternalSetVoltage().
  // Use when an external agent (e.g. QSpice) is driving the pin.
  bool pinExternalSetVoltage(jobject pin, double v);

  bool ok() const { return obj_ != nullptr; }

private:
  JNIEnv *env() const { return host_.env(); }
  bool    checkException(const char *where);
  jstring toJString(const std::string &s);
  bool    initPinClass(); // called lazily on first getPin()

  JvmHost &host_;

  // ── QMdbCS class/object ───────────────────────────────────────────────
  jclass  cls_ = nullptr;
  jobject obj_ = nullptr;

  // QMdbCS method IDs -- cached in construct()
  jmethodID midCtor_                  = nullptr;
  jmethodID midGetVersion_            = nullptr; // static getVersion() -> String
  jmethodID midLoadFile_              = nullptr;
  jmethodID midConnect_               = nullptr;
  jmethodID midProgram_               = nullptr;
  jmethodID midStepInstr_             = nullptr;
  jmethodID midReset_                 = nullptr;
  jmethodID midSetVDD_                = nullptr;
  jmethodID midGetToolProps_          = nullptr;
  jmethodID midDisconnect_            = nullptr;
  jmethodID midDestroy_               = nullptr;
  jmethodID midGetPin_                = nullptr;
  jmethodID midGetPinState_           = nullptr; // getPinState(Pin) -> long  [restored: see getPinState()]
  jmethodID midPinIsAnalog_           = nullptr; // Pin.isAnalog()         -> boolean
  jmethodID midPinIsDigitalOutput_    = nullptr; // Pin.isDigitalOutput()  -> boolean
  jmethodID midPinGetVoltage_         = nullptr; // Pin.getVoltage()       -> double
  jmethodID midPinExternalSetVoltage_ = nullptr; // pinExternalSetVoltage(Pin,double)

  // ── Pin interface class ───────────────────────────────────────────────
  // Retained as a type token for JNI parameter passing only.
  // Individual Pin method IDs are not called directly from C++.
  jclass pinCls_ = nullptr;

  // ── Stdout/stderr capture state ─────────────────────────────────────────
  // Set true once installStdoutCapture() successfully registers handlers
  // for this instance's JVM, so the destructor knows to erase this
  // instance's entries from the static JavaVM* -> handler maps (stdout and
  // stderr maps are separate; both keyed by the same JavaVM*).
  bool stdoutCaptureInstalled_ = false;
};
