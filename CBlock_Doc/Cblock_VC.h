/*==============================================================================
 * Cblock_VC.h -- Header file for custom QSpice CBlock template for MSVC.
 *
 * Note:  This header contains both declarations and implementations.  This is
 * bad coding style but, for a single compilation unit (i.e., a single *.cpp),
 * it's simple and doesn't require more complicated multi-object compiles/links.
 *============================================================================*/
#ifndef CBLOCK_VC_H
#define CBLOCK_VC_H

#include <cstdio>
#include <chrono>
#include <thread>

#ifndef _MSVC_LANG
#error This header requires MSVC
#elif _MSVC_LANG < 201703L
#pragma message("This header requires C++17 standard or newer")
#endif

// the data type union for ports and attributes
union uData {
  bool                   b;
  char                   c;
  unsigned char          uc;
  short                  s;
  unsigned short         us;
  int                    i;
  unsigned int           ui;
  float                  f;
  double                 d;
  long long int          i64;
  unsigned long long int ui64;
  char                  *str;
  unsigned char         *bytes;
};

#define msg(...) msg_(__LINE__, __VA_ARGS__)

// QSpice printf() to Output window with embedded process waits...
template <typename... Args>
void msg_(int lineNbr, const char *fmt, Args... args) {
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  fflush(stdout);
  fprintf(stdout, PROGRAM_INFO " @%d: ", lineNbr);
  fprintf(stdout, fmt, args...);
  fflush(stdout);
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
}

// int DllMain() must exist and return 1 for a process to load the .DLL
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

#endif   // CBLOCK_VC_H
/*==============================================================================
 * End of Cblock_VC.h
 *============================================================================*/