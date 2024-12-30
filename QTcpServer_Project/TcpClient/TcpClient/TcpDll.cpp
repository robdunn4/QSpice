//------------------------------------------------------------------------------
// This file is part of the QTcpServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demontration code.
//------------------------------------------------------------------------------

#include "..\TcpClient\TcpClient.h"
#include <memory>

/*
 *  QSpice declarations/definitions -- most not needed in this code
 */
extern "C" __declspec(dllexport) int (*Display)(const char* format, ...) = 0;
// extern "C" __declspec(dllexport) const double*      DegreesC             = 0;
extern "C" __declspec(dllexport) const int*         StepNumber   = 0;
extern "C" __declspec(dllexport) const int*         NumberSteps  = 0;
extern "C" __declspec(dllexport) const char* const* InstanceName = 0;
// extern "C" __declspec(dllexport) const char*        QUX                  = 0;
// extern "C" __declspec(dllexport) const bool*        ForKeeps             = 0;
// extern "C" __declspec(dllexport) const bool*        HoldICs              = 0;
// extern "C" __declspec(dllexport) int (*DFFT)(
//     struct sComplex* u, bool inv, unsigned int N, double scale) = 0;

/*
 *standard QSpice stuff...
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
 * Per-instance component data -- see project documentation
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

  // comparison operators for Trunc() -- implement those here (see doc)
  bool operator==(const InstData& rhs) { return true; }
  bool operator!=(const InstData& rhs) { return !(*this == rhs); }

  // pClient is a shared pointer to deal with Trunc() issues
  std::shared_ptr<TcpClient> pClient;
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
  Display("%s\n", inst->pClient->getLastSysErrMsg());
}

/*
 *QSpice evaluation function
 */
extern "C" __declspec(dllexport) void tcpdll(
    pInstData* opaque, double t, union uData* data)
{
  double      Vin        = data[0].d;   // input
  const char* ServerName = data[1].str; // input parameter
  const char* PortNbr    = data[2].str; // input parameter
  int         Gain       = data[3].i;   // input parameter
  double&     Vout       = data[4].d;   // output

  // if gAbortFlg is set, we're just waiting on MaxExtStepSize() to force abort
  if (gAbortFlg) return;

  pInstData inst = *opaque;

  // per-instance initialization
  if (!inst)
  {
    // allocate instance -- error handling omitted...
    *opaque = inst = new InstData(ServerName, PortNbr);

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
    Display(
        "Connected to server.  Host=%s, Port=%d.  Server version %d.%d.%d\n",
        inst->pClient->getHostName(), inst->pClient->getPortNbr(), verMajor,
        verMinor, verDot);

    // log component instance for this connection on server (optional/useful)
    char buf[256];
    snprintf(buf, sizeof(buf), "Client instance=%s", *InstanceName);
    if (!inst->pClient->logMsg(buf))
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

    // end of instance initialization
  }

  // Implement module evaluation code here:
  Vout = Vin * Gain;

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
 * Trunc() -- here doesn't do anything useful but needed to test/ensure that the
 * instance copy doesn't break anything...
 */
extern "C" __declspec(dllexport) void Trunc(
    InstData* inst, double t, union uData* data, double* timestep)
{
  const double ttol = 1e-9; // 1 nsec default tolerance -- change as appropriate
  if (*timestep > ttol)
  {
    // create a copy of the instance data -- see InstData struct
    InstData  tmp  = *inst; // temporary copy
    pInstData pTmp = &tmp;  // make pointer for modern C++ compiler happiness
    tcpdll(&pTmp, t, data); // call evaluation function with temporary copy

    // compare relevant portions of instance data to determine if timestep
    // should be reduced.  here we compare instances grossly (see project
    // documentation)
    if (tmp != *inst) *timestep = ttol;
  }
} // end of Trunc()

/*
 * Destroy() -- here we do the one-time finalization stuff and release the
 * per-instance data
 * ??? revise above
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
    Display("Executing commands from instance %s\n", *InstanceName);

    // do demonstration stuff
    bool res;
    res = inst->pClient->echoMsg("Echo message text...");
    if (!res)
    {
      Display("Echo command failed\n");
      ServerError(inst);
      return;
    }
    Display("Echo command succeeded\n");

    int pid = -1;
    res     = inst->pClient->launchCmd("cmd.exe /c Test.cmd Launch", &pid);
    // res = inst->pClient->launchCmd("Test.cmd Launch", &pid);
    if (!res)
    {
      Display("Launch command failed\n");
      ServerError(inst);
      return;
    }
    Display("Launch command succeeded, PID=%d\n", pid);

    // res = inst->pClient->launchCmd(
    //     "cmd.exe /c ../../QAlert/QAlert.exe Launch", &pid);
    res = inst->pClient->launchCmd("../../QAlert/QAlert.exe Launch", &pid);
    if (!res)
    {
      Display("Launch command failed\n");
      ServerError(inst);
      return;
    }
    Display("Launch command succeeded, PID=%d\n", pid);

    int retCode = -1;
    // res         = inst->pClient->runCmd("cmd.exe /c Test.cmd Run", &retCode);
    res = inst->pClient->runCmd("cmd.exe /c Test.cmd Run", &retCode);
    if (!res)
    {
      Display("Run command failed\n");
      ServerError(inst);
      return;
    }
    Display("Run command returned %d\n", retCode);

    res = inst->pClient->alertMsg("Simulation is complete");
    if (!res)
    {
      Display("Alert command failed\n");
      ServerError(inst);
      return;
    }
    Display("Alert command succeeded\n");
  }

  Display("Destroy() instance %s\n", *InstanceName);
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
  Display("PostProcess() instance %s\n", *InstanceName);
  gPostProcess = true;
} // end of PostProcess()
