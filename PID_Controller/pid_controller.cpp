// Automatically generated C++ file on Fri Oct  6 14:10:25 2023
//
// To build with Digital Mars C++ Compiler:
//
//    dmc -mn -WD pid_controller_fb.cpp kernel32.lib

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <cstdlib>

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

void msg(const char *fmt, ...) {   // for diagnostic print statements
  msleep(30);
  fflush(stdout);
  va_list args = {0};
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  fflush(stdout);
  msleep(30);
}

// int DllMain() must exist and return 1 for a process to load the .DLL
// See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain for more
// information.
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

// #undef pin names lest they collide with names in any header file(s) you might
// include.
#undef setpt
#undef ctrl
#undef clk
#undef fb

struct InstData {
  bool   clk_n1;      // clk[n-1]
  double error_n1;    // error[n-1]
  double errorI_n1;   // errorI[n-1]
  double lastT;

  // technically, we need initialize only non-zero data members
  InstData() : clk_n1(0.0), error_n1(0.0), errorI_n1(0.0), lastT(0.0){};
};

#define UDATA(data)                                                            \
  double  setpt = data[0].d; /* input */                                       \
  bool    clk   = data[1].b; /* input*/                                        \
  double  fb    = data[2].d; /* input*/                                        \
  double  Kp    = data[3].d; /* input parameter : proportional*/               \
  double  Ki    = data[4].d; /* input parameter : integral*/                   \
  double  Kd    = data[5].d; /* input parameter : derivative*/                 \
  double  Kv    = data[6].d; /* input parameter : overall gain*/               \
  bool    Itype = data[7].b; /* 0=rectangular; 1=trapezoidal */                \
  double &ctrl  = data[8].d; /* output*/

extern "C" __declspec(dllexport) void pid_controller(
    InstData **opaque, double t, union uData *data) {
  UDATA(data);

  if (!*opaque) {
    // allocate instance
    *opaque = new InstData;

    if (!*opaque) {
      // terminate with prejudice
      msg("pid_controller error.  Unable to allocate memory.  Terminating "
          "simulation.\n");
      std::exit(1);
    }

    msg("pid_controller: Kp=%f, Ki=%f, Kd=%f, Kv=%f, Integration Method=%s\n",
        Kp, Ki, Kd, Kv, Itype ? "Trapezoidal" : "Rectangular");
  }

  InstData *inst = *opaque;

  // Implement module evluation code here:
  if (clk && !inst->clk_n1)   // rising edge
  {
    // time between samples : calculate Tsampling
    double Tsampling =
        t - inst->lastT;   // Tsampling = current time - last rising edge time
    inst->lastT = t;

    // calculate error
    double error = Kv * (setpt - fb);

    // calculate proportional, integral and derivative error
    double errorP = error;

    double errorI;
    if (Itype)   // trapezoidal rule
      errorI = (error + inst->error_n1) * Tsampling / 2 + inst->errorI_n1;
    else   // rectangular method
      errorI = error * Tsampling + inst->errorI_n1;

    double errorD = (error - inst->error_n1) / Tsampling;

    // PID formula
    ctrl = Kp * errorP + Ki * errorI + Kd * errorD;

    // store [n-1] sampling
    inst->error_n1  = error;    // error[n-1] = error[n]
    inst->errorI_n1 = errorI;   // errorI[n-1] = errorI[n]
  }

  // save clock state
  inst->clk_n1 = clk;
}

extern "C" __declspec(dllexport) void Trunc(
    InstData *inst, double t, union uData *data, double *timestep) {
  UDATA(data);
  const double ttol = 1e-9;

  // if not rising clock edge, we're done
  if (clk == inst->clk_n1 || !clk) { return; }

  *timestep = ttol;
}

extern "C" __declspec(dllexport) void Destroy(InstData *inst) {
  // free instance memory
  delete inst;
}
