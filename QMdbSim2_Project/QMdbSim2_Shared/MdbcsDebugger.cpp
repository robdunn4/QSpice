//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
#include "MdbcsDebugger.h"
#include <bit>
#include <iostream>

static const char *kQMdbCSClass = "com/microchip/mdbcs/QMdbCS";
static const char *kPinClass    = "com/microchip/mdbcs/Pin";

MdbcsDebugger::MdbcsDebugger(JvmHost &host) : host_(host) {}

MdbcsDebugger::~MdbcsDebugger() {
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

bool MdbcsDebugger::construct(const std::string &device,
                              const std::string &tool, bool asDebugger) {
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
