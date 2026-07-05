//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// MdbConfig.h -- Loads QMdbCfg.ini and derives all JVM/MDBCS paths needed
// to configure MdbSim.
//
// QMdbCfg.ini specifies three roots; all other paths are derived:
//   MplabxRoot  -- MPLAB X installation directory
//   JdkRoot     -- JDK installation directory
//   QMdbcsJar   -- Path to qmdbcs.jar (fat JAR: QMdbCS facade + MDBCS);
//                  may be absolute or relative to the ini file's directory.
//
// Usage:
//   MdbConfig cfg;
//   if (!cfg.load(iniPath)) { /* cfg.getLastErrMsg() */ }
//   MdbSim::Config simCfg = cfg.toSimConfig();
//------------------------------------------------------------------------------
#pragma once
#include <string>

class MdbConfig {
public:
  MdbConfig()  = default;
  ~MdbConfig() = default;

  // Load ini file from the given path.
  // If iniPath == "." the ini file is looked for in the DLL's directory.
  // If iniPath is a directory (no .ini extension) QMdbCfg.ini is appended.
  bool load(const char *iniPath);

  // Accessors for derived paths
  const std::string &getIniFile() const { return iniFile_; }
  const std::string &getMplabxRoot() const { return mplabxRoot_; }
  const std::string &getJdkRoot() const { return jdkRoot_; }
  const std::string &getQMdbcsJar() const { return qmdbcsJar_; }
  const std::string &getJvmDll() const { return jvmDll_; }
  const std::string &getPacksFolder() const { return packsFolder_; }
  const std::string &getThirdparty() const { return thirdparty_; }

  bool        getErrState() const { return errState_; }
  const char *getLastErrMsg() const { return lastErrMsg_.c_str(); }

private:
  std::string iniFile_; // resolved full path to the ini file
  std::string mplabxRoot_;
  std::string jdkRoot_;
  std::string qmdbcsJar_; // fat JAR: QMdbCS facade + all MDBCS classes

  // derived paths
  std::string jvmDll_;
  std::string packsFolder_;
  std::string thirdparty_;

  bool        errState_   = false;
  std::string lastErrMsg_ = "No errors";

  void        setError(const char *msg);
  std::string resolvePath(const char *iniPath);
  std::string resolveRelativePath(const std::string &path,
                                  const std::string &baseDir);
  std::string readIniString(const char *key, const char *iniFile);
  bool        pathExists(const std::string &path);
  bool        isJvmDll32Bit(const std::string &path);
};
