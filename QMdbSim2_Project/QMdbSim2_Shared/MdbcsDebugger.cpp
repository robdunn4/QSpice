//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
#include "MdbcsDebugger.h"
#include <bit>
#include <iostream>
#include <mutex>
#include <unordered_map>

static const char *kQMdbCSClass = "com/microchip/mdbcs/QMdbCS";
static const char *kPinClass    = "com/microchip/mdbcs/Pin";

// ── Stdout/stderr capture: JVM* -> handler maps ─────────────────────────────
// JNI requires a plain function pointer for RegisterNatives, not a member
// function, so the native callbacks below are free-standing and look up the
// right MdbcsDebugger instance's handler by JavaVM* (each MdbSim/InstData
// owns one JVM, so this is a 1:1 mapping in practice, but keyed by JavaVM*
// rather than assumed-singleton in case that ever changes).
//
// Stdout and stderr are kept in separate maps (rather than one map of a
// struct-of-two-handlers) so each native callback's lookup/dispatch is
// independent and a JVM with only one stream captured doesn't need a
// placeholder entry for the other.
namespace {
  std::mutex g_stdoutHandlerMutex;
  std::unordered_map<JavaVM *, MdbcsDebugger::StdoutLineHandler> g_stdoutHandlers;
  std::unordered_map<JavaVM *, MdbcsDebugger::StdoutLineHandler> g_stderrHandlers;

  void dispatchLine(JNIEnv *env, jstring jline,
                    std::mutex &mutex,
                    std::unordered_map<JavaVM *, MdbcsDebugger::StdoutLineHandler> &handlers) {
    JavaVM *jvm = nullptr;
    env->GetJavaVM(&jvm);

    MdbcsDebugger::StdoutLineHandler handler;
    {
      std::lock_guard<std::mutex> lock(mutex);
      auto it = handlers.find(jvm);
      if (it == handlers.end()) return; // no handler registered for this JVM
      handler = it->second; // copy out; don't hold the lock during user code
    }
    if (!handler) return;

    const char *utf = env->GetStringUTFChars(jline, nullptr);
    if (!utf) return;
    std::string line(utf);
    env->ReleaseStringUTFChars(jline, utf);

    try {
      handler(line);
    } catch (...) {
      // A C++ exception must never unwind across a JNI-called frame --
      // that is undefined behavior. Swallow it here.
    }
  }

  void JNICALL nativeHandleStdoutLine(JNIEnv *env, jclass /*clazz*/, jstring jline) {
    dispatchLine(env, jline, g_stdoutHandlerMutex, g_stdoutHandlers);
  }

  void JNICALL nativeHandleStderrLine(JNIEnv *env, jclass /*clazz*/, jstring jline) {
    dispatchLine(env, jline, g_stdoutHandlerMutex, g_stderrHandlers);
  }
} // namespace

MdbcsDebugger::MdbcsDebugger(JvmHost &host) : host_(host) {}

MdbcsDebugger::~MdbcsDebugger() {
  if (stdoutCaptureInstalled_) {
    std::lock_guard<std::mutex> lock(g_stdoutHandlerMutex);
    g_stdoutHandlers.erase(host_.jvm());
    g_stderrHandlers.erase(host_.jvm());
  }
  if (obj_) {
    env()->DeleteGlobalRef(obj_);
    obj_ = nullptr;
  }
  if (cls_) {
    env()->DeleteGlobalRef(cls_);
    cls_ = nullptr;
  }
  if (pinCls_) {
    env()->DeleteGlobalRef(pinCls_);
    pinCls_ = nullptr;
  }
}

bool MdbcsDebugger::checkException(const char *where) {
  if (!env()->ExceptionCheck()) return false;
  std::cerr << "[FAIL] Java exception in " << where << "\n";
  env()->ExceptionDescribe();
  env()->ExceptionClear();
  return true;
}

jstring MdbcsDebugger::toJString(const std::string &s) {
  return env()->NewStringUTF(s.c_str());
}

// ── QMdbCS API ───────────────────────────────────────────────────────────────

// ── QMdbCS class/object construction ──────────────────────────────────────────

