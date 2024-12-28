// Automatically generated C++ file on Mon Sep  2 00:56:05 2024
//
// To build with Digital Mars C++ Compiler:
//
//    dmc -mn -WD pid_controller.cpp kernel32.lib

#include <malloc.h>

extern "C" __declspec(dllexport) int (*Display)(const char *format, ...) = 0; // works like printf()
extern "C" __declspec(dllexport) const double *DegreesC                  = 0; // pointer to current circuit temperature

union uData
{
   bool b;
   char c;
   unsigned char uc;
   short s;
   unsigned short us;
   int i;
   unsigned int ui;
   float f;
   double d;
   long long int i64;
   unsigned long long int ui64;
   char *str;
   unsigned char *bytes;
};

// int DllMain() must exist and return 1 for a process to load the .DLL
// See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain for more information.
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) { return 1; }

void bzero(void *ptr, unsigned int count)
{
   unsigned char *first = (unsigned char *) ptr;
   unsigned char *last  = first + count;
   while(first < last)
      *first++ = '\0';
}

// #undef pin names lest they collide with names in any header file(s) you might include.
#undef setpt
#undef ctrl
#undef clk
#undef fb

struct sPID_CONTROLLER
{
  // declare the structure here
  bool   clk_n1;      // clk[n-1]
  double error_n1;    // error[n-1]
  double errorI_n1;   // errorI[n-1]
  double errorD_n1;   // errorD[n-1]
  double lastT;
};

extern "C" __declspec(dllexport) void pid_controller(struct sPID_CONTROLLER **opaque, double t, union uData *data)
{
   double  setpt = data[0].d; // input
   bool    clk   = data[1].b; // input
   double  fb    = data[2].d; // input
   double  Kp    = data[3].d; // input parameter
   double  Ki    = data[4].d; // input parameter
   double  Kd    = data[5].d; // input parameter
   double  Kv    = data[6].d; // input parameter
   bool    Itype = data[7].b; // input parameter
   double &ctrl  = data[8].d; // output

   if(!*opaque)
   {
      *opaque = (struct sPID_CONTROLLER *) malloc(sizeof(struct sPID_CONTROLLER));
      bzero(*opaque, sizeof(struct sPID_CONTROLLER));

      Display("pid_controller: Kp=%f, Ki=%f, Kd=%f, Kv=%f, Integration Method=%s\n",Kp, Ki, Kd, Kv, Itype ? "Trapezoidal" : "Rectangular");
   }
   struct sPID_CONTROLLER *inst = *opaque;

// Implement module evaluation code here:
  if (clk && !inst->clk_n1)   // rising edge
  {
    // time between samples : calculate Tsampling
    double Tsampling = t - inst->lastT;   // Tsampling = current time - last rising edge time
    inst->lastT      = t;

    // calculate error
    double error = Kv * (setpt - fb);

    // calculate proportional, integral and derivative error
    double errorP = error;

    double errorI;
    if (Itype)   // trapezoidal rule
      errorI = (error + inst->error_n1) * Tsampling / 2 + inst->errorI_n1;
    else   // rectangular method
      errorI = error * Tsampling + inst->errorI_n1;

    double errorD = (error - inst->error_n1) / Tsampling;   // matlab : matched
    //double errorD = (error - inst->error_n1) / Tsampling * 2 - inst->errorD_n1;   // matlab : tustin
    //double errorD = 50*(error - inst->error_n1) + (1-50*Tsampling)*inst->errorD_n1;   // Forward Euler
    //double errorD = 1/(1+1e6*Tsampling)*(1e6*(error - inst->error_n1) + inst->errorD_n1);  // Backward Euler
    //double errorD = (1/(1+100*Tsampling/2)*(100*(error - inst->error_n1)) + (1-100*Tsampling/2)*inst->errorD_n1);  // Trapezoidal Derivative

    // PID formula
    ctrl = Kp * errorP + Ki * errorI + Kd * errorD;

    // store [n-1] sampling
    inst->error_n1  = error;    // error[n-1] = error[n]
    inst->errorI_n1 = errorI;   // errorI[n-1] = errorI[n]
    inst->errorD_n1 = errorD;   // errorD[n-1] = errorD[n]
  }

  // save clock state
  inst->clk_n1 = clk;
}

extern "C" __declspec(dllexport) double MaxExtStepSize(struct sPID_CONTROLLER *inst)
{
   return 1e308; // implement a good choice of max timestep size that depends on struct sPID_CONTROLLER
}

extern "C" __declspec(dllexport) void Trunc(struct sPID_CONTROLLER *inst, double t, union uData *data, double *timestep)
{ // limit the timestep to a tolerance if the circuit causes a change in struct sPID_CONTROLLER
   const double ttol = 10e-12; // 10ps default tolerance
   if(*timestep > ttol)
   {
      struct sPID_CONTROLLER tmp = *inst;
      pid_controller(&(&tmp), t, data);

      if(tmp.clk_n1 != inst->clk_n1 & !inst->clk_n1)  // clock rising edge reduce timestep to TTOL
         *timestep = ttol;
   }
}

extern "C" __declspec(dllexport) void Destroy(struct sPID_CONTROLLER *inst)
{
   free(inst);
}
