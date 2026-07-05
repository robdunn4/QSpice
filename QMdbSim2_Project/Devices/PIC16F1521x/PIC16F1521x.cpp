//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
// This is a template file used by QSymGen3 to produce QMdbSim2 Device DLL code.

// see PerfTimer.h...
#define PERFTIMER_ENABLED
// #define PERFTIMER_DISABLED

#include "MdbConfig.h"
#include "MdbSim.h"
#include "PerfTimer.h"
#include <sstream>

// some performance timer constants
const char *PT_QsSim  = "QSpice Sim Time";     // total simulation time (approx)
const char *PT_Phase1 = "MDB Phase 1 Startup"; // mostly load Java
const char *PT_Phase2 = "MDB Phase 2 Startup"; // initialize MDB for device
const char *PT_Step   = "MDB Step Calls";      // mostly main loop MDB step time

/*
 *  QSpice declarations/definitions
 */
extern "C" __declspec(dllexport) int (*Display)(const char *format, ...) = 0;
extern "C" __declspec(dllexport) const bool *HoldICs                     = 0;
extern "C" __declspec(dllexport) const bool *ForKeeps                    = 0;

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

int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

/*
 * Per-instance component data
 */
struct InstData {
  explicit InstData() {}
  ~InstData() { mdb.stopSim(); }

  MdbSim      mdb;
  std::string deviceName;
  std::string vddName;
  bool        lastClkState = 0;
  bool        simStarted   = false; // true once startSim() succeeds
  PerfTimer   pt;
};

// pointer typedef for convenience
typedef InstData *pInstData;

/*
 * global variables
 */
bool gAbortFlg = false; // for "graceful" early termination

/*
 * convenience function to display error and set up to abort simulation
 */
void SimError(pInstData inst) {
  gAbortFlg = true;
  Display(inst->mdb.getLastErrMsg());
  Display("\n");
}

/*
 * QSpice evaluation function
 */
