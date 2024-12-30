//------------------------------------------------------------------------------
// This file is part of the QTcpServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demontration code.
//------------------------------------------------------------------------------

#include "TcpClient.h"

// simple-minded macro to suppress debugging messages in release compiles
#ifdef _DEBUG
#  define dprintf(s, ...) printf("*** " s, ##__VA_ARGS__)
#else
#  define dprintf(s, ...)
#endif

// QSpice declarations to ensure availability in this unit
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

// launch command ('l') -- non-blocking
bool TcpClient::launchCmd(const char* cmd, int* pid)
{
  makeOutMsg('l', cmd);
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  if (msgBuf[0] != 'L') return false;

  // parse return code into retCode
  if (pid) *pid = ::atoi(msgBuf + 2);
  return true;
}

// log message on server ('m')
bool TcpClient::logMsg(const char* msg)
{
  makeOutMsg('m', msg);
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  return msgBuf[0] == 'M';
}

// run command ('r') -- blocking
bool TcpClient::runCmd(const char* cmd, int* retCode)
{
  makeOutMsg('r', cmd);
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  if (msgBuf[0] != 'R') return false;

  // parse return code into retCode
  if (retCode) *retCode = ::atoi(msgBuf + 2);
  return true;
}

// set CWD for socket ('w') to the simulation CWD
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
  makeOutMsg('w', cwd);
  if (!sendRecv(msgBuf, msgBuf, sizeof(msgBuf))) return false;
  return msgBuf[0] == 'W';
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

// utility function to construct an outgoing message
void TcpClient::makeOutMsg(char msgChar, const char* msgTxt)
{
  // ensure that msgTxt doesn't overflow buffer
  msgBuf[0] = msgChar;
  msgBuf[1] = ' ';
  ::memcpy(msgBuf + 2, msgTxt, sizeof(msgBuf) - 3);
  msgBuf[sizeof(msgBuf) - 1] = 0; // ensure null-terminated string
}
