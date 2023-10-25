// Automatically generated C++ file on Thu Oct 12 14:38:07 2023
//
// To build with Digital Mars C++ Compiler:
//
//    dmc -mn -WD evilamp.cpp kernel32.lib

#include <stdio.h>
#include <malloc.h>
#include <stdarg.h>
#include <time.h>

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

void display(const char *fmt, ...) {   // for diagnostic print statements
  msleep(30);
  fflush(stdout);
  va_list args = {0};
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  fflush(stdout);
  msleep(30);
}

void bzero(void *ptr, unsigned int count) {
  unsigned char *first = (unsigned char *)ptr;
  unsigned char *last  = first + count;
  while (first < last) *first++ = '\0';
}

// #undef pin names lest they collide with names in any header file(s) you might
// include.
#undef In
#undef Out

struct sEVILAMP {
  // declare the structure here
};

extern "C" __declspec(dllexport) void evilamp(
    struct sEVILAMP **opaque, double t, union uData *data) {
  double  In   = data[0].d;   // input
  int     Gain = data[1].i;   // input parameter
  double &Out  = data[2].d;   // output

  if (!*opaque) {
    *opaque = (struct sEVILAMP *)malloc(sizeof(struct sEVILAMP));
    bzero(*opaque, sizeof(struct sEVILAMP));
  }
  struct sEVILAMP *inst = *opaque;

  // Implement module evaluation code here:
  Out = In * Gain;
}

extern "C" __declspec(dllexport) double MaxExtStepSize(struct sEVILAMP *inst) {
  return 1e308;   // implement a good choice of max timestep size that depends
                  // on struct sEVILAMP
}

extern "C" __declspec(dllexport) void Trunc(struct sEVILAMP *inst, double t,
    union uData *data,
    double *timestep) {   // limit the timestep to a tolerance if the circuit
                          // causes a change in struct sEVILAMP
  const double ttol = 1e-9;
  if (*timestep > ttol) {
    double &Out = data[2].d;   // output

    // Save output vector
    const double _Out = Out;

    struct sEVILAMP tmp = *inst;
    evilamp(&(&tmp), t, data);
    // if(tmp != *inst) // implement a meaningful way to detect if the state has
    // changed
    //    *timestep = ttol;

    // Restore output vector
    Out = _Out;
  }
}

extern "C" __declspec(dllexport) void Destroy(struct sEVILAMP *inst) {
  free(inst);
}
