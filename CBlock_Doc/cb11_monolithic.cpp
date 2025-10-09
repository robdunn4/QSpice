// Example code for C-Block Basics #11
//
// To build with Digital Mars C++ Compiler:
//
//    dmc -mn -WD cb11_monolithic.cpp kernel32.lib

#include <malloc.h>
#include <stdio.h>

// externs not used in this project have been removed
extern "C" __declspec(dllexport) void (*Display)(const char *format, ...) = 0;
extern "C" __declspec(dllexport) void (*EXIT)(const char *format, ...)    = 0;
extern "C" __declspec(dllexport) const int         *StepNumber            = 0;
extern "C" __declspec(dllexport) const char *const *InstanceName          = 0;
extern "C" __declspec(dllexport) const bool        *ForKeeps              = 0;

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
#undef IN
#undef OUT
#undef EN

/*
 * Per-instance Data
 */
struct InstData {
  FILE       *hLogFile;      // logfile name for component instance
  const char *instName;      // instance name for logging
  bool        postProcess;   // end of simulation (post-process) flag
};

/*
 * Evaluation Function ("EvalFunc()" in the documentation)
 */
extern "C" __declspec(dllexport) void cb11_monolithic(
    InstData **opaque, double t, uData *data) {
  double      IN      = data[0].d;     // input
  bool        EN      = data[1].b;     // input
  int         Gain    = data[2].i;     // input parameter
  const char *LogName = data[3].str;   // input parameter
  double     &OUT     = data[4].d;     // output

  // note:  the template generator places the following below the initialization
  // section; moved here to make it available inside the initialization code
  InstData *inst = *opaque;

  if (!inst) { /* begin one-time initialization section */
    inst = *opaque    = (InstData *)calloc(1, sizeof(InstData));
    inst->postProcess = false;

    // *InstanceName is invalid during Trunc() calls; save the pointer in case
    // we want to use it for debugging during Trunc() evaluations
    inst->instName = *InstanceName;

    // open logfile as named by user; overwrite if first step, else append
    // (error checking omitted)
    const char *fMode = *StepNumber > 1 ? "a" : "w";
    inst->hLogFile    = fopen(LogName, fMode);
    if (!inst->hLogFile) { EXIT("Error opening logfile \"%s\"\n", LogName); }
  } /* end one-time initialization section */

  // evaluation code starts here
  if (!EN) IN = 0;
  OUT = IN * Gain;

  // write data to logfile
  if (*ForKeeps)   // called directly by the simulator
    fprintf(inst->hLogFile,
        "EvalFunc(): InstName=%s, Gain=%d, EN=%d, IN=%g, OUT=%g\n",
        *InstanceName, Gain, EN, IN, OUT);
  else   // called indirectly from Trunc()
    fprintf(inst->hLogFile,
        "Trunc():    InstName=%s, Gain=%d, EN=%d, IN=%g, OUT=%g\n",
        inst->instName, Gain, EN, IN, OUT);
}

/*
 * MaxExtStepSize()
 */
extern "C" __declspec(dllexport) double MaxExtStepSize(
    InstData *inst, double t) {
  double retVal = 1e308;

  // logic that might change retVal omitted for demo
  fprintf(inst->hLogFile, "MaxStep():  InstName=%s\n", *InstanceName);
  return retVal;
}

/*
 * Trunc()
 */
extern "C" __declspec(dllexport) void Trunc(
    InstData *inst, double t, uData *data, double *timestep) {
  const double ttol = 1e-9;   // 1ns default tolerance

  if (*timestep > ttol) {
    InstData tmp = *inst;
    // cb9_monolithic(&(&tmp), t, data); // modern compilers don't allow this
    InstData *pTmp = &tmp;   // for modern compilers
    cb11_monolithic(&pTmp, t, data);

    // logic that might change *timestep value omitted for demo

    // if(tmp != *inst) // implement a meaningful way to detect if the state has
    // changed
    //    *timestep = ttol;
  }
}

/*
 * Destroy()
 */
extern "C" __declspec(dllexport) void Destroy(struct InstData *inst) {
  // log end of step message
  fprintf(inst->hLogFile, "*** End Of Step #%d InstName=%s\n", *StepNumber,
      *InstanceName);

  // log end of step message
  if (inst->postProcess)
    fprintf(
        inst->hLogFile, "*** End Of Simulation InstName=%s\n", *InstanceName);

  // close logfile...
  fflush(inst->hLogFile);
  fclose(inst->hLogFile);

  // free allocated per-instance data
  free(inst);
}

/*
 * PostProcess()
 */
extern "C" __declspec(dllexport) void PostProcess(struct InstData *inst) {
  // set end of simulation flag
  inst->postProcess = true;
}
