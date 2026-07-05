//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <jni.h>
#include <string>
#include <vector>
#include <windows.h>

class JvmHost {
public:
  struct Config {
    std::string jvmDllPath;    // full path to jvm.dll
    std::string mdbcsJar;      // full path to mdbcs.jar
    std::string mplabxRoot;    // e.g. C:\Program Files\Microchip\MPLABX\v6.30
    std::string packsFolder;   // -Dpackslib.packsfolder value
    std::string thirdpartyLib; // -Dmplabx.thirdparty.lib.path value
  };

  JvmHost() = default;
  ~JvmHost();
  JvmHost(const JvmHost &)            = delete;
  JvmHost &operator=(const JvmHost &) = delete;

  bool    start(const Config &cfg);
  bool    ok() const { return env_ != nullptr; }
  JNIEnv *env() const { return env_; }
  JavaVM *jvm() const { return jvm_; }

private:
  std::string buildClasspath(const Config &cfg);
  void collectJarsInDir(const std::string &dir, std::vector<std::string> &out);

  HMODULE jvmDll_ = nullptr;
  JavaVM *jvm_    = nullptr;
  JNIEnv *env_    = nullptr;
};
