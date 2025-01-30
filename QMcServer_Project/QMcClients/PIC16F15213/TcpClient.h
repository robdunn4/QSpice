//------------------------------------------------------------------------------
// This file is part of the QMcServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demontration code.
//
// Note:  Requires C++20...
//------------------------------------------------------------------------------

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "TcpSocket.h"

// pin state constants for convenience
const bool PIN_ANALOG  = 0;
const bool PIN_DIGITAL = 1;
const bool PIN_OUTPUT  = 0;
const bool PIN_INPUT   = 1;
const bool PIN_LOW     = 0;
const bool PIN_HIGH    = 1;

// structure for pin state
class PinState
{
public:
  PinState() {};

  bool daState : 1 = PIN_ANALOG;
  bool ioState : 1 = PIN_INPUT;
  bool hlState : 1 = PIN_LOW;

  inline bool isDigital() { return daState == PIN_DIGITAL; }
  inline bool isAnalog() { return !isDigital(); }
  inline bool isInput() { return ioState == PIN_INPUT; }
  inline bool isOutput() { return !isInput(); }
  inline bool isHigh() { return hlState == PIN_HIGH; }
  inline bool isLow() { return !isHigh(); }
};

//-----

class TcpClient : public TcpSocket
{
public:
  explicit TcpClient(const char* serverName, const char* portNbr);
  ~TcpClient();

  // message handlers
  bool alertMsg(const char* msg);
  bool echoMsg(const char* msg);
  bool logMsg(const char* msg);
  bool setCwd();                // set to current simulation folder
  bool setCwd(const char* cwd); // set to specific folder
  bool getServerVer(int& verMaj, int& verMin, int& verDot);

  // simulator commands
  bool startSim(
      const char* simName, const char* deviceName, const char* pgmPath);
  bool stopSim();
  bool stepInst();
  bool getPin(const char* pinName, PinState& pinState);
  bool setPin(const char* pinName, bool toHigh);
  bool setPin(const char* pinName, double toVoltage);

protected:
  // I/O buffer shared across instances (beware)
  static char msgBuf[DEFAULT_BUFLEN];

  void makeOutMsg(char msgChar, const char* msgTxt);
};

#endif // TCPCLIENT_H