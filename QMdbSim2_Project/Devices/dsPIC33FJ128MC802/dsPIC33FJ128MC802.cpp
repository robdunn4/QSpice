//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/*
 * This template is used by QSymGen3 to generate DLL code.
 * Revision Date:  2026.07.25
 */
// see PerfTimer.h...
#define PERFTIMER_ENABLED
// #define PERFTIMER_DISABLED

#include <MdbConfig.h>
#include <MdbSim.h>
#include <PerfTimer.h>
#include <format>
#include <functional>
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
 * Java stdout/stderr line handlers -- forwards lines to Display() instead of
 * allowing Java to write to stdout (which isn't handled well in the timer-based
 * QSpice stdout servicing); could also be used to filter "noise" from simulator
 * output
 *
 * Note:  These functions probably belong in the shared code library rather than
 * in this device-specific template code; currently here in case we find
 * something in the simulator that is specific to a given device
 *
 * Note:  We could enable/disable suppressing simulator output at runtime using
 * QMdbCfg.ini or a per-instance attribute flag in device symbol; do we need
 * that flexibility?
 */
void HandleJavaStdoutLine(pInstData inst, const std::string &line) {
  if (line.empty()) return;
  Display("%s\n", line.c_str());
}

void HandleJavaStderrLine(pInstData inst, const std::string &line) {
  if (line.empty()) return;
  Display("%s\n", line.c_str());
}

/*
 * QSpice evaluation function
 */
