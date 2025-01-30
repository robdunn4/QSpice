//------------------------------------------------------------------------------
// This file is part of the QMcServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demontration code.
//------------------------------------------------------------------------------

#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <winsock2.h>

constexpr int DEFAULT_BUFLEN = 2048; // shared I/O buffer size

class TcpWsa; // forward declaration

// TcpSocket class -- assumes that WSA has been initialized...
class TcpSocket
{
protected:
  TcpSocket(const char* serverName, const char* portNbr);
  ~TcpSocket();

public:
  void disconnect();
  bool sendRecv(const char* outBuf, char* inBuf, int inBufSize);

  const char* getLastSysErrMsg();
  int         getLastSysErrCode() const { return lastSysErr; }

  bool isConnected() const { return bConnected; }

  const char* getHostName() const;
  int         getPortNbr() const;

protected:
  SOCKET socket {INVALID_SOCKET};
  int    lastErrCode {0};
  char*  lastErrMsg {nullptr};
  bool   bConnected {false};

  char* serverName {nullptr};
  char* portNbr {nullptr};

  bool sendBuf(const char* buf);
  bool recvBuf(char* buf, int bufSize);

  void setLastErr();            // set from system err state
  void setLastErr(int errCode); // set to specific err state
  char sysErrMsg[256] {0};      // last system socket error message
  int  lastSysErr = 0;          // last system error code

  const TcpWsa* gWsaInst; // each instance will start/cleanup WSA
};

#endif // TCPSOCKET_H