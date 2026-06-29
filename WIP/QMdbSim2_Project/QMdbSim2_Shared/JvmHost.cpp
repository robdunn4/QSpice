//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
#include "JvmHost.h"
#include <iostream>
#include <vector>
#pragma comment(lib, "Version.lib")

using CreateJavaVMFn = jint(JNICALL *)(JavaVM **, void **, void *);

static std::wstring toWide(const std::string &s) {
  if (s.empty()) return {};
  int          len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
  std::wstring w(len, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
  return w;
}

JvmHost::~JvmHost() {
  if (jvm_) {
    jvm_->DestroyJavaVM();
    jvm_ = nullptr;
    env_ = nullptr;
  }
  if (jvmDll_) {
    FreeLibrary(jvmDll_);
    jvmDll_ = nullptr;
  }
}

bool JvmHost::start(const Config &cfg) {
  // Derive the jre\bin\server directory from the jvm.dll full path and add
  // it to the process DLL search path.  This allows LoadLibraryExW to find
  // jvm.dll's own dependencies (e.g. jimage.dll, java.dll) without requiring
  // the caller to set PATH.  AddDllDirectory is additive and process-scoped;
  // we intentionally do not call RemoveDllDirectory since all DLLs in this
  // process share the same JDK and JNI_CreateJavaVM enforces a single JVM.
  std::string jvmDir = cfg.jvmDllPath;
  size_t      sep    = jvmDir.find_last_of("\\/");
  if (sep != std::string::npos) jvmDir = jvmDir.substr(0, sep);

  DLL_DIRECTORY_COOKIE dirCookie = AddDllDirectory(toWide(jvmDir).c_str());
  if (!dirCookie) {
    std::cerr << "[WARN] AddDllDirectory failed for: " << jvmDir
              << "  err=" << GetLastError() << "\n";
    // Non-fatal: LoadLibraryExW may still succeed if PATH is set.
  }

  // Load jvm.dll by full path; LOAD_LIBRARY_SEARCH_USER_DIRS ensures the
  // directory added above is consulted for jvm.dll's dependencies.
  jvmDll_ = LoadLibraryExW(toWide(cfg.jvmDllPath).c_str(), NULL,
                           LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
                               LOAD_LIBRARY_SEARCH_USER_DIRS);
  if (!jvmDll_) {
    std::cerr << "[FAIL] LoadLibraryExW: " << cfg.jvmDllPath
              << "  err=" << GetLastError() << "\n";
    return false;
  }

  CreateJavaVMFn pfnCreate = reinterpret_cast<CreateJavaVMFn>(
      GetProcAddress(jvmDll_, "JNI_CreateJavaVM"));
  if (!pfnCreate) {
    std::cerr << "[FAIL] GetProcAddress(JNI_CreateJavaVM)\n";
    return false;
  }

  std::string cp    = buildClasspath(cfg);
  std::string cpOpt = "-Djava.class.path=" + cp;
  std::string pkOpt = "-Dpackslib.packsfolder=" + cfg.packsFolder;
  std::string tpOpt = "-Dmplabx.thirdparty.lib.path=" + cfg.thirdpartyLib;

  // Redirect the NetBeans user directory to the null device so the platform
  // logging infrastructure does not create MPLABXLog.xml (and other cache/
  // settings files) in the process working directory.  Comment this out if
  // you need to capture the NetBeans/MPLAB X log for diagnostics.
  std::string nbOpt = "-Dnetbeans.user=NUL";

  // Grant reflective access to java.lang.ref internals for unnamed modules.
  // Without this, the NetBeans ActiveQueue (used by MDBCS) emits two
  // "Cannot hack ReferenceQueue to fix bug #206621!" warnings on JDK 9+
  // because it tries to setAccessible() on ReferenceQueue.lock.  The warning
  // is non-fatal but noisy; this suppresses it cleanly.
  // NOTE: --add-opens is a Java 9+ option and must NOT be passed to Java 8.
  // We detect the major version from the JVM DLL's embedded version resource.
  // If detection fails we err on the side of omitting the flag (safe for
  // both Java 8 and 9+: on 9+ the warning reappears but the JVM still starts).
  std::string aoOpt = "--add-opens=java.base/java.lang.ref=ALL-UNNAMED";
  bool        useAoOpt = false;
  {
    DWORD  verInfoSize = GetFileVersionInfoSizeA(cfg.jvmDllPath.c_str(), nullptr);
    if (verInfoSize > 0) {
      std::vector<BYTE> verBuf(verInfoSize);
      if (GetFileVersionInfoA(cfg.jvmDllPath.c_str(), 0, verInfoSize,
                              verBuf.data())) {
        VS_FIXEDFILEINFO *fi = nullptr;
        UINT              fiLen = 0;
        if (VerQueryValueA(verBuf.data(), "\\",
                           reinterpret_cast<LPVOID *>(&fi), &fiLen) &&
            fi) {
          // FILEVERSION major is the high word of dwFileVersionMS
          int jvmMajor = static_cast<int>(HIWORD(fi->dwFileVersionMS));
          std::cout << "[  JVM ] Detected JVM major version: " << jvmMajor
                    << "\n";
          useAoOpt = (jvmMajor >= 9);
        }
      }
    }
    if (!useAoOpt) {
      std::cout << "[  JVM ] Omitting --add-opens (Java 8 or version "
                   "detection failed)\n";
    }
  }

  JavaVMOption opts[5]{};
  opts[0].optionString = const_cast<char *>(cpOpt.c_str());
  opts[1].optionString = const_cast<char *>(pkOpt.c_str());
  opts[2].optionString = const_cast<char *>(tpOpt.c_str());
  opts[3].optionString = const_cast<char *>(nbOpt.c_str());
  if (useAoOpt)
    opts[4].optionString = const_cast<char *>(aoOpt.c_str());

  JavaVMInitArgs args{};
  args.version            = JNI_VERSION_1_8;
  args.nOptions           = useAoOpt ? 5 : 4;
  args.options            = opts;
  args.ignoreUnrecognized = JNI_FALSE;

  jint rc = pfnCreate(&jvm_, reinterpret_cast<void **>(&env_), &args);
  if (rc != JNI_OK || !env_) {
    std::cerr << "[FAIL] JNI_CreateJavaVM rc=" << rc << "\n";
    jvm_ = nullptr;
    env_ = nullptr;
    return false;
  }

  // Report actual JVM version (jint: high 16 bits = major, low 16 bits = minor).
  jint jvmVer = env_->GetVersion();
  int  major  = (jvmVer >> 16) & 0xFFFF;
  int  minor  = jvmVer & 0xFFFF;
  std::cout << "[  OK  ] JVM started (JNI " << major << "." << minor << ")\n";
  return true;
}

std::string JvmHost::buildClasspath(const Config &cfg) {
  std::vector<std::string> jars;
  jars.push_back(cfg.mdbcsJar);

  std::string              plat     = cfg.mplabxRoot + "\\mplab_platform";
  std::vector<std::string> scanDirs = {
      plat + "\\mdbcore\\modules\\ext",
      plat + "\\mdbcore\\modules",
      plat + "\\mplablibs\\modules\\ext",
      plat + "\\mplablibs\\modules",
  };
  for (const std::string &dir : scanDirs) collectJarsInDir(dir, jars);

  std::vector<std::string> singles = {
      plat + "\\platform\\lib\\org-openide-util.jar",
      plat + "\\platform\\lib\\org-openide-modules.jar",
      plat + "\\platform\\lib\\org-openide-util-lookup.jar",
      plat + "\\platform\\core\\org-openide-filesystems.jar",
      plat + "\\ide\\modules\\org-netbeans-api-debugger.jar",
  };
  for (const std::string &s : singles) jars.push_back(s);

  std::cout << "[  CP  ] " << jars.size() << " JARs on classpath.\n";

  std::string cp;
  for (size_t i = 0; i < jars.size(); ++i) {
    if (i) cp += ';';
    cp += jars[i];
  }
  return cp;
}

void JvmHost::collectJarsInDir(const std::string        &dir,
                               std::vector<std::string> &out) {
  std::string      pattern = dir + "\\*.jar";
  WIN32_FIND_DATAA fd{};
  HANDLE           h = FindFirstFileA(pattern.c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE) return;
  do {
    out.push_back(dir + "\\" + fd.cFileName);
  } while (FindNextFileA(h, &fd));
  FindClose(h);
}
