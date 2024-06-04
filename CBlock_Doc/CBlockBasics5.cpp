//==============================================================================
// CBlockBasics5.cpp -- Component with internal clock reference.
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

// per-instance data
struct InstData {
  // input parameters
  double ttol;   // time tolerance
  double freq;   // clock frequency (Hz)

  // clock management
  double       next_t;    // next clock tick simulation clock time
  double       incr_t;    // simulation time step increment to next clock tick
  unsigned int tickCnt;   // tick counter
};

// calculate simulation clock time for a clock tick with minimal rounding error.
inline double calcTickTime(InstData *inst) {
  return inst->tickCnt / inst->freq;
}

extern "C" __declspec(dllexport) void cblockbasics5(
    InstData **opaque, double t, uData *data) {
  const double &TTOL = data[0].d;   // input parameter
  const double &FREQ = data[1].d;   // input parameter
  double       &Vout = data[2].d;   // output

  InstData *inst = *opaque;

  // initialization code section
  if (!inst) {
    // allocate & zero per-instance data
    inst = *opaque = (InstData *)calloc(sizeof(InstData), 1);

    // initialize instance data fields
    inst->ttol    = TTOL;   // note: no error checking...
    inst->freq    = FREQ;   // note: no error checking...
    inst->tickCnt = 0;
    inst->incr_t = inst->next_t = calcTickTime(inst);

    // initialize output port(s)
    Vout = 1;   // high puts first cycle low
  }

  // default next step increment...
  inst->incr_t = inst->next_t - t;

  // clock state change triggered
  if (t >= inst->next_t) {
    Vout = !Vout;
    inst->tickCnt++;
    inst->next_t = calcTickTime(inst);
    inst->incr_t = inst->ttol;
  }
}

extern "C" __declspec(dllexport) double MaxExtStepSize(InstData *inst) {
  const double abortSim = -1e308;   // return abortSim to terminate simulation
  const double forever  = 1e308;    // return forever to not limit timestep size

  if (inst->incr_t <= 0) return forever;
  return inst->incr_t;
}

extern "C" __declspec(dllexport) void Destroy(InstData *inst) { free(inst); }

int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}
//==============================================================================
// End of CBlockBasics5.cpp
//==============================================================================
