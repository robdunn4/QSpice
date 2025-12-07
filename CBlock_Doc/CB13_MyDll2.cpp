//------------------------------------------------------------------------------
// CB13_MyDll2.cpp -- Example code for C-Block Basics #13.
//------------------------------------------------------------------------------
// Full project code and documentation is available here:
//    https://github.com/robdunn4/QSpice
//
// This code compiles successfully using the Digital Mars C++ Compiler:
//    dmc -mn -WD cb13_mydll2.cpp kernel32.lib
//

#include <malloc.h>
#include <windows.h>

extern "C" __declspec(dllexport) void (*Display)(const char *format, ...) = 0;
extern "C" __declspec(dllexport) void (*EXIT)(const char *format, ...)    = 0;

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
  double *pSharedTimeStep;
  double *pSharedInVal;
  double *pSharedOutVal;
};

extern "C" __declspec(dllexport) void cb13_mydll2(
    InstData **opaque, double t, uData *data) {
  double  In       = data[0].d;   // input
  double &Out      = data[1].d;   // output
  double &SharedIn = data[2].d;   // output
  double &TDiff    = data[3].d;   // output

  InstData *inst = *opaque;
  if (!inst) {
    inst = *opaque = (InstData *)calloc(1, sizeof(InstData));

    // get pointers to shared variables in mydll1.dll
    HINSTANCE handle      = LoadLibrary("cb13_mydll1.dll");
    inst->pSharedTimeStep = (double *)GetProcAddress(handle, "sharedTimeStep");
    inst->pSharedInVal    = (double *)GetProcAddress(handle, "sharedInVal");
    inst->pSharedOutVal   = (double *)GetProcAddress(handle, "sharedOutVal");

    if (!handle || !inst->pSharedTimeStep || !inst->pSharedInVal ||
        !inst->pSharedOutVal) {
      EXIT("mydll2 error: could not get shared variable addresses\n");
    }
  }

  // Implement module evaluation code here:
  double sharedTimeStep = *inst->pSharedTimeStep;
  double sharedInVal    = *inst->pSharedInVal;
  double sharedOutVal   = *inst->pSharedOutVal;

  // check evaluation order (mydll1 should be called/evaluated before mydll2)
  static bool firstTime = true;
  if (firstTime && t > 0) {
    firstTime = false;
    if (t != sharedTimeStep)
      Display("MYDLL2 WARNING: sharedTimeStep=%g != t=%g -- Check netlist!\n",
          sharedTimeStep, t);
  }

  SharedIn = sharedInVal;
  Out      = In;
  TDiff    = t - sharedTimeStep;
}

extern "C" __declspec(dllexport) void Destroy(InstData *inst) { free(inst); }
