// Automatically generated C++ file on Tue Dec 16 15:12:38 2025
//
// To build with Digital Mars C++ Compiler:
//
//    dmc -mn -WD cb14_dll.cpp kernel32.lib

extern "C" __declspec(dllexport) void (*Display)(const char *format, ...) = 0;

// clang-format off
// extern "C" __declspec(dllexport) void (*EXIT)(const char *format, ...)          = 0; // print message like printf() but exit(0) afterward
// extern "C" __declspec(dllexport) const double *DegreesC                         = 0; // pointer to current circuit temperature
// extern "C" __declspec(dllexport) const int *StepNumber                          = 0; // pointer to current step number
// extern "C" __declspec(dllexport) const int *NumberSteps                         = 0; // pointer to estimated number of steps
// extern "C" __declspec(dllexport) const char* const *InstanceName                = 0; // pointer to address of instance name
// extern "C" __declspec(dllexport) const char *QUX                                = 0; // path to QUX.exe
// extern "C" __declspec(dllexport) const bool *ForKeeps                           = 0; // pointer to whether being evaluated non-hypothetically
// extern "C" __declspec(dllexport) const bool *HoldICs                            = 0; // pointer to whether instance initial conditions are being held
// extern "C" __declspec(dllexport) const void *GUI_HWND                           = 0; // pointer to Window handle of QUX.exe
// extern "C" __declspec(dllexport) const double *CKTtime                          = 0;
// extern "C" __declspec(dllexport) const double *CKTdelta                         = 0;
// extern "C" __declspec(dllexport) const int *IntegrationOrder                    = 0;
// extern "C" __declspec(dllexport) const char *InstallDirectory                   = 0;
// extern "C" __declspec(dllexport) double (*EngAtof)(const char **string)         = 0;
// extern "C" __declspec(dllexport) const char *(*BinaryFormat)(unsigned int data)                          = 0; // BinaryFormat(0x1C) returns "0b00011100"
// extern "C" __declspec(dllexport) const char *(*EngFormat   )(double x, const char *units, int numDgts)   = 0; // EngFormat(1e-6, "s", 6) returns "1�s"
// extern "C" __declspec(dllexport) int (*DFFT)(struct sComplex *u, bool inv, unsigned int N, double scale) = 0; // Discrete Fast Fourier Transform
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

extern "C" __declspec(dllexport) void cb14_dll(
    void **opaque, double t, union uData *data) {
  // double      In      = data[0].d;     // input
  // bool        bEnable = data[1].b;     // input parameter
  // int         iGain   = data[2].i;     // input parameter
  // const char *sVal    = data[3].str;   // input parameter
  // double      fVal    = data[4].d;     // input parameter
  // double     &Out     = data[5].d;     // output
  double      In     = data[0].d;     // input
  bool        Enable = data[1].b;     // input parameter
  int         Gain   = data[2].i;     // input parameter
  const char *StrVal = data[3].str;   // input parameter
  double      FltVal = data[4].d;     // input parameter
  double     &Out    = data[5].d;     // output

  // Implement module evaluation code here:
  Out = In * (Enable ? Gain : 0);

  // limit msgs to Output window...
  static int ctr = 0;
  if (t && ctr++ < 5)
    Display("t=%g, In=%g, Out=%g, Enable=%i, Gain=%i, StrVal=%s, FltVal=%f\n",
        t, In, Out, Enable, Gain, StrVal, FltVal);
}
