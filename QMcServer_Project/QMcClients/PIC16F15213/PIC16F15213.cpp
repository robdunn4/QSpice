//------------------------------------------------------------------------------
// This file is part of the QMcServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demontration code.
//------------------------------------------------------------------------------
#include "TcpClient.h"

#include <format>
#include <memory>
#include <string>

/*
 *  QSpice declarations/definitions -- most not needed in this code
 */
extern "C" __declspec(dllexport) int (*Display)(const char* format, ...) = 0;
// extern "C" __declspec(dllexport) const double*      DegreesC             = 0;
// extern "C" __declspec(dllexport) const int*         StepNumber           = 0;
// extern "C" __declspec(dllexport) const int*         NumberSteps          = 0;
extern "C" __declspec(dllexport) const char* const* InstanceName = 0;
// extern "C" __declspec(dllexport) const char*        QUX                  = 0;
// extern "C" __declspec(dllexport) const bool*        ForKeeps             = 0;
extern "C" __declspec(dllexport) const bool* HoldICs = 0;
// extern "C" __declspec(dllexport) int (*DFFT)(
//     struct sComplex* u, bool inv, unsigned int N, double scale) = 0;

/*
 * standard QSpice stuff...
 */
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

/*
 * standard QSpice stuff...
 */
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
  explicit InstData(
      const char* hostName = "localhost", const char* portNbr = "666")
  {
    pClient = std::shared_ptr<TcpClient>(new TcpClient(hostName, portNbr));
  }
  ~InstData() {}

  // pClient is a shared pointer to ensure release
  std::shared_ptr<TcpClient> pClient;

  // clock state stuff
  bool lastClkState = 0;

  // pin states
  PinState RA0, RA1, RA2;
};

// pointer typedef for convenience
typedef InstData* pInstData;

/*
 * global variables
 */
bool gAbortFlg    = false; // for "graceful" early termination
int  gInstCnt     = 0;     // instance reference counter
bool gPostProcess = false; // set in PostProcess()

const char* ServerErr =
    "Error communicating with server.  Terminating simulation.\n";

/*
 * convenience function to display error and set up to abort simulation
 */
void ServerError(pInstData inst)
{
  gAbortFlg = true;
  Display(ServerErr);
  // Display("%s\n", inst->pClient->getLastSysErrMsg());
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
  int&        ERR        = data[21].i;   // output

  // if gAbortFlg is set, we're just waiting on MaxExtStepSize() to force abort
  if (gAbortFlg) return;

  pInstData inst = *opaque;

  // per-instance initialization
  if (!inst)
  {
    Display("Connecting to server: Host=\"%s\", Port=\"%s\"...\n", ServerName,
        PortNbr);

    // allocate instance -- error handling omitted...
    *opaque = inst     = new InstData(ServerName, PortNbr);
    inst->lastClkState = CLK;

    // check for error
    if (!inst->pClient->isConnected())
    {
      ServerError(inst);
      return;
    }

    // display server version -- could test for minimum requirement
    int verMajor = 0;
    int verMinor = 0;
    int verDot   = 0;
    if (!inst->pClient->getServerVer(verMajor, verMinor, verDot))
    {
      ServerError(inst);
      return;
    }
    Display("Connected to server.  Server version %d.%d.%d\n", verMajor,
        verMinor, verDot);

    // log component instance for this connection on server (optional/useful)
    std::string msg;
    msg = std::format("Client instance={}", *InstanceName);
    if (!inst->pClient->logMsg(msg.c_str()))
    {
      ServerError(inst);
      return;
    }

    // show current CWD in server log before changing (optional)
    if (!inst->pClient->setCwd("."))
    {
      Display("Unable to set CWD.  Terminating simulation...\n");
      gAbortFlg = true;
    }

    // set working directory to simulation CWD
    if (!inst->pClient->setCwd())
    {
      Display("Unable to set CWD.  Terminating simulation...\n");
      gAbortFlg = true;
    }

    // increment instance reference counter
    gInstCnt++;

    // start MDB simulator on server
    bool res = inst->pClient->startSim("MDB", "PIC16F15213", McPgm);
    if (!res)
    {
      Display("Unable to start MDB simulator\n");
      gAbortFlg = true;
    }

    // TODO: get/set initial tri-state pin states from uC
    // omitting error checking for now....
    inst->pClient->getPin("RA0", inst->RA0);
    inst->pClient->getPin("RA1", inst->RA1);
    inst->pClient->getPin("RA2", inst->RA2);

    RA0_C = inst->RA0.isInput();
    RA1_C = inst->RA1.isInput();
    RA2_C = inst->RA2.isInput();

    RA0_O = inst->RA0.isHigh() ? VDD : 0;
    RA1_O = inst->RA1.isHigh() ? VDD : 0;
    RA2_O = inst->RA2.isHigh() ? VDD : 0;

    // end of instance initialization
  }

  // TODO: handle errors

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
  // set input pin states in simulator
  if (inst->RA0.isInput()) inst->pClient->setPin("RA0", RA0_I);
  if (inst->RA1.isInput()) inst->pClient->setPin("RA1", RA1_I);
  if (inst->RA2.isInput()) inst->pClient->setPin("RA2", RA2_I);

  // step simulation by one uC instruction
  bool res = inst->pClient->stepInst();

  // refresh pin states
  inst->pClient->getPin("RA0", inst->RA0);
  inst->pClient->getPin("RA1", inst->RA1);
  inst->pClient->getPin("RA2", inst->RA2);

  // set component tri-state pin config
  RA0_C = inst->RA0.isInput();
  RA1_C = inst->RA1.isInput();
  RA2_C = inst->RA2.isInput();

  // shouldn't matter if setting inputs, xxx_C controls component port direction
  RA0_O = inst->RA0.isHigh() ? VDD : 0;
  RA1_O = inst->RA1.isHigh() ? VDD : 0;
  RA2_O = inst->RA2.isHigh() ? VDD : 0;

} // end of evaluation function

/*
 * MaxExtStepSize() -- used here to prematurely terminate simulation gracefully
 */
extern "C" __declspec(dllexport) double MaxExtStepSize(pInstData inst)
{
  const double forever  = 1e308;
  const double abortSim = -1e308;

  if (gAbortFlg)
  {
    Display("Aborting simulation...\n");
    return abortSim;
  }

  return forever;
} // end of MaxExtStepSize()

/*
 * Destroy() -- here we do the one-time finalization stuff and release the
 * per-instance data
 */
extern "C" __declspec(dllexport) void Destroy(pInstData inst)
{
  // decrement instance reference counter
  gInstCnt--;

  // do stuff for each instance for each simulation step here...

  if (!gInstCnt)
  {
    // do stuff for each simulation step only once here...
  }

  if (!gInstCnt && gPostProcess)
  {
    // do stuff only once at end of simulation here...
  }

  delete inst;
} // end of Destroy()

/*
 * PostProcess() -- called for each instance when final simulation step is
 * complete.  here, we just set the global flag and rely on Destroy() to handle
 * various cases
 *
 */
extern "C" __declspec(dllexport) void PostProcess(pInstData inst)
{
  gPostProcess = true;
} // end of PostProcess()
