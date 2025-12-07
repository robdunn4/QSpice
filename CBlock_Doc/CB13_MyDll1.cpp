//------------------------------------------------------------------------------
// CB13_MyDll1.cpp -- Example code for C-Block Basics #13.
//------------------------------------------------------------------------------
// Full project code and documentation is available here:
//    https://github.com/robdunn4/QSpice
//
// This code compiles successfully using the Digital Mars C++ Compiler:
//    dmc -mn -WD cb13_mydll1.cpp kernel32.lib
//

#include <malloc.h>

extern "C" __declspec(dllexport) void (*Display)(const char *format, ...) = 0;
extern "C" __declspec(dllexport) void (*EXIT)(const char *format, ...)    = 0;

// variables shared with mydll2
extern "C" __declspec(dllexport) double sharedTimeStep = 0.0;
extern "C" __declspec(dllexport) double sharedInVal    = 0.0;
extern "C" __declspec(dllexport) double sharedOutVal   = 0.0;

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

// #undef pin names lest they collide with names in any header file(s) you might
// include.
#undef In
#undef Out

struct InstData {
  // int ctr;
};

extern "C" __declspec(dllexport) void cb13_mydll1(
    InstData **opaque, double t, uData *data) {
  double  In  = data[0].d;   // input
  double &Out = data[1].d;   // output

  InstData *inst = *opaque;
  if (!inst) { inst = *opaque = (InstData *)calloc(1, sizeof(InstData)); }

  // Implement module evaluation code here:
  Out = In;

  sharedTimeStep = t;
  sharedInVal    = In;
  sharedOutVal   = Out;
}

extern "C" __declspec(dllexport) void Destroy(InstData *inst) { free(inst); }