bool MdbcsDebugger::resolveClass() {
  jclass localCls = env()->FindClass(kQMdbCSClass);
  if (!localCls || checkException("FindClass(QMdbCS)")) return false;
  cls_ = static_cast<jclass>(env()->NewGlobalRef(localCls));
  env()->DeleteLocalRef(localCls);

  // All signatures verified with: javap -p -s com.microchip.mdbcs.QMdbCS
  midCtor_      = env()->GetMethodID(cls_, "<init>",
                                     "(Ljava/lang/String;Ljava/lang/String;Z)V");
  midGetVersion_ = env()->GetStaticMethodID(cls_, "getVersion",
                                            "()Ljava/lang/String;");
  midLoadFile_  = env()->GetMethodID(cls_, "loadFile",  "(Ljava/lang/String;)V");
  midConnect_   = env()->GetMethodID(cls_, "connect",   "()V");
  midProgram_   = env()->GetMethodID(cls_, "program",   "()V");
  midStepInstr_ = env()->GetMethodID(cls_, "stepInstr", "()J");
  midReset_     = env()->GetMethodID(cls_, "reset",     "()V");
  midSetVDD_    = env()->GetMethodID(cls_, "setVDD",    "(F)V");
  midGetToolProps_ = env()->GetMethodID(cls_, "getToolProperties",
                                        "()Ljava/util/Properties;");
  midDisconnect_ = env()->GetMethodID(cls_, "disconnect", "()V");
  midDestroy_    = env()->GetMethodID(cls_, "destroy",    "()V");
  midGetPin_     = env()->GetMethodID(cls_, "getPin",
                                      "(Ljava/lang/String;)Lcom/microchip/mdbcs/Pin;");
  midGetPinState_ = env()->GetMethodID(cls_, "getPinState",
                                       "(Lcom/microchip/mdbcs/Pin;)J");
  midPinExternalSetVoltage_ = env()->GetMethodID(cls_, "pinExternalSetVoltage",
                                                 "(Lcom/microchip/mdbcs/Pin;D)V");

  if (checkException("GetMethodID(QMdbCS)")) return false;

  if (!midCtor_      || !midGetVersion_ ||
      !midLoadFile_  || !midConnect_   || !midProgram_  ||
      !midStepInstr_ || !midReset_     || !midSetVDD_    ||
      !midGetToolProps_ || !midDisconnect_ || !midDestroy_ ||
      !midGetPin_    || !midGetPinState_ || !midPinExternalSetVoltage_) {
    std::cerr << "[FAIL] One or more QMdbCS method IDs not found -- check "
                 "signatures\n";
    return false;
  }

  return true;
}

bool MdbcsDebugger::instantiate(const std::string &device,
                                const std::string &tool, bool asDebugger) {
  jstring jDev  = toJString(device);
  jstring jTool = toJString(tool);
  jobject local = env()->NewObject(cls_, midCtor_, jDev, jTool,
                                   static_cast<jboolean>(asDebugger));
  env()->DeleteLocalRef(jDev);
  env()->DeleteLocalRef(jTool);
  if (checkException("QMdbCS()") || !local) return false;

  obj_ = env()->NewGlobalRef(local);
  env()->DeleteLocalRef(local);
  return true;
}

bool MdbcsDebugger::construct(const std::string &device,
                              const std::string &tool, bool asDebugger) {
  return resolveClass() && instantiate(device, tool, asDebugger);
}

std::string MdbcsDebugger::getQMdbCSVersion() {
  jstring jver = static_cast<jstring>(
      env()->CallStaticObjectMethod(cls_, midGetVersion_));
  if (checkException("getQMdbCSVersion") || !jver) return {};
  const char *chars = env()->GetStringUTFChars(jver, nullptr);
  std::string ver   = chars ? chars : "";
  env()->ReleaseStringUTFChars(jver, chars);
  env()->DeleteLocalRef(jver);
  return ver;
}

bool MdbcsDebugger::loadFile(const std::string &elfPath) {
  jstring j = toJString(elfPath);
  env()->CallVoidMethod(obj_, midLoadFile_, j);
  env()->DeleteLocalRef(j);
  return !checkException("loadFile");
}

bool MdbcsDebugger::connect() {
  env()->CallVoidMethod(obj_, midConnect_);
  return !checkException("connect");
}

bool MdbcsDebugger::program() {
  env()->CallVoidMethod(obj_, midProgram_);
  return !checkException("program");
}

long long MdbcsDebugger::stepInstr() {
  jlong pc = env()->CallLongMethod(obj_, midStepInstr_);
  if (checkException("stepInstr")) return -1LL;
  return static_cast<long long>(pc);
}

