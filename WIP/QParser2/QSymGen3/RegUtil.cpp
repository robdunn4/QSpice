//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "RegUtil.h"
#include "Config.h"
#include <windows.h>

std::string RegUtil::getQSpiceInstallDir() {
  // full subkey path: "Software\Marcus Aurelius Software LLC\InstallDir"
  std::string subKey = std::string(Config::qspiceRegKey) + "\\" +
                       std::string(Config::qspiceRegSubKey);

  HKEY    hKey   = nullptr;
  LSTATUS status = RegOpenKeyExA(HKEY_CURRENT_USER, subKey.c_str(), 0,
                                 KEY_READ, &hKey);
  if (status != ERROR_SUCCESS) return "";

  char  buf[MAX_PATH] = {};
  DWORD bufSize       = sizeof(buf);
  DWORD type          = 0;

  // query the default value (nullptr value name)
  status = RegQueryValueExA(hKey, nullptr, nullptr, &type,
                            reinterpret_cast<LPBYTE>(buf), &bufSize);
  RegCloseKey(hKey);

  if (status != ERROR_SUCCESS) return "";
  if (type != REG_SZ && type != REG_EXPAND_SZ) return "";

  return std::string(buf);
}
