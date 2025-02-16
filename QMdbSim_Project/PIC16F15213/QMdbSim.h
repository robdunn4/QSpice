//------------------------------------------------------------------------------
// This file is part of the QMdbSim project, a Microchip Simulator framework for
// QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
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
const bool PIN_LOW     = 0;
const bool PIN_HIGH    = 1;

// structure for pin state
struct PinState
{
  PinState() : daState(PIN_ANALOG), ioState(PIN_INPUT), hlState(PIN_LOW) {};

  bool daState : 1;
  bool ioState : 1;
  bool hlState : 1;

  inline bool isDigital() const { return daState == PIN_DIGITAL; }
  inline bool isAnalog() const { return !isDigital(); }
  inline bool isInput() const { return ioState == PIN_INPUT; }
  inline bool isOutput() const { return !isInput(); }
  inline bool isHigh() const { return hlState == PIN_HIGH; }
  inline bool isLow() const { return !isHigh(); }
};

enum SimState
{
  NotStarted,
  Running,
  Stopped,
  ErrState
};

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
  bool getPin(const char* pinName, PinState& pinState);
  bool setPin(const char* pinName, bool toHigh);
  bool setPin(const char* pinName, double toVoltage);

protected:
  SimState simState = NotStarted;

  std::string lastErrMsg = "No errors";
  std::string sDevName;
  std::string sPgmPath;

  HANDLE mdbStdIn_Rd  = nullptr;
  HANDLE mdbStdIn_Wr  = nullptr;
  HANDLE mdbStdOut_Rd = nullptr;
  HANDLE mdbStdOut_Wr = nullptr;
  HANDLE mdbStdErr_Rd = nullptr;
  HANDLE mdbStdErr_Wr = nullptr;

  StringList sList;

  // note:  the receive buffer is fixed size and shared across instances!
  static char recvBuf[];

  void setError(const char* msg);
  bool createPipes();
  bool createMdbProcess();

  bool sendBuffer(const char* cmd);
  bool recvBuffer();
  bool sendRecvBuffer(const char* cmd);
};