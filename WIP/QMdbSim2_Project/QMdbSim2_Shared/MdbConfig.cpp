//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
#include "MdbConfig.h"
#include <Windows.h>
#include <algorithm>

//------------------------------------------------------------------------------
// load() -- resolve the ini file path and read configuration
//------------------------------------------------------------------------------
bool MdbConfig::load(const char *iniPath) {
  // resolve the actual ini file path
  std::string ini = resolvePath(iniPath);
  if (errState_) return false;

  if (!pathExists(ini)) {
    setError(("QMdbCfg: ini file not found: " + ini).c_str());
    return false;
  }
  iniFile_ = ini;

  // read the three required keys from [QMdbCfg] section
  mplabxRoot_ = readIniString("MplabxRoot", ini.c_str());
  jdkRoot_    = readIniString("JdkRoot", ini.c_str());
  qmdbcsJar_  = readIniString("QMdbcsJar", ini.c_str());

  if (mplabxRoot_.empty()) {
    setError("QMdbCfg: MplabxRoot not found in ini file");
    return false;
  }
  if (jdkRoot_.empty()) {
    setError("QMdbCfg: JdkRoot not found in ini file");
    return false;
  }
  if (qmdbcsJar_.empty()) {
    setError("QMdbCfg: QMdbcsJar not found in ini file");
    return false;
  }

  // remove any trailing backslashes from root paths
  auto stripTrailing = [](std::string &s) {
    while (!s.empty() && (s.back() == '\\' || s.back() == '/')) s.pop_back();
  };
  stripTrailing(mplabxRoot_);
  stripTrailing(jdkRoot_);

  // resolve QMdbcsJar relative to the ini file's directory if not absolute
  std::string iniDir = iniFile_;
  size_t      iniSep = iniDir.find_last_of("\\/");
  if (iniSep != std::string::npos) iniDir = iniDir.substr(0, iniSep);
  qmdbcsJar_ = resolveRelativePath(qmdbcsJar_, iniDir);

  // derive all paths from the roots
  // jvmDll_      = jdkRoot_ + "\\jre\\bin\\server\\jvm.dll";
  jvmDll_      = jdkRoot_ + "\\bin\\server\\jvm.dll";
  packsFolder_ = mplabxRoot_ + "\\packs";
  thirdparty_  = mplabxRoot_ + "\\mplab_platform\\thirdparty";

  // validate derived paths exist
  if (!pathExists(jvmDll_)) {
    setError(("QMdbCfg: jvm.dll not found: " + jvmDll_).c_str());
    return false;
  }
  // QSpice is a 32-bit process; jvm.dll must also be 32-bit.  A 64-bit
  // jvm.dll will silently fail at LoadLibrary time, so we catch it here
  // with a clear diagnostic instead.
  if (!isJvmDll32Bit(jvmDll_)) {
    setError(("QMdbCfg: jvm.dll is not 32-bit -- QSpice requires a 32-bit "
              "JRE.  The MPLabX bundled JRE is 64-bit; install a separate "
              "32-bit JRE and set JdkRoot accordingly.  Found: " +
              jvmDll_)
                 .c_str());
    return false;
  }
  if (!pathExists(qmdbcsJar_)) {
    setError(("QMdbCfg: qmdbcs.jar not found: " + qmdbcsJar_).c_str());
    return false;
  }
  if (!pathExists(packsFolder_)) {
    setError(("QMdbCfg: packs folder not found: " + packsFolder_).c_str());
    return false;
  }
  if (!pathExists(thirdparty_)) {
    setError(("QMdbCfg: thirdparty folder not found: " + thirdparty_).c_str());
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
// resolvePath() -- resolve iniPath to a full path to the ini file
//------------------------------------------------------------------------------
std::string MdbConfig::resolvePath(const char *iniPath) {
  std::string path = iniPath ? iniPath : ".";

  // "." means use the DLL's directory
  if (path == ".") {
    char    dllPath[MAX_PATH] = {};
    HMODULE hMod              = NULL;

    // get handle to this DLL (not the exe)
    // Use a static local as the address anchor -- a pointer-to-member cannot
    // be cast to LPCSTR, but a pointer to a plain data object can.
    static const char anchor = 0;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       &anchor, &hMod);

    if (!GetModuleFileNameA(hMod, dllPath, MAX_PATH)) {
      setError("QMdbCfg: failed to get DLL path");
      return {};
    }

    // strip filename, keep directory
    path       = dllPath;
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) path = path.substr(0, pos);
  }

  // if path doesn't end in .ini, append QMdbCfg.ini
  std::string lower = path;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  if (lower.size() < 4 || lower.substr(lower.size() - 4) != ".ini") {
    // strip any trailing backslash before appending
    if (!path.empty() && (path.back() == '\\' || path.back() == '/'))
      path.pop_back();
    path += "\\QMdbCfg.ini";
  }

  return path;
}