bool MdbcsDebugger::reset() {
  env()->CallVoidMethod(obj_, midReset_);
  if (env()->ExceptionCheck()) {
    // The PIC16F15213 simulator skips the RESET state and goes directly to
    // HALTED, causing reset() to throw.  Treat as non-fatal.
    std::cerr << "[WARN] reset() threw (non-fatal on simulator)\n";
    env()->ExceptionDescribe();
    env()->ExceptionClear();
  }
  return true;
}

bool MdbcsDebugger::setVDD(double vdd) {
  // Java signature is setVDD(float), so cast to jfloat.
  env()->CallVoidMethod(obj_, midSetVDD_, static_cast<jfloat>(vdd));
  return !checkException("setVDD");
}

bool MdbcsDebugger::setConciseMode(bool concise) {
  // Calls QMdbCS.getToolProperties().setProperty("simulatordisplay.concisemode",
  // "true"/"false").  getToolProperties() returns a java.util.Properties object.
  jobject props = env()->CallObjectMethod(obj_, midGetToolProps_);
  if (checkException("getToolProperties") || !props) return false;

  jclass    propsCls = env()->GetObjectClass(props);
  jmethodID midSet   = env()->GetMethodID(
      propsCls, "setProperty",
      "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");
  env()->DeleteLocalRef(propsCls);
  if (checkException("GetMethodID(setProperty)") || !midSet) return false;

  jstring key = toJString("simulatordisplay.concisemode");
  jstring val = toJString(concise ? "true" : "false");
  jobject ret = env()->CallObjectMethod(props, midSet, key, val);
  env()->DeleteLocalRef(key);
  env()->DeleteLocalRef(val);
  if (ret) env()->DeleteLocalRef(ret);
  env()->DeleteLocalRef(props);

  return !checkException("setProperty(concisemode)");
}

bool MdbcsDebugger::disconnect() {
  env()->CallVoidMethod(obj_, midDisconnect_);
  return !checkException("disconnect");
}

bool MdbcsDebugger::destroy() {
  env()->CallVoidMethod(obj_, midDestroy_);
  return !checkException("destroy");
}

// ── Stdout/stderr capture ───────────────────────────────────────────────────
//
// StdoutFilter was merged into QMdbCS.java (no longer a separate class), so
// this reuses the already-cached QMdbCS class global ref (cls_) rather than
// doing its own FindClass/NewGlobalRef.  See QMdbCS.installStdoutCapture()
// and QMdbCS.nativeHandleStdoutLine()/nativeHandleStderrLine() for the Java
// side.

bool MdbcsDebugger::installStdoutCapture(StdoutLineHandler stdoutHandler,
                                         StdoutLineHandler stderrHandler) {
  JNINativeMethod methods[] = {
      {const_cast<char *>("nativeHandleStdoutLine"),
       const_cast<char *>("(Ljava/lang/String;)V"),
       reinterpret_cast<void *>(&nativeHandleStdoutLine)},
      {const_cast<char *>("nativeHandleStderrLine"),
       const_cast<char *>("(Ljava/lang/String;)V"),
       reinterpret_cast<void *>(&nativeHandleStderrLine)}};

  bool registerFailed = (env()->RegisterNatives(cls_, methods, 2) != 0) ||
                        checkException("RegisterNatives(QMdbCS)");
  if (registerFailed) return false;

  jmethodID midInstall = env()->GetStaticMethodID(cls_, "installStdoutCapture", "()V");
  if (checkException("GetStaticMethodID(installStdoutCapture)") || !midInstall) {
    return false;
  }

  // Register the handlers BEFORE calling install() -- install() itself
  // could theoretically trigger output (it doesn't today, but this
  // ordering means both handlers are live the instant System.out/err are
  // swapped, with no window where a captured line would find no handler
  // registered).
  JavaVM *jvm = host_.jvm();
  {
    std::lock_guard<std::mutex> lock(g_stdoutHandlerMutex);
    g_stdoutHandlers[jvm] = std::move(stdoutHandler);
    g_stderrHandlers[jvm] = std::move(stderrHandler);
  }
  stdoutCaptureInstalled_ = true;

  env()->CallStaticVoidMethod(cls_, midInstall);
  return !checkException("QMdbCS.installStdoutCapture()");
}

// ── Pin access ────────────────────────────────────────────────────────────────

