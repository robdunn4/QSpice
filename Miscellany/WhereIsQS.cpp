//------------------------------------------------------------------------------
// WhereIsQS.cpp -- demonstrates how to retrieve the QSpice installation
// folder location from the Windows Registry.
//------------------------------------------------------------------------------
//
// While this code could be used in C-Block/DLL code, QSpice DLL templates
// include a pointer to the QUX.exe path.  This code is really intended for use
// in non-C-Block projects (command line scripting, GUI projects, etc.).
//
// As is, this code produces WhereIsQS.exe.  When run from the command line, it
// writes the QSpice installation folder name to stdout and returns exit code 0
// when successful.  On failure, it returns exit code -1 and writes an error
// message to stdout.
//
// To compile this code with the QSpice DMC compiler:
//     dmc.exe -DTS_DMC -mn -W WhereIsQS.cpp kernel32.lib advapi32.lib
//

#include <Windows.h>
#include <stdio.h>

int main(int argc, char **argv) {
  HKEY  hKey;
  DWORD dwType;
  TCHAR szData[256];
  DWORD dwSize = sizeof(szData);
  int   err    = false;

  const char qsKey[] = "Software\\Marcus Aurelius Software LLC\\InstallDir";

  err = RegOpenKeyExA(HKEY_CURRENT_USER, qsKey, 0, KEY_READ, &hKey) !=
      ERROR_SUCCESS;

  if (!err) {
    err = RegQueryValueExA(hKey, "", NULL, &dwType,
              reinterpret_cast<LPBYTE>(szData), &dwSize) != ERROR_SUCCESS;

    if (!err) err = !(dwType == REG_SZ || dwType == REG_EXPAND_SZ);

    RegCloseKey(hKey);
  }

  if (err) {
    printf("Unable to locate registry entry (HKEY_CURRENT_USER\\%s)\n", qsKey);
    return -1;
  }

  printf("\"%s\"\n", szData);
  return 0;
}