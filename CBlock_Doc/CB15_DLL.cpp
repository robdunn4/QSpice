// Automatically generated C++ file on Sat Mar 28 13:18:16 2026
//
// To build with Digital Mars C++ Compiler:
//
//    dmc -mn -WD -o cb15_dll.cpp kernel32.lib

#include <malloc.h>
#include <string.h>

// clang-format off
extern "C" __declspec(dllexport) void (*Display)(
    const char *format, ...) = 0;   // works like printf()
extern "C" __declspec(dllexport) void (*EXIT)(const char *format,
    ...) = 0;   // print message like printf() but exit(0) afterward
extern "C" __declspec(dllexport) const double *DegreesC =
    0;   // pointer to current circuit temperature
extern "C" __declspec(dllexport) const int *StepNumber =
    0;   // pointer to current step number
extern "C" __declspec(dllexport) const int *NumberSteps =
    0;   // pointer to estimated number of steps
extern "C" __declspec(dllexport) const char *const *InstanceName =
    0;   // pointer to address of instance name
extern "C" __declspec(dllexport) const char *QUX = 0;   // path to QUX.exe
extern "C" __declspec(dllexport) const bool *ForKeeps =
    0;   // pointer to whether being evaluated non-hypothetically
extern "C" __declspec(dllexport) const bool *HoldICs =
    0;   // pointer to whether instance initial conditions are being held
extern "C" __declspec(dllexport) const void *GUI_HWND =
    0;   // pointer to Window handle of QUX.exe
extern "C" __declspec(dllexport) const double *CKTtime                  = 0;
extern "C" __declspec(dllexport) const double *CKTdelta                 = 0;
extern "C" __declspec(dllexport) const int    *IntegrationOrder         = 0;
extern "C" __declspec(dllexport) const char   *InstallDirectory         = 0;
extern "C" __declspec(dllexport) double (*EngAtof)(const char **string) = 0;
extern "C" __declspec(dllexport) const char *(*BinaryFormat)(
    unsigned int data) = 0;   // BinaryFormat(0x1C) returns "0b00011100"
extern "C" __declspec(dllexport)
const char *(*EngFormat)(double x, const char *units,
    int numDgts) = 0;   // EngFormat(1e-6, "s", 6) returns "1�s"
extern "C" __declspec(dllexport) int (*DFFT)(struct sComplex *u, bool inv,
    unsigned int N, double scale) = 0;   // Discrete Fast Fourier Transform
extern "C"
    __declspec(dllexport) void (*bzero)(void *ptr, unsigned int count) = 0;
// clang-format on

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
#undef En

const int LP_Flag = 1;
const int HP_Flag = 2;
const int BP_Flag = LP_Flag | HP_Flag;

const char *sFilter[] = {
    "Low-Pass Filter", "High-Pass Filter", "Band-Pass Filter"};

struct InstData {
  int  lp3dB;
  int  hp3dB;
  int  filterType;
  bool useEnablePin;
};

extern "C" __declspec(dllexport) void cb15_dll(
    InstData **opaque, double t, union uData *data) {
  double      In         = data[0].d;     // input
  double      _E_n       = data[1].d;     // input
  const char *FilterType = data[2].str;   // input parameter
  bool        EnablePin  = data[3].b;     // input parameter
  int         LP3dB      = data[4].i;     // input parameter
  int         HP3dB      = data[5].i;     // input parameter
  double     &Out        = data[6].d;     // output
  InstData   *inst       = *opaque;

  if (!inst) {
    inst = *opaque = (InstData *)calloc(sizeof(InstData), 1);

    // initialize
    inst->lp3dB = LP3dB;
    inst->hp3dB = HP3dB;

    if (!strcmp(FilterType, "lp")) inst->filterType = LP_Flag;
    else if (!strcmp(FilterType, "hp"))
      inst->filterType = HP_Flag;
    else
      inst->filterType = BP_Flag;

    Display("Filter Type=%s, EnablePin=%d, LP3dB=%d, HP3dB=%d\n",
        sFilter[inst->filterType - 1], EnablePin, LP3dB, HP3dB);
  }

  // Implement module evaluation code here:
  // if using EnablePin and disabled (high), quick out
  if (EnablePin && _E_n) {
    Out = 0;
    return;
  }

  switch (inst->filterType) {
  case LP_Flag:
    // do low-pass stuff
    Out = In * In;   // just doing something for demo purposes
    break;
  case HP_Flag:
    // do high-pass stuff
    Out = In * In;   // just doing something for demo purposes
    break;
  default:
    // do band-pass stuff
    Out = In * In;   // just doing something for demo purposes
  }
}

extern "C" __declspec(dllexport) void Destroy(struct sCB15_DLL *inst) {
  free(inst);
}