extern "C" __declspec(dllexport) void
dspic33fj128mc802(pInstData *opaque, double t, uData *data) {
     double       VDD         = data[ 0].d  ; // input
     double       AVDD        = data[ 1].d  ; // input
     double       AVSS        = data[ 2].d  ; // input
     double       MCLR        = data[ 3].d  ; // input
     double       RA0_I       = data[ 4].d  ; // input
     double       RA1_I       = data[ 5].d  ; // input
     double       RA2_I       = data[ 6].d  ; // input
     double       RA3_I       = data[ 7].d  ; // input
     double       RA4_I       = data[ 8].d  ; // input
     double       RB0_I       = data[ 9].d  ; // input
     double       RB1_I       = data[10].d  ; // input
     double       RB2_I       = data[11].d  ; // input
     double       RB3_I       = data[12].d  ; // input
     bool         SIMCLK      = data[13].b  ; // input
     double       RB4_I       = data[14].d  ; // input
     double       RB5_I       = data[15].d  ; // input
     double       RB6_I       = data[16].d  ; // input
     double       RB7_I       = data[17].d  ; // input
     double       RB8_I       = data[18].d  ; // input
     double       RB9_I       = data[19].d  ; // input
     double       RB10_I      = data[20].d  ; // input
     double       RB11_I      = data[21].d  ; // input
     double       RB12_I      = data[22].d  ; // input
     double       RB13_I      = data[23].d  ; // input
     double       RB14_I      = data[24].d  ; // input
     double       RB15_I      = data[25].d  ; // input
     double       VCAP        = data[26].d  ; // input
     const char * DevPartName = data[27].str; // input parameter
     const char * DevPgmPath  = data[28].str; // input parameter
     const char * CfgIniPath  = data[29].str; // input parameter
     double      &RA0_O       = data[30].d  ; // output
     bool        &RA0_C       = data[31].b  ; // output
     double      &RA1_O       = data[32].d  ; // output
     bool        &RA1_C       = data[33].b  ; // output
     double      &RA2_O       = data[34].d  ; // output
     bool        &RA2_C       = data[35].b  ; // output
     double      &RA3_O       = data[36].d  ; // output
     bool        &RA3_C       = data[37].b  ; // output
     double      &RA4_O       = data[38].d  ; // output
     bool        &RA4_C       = data[39].b  ; // output
     double      &RB0_O       = data[40].d  ; // output
     bool        &RB0_C       = data[41].b  ; // output
     double      &RB1_O       = data[42].d  ; // output
     bool        &RB1_C       = data[43].b  ; // output
     double      &RB2_O       = data[44].d  ; // output
     bool        &RB2_C       = data[45].b  ; // output
     double      &RB3_O       = data[46].d  ; // output
     bool        &RB3_C       = data[47].b  ; // output
     double      &RB4_O       = data[48].d  ; // output
     bool        &RB4_C       = data[49].b  ; // output
     double      &RB5_O       = data[50].d  ; // output
     bool        &RB5_C       = data[51].b  ; // output
     double      &RB6_O       = data[52].d  ; // output
     bool        &RB6_C       = data[53].b  ; // output
     double      &RB7_O       = data[54].d  ; // output
     bool        &RB7_C       = data[55].b  ; // output
     double      &RB8_O       = data[56].d  ; // output
     bool        &RB8_C       = data[57].b  ; // output
     double      &RB9_O       = data[58].d  ; // output
     bool        &RB9_C       = data[59].b  ; // output
     double      &RB10_O      = data[60].d  ; // output
     bool        &RB10_C      = data[61].b  ; // output
     double      &RB11_O      = data[62].d  ; // output
     bool        &RB11_C      = data[63].b  ; // output
     double      &RB12_O      = data[64].d  ; // output
     bool        &RB12_C      = data[65].b  ; // output
     double      &RB13_O      = data[66].d  ; // output
     bool        &RB13_C      = data[67].b  ; // output
     double      &RB14_O      = data[68].d  ; // output
     bool        &RB14_C      = data[69].b  ; // output
     double      &RB15_O      = data[70].d  ; // output
     bool        &RB15_C      = data[71].b  ; // output
  
  double &vddRef = VDD;

  // if gAbortFlg is set, we're just waiting on MaxExtStepSize() to force abort
  if (gAbortFlg) return;

  pInstData inst = *opaque;

  // ── Phase 1: allocate instance and configure paths (first call only) ──
  // Done immediately regardless of VDD or HoldICs state.  Does not start
  // the simulator -- that requires a valid VDD which QSpice may not have
  // provided yet.
  if (!inst) {
    inst = *opaque = new InstData();
    inst->pt.start(PT_QsSim);

    /*** Begin QSymGen3 Code Snippet ***/
    // resolves QSpice-downcased device name to canonical case for MDBCS
    inst->deviceName = [DevPartName]() -> std::string {
        static constexpr std::string_view deviceNames[] = {
            "dsPIC33FJ128MC802",
            "dsPIC33FJ128MC202",
            "dsPIC33FJ64MC802",
            "dsPIC33FJ64MC202",
            "dsPIC33FJ32MC302",
        };
        for (const auto &name : deviceNames)
            if (_stricmp(DevPartName, name.data()) == 0) return std::string(name);
        Display(std::format("Unknown device \"{}\"; check P record in pin defs.", DevPartName).c_str());
        gAbortFlg = true;
        return "";
    }();
    
    // register pin/port/name mappings
    inst->vddName = "VDD";
    inst->mdb.addPinPortMap("AVDD", &AVDD);
    inst->mdb.addPinPortMap("AVSS", &AVSS);
    inst->mdb.addPinPortMap("MCLR", &MCLR);
    inst->mdb.addPinPortMap("RA0", &RA0_I, &RA0_O, &RA0_C);
    inst->mdb.addPinPortMap("RA1", &RA1_I, &RA1_O, &RA1_C);
    inst->mdb.addPinPortMap("RA2", &RA2_I, &RA2_O, &RA2_C);
    inst->mdb.addPinPortMap("RA3", &RA3_I, &RA3_O, &RA3_C);
    inst->mdb.addPinPortMap("RA4", &RA4_I, &RA4_O, &RA4_C);
    inst->mdb.addPinPortMap("RB0", &RB0_I, &RB0_O, &RB0_C);
    inst->mdb.addPinPortMap("RB1", &RB1_I, &RB1_O, &RB1_C);
    inst->mdb.addPinPortMap("RB2", &RB2_I, &RB2_O, &RB2_C);
    inst->mdb.addPinPortMap("RB3", &RB3_I, &RB3_O, &RB3_C);
    inst->mdb.addPinPortMap("RB4", &RB4_I, &RB4_O, &RB4_C);
    inst->mdb.addPinPortMap("RB5", &RB5_I, &RB5_O, &RB5_C);
    inst->mdb.addPinPortMap("RB6", &RB6_I, &RB6_O, &RB6_C);
    inst->mdb.addPinPortMap("RB7", &RB7_I, &RB7_O, &RB7_C);
    inst->mdb.addPinPortMap("RB8", &RB8_I, &RB8_O, &RB8_C);
    inst->mdb.addPinPortMap("RB9", &RB9_I, &RB9_O, &RB9_C);
    inst->mdb.addPinPortMap("RB10", &RB10_I, &RB10_O, &RB10_C);
    inst->mdb.addPinPortMap("RB11", &RB11_I, &RB11_O, &RB11_C);
    inst->mdb.addPinPortMap("RB12", &RB12_I, &RB12_O, &RB12_C);
    inst->mdb.addPinPortMap("RB13", &RB13_I, &RB13_O, &RB13_C);
    inst->mdb.addPinPortMap("RB14", &RB14_I, &RB14_O, &RB14_C);
    inst->mdb.addPinPortMap("RB15", &RB15_I, &RB15_O, &RB15_C);
    inst->mdb.addPinPortMap("VCAP", &VCAP);
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
    if (!inst->mdb.setVDD(inst->vddName.c_str(), vddRef)) {
      SimError(inst);
      return;
    }

    // bind handlers and instance pointer to functions callable from Java
    // callback
    MdbSim::StdoutLineHandler stdoutHandler =
        std::bind(&HandleJavaStdoutLine, inst, std::placeholders::_1);
    MdbSim::StdoutLineHandler stderrHandler =
        std::bind(&HandleJavaStderrLine, inst, std::placeholders::_1);

    // start JVM, load program, connect simulator, cache pin objects
    inst->pt.start(PT_Phase2);
    if (!inst->mdb.startSim(inst->deviceName.c_str(), DevPgmPath, stdoutHandler,
                            stderrHandler)) {
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
 * Destroy() -- release per-instance data
 */
extern "C" __declspec(dllexport) void Destroy(pInstData inst) {
  inst->pt.stop(PT_QsSim);

  // print performance timer info if enabled
  std::ostringstream oss;
  inst->pt.report(oss);
  if (!oss.str().empty()) Display("%s", oss.str().c_str());

  delete inst;
} // end of Destroy()