extern "C" __declspec(dllexport) void
pic16f1521x(pInstData *opaque, double t, uData *data) {
     double       VDD         = data[ 0].d  ; // input
     double       RA0_I       = data[ 1].d  ; // input
     double       RA1_I       = data[ 2].d  ; // input
     double       RA2_I       = data[ 3].d  ; // input
     double       RA3         = data[ 4].d  ; // input
     double       RA4_I       = data[ 5].d  ; // input
     double       RA5_I       = data[ 6].d  ; // input
     bool         SIMCLK      = data[ 7].b  ; // input
     const char * DevPartName = data[ 8].str; // input parameter
     const char * DevPgmPath  = data[ 9].str; // input parameter
     const char * CfgIniPath  = data[10].str; // input parameter
     double      &RA0_O       = data[11].d  ; // output
     bool        &RA0_C       = data[12].b  ; // output
     double      &RA1_O       = data[13].d  ; // output
     bool        &RA1_C       = data[14].b  ; // output
     double      &RA2_O       = data[15].d  ; // output
     bool        &RA2_C       = data[16].b  ; // output
     double      &RA4_O       = data[17].d  ; // output
     bool        &RA4_C       = data[18].b  ; // output
     double      &RA5_O       = data[19].d  ; // output
     bool        &RA5_C       = data[20].b  ; // output
  

  // if gAbortFlg is set, we're just waiting on MaxExtStepSize() to force abort
  if (gAbortFlg) return;

  pInstData inst = *opaque;

  // ── Phase 1: allocate instance and configure paths (first call only) ──
  // Done immediately regardless of VDD or HoldICs state.  Does not start
  // the simulator -- that requires a valid VDD which QSpice may not have
  // provided yet.
  if (!inst) {
    // for debugging, let's show the passed parameters; note that the MDB
    // simulator is case-sensitive...
    Display("DevPartName=%s, DevPgmPath=%s, CfgIniPath=%s\n", DevPartName,
            DevPgmPath, CfgIniPath);

    *opaque = inst = new InstData();
    inst->pt.start(PT_QsSim);

    /*** Begin QSymGen3 Code Snippet ***/
    // resolves QSpice-downcased device name to canonical case for MDBCS
    inst->deviceName = [DevPartName]() -> std::string {
        static constexpr std::string_view deviceNames[] = {
            "PIC16F15213",
            "PIC16F15214",
        };
        for (const auto &name : deviceNames)
            if (_stricmp(DevPartName, name.data()) == 0) return std::string(name);
        Display(std::format("Unknown device \"{}\"; check P record in pin defs.", DevPartName).c_str());
        gAbortFlg = true;
        return "";
    }();
    
    // register pin/port/name mappings
    inst->vddName = "VDD";
    inst->mdb.addPinPortMap("RA0", &RA0_I, &RA0_O, &RA0_C);
    inst->mdb.addPinPortMap("RA1", &RA1_I, &RA1_O, &RA1_C);
    inst->mdb.addPinPortMap("RA2", &RA2_I, &RA2_O, &RA2_C);
    inst->mdb.addPinPortMap("RA3", &RA3);
    inst->mdb.addPinPortMap("RA4", &RA4_I, &RA4_O, &RA4_C);
    inst->mdb.addPinPortMap("RA5", &RA5_I, &RA5_O, &RA5_C);
    /*** End QSymGen3 Code Snippet ***/

        // load MdbConfig.ini from MdbSimPath ("." = DLL directory)
        MdbConfig cfg;
    if (!cfg.load(CfgIniPath)) {
      Display("MdbConfig error: %s\n", cfg.getLastErrMsg());
      SimError(inst);
      return;
    }

    // display resolved configuration paths for diagnostics
    Display("MdbConfig resolved paths:\n");
    Display("  Ini:        %s\n", cfg.getIniFile().c_str());
    Display("  MplabxRoot: %s\n", cfg.getMplabxRoot().c_str());
    Display("  JdkRoot:    %s\n", cfg.getJdkRoot().c_str());
    Display("  QMdbcsJar:  %s\n", cfg.getQMdbcsJar().c_str());
    Display("  JvmDll:     %s\n", cfg.getJvmDll().c_str());

    // build MdbSim::Config from the resolved paths and pass to MdbSim
    MdbSim::Config simCfg;
    simCfg.jvmDllPath    = cfg.getJvmDll();
    simCfg.mdbcsJar      = cfg.getQMdbcsJar();
    simCfg.mplabxRoot    = cfg.getMplabxRoot();
    simCfg.packsFolder   = cfg.getPacksFolder();
    simCfg.thirdpartyLib = cfg.getThirdparty();
    inst->pt.start(PT_Phase1);
    inst->mdb.configure(simCfg);
    inst->pt.stop(PT_Phase1);
  }

  // ── Phase 2: start simulator once QSpice VDD is valid ─────────────────
  // Deferred from Phase 1 because QSpice may supply 0V on the VDD pin
  // during early initialization calls.  VDD is read directly from the
  // QSpice pin here so the device voltage matches the actual schematic value
  // without hardcoding.
  if (!inst->simStarted) {
    if (!*ForKeeps) return; // QSpice initializing; VDD may be invalid -- wait

    // setVDD() must be called before startSim(); cannot be set/changed once
    // simulation is running; VDD name can vary by device and is set in phase 1
    // code snippet above
    if (!inst->mdb.setVDD(inst->vddName.c_str(), VDD)) {
      SimError(inst);
      return;
    }

    // start JVM, load program, connect simulator, cache pin objects
    // TBD:  Device name should come in as parameter from QSymGen2-generated
    //       device symbol user selection value
    inst->pt.start(PT_Phase2);

    // OK, here's a problem:  The device name is used to load the appropriate
    // simulator back-end.  the device name is case-sensitive in the simulator
    // lookup.  For example, "ATtiny85" will be found.  "ATTINY85" and
    // "attiny85" will not be found and will throw a rather vague error.  (Maybe
    // I can find a more specific error?)
    //
    // Unfortunately, QSpice passes the selected device from our
    // multiple-choice in lower case.  And we can't simply up-case/lower-case.
    // We need either: (1) Microchip makes device name loads case-insensitive
    // (unlikely but, if so, long wait); (2) we find a way to get a table of
    // supported devices and map to the proper case entry; (3) ask Mike E for a
    // way to pass parameters without downcasing in the netlist; or (4) find a
    // way to pass an array of proper case names from QSymGen3 pin list parsing
    // into the minimal custom code in this template and search that ourselves
    // to get the properly cased device name.  Item 4 could be done in the same
    // way as the device-specific voltaage name (VDD).
    //
    // TBD:  I used (4).  Revise documentation....

    if (!inst->mdb.startSim(inst->deviceName.c_str(), DevPgmPath)) {
      SimError(inst);
      inst->pt.stop(PT_Phase2);
      return;
    }
    inst->pt.stop(PT_Phase2);

    Display("%s / QMdbCS %s -- MDB simulator ready:\n  Device:  \"%s\"\n  "
            "Program: \"%s\"\n"
            "  VDD:     %.2fV\n",
            inst->mdb.getVerInfo(), inst->mdb.getQMdbCSVersion().c_str(),
            DevPartName, DevPgmPath, VDD);

    // get initial pin states from MDB
    if (!inst->mdb.getPinStates()) {
      SimError(inst);
      return;
    }

    // set I/O direction of tri-state ports (QSpice side)
    inst->mdb.setCtrlPorts();

    // set initial output port states (QSpice side)
    inst->mdb.setOutPorts();

    inst->simStarted = true;
  } // end if (!inst->simStarted)

  /*
   * evaluation code begins here...
   */

  // hold off clocking while QSpice is initializing or in Trunc() (don't use
  // Trun()!)
  if (*HoldICs || !*ForKeeps) {
    inst->lastClkState = SIMCLK;
    return;
  }

  // check for falling edge of CLK
  if (inst->lastClkState == SIMCLK) return;
  inst->lastClkState = SIMCLK;
  if (SIMCLK) return;

  // timer
  PerfTimer::ScopedTimer ptStep(inst->pt, PT_Step);

  // set input pin states in MDB simulator from QSpice ports
  if (!inst->mdb.setInPins()) {
    SimError(inst);
    return;
  }

  // step MDB simulation by one uC instruction
  if (!inst->mdb.stepInst()) {
    SimError(inst);
    return;
  }

  // refresh pin states from MDB
  if (!inst->mdb.getPinStates()) {
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
extern "C" __declspec(dllexport) double MaxExtStepSize(pInstData inst) {
  const double forever  = 1e308;
  const double abortSim = -1e308;

  if (gAbortFlg || inst->mdb.getErrState()) {
    Display("Aborting simulation...\n");
    return abortSim;
  }

  return forever;
} // end of MaxExtStepSize()

/*
 * Destroy() -- release the per-instance data
 */
extern "C" __declspec(dllexport) void Destroy(pInstData inst) {
  inst->pt.stop(PT_QsSim);

  std::ostringstream oss;
  inst->pt.report(oss);
  Display("%s", oss.str().c_str());

  delete inst;
} // end of Destroy()