//------------------------------------------------------------------------------
// resolveRelativePath() -- if path is relative, anchor it to baseDir
//
// Absolute paths (drive-letter or UNC) are returned unchanged.
// Relative paths are joined with baseDir and canonicalised via
// GetFullPathNameA so that ".." segments are collapsed.
//------------------------------------------------------------------------------
std::string MdbConfig::resolveRelativePath(const std::string &path,
                                           const std::string &baseDir) {
  // Treat as absolute if it starts with a drive letter+colon or UNC prefix.
  if (path.size() >= 2 &&
      ((isalpha((unsigned char)path[0]) && path[1] == ':') ||
       (path[0] == '\\' && path[1] == '\\')))
    return path;

  // Relative -- combine with baseDir and canonicalise.
  std::string combined = baseDir + "\\" + path;
  char        buf[MAX_PATH * 2] = {};
  if (GetFullPathNameA(combined.c_str(), sizeof(buf), buf, nullptr))
    return std::string(buf);

  return combined; // fallback: return as-is if API fails
}

//------------------------------------------------------------------------------
// readIniString() -- read a string value from [QMdbCfg] section
//------------------------------------------------------------------------------
std::string MdbConfig::readIniString(const char *key, const char *iniFile) {
  char  buf[MAX_PATH * 2] = {};
  DWORD result =
      GetPrivateProfileStringA("QMdbCfg", key, "", buf, sizeof(buf), iniFile);
  if (result == 0) return {};
  return std::string(buf);
}

//------------------------------------------------------------------------------
// isJvmDll32Bit() -- return true if the PE machine type is
// IMAGE_FILE_MACHINE_I386
//
// QSpice is a 32-bit (Win32) process.  LoadLibraryExW will fail silently if
// jvm.dll is 64-bit, so we inspect the PE header here and report a clear
// error rather than letting the failure surface deep inside JvmHost.
//------------------------------------------------------------------------------
bool MdbConfig::isJvmDll32Bit(const std::string &path) {
  HANDLE hFile =
      CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) return false;

  // read DOS header to locate PE header offset
  IMAGE_DOS_HEADER dosHdr    = {};
  DWORD            bytesRead = 0;
  if (!ReadFile(hFile, &dosHdr, sizeof(dosHdr), &bytesRead, nullptr) ||
      bytesRead != sizeof(dosHdr) || dosHdr.e_magic != IMAGE_DOS_SIGNATURE) {
    CloseHandle(hFile);
    return false;
  }

  // seek to PE header and read machine type
  DWORD peSignature = 0;
  SetFilePointer(hFile, dosHdr.e_lfanew, nullptr, FILE_BEGIN);
  if (!ReadFile(hFile, &peSignature, sizeof(peSignature), &bytesRead,
                nullptr) ||
      bytesRead != sizeof(peSignature) || peSignature != IMAGE_NT_SIGNATURE) {
    CloseHandle(hFile);
    return false;
  }

  IMAGE_FILE_HEADER fileHdr = {};
  if (!ReadFile(hFile, &fileHdr, sizeof(fileHdr), &bytesRead, nullptr) ||
      bytesRead != sizeof(fileHdr)) {
    CloseHandle(hFile);
    return false;
  }

  CloseHandle(hFile);
  return fileHdr.Machine == IMAGE_FILE_MACHINE_I386;
}

//------------------------------------------------------------------------------
// pathExists() -- check that a file or directory exists
//------------------------------------------------------------------------------
bool MdbConfig::pathExists(const std::string &path) {
  return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

//------------------------------------------------------------------------------
// setError() -- record error state and message
//------------------------------------------------------------------------------
void MdbConfig::setError(const char *msg) {
  if (errState_) return; // keep first error
  errState_   = true;
  lastErrMsg_ = msg ? msg : "Unknown error";
}
