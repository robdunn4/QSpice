//------------------------------------------------------------------------------
// This file is part of the QMdbSim project, a Microchip Simulator framework for
// QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/* Notes:
 *
 * This is the QSpice component code for a Microchip PIC16F15213 device.  If you
 * want to create a new Microchip device component, this is the only code that
 * you need to change (at least, in theory).
 */
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
  explicit InstData() {}
  ~InstData() { mdb.stopSim(); }

  MdbSim mdb;
  bool   lastClkState = 0;
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
  double      VDD        = data[0].d;   // input
  double      RA0_I      = data[1].d;   // input
  double      RA1_I      = data[2].d;   // input
  double      RA2_I      = data[3].d;   // input
  double      RA3_I      = data[4].d;   // input
  double      RA4_I      = data[5].d;   // input
  double      RA5_I      = data[6].d;   // input
  bool        CLK        = data[7].b;   // input
  const char* MdbSimPath = data[8].str; // input parameter
  const char* McPgm      = data[9].str; // input parameter
  double&     RA0_O      = data[10].d;  // output
  bool&       RA0_C      = data[11].b;  // output
  double&     RA1_O      = data[12].d;  // output
  double&     RA2_O      = data[13].d;  // output
  double&     RA4_O      = data[14].d;  // output
  double&     RA5_O      = data[15].d;  // output
  bool&       RA1_C      = data[16].b;  // output
  bool&       RA2_C      = data[17].b;  // output
  bool&       RA4_C      = data[18].b;  // output
  bool&       RA5_C      = data[19].b;  // output

  // if gAbortFlg is set, we're just waiting on MaxExtStepSize() to force abort
  if (gAbortFlg) return;

  pInstData inst = *opaque;

  // per-instance initialization section
  if (!inst)
  {
    // allocate instance -- error handling omitted...
    *opaque = inst = new InstData();

    // add all pin/port/name mappings to list
    inst->mdb.addPinPortMap("RA0", &RA0_I, &RA0_O, &RA0_C);
    inst->mdb.addPinPortMap("RA1", &RA1_I, &RA1_O, &RA1_C);
    inst->mdb.addPinPortMap("RA2", &RA2_I, &RA2_O, &RA2_C);
    inst->mdb.addPinPortMap("RA3", &RA3_I);
    inst->mdb.addPinPortMap("RA4", &RA4_I, &RA4_O, &RA4_C);
    inst->mdb.addPinPortMap("RA5", &RA5_I, &RA5_O, &RA5_C);

    // set user-supplied path to MDB simulator (only first component instance)
    if (!gMdbSimPath) gMdbSimPath = MdbSimPath;

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

    // set device VDD -- note that we're doing this only once and any changes
    // to QSpice VDD don't get passed to MDB (so no brown-out detection support
    // even if simulator supports it)
    if (!inst->mdb.setVDD(VDD))
    {
      SimError(inst);
      return;
    }

    // get initial pin states from MDB
    if (!inst->mdb.getPinStates())
    {
      SimError(inst);
      return;
    }

    // set I/O direction of tri-state ports (QSpice side)
    inst->mdb.setCtrlPorts();

    // set initial output port states (QSpice side)
    inst->mdb.setOutPorts();
  } // end of instance initialization

  /*
   * evaluation code begins here...
   */

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

  //  set input pin states in MDB simulator from QSpice ports
  if (!inst->mdb.setInPins())
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

  // refresh pin states from MDB
  if (!inst->mdb.getPinStates())
  {
    SimError(inst);
    return;
  }

  // set component tri-state port config from MDB
  inst->mdb.setCtrlPorts();

  // set output ports from MDB states
  inst->mdb.setOutPorts();
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
