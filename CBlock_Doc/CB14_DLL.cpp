//------------------------------------------------------------------------------
// CB14_Dll.cpp -- Example code for C-Block Basics #14.
//------------------------------------------------------------------------------
// Full project code and documentation is available here:
//    https://github.com/robdunn4/QSpice
//
// This code compiles successfully using the Digital Mars C++ Compiler:
//    dmc -mn -WD cb14_dll.cpp kernel32.lib
//

extern "C" __declspec(dllexport) void (*Display)(const char *format, ...) = 0;

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
