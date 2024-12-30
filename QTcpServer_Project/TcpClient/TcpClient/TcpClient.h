//------------------------------------------------------------------------------
// This file is part of the QTcpServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demontration code.
//------------------------------------------------------------------------------

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "TcpSocket.h"

class TcpClient : public TcpSocket
{
public:
  explicit TcpClient(const char* serverName, const char* portNbr);
  ~TcpClient();

  // message handlers
  bool alertMsg(const char* msg);
  bool echoMsg(const char* msg);
  bool launchCmd(const char* cmd, int* pid = nullptr);
  bool logMsg(const char* msg);
  bool runCmd(const char* cmd, int* retCode = nullptr);
  bool setCwd();                // set to current simulation folder
  bool setCwd(const char* cwd); // set to specific folder
  bool getServerVer(int& verMaj, int& verMin, int& verDot);

protected:
  // I/O buffer shared across instances (beware)
  static char msgBuf[DEFAULT_BUFLEN];

  void makeOutMsg(char msgChar, const char* msgTxt);
};

#endif // TCPCLIENT_H