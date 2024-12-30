//------------------------------------------------------------------------------
// This file is part of the QTcpServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demontration code.
//------------------------------------------------------------------------------

#include "TcpSocket.h"
#include <stdio.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

// simple-minded macro to suppress debugging messages in release compiles
#ifdef _DEBUG
#  define dprintf(s, ...) printf("*** " s, ##__VA_ARGS__)
#else
#  define dprintf(s, ...)
#endif

// utility class to ensure matched WSAStartup()/WSACleanup() calls
class TcpWsa
{
public:
  TcpWsa()
  {
    // initialize WSA
    WSAData wsaData;
    int     iResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
      // this shouldn't happen -- abort simulation with prejudice...
      dprintf("WSAStartup() failed with error: %d in " __FILE__
              ".  Aborting simulation ungracefully...\n",
          iResult);
      // abort(); // let user do this???
    }
    bValid = true;
  }

  ~TcpWsa()
  {
    if (bValid)
    {
      int res = ::WSACleanup();
      if (res) { dprintf("WSACleanup() failed.\n"); }
      bValid = false;
    }
  }

  bool isValid() const { return bValid; }

protected:
  bool bValid {false};
};

/*
 * TcpSocket class implementation
 */
TcpSocket::TcpSocket(const char* serverName, const char* portNbr) :
    serverName(_strdup(serverName)), portNbr(_strdup(portNbr)),
    gWsaInst(new TcpWsa())
{
  // WSA must already have successfully initialized
  if (!gWsaInst->isValid())
  {
    setLastErr(WSANOTINITIALISED); // set to specific code
    return;
  }

  ADDRINFOA* result = NULL;
  ADDRINFOA* ptr    = NULL;
  ADDRINFOA  hints;

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  // Resolve the server address and port
  int iResult = ::getaddrinfo(serverName, portNbr, &hints, &result);
  if (iResult != 0)
  {
    dprintf("getaddrinfo() failed with error: %d\n", iResult);
    setLastErr();
    return;
  }

  // Attempt to connect to an address until one succeeds
  for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
  {
    // Create a SOCKET for connecting to server
    socket = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (socket == INVALID_SOCKET)
    {
      dprintf("socket() failed with error: %ld\n", ::WSAGetLastError());
      setLastErr(); // need to work out this last error msg thingy
      return;
    }

    // Connect to server
    iResult = ::connect(socket, ptr->ai_addr, (int) ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
      ::closesocket(socket);
      socket = INVALID_SOCKET;
      continue;
    }
    break;
  }

  // save last system error (if any) before freeing address info
  setLastErr();
  ::freeaddrinfo(result);

  // if not a valid socket, socket connection failed
  if (socket == INVALID_SOCKET)
  {
    dprintf("Unable to connect to server\n");
    return;
  }

  dprintf("Connected to server \"%s\" on port \"%s\"\n", serverName, portNbr);
  bConnected = true;
  setLastErr();
}

TcpSocket::~TcpSocket()
{
  if (bConnected)
  {
    int iResult = ::closesocket(socket);
    if (iResult == SOCKET_ERROR)
    {
      // note:  this can be caused by not managing the instance lifetime
      // carefully, e.g., using global shared pointer and relying on runtime to
      // clean up before WSA gets unloaded by process termination sequence (?)
      // it can be safely ignored...
      dprintf("closesocket() failed with error: %d\n", ::WSAGetLastError());
    }
  }
  delete gWsaInst;

  if (serverName) free(serverName);
  if (portNbr) free(portNbr);
}

void TcpSocket::disconnect()
{
  setLastErr();
  if (bConnected)
  {
    // shutdown the connection when no more data will be sent
    int iResult = ::shutdown(socket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
      dprintf("::shutdown() failed with error: %d\n", ::WSAGetLastError());
      setLastErr();
    }
  }
  bConnected = false;
}

// returns true only if both send and receive succeed
bool TcpSocket::sendRecv(const char* outBuf, char* inBuf, int inBufSize)
{
  return sendBuf(outBuf) && recvBuf(inBuf, inBufSize);
}

const char* TcpSocket::getLastSysErrMsg()
{ // initialize to empty string
  sysErrMsg[0] = '\0';

  int len = ::FormatMessageA(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, // flags
      NULL,                                                       // lpsource
      lastSysErr,                                                 // message id
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),                  // languageid
      sysErrMsg,         // output buffer
      sizeof(sysErrMsg), // size of msgbuf, bytes
      NULL);             // va_list of arguments

  // FormatMessage() annoyingly adds newline and hard stop ('.') -- remove them
  while (len && isspace(sysErrMsg[len - 1])) sysErrMsg[--len] = 0;
  if (len && sysErrMsg[len - 1] == '.') sysErrMsg[--len] = 0;

  // if still empty string, make a custom msg
  if (!*sysErrMsg)
    ::snprintf(
        sysErrMsg, sizeof(sysErrMsg), "Undefined error code=%d", lastSysErr);

  return sysErrMsg;
}

const char* TcpSocket::getHostName() const { return serverName; }

int TcpSocket::getPortNbr() const { return ::atoi(portNbr); }

// buf must be ASCIIZ (null-terminated)
bool TcpSocket::sendBuf(const char* buf)
{
  setLastErr();

  if (!bConnected)
  {
    setLastErr();
    return false;
  }

  int iResult = ::send(socket, buf, (int) strlen(buf), 0);
  if (iResult == SOCKET_ERROR)
  {
    dprintf("::send() failed with error: %d\n", ::WSAGetLastError());
    setLastErr();
    bConnected = false;
    return false;
  }

  dprintf("Data sent: %s\n", buf);
  return true;
}

bool TcpSocket::recvBuf(char* buf, int bufSize)
{
  setLastErr();

  if (!bConnected)
  {
    setLastErr();
    return false;
  }

  int iResult = ::recv(socket, buf, bufSize - 1, 0);
  if (iResult > 0)
  {
    buf[iResult] = 0; // ensure null-termination (belt + suspenders)

    // trim trailing whitespace
    for (int i = ::strlen(buf); i > 0; i--)
    {
      if (!::isspace((char) buf[i])) break;
      buf[i] = 0;
    }
    dprintf("Data received: %s\n", buf);
    return true;
  }

  // connection "gracefully" closed?
  if (iResult == 0)
  {
    dprintf("Connection closed\n");
    setLastErr();
    bConnected = false;
    return false;
  }

  // iResult < 0 is error; hopefully a packet size > bufSize triggers this
  dprintf("::recv() failed with error: %d\n", ::WSAGetLastError());
  setLastErr();
  bConnected = false;
  return false;
}

void TcpSocket::setLastErr() { setLastErr(::WSAGetLastError()); }

void TcpSocket::setLastErr(int errCode) { lastSysErr = errCode; }
