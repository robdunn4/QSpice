//==============================================================================
// CBlockBasics4.cpp -- Example code for CBlocksBasics4.
//==============================================================================
#include <malloc.h>

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

// #undef pin names lest they collide with names in any header file(s) you might
// include.
#undef Vin
#undef Vref
#undef Vout
#undef Vcc

// per-instance data
struct InstData {
  double ttol;   // time tolerance
};

// The "Evaluation Function" (required)
extern "C" __declspec(dllexport) void cblockbasics4(
    InstData **opaque, double t, uData *data) {
  double  Vin  = data[0].d;   // input
  double  Vref = data[1].d;   // input
  double  Vcc  = data[2].d;   // input
  double  TTOL = data[3].d;   // input parameter
  double &Vout = data[4].d;   // output

  InstData *inst = *opaque;

  // initialization code section
  if (!inst) {
    // allocate per-instance data
    inst = *opaque = (InstData *)calloc(sizeof(InstData), 1);

    // save TTOL from schematic attribute
    inst->ttol = TTOL;   // note:  no error checking < 0...
  }

  // implementation code section
  Vout = Vin < Vref ? 0 : Vcc;
}

// Trunc() (optional)
extern "C" __declspec(dllexport) void Trunc(
    InstData *inst, double t, uData *data, double *timestep) {

  if (!inst->ttol) return;   // do nothing if disabled

  if (*timestep > inst->ttol) {
    // Save a copy of the output vector
    double      &Vout  = data[4].d;
    const double _Vout = Vout;

    // create temporary copy of instance data
    InstData  tmp  = *inst;
    InstData *pTmp = &tmp;

    // call the evaluation function with the temporary instance data
    cblockbasics4(&pTmp, t, data);

    // implement a meaningful way to detect if the state has changed...
    if (Vout != _Vout) { *timestep = inst->ttol; }

    // Restore output vector
    Vout = _Vout;
  }
}

// Destroy() (required)
extern "C" __declspec(dllexport) void Destroy(InstData *inst) {
  // free the allocated per-instance data
  free(inst);
}

// DllMain() (required and must return 1)
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}
//==============================================================================
// End of CBlockBasics4.cpp
//==============================================================================