// Looks up the Pin interface class once for type-checking purposes.
// Called lazily from the first getPin() call.
bool MdbcsDebugger::initPinClass() {
  if (pinCls_) return true;
  jclass local = env()->FindClass(kPinClass);
  if (!local || checkException("FindClass(Pin)")) return false;
  pinCls_ = static_cast<jclass>(env()->NewGlobalRef(local));
  env()->DeleteLocalRef(local);

  // Cache Pin instance method IDs.
  // Signatures verified with: javap -p -s com.microchip.mdbcs.Pin
  midPinIsAnalog_        = env()->GetMethodID(pinCls_, "isAnalog",        "()Z");
  midPinIsDigitalOutput_ = env()->GetMethodID(pinCls_, "isDigitalOutput", "()Z");
  midPinGetVoltage_      = env()->GetMethodID(pinCls_, "getVoltage",      "()D");
  if (checkException("GetMethodID(Pin)") ||
      !midPinIsAnalog_ || !midPinIsDigitalOutput_ || !midPinGetVoltage_) {
    std::cerr << "[FAIL] One or more Pin method IDs not found\n";
    return false;
  }
  return true;
}

jobject MdbcsDebugger::getPin(const std::string &name) {
  if (!initPinClass()) return nullptr;

  jstring jName = toJString(name);
  jobject local = env()->CallObjectMethod(obj_, midGetPin_, jName);
  env()->DeleteLocalRef(jName);
  if (checkException("getPin") || !local) return nullptr;

  jobject global = env()->NewGlobalRef(local);
  env()->DeleteLocalRef(local);
  return global;
}

void MdbcsDebugger::releasePin(jobject pin) {
  if (pin) env()->DeleteGlobalRef(pin);
}

// ── Aggregated pin state ──────────────────────────────────────────────────────

bool MdbcsDebugger::getPinState(jobject pin, PinState &pinState) {
  // Single JNI call to QMdbCS.getPinState(Pin) -> packed long.
  // Voltage is encoded as IEEE 754 float bits (32-bit precision, ~7 sig. digits).
  // The float->double precision loss was previously suspected as the cause of
  // QSpice solver non-convergence in 64-bit math mode; that was ruled out --
  // the actual cause was hardwired RA4/RA5 pins registered via addPinPortMap().
  // Single-call design restored for performance.
  //
  // Packed long layout (matches QMdbCS.java):
  //   Bits 63..32  voltage as IEEE 754 float bits
  //   Bit  1       1 = digital output, 0 = input / hi-Z
  //   Bit  0       1 = analog, 0 = digital
  jlong packed = env()->CallLongMethod(obj_, midGetPinState_, pin);
  if (checkException("getPinState")) return false;
  jint  floatBits = static_cast<jint>(packed >> 32);
  float voltage   = std::bit_cast<float>(floatBits);
  pinState.voltage = static_cast<double>(voltage);
  pinState.ioState = (packed & 2) ? PIN_OUTPUT : PIN_INPUT;
  pinState.daState = (packed & 1) ? PIN_ANALOG  : PIN_DIGITAL;
  return true;

  // Previous implementation (three JNI calls per pin per step):
  // Replaced packed-long with direct Pin calls to restore double precision
  // after float->double precision loss was suspected (incorrectly) to cause
  // QSpice solver non-convergence.  Kept for reference.
  //
  // jboolean analog  = env()->CallBooleanMethod(pin, midPinIsAnalog_);
  // if (checkException("Pin.isAnalog")) return false;
  // jboolean output  = !analog &&
  //                    env()->CallBooleanMethod(pin, midPinIsDigitalOutput_);
  // if (checkException("Pin.isDigitalOutput")) return false;
  // jdouble  voltage = env()->CallDoubleMethod(pin, midPinGetVoltage_);
  // if (checkException("Pin.getVoltage")) return false;
  // pinState.voltage = static_cast<double>(voltage);
  // pinState.ioState = output  ? PIN_OUTPUT  : PIN_INPUT;
  // pinState.daState = analog  ? PIN_ANALOG  : PIN_DIGITAL;
  // return true;
}

// ── Pin write ─────────────────────────────────────────────────────────────────

bool MdbcsDebugger::pinExternalSetVoltage(jobject pin, double v) {
  // Drives the pin as an external stimulus via QMdbCS.pinExternalSetVoltage().
  // Use this (not a direct Pin call) when QSpice is the source of the signal.
  env()->CallVoidMethod(obj_, midPinExternalSetVoltage_, pin,
                        static_cast<jdouble>(v));
  return !checkException("pinExternalSetVoltage");
}
