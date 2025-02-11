//------------------------------------------------------------------------------
// This file is part of the QMdbSim project, a Microchip Simulator framework for
// QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
#include "QMdbSim.h"

/*
 *  QSpice declarations/definitions
 */
extern "C" __declspec(dllexport) int (*Display)(const char* format, ...) = 0;
extern "C" __declspec(dllexport) const bool* HoldICs                     = 0;

union uData
{
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
  char*                  str;
  unsigned char*         bytes;
};

int __stdcall DllMain(void* module, unsigned int reason, void* reserved)
{
  return 1;
}

/*
 * Per-instance component data
 */
struct InstData
{
  // constructors/destructors
  explicit InstData() {}
  ~InstData() { mdb.stopSim(); }

  MdbSim mdb;

  bool lastClkState = 0;

  // pin states
  PinState RA0, RA1, RA2, /*RA3, */ RA4, RA5; // RA3 is input-only
};

// pointer typedef for convenience
typedef InstData* pInstData;

/*
 * global variables
 */
bool gAbortFlg = false; // for "graceful" early termination

/*
 * convenience function to display error and set up to abort simulation
 */
void SimError(pInstData inst)
{
  gAbortFlg = true;
  Display(inst->mdb.getLastErrMsg());
  Display("\n");
}

/*
 * QSpice evaluation function
 */
extern "C" __declspec(dllexport) void pic16f15213(
    pInstData* opaque, double t, union uData* data)
{
  double      VDD        = data[0].d;    // input
  double      RA0_I      = data[1].d;    // input
  double      RA1_I      = data[2].d;    // input
  double      RA2_I      = data[3].d;    // input
  double      RA3_I      = data[4].d;    // input
  double      RA4_I      = data[5].d;    // input
  double      RA5_I      = data[6].d;    // input
  bool        CLK        = data[7].b;    // input
  const char* ServerName = data[8].str;  // input parameter
  const char* PortNbr    = data[9].str;  // input parameter
  const char* McPgm      = data[10].str; // input parameter
  double&     RA0_O      = data[11].d;   // output
  double&     RA0_C      = data[12].d;   // output
  double&     RA1_O      = data[13].d;   // output
  double&     RA2_O      = data[14].d;   // output
  double&     RA4_O      = data[15].d;   // output
  double&     RA5_O      = data[16].d;   // output
  double&     RA1_C      = data[17].d;   // output
  double&     RA2_C      = data[18].d;   // output
  double&     RA4_C      = data[19].d;   // output
  double&     RA5_C      = data[20].d;   // output

  // if gAbortFlg is set, we're just waiting on MaxExtStepSize() to force abort
  if (gAbortFlg) return;

  pInstData inst = *opaque;

  // per-instance initialization
  if (!inst)
  {
    // allocate instance -- error handling omitted...
    *opaque = inst = new InstData();

    // display some version information
    Display(
        "%s loading MDB simulator process:\n  MDB Path: \"%s\"\n  Device:   "
        "\"%s\"\n  Program:  \"%s\"\n",
        inst->mdb.getVerInfo(), MdbSimPath, "PIC16F15213", McPgm);

    // start MDB simulator on server
    if (!inst->mdb.startSim("PIC16F15213", McPgm))
    {
      SimError(inst);
      return;
    }
    Display("MDB simulator loaded/configured successfully...\n");

    // get initial output pin states from uC
    if (!inst->mdb.getPin("RA0", inst->RA0) ||
        !inst->mdb.getPin("RA1", inst->RA1) ||
        !inst->mdb.getPin("RA2", inst->RA2) ||
        !inst->mdb.getPin("RA4", inst->RA4) ||
        !inst->mdb.getPin("RA5", inst->RA5))
    {
      SimError(inst);
      return;
    }

    // set I/O direction of tri-state ports (QSpice side)
    RA0_C = inst->RA0.isInput();
    RA1_C = inst->RA1.isInput();
    RA2_C = inst->RA2.isInput();
    RA4_C = inst->RA4.isInput();
    RA5_C = inst->RA5.isInput();

    // set initial output states
    RA0_O = inst->RA0.isHigh() ? VDD : 0;
    RA1_O = inst->RA1.isHigh() ? VDD : 0;
    RA2_O = inst->RA2.isHigh() ? VDD : 0;
    RA4_O = inst->RA4.isHigh() ? VDD : 0;
    RA5_O = inst->RA5.isHigh() ? VDD : 0;
  } // end of instance initialization

  // evaluation code begins here...

  // is QSpice initializing?
  if (*HoldICs)
  {
    inst->lastClkState = CLK;
    return;
  }

  // check for falling edge of CLK
  if (inst->lastClkState == CLK) return;
  inst->lastClkState = CLK;
  if (CLK) return;

  // have CLK falling edge so the fun begins...
  // set input pin states in MDB simulator
  if ((inst->RA0.isInput() && !inst->mdb.setPin("RA0", RA0_I)) ||
      (inst->RA1.isInput() && !inst->mdb.setPin("RA1", RA1_I)) ||
      (inst->RA2.isInput() && !inst->mdb.setPin("RA2", RA2_I)) ||
      (!inst->mdb.setPin("RA3", RA3_I)) ||
      (inst->RA4.isInput() && !inst->mdb.setPin("RA4", RA4_I)) ||
      (inst->RA5.isInput() && !inst->mdb.setPin("RA5", RA5_I)))
  {
    SimError(inst);
    return;
  }

  // step MDB simulation by one uC instruction
  if (!inst->mdb.stepInst())
  {
    SimError(inst);
    return;
  }

  // refresh pin states
  if (!inst->mdb.getPin("RA0", inst->RA0) ||
      !inst->mdb.getPin("RA1", inst->RA1) ||
      !inst->mdb.getPin("RA2", inst->RA2) ||
      !inst->mdb.getPin("RA4", inst->RA4) ||
      !inst->mdb.getPin("RA5", inst->RA5))
  {
    SimError(inst);
    return;
  }

  // set component tri-state pin config
  RA0_C = inst->RA0.isInput();
  RA1_C = inst->RA1.isInput();
  RA2_C = inst->RA2.isInput();
  RA4_C = inst->RA4.isInput();
  RA5_C = inst->RA5.isInput();

  // shouldn't matter if setting inputs, xxx_C controls component port direction
  RA0_O = inst->RA0.isHigh() ? VDD : 0;
  RA1_O = inst->RA1.isHigh() ? VDD : 0;
  RA2_O = inst->RA2.isHigh() ? VDD : 0;
  RA4_O = inst->RA4.isHigh() ? VDD : 0;
  RA5_O = inst->RA5.isHigh() ? VDD : 0;
} // end of evaluation function

/*
 * MaxExtStepSize() -- used here to prematurely terminate simulation gracefully
 */
extern "C" __declspec(dllexport) double MaxExtStepSize(pInstData inst)
{
  const double forever  = 1e308;
  const double abortSim = -1e308;

  if (gAbortFlg || inst->mdb.getErrState())
  {
    Display("Aborting simulation...\n");
    return abortSim;
  }

  return forever;
} // end of MaxExtStepSize()

/*
 * Destroy() release the per-instance data
 */
extern "C" __declspec(dllexport) void Destroy(pInstData inst)
{
  delete inst;
} // end of Destroy()
