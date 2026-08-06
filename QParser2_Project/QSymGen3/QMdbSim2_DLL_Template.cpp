//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/* This is the template used by QSymGen3 to generate DLL code.
 * Revision Date:  2026.07.04
 */
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
__EVAL_FUNC_NAME__(pInstData *opaque, double t, uData *data) {
  __UDATA_CODE_SNIPPET__

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
    // Display("DevPartName=%s, DevPgmPath=%s, CfgIniPath=%s\n", DevPartName,
    //        DevPgmPath, CfgIniPath);

    *opaque = inst = new InstData();
    inst->pt.start(PT_QsSim);

    __QSymGen3_Code_Snippet__

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
    if (!inst->mdb.setVDD(inst->vddName.c_str(), vddRef)) {
      SimError(inst);
      return;
    }

    // start JVM, load program, connect simulator, cache pin objects
    inst->pt.start(PT_Phase2);
    if (!inst->mdb.startSim(inst->deviceName.c_str(), DevPgmPath)) {
      SimError(inst);
      inst->pt.stop(PT_Phase2);
      return;
    }
    inst->pt.stop(PT_Phase2);

    Display("%s / QMdbCS %s -- MDB simulator ready:\n  Device:  \"%s\"\n  "
            "Program: \"%s\"\n  VDD:     %.2fV\n",
            inst->mdb.getVerInfo(), inst->mdb.getQMdbCSVersion().c_str(),
            DevPartName, DevPgmPath, inst->mdb.getVDD());

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
  }

  /***********************************
   *** evaluation code begins here ***
   ***********************************/

  // hold off clocking while QSpice is initializing or in Trunc()
  // (don't use Trun() unless you really understand the code!)
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

  // step MDB simulation by one machine instruction
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
 * MaxExtStepSize() -- used here to prematurely terminate simulation
 * gracefully
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
