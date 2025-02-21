//------------------------------------------------------------------------------
// This file is part of the QMdbSim project, a Microchip Simulator framework for
// QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/* Notes:
 *
 * This code provides the low-level interface to MDB.  If you want to create a
 * new Microchip device, you should not need to modify this code (at least, in
 * theory).
 */
#pragma once

#include <Windows.h>
#include <string>
#include <vector>

typedef std::vector<std::string> StringList;

extern const char* gMdbSimPath;

// pin state constants for convenience
const bool PIN_ANALOG  = 0;
const bool PIN_DIGITAL = 1;
const bool PIN_OUTPUT  = 0;
const bool PIN_INPUT   = 1;

// structure for pin state
struct PinState
{
  PinState() : daState(PIN_ANALOG), ioState(PIN_INPUT) {};

  bool   daState;
  bool   ioState;
  double voltage = 0;

  inline bool isDigital() const { return daState == PIN_DIGITAL; }
  inline bool isAnalog() const { return !isDigital(); }
  inline bool isInput() const { return ioState == PIN_INPUT; }
  inline bool isOutput() const { return !isInput(); }
};

// class to map port names to tri-state (or input-only) ports
class PinPortMap
{
public:
  PinPortMap(const char* const pinName, double* const inPort,
      double* const outPort = NULL, bool* const dirPort = NULL) :
      pinName(pinName), inPort(inPort), outPort(outPort), dirPort(dirPort) {};
  ~PinPortMap() {};

  const char* const pinName;
  double* const     inPort;
  double*           outPort;
  bool*             dirPort;

  PinState pinState;
};

typedef std::vector<PinPortMap> PinPortMapList;

// simulation states
enum SimState
{
  NotStarted,
  Running,
  Stopped,
  ErrState
};

// MDB simulator interface class
class MdbSim
{
public:
  MdbSim();
  ~MdbSim();

  inline SimState    getSimState() { return simState; }
  inline bool        getErrState() { return simState == ErrState; }
  inline const char* getLastErrMsg() { return lastErrMsg.c_str(); }
  const char*        getVerInfo();

  // simulator commands
  bool startSim(const char* deviceName, const char* pgmPath);
  bool stopSim();
  bool stepInst();
  bool getPinState(const char* pinName, PinState& pinState);
  bool setPin(const char* pinName, double toVoltage);

  // support for single call updates
  void addPinPortMap(const char* const pinName, double* const inPort,
      double* const outPort = NULL, bool* const dirPort = NULL);
  bool getPinStates();
  void setCtrlPorts();
  bool setInPins();
  void setOutPorts();
  bool setVDD(double vdd);

protected:
  SimState simState = NotStarted;

  double vddV = 5.0;

  std::string lastErrMsg = "No errors";
  std::string sDevName;
  std::string sPgmPath;

  HANDLE mdbStdIn_Rd  = nullptr;
  HANDLE mdbStdIn_Wr  = nullptr;
  HANDLE mdbStdOut_Rd = nullptr;
  HANDLE mdbStdOut_Wr = nullptr;
  HANDLE mdbStdErr_Rd = nullptr;
  HANDLE mdbStdErr_Wr = nullptr;

  StringList     sList;
  PinPortMapList ppmList;

  // note:  the receive buffer is fixed size and shared across instances!
  static char recvBuf[];

  void setError(const char* msg);
  bool createPipes();
  bool createMdbProcess();

  bool sendBuffer(const char* cmd);
  bool recvBuffer();
  bool sendRecvBuffer(const char* cmd);
};