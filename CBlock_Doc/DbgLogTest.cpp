/*==============================================================================
 * DbgLogTest.cpp -- A test component for the DbgLog.h data logger.
 *
 * This file does little beyond demonstrating the DbgLog.h stuff and showing
 * when and in what order QSpice calls the various exported functions.  (More
 * often than I expected.)
 *============================================================================*/
// To build with Digital Mars C++ Compiler:
//    dmc -mn -WD dbglog.cpp kernel32.lib

#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <time.h>

// must declare DbgLog instance for logging; instance name must be dbgLog;
// change file name and max line limit if desired.
#include "dbglog.h"
DbgLog dbgLog("@qdebug.log", 100);

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

// int DllMain() must exist and return 1 for a process to load the .DLL
// See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain for more
// information.
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

void display(const char *fmt, ...) {
  msleep(30);
  fflush(stdout);
  va_list args = {0};
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  fflush(stdout);
  msleep(30);
}

// #undef pin names lest they collide with names in any header file(s) you might
// include.
#undef In
#undef Out

struct InstData {
  // no per-instance data for this demo
};
typedef InstData *pInstData;

// The "Evaluation Function"
extern "C" __declspec(dllexport) void dbglogtest(
    pInstData *opaque, double t, uData *data) {
  double  In  = data[0].d;   // input
  double &Out = data[1].d;   // output

  if (!*opaque) {
    *opaque = new InstData;
    if (!*opaque) {
      // terminate with prejudice
      display(
          "*** Unable to allocate memory in dbglog.cpp.  Terminating "
          "simulation.\n");
      exit(1);
    }
    LOGT("Evaluation Function called, per-instance data initialized.");
  }

  InstData *inst = *opaque;

  Out = In * 2.0;

  LOGT("Evaluation Function called.");
}

extern "C" __declspec(dllexport) double MaxExtStepSize(InstData *inst) {

  LOG("MaxExtStepSize() called.");

  return 1e308;
}

extern "C" __declspec(dllexport) void Trunc(
    InstData *inst, double t, uData *data, double *timestep) {

  LOGT("Trunc() called with *timestep=%0.16lf", *timestep);
}

extern "C" __declspec(dllexport) void Destroy(InstData *inst) { delete inst; }
