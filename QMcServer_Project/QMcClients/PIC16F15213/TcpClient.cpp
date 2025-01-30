//------------------------------------------------------------------------------
// This file is part of the QMcServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demontration code.
//
// Note:  Requires C++20...
//------------------------------------------------------------------------------
#include "TcpClient.h"

#include <format> // requies C++20...
#include <string>

// simple-minded macro to suppress debugging messages in release compiles
#ifdef _DEBUG
#  define dprintf(s, ...) printf("*** " s, ##__VA_ARGS__)
#else
#  define dprintf(s, ...)
#endif

// QSpice declarations to ensure availability in this unit (for debugging)
extern "C" __declspec(dllexport) int (*Display)(const char* format, ...);

/*
 * TcpClient class implementation
 */
char TcpClient::msgBuf[DEFAULT_BUFLEN] = {0}; // shared/static

TcpClient::TcpClient(const char* serverName, const char* portNbr) :
    TcpSocket(serverName, portNbr) {};

TcpClient::~TcpClient() {}

// *** message wrappers for convenience

// alert msg ('a')
bool TcpClient::alertMsg(const char* msg)
{
  makeOutMsg('a', msg);
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  return msgBuf[0] == 'A';
}

// echo msg ('e')
bool TcpClient::echoMsg(const char* msg)
{
  makeOutMsg('e', msg);
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  return msgBuf[0] == 'E';
}

// log message on server ('m')
bool TcpClient::logMsg(const char* msg)
{
  makeOutMsg('m', msg);
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  return msgBuf[0] == 'M';
}

// set CWD for socket ('c') to the simulation CWD
bool TcpClient::setCwd()
{
  // is the path too long?
  char cwd[sizeof(msgBuf) - 2];
  int  sz = GetCurrentDirectoryA(0, nullptr);
  if (sz > sizeof(cwd)) return false;

  if (!GetCurrentDirectoryA(sizeof(cwd), cwd)) return false;

  return setCwd(cwd);
}

// set CWD for socket ('w') to specific folder
bool TcpClient::setCwd(const char* cwd)
{
  makeOutMsg('c', cwd);
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  return msgBuf[0] == 'C';
}

// get server version info ('v')
bool TcpClient::getServerVer(int& verMaj, int& verMin, int& verDot)
{
  makeOutMsg('v', "");

  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  if (msgBuf[0] != 'V') return false;

  // parse server version info -- expected format is "V v#.#.#\0"
  // could use a more sophisticated method but for now, do it manually
  char* s = msgBuf + 2;
  verMaj  = ::atoi(s);
  while (*s != '.' && *s) s++;
  s++;
  verMin = ::atoi(s);
  while (*s != '.' && *s) s++;
  s++;
  verDot = ::atoi(s);

  return true;
}

// === simulation ===

/*
 * startSim() - start simulation on server
 *   simName - simulator identifier, e.g., "MDB"
 *   deviceName - device name, e.g., "PIC16F15213"
 *   pgmPath - path to device program, e.g., *.hex or *.elf file
 */
bool TcpClient::startSim(
    const char* simName, const char* deviceName, const char* pgmPath)
{
  std::string msg;
  msg = std::format("{} {} {}", simName, deviceName, pgmPath);
  makeOutMsg('l', msg.c_str());
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  return msgBuf[0] == 'L';
}

/*
 * stopSim() - stop simulation on server
 */
bool TcpClient::stopSim()
{
  makeOutMsg('q', "");
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  return msgBuf[0] == 'Q';
}

/*
 * stepInst() - step simulation by one instruction cycle
 */
bool TcpClient::stepInst()
{
  makeOutMsg('s', "");
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  return msgBuf[0] == 'S';
}

/*
 * getPin() - get pin state information
 *   pinName - name of pin, e.g., "RA0"
 *   pinState - PINSTATE structure for returned state information
 */
bool TcpClient::getPin(const char* pinName, PinState& pinState)
{
  makeOutMsg('r', pinName);
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  if (msgBuf[0] != 'R') return false;

  char* stateData  = msgBuf + 2;
  pinState.daState = stateData[0] == 'D'; // first char
  pinState.ioState = stateData[1] == 'I'; // second char
  pinState.hlState = stateData[2] == 'H'; // third char

  return true;
}

/*
 * setPin() - set pin to digital high/low
 */
bool TcpClient::setPin(const char* pinName, bool toHigh)
{
  std::string msg;
  msg = std::format("{} {}", pinName, toHigh ? "HIGH" : "LOW");

  makeOutMsg('w', msg.c_str());
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  return msgBuf[0] == 'W';
}

/*
 * setPin() - set pin to voltage
 */
bool TcpClient::setPin(const char* pinName, double toVoltage)
{
  std::string msg;
  msg = std::format("{} {}V", pinName, toVoltage);

  makeOutMsg('w', msg.c_str());
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  return msgBuf[0] == 'W';
}

// utility function to construct an outgoing message
void TcpClient::makeOutMsg(char msgChar, const char* msgTxt)
{
  // TODO: add return if overflow when constructing message
  // ensure that msgTxt doesn't overflow buffer
  msgBuf[0] = msgChar;
  msgBuf[1] = ' ';
  msgBuf[2] = '\0';
  ::memcpy(msgBuf + 2, msgTxt, sizeof(msgBuf) - 3);
  msgBuf[sizeof(msgBuf) - 1] = '\0'; // ensure null-terminated string
}
