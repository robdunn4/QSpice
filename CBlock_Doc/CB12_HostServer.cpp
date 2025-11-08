//------------------------------------------------------------------------------
// CB12_HostServer.cpp -- Example host server code for C-Block Basics #12.
//------------------------------------------------------------------------------
// Full project code and documentation is available here:
//    https://github.com/robdunn4/QSpice
//
// Requires C++20 or later.  To compile using MSVC 2022:
//   cl.exe /std:c++20 /EHsc /Od /DEBUG /Z7 CB12_HostServer.cpp /link
//      /out:CB12_HostServer.exe
//
// To start server: CB12_HostServer.exe <port>
// To close server: <CTRL-C>
//

// clang-format off // prevent sorting of #includes
#include <winsock2.h>
#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <syncstream>   // to protect syncOut with mutex (C++20)
// clang-format on

#pragma comment(lib, "ws2_32.lib")

#undef IN
#undef OUT

// macro for synchronized output to console to prevent cout interleaving
#define syncOut std::osyncstream(std::cout)

// default template message IDs
#define PORT_EVALUATE     2
#define PORT_MAX_STEPSIZE 3
#define PORT_TRUNCATE     4

// custom message IDs
#define PORT_LOGNAME     1024   // set logfile name string
#define PORT_INSTNAME    1025   // set instance name string
#define PORT_GAIN        1026   // set gain int val
#define PORT_STEPNBR     1027   // set step # int val
#define PORT_DESTROY     1028   // event: client Destroy()
#define PORT_POSTPROCESS 1029   // event: client PostProcess()
#define PORT_INITIALZE   1030   // cmd: initialize server with current values
#define PORT_CLOSESERVER 1031   // cmd: close server
#define PORT_GETPORTNBR  1032   // get: return connection port number to client

#define MAX_BUFFER_SIZE 1024   // msg buffer size; must match client

// Command IDs (mapping to unnecessary enum)
enum CommandID : uint32_t {
  CmdEvaluate        = PORT_EVALUATE,
  CmdMaxStep         = PORT_MAX_STEPSIZE,
  CmdTruncate        = PORT_TRUNCATE,
  CmdLogName         = PORT_LOGNAME,
  CmdInstName        = PORT_INSTNAME,
  CmdGain            = PORT_GAIN,
  CmdStepNbr         = PORT_STEPNBR,
  CmdDestroy         = PORT_DESTROY,
  CmdPostProcess     = PORT_POSTPROCESS,
  CmdInitialize      = PORT_INITIALZE,
  CmdCloseConnection = PORT_CLOSESERVER,
  CmdGetPortNbr      = PORT_GETPORTNBR
};

// Example per-client instance data structure
struct InstData {
  InstData() = delete;   // prevent default construction
  InstData(unsigned int port) : portNbr(port) {}

  // do not change these...
  bool         initialized = false;   // initialization flag
  bool         postProcess = false;   // end simulation flag
  unsigned int portNbr     = 0;       // port number (for messages & logging)

  // modify the below to match user requirements...
  int         gainVal = 0;   // input Gain attribute
  int         stepNbr = 0;   // current simulation step number
  std::string instName;      // component instance name
  std::string logName;       // logfile path
};

// Shared globals
std::vector<SOCKET>      g_clientSockets;
std::vector<std::thread> g_clientThreads;
std::atomic<bool>        g_running(true);
SOCKET                   g_serverSocket = INVALID_SOCKET;

// Forward declarations
void handleClient(SOCKET clientSocket, sockaddr_in clientAddr);
void performCommand(SOCKET clientSocket, uint32_t cmdID, InstData *instData,
    char *buffer, int bytesReceived);

// Command-specific handlers
void   handleCmdEvaluate(char *buffer, int bytesReceived, InstData *instData);
double handleCmdMaxStep(char *buffer, int bytesReceived, InstData *instData);
double handleCmdTruncate(char *buffer, int bytesReceived, InstData *instData);
void   handleCmdLogName(char *buffer, int bytesReceived, InstData *instData);
void   handleCmdInstName(char *buffer, int bytesReceived, InstData *instData);
void   handleCmdGain(char *buffer, int bytesReceived, InstData *instData);
void   handleCmdStepNbr(char *buffer, int bytesReceived, InstData *instData);
void   handleCmdDestroy(char *buffer, int bytesReceived, InstData *instData);
void handleCmdPostProcess(char *buffer, int bytesReceived, InstData *instData);
bool handleCmdInitialize(char *buffer, int bytesReceived, InstData *instData);
void handleCmdCloseConnection(
    char *buffer, int bytesReceived, InstData *instData);

// Ctrl+C handler
BOOL WINAPI consoleHandler(DWORD signal) {
  if (signal == CTRL_C_EVENT) {
    syncOut << "\n[Server] Ctrl+C detected. Shutting down gracefully..."
            << std::endl;
    g_running = false;

    // Close server socket to unblock accept()
    if (g_serverSocket != INVALID_SOCKET) {
      closesocket(g_serverSocket);
      g_serverSocket = INVALID_SOCKET;
    }

    // Close all client sockets
    for (auto sock : g_clientSockets) {
      shutdown(sock, SD_BOTH);
      closesocket(sock);
    }

    return TRUE;   // Prevent default termination
  }
  return FALSE;
}

// main entry point
int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    return -1;
  }

  size_t ndx  = 0;
  int    port = std::stoi(argv[1], &ndx);
  if (ndx < strlen(argv[1])) {
    std::cerr << "[Server] Invalid port number: " << argv[1] << std::endl;
    return -1;
  }

  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    std::cerr << "[Server] WSAStartup failed." << std::endl;
    return -1;
  }

  SetConsoleCtrlHandler(consoleHandler, TRUE);

  g_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (g_serverSocket == INVALID_SOCKET) {
    std::cerr << "[Server] Error creating socket." << std::endl;
    WSACleanup();
    return -1;
  }

  sockaddr_in serverAddr{};
  serverAddr.sin_family      = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port        = htons(port);

  if (bind(g_serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) ==
      SOCKET_ERROR) {
    std::cerr << "[Server] Bind failed." << std::endl;
    closesocket(g_serverSocket);
    WSACleanup();
    return -1;
  }

  if (listen(g_serverSocket, SOMAXCONN) == SOCKET_ERROR) {
    std::cerr << "[Server] Listen failed." << std::endl;
    closesocket(g_serverSocket);
    WSACleanup();
    return -1;
  }

  syncOut << "[Server] Listening on port " << port << "..." << std::endl;
  syncOut << "[Server] Press Ctrl+C to stop." << std::endl;

  while (g_running) {
    sockaddr_in clientAddr{};
    int         clientAddrSize = sizeof(clientAddr);

    SOCKET clientSocket =
        accept(g_serverSocket, (sockaddr *)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
      if (g_running) std::cerr << "[Server] Accept failed." << std::endl;
      break;   // stop if shutdown triggered
    }

    g_clientThreads.emplace_back(handleClient, clientSocket, clientAddr);
    g_clientSockets.push_back(clientSocket);
  }

  syncOut << "[Server] Waiting for client threads to finish..." << std::endl;
  for (auto &t : g_clientThreads)
    if (t.joinable()) t.join();

  WSACleanup();
  syncOut << "[Server] Shutdown complete." << std::endl;
  return 0;
}

// client thread handler
void handleClient(SOCKET clientSocket, sockaddr_in clientAddr) {
  // per-instance data
  InstData instData(ntohs(clientAddr.sin_port));
  char     buffer[MAX_BUFFER_SIZE];

  syncOut << "[Server] Client connected on port " << instData.portNbr << "."
          << std::endl;

  while (g_running) {
    int bytesReceived = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
      syncOut << "[" << instData.portNbr << "] Client disconnected."
              << std::endl;
      break;
    }

    if (bytesReceived < 4) {
      syncOut << "[" << instData.portNbr << "] Invalid message (too short)."
              << std::endl;
      break;   // this shouldn't happen -- need to abort connection???
    }

    uint32_t cmdID;
    memcpy(&cmdID, buffer, sizeof(uint32_t));
    performCommand(clientSocket, cmdID, &instData, buffer, bytesReceived);

    if (cmdID == CmdCloseConnection) break;
  }

  closesocket(clientSocket);
}

// template to override Winsocks send() for convenience in switch
template <typename T> int send(SOCKET s, T val, int flags) {
  // error handling omitted
  return send(s, reinterpret_cast<const char *>(&val), sizeof(val), flags);
}

// evaluate command and send response.  return when complete.
void performCommand(SOCKET clientSocket, uint32_t cmdID, InstData *instData,
    char *buffer, int bytesReceived) {
  int32_t retVal = cmdID;   // default return is simply cmdID

  switch (cmdID) {
  case CmdEvaluate:
    handleCmdEvaluate(buffer, bytesReceived, instData);
    // error handling omitted
    send(clientSocket, buffer, 8, 0);
    return;

  case CmdMaxStep:
    // error handling omitted
    send(clientSocket, handleCmdMaxStep(buffer, bytesReceived, instData), 0);
    return;

  case CmdTruncate:
    // error handling omitted
    send(clientSocket, handleCmdTruncate(buffer, bytesReceived, instData), 0);
    return;

  case CmdLogName:
    handleCmdLogName(buffer, bytesReceived, instData);
    break;

  case CmdInstName:
    handleCmdInstName(buffer, bytesReceived, instData);
    break;

  case CmdGain:
    handleCmdGain(buffer, bytesReceived, instData);
    break;

  case CmdStepNbr:
    handleCmdStepNbr(buffer, bytesReceived, instData);
    break;

  case CmdDestroy:
    handleCmdDestroy(buffer, bytesReceived, instData);
    break;

  case CmdPostProcess:
    handleCmdPostProcess(buffer, bytesReceived, instData);
    break;

  case CmdInitialize:
    if (!handleCmdInitialize(buffer, bytesReceived, instData)) { return; }
    break;

  case CmdCloseConnection:
    // don't respond, we're done...
    handleCmdCloseConnection(buffer, bytesReceived, instData);
    return;

  case CmdGetPortNbr:
    // error handling omitted
    send(clientSocket, instData->portNbr, 0);
    return;

  default:
    // this should abort?
    syncOut << "[" << instData->portNbr << ":" << instData->instName
            << "] Unknown command ID: " << cmdID << std::endl;
    return;
  }

  // return value of retVal; error handling omitted
  send(clientSocket, retVal, 0);
}

//  === Command Handlers ===

void handleCmdEvaluate(char *buffer, int bytesReceived, InstData *instData) {
  // syncOut << "[" << instData->portNbr << ":" << instData->instName
  //           << "] Processing CmdEvaluate..." << std::endl;
  assert((bytesReceived == 28) && "Invalid message size for CmdEvaluate");

  double *inputs  = reinterpret_cast<double *>(buffer + 4);
  double *outputs = reinterpret_cast<double *>(buffer);

  // must match client!
  const double t   = inputs[0];
  const double IN  = inputs[1];
  const double EN  = inputs[2];
  double      &OUT = outputs[0];

  // calculate/set output
  OUT = EN ? IN * instData->gainVal : 0;   // for example only
}

double handleCmdMaxStep(char *buffer, int bytesReceived, InstData *instData) {
  // syncOut << "[" << instData->portNbr << ":" << instData->instName
  //           << "] Processing CmdMaxStep..." << std::endl;
  assert((bytesReceived == 12) && "Invalid message size for CmdMaxStep");

  const double t      = *((double *)(buffer + 4));
  double       retVal = 1e308;   // set default

  // return a number other than 1e308 to stipulate a max timestep.
  // logic that might change retVal omitted for demo
  return retVal;
}

double handleCmdTruncate(char *buffer, int bytesReceived, InstData *instData) {
  // syncOut << "[" << instData->portNbr << ":" << instData->instName
  //           << "] Processing CmdTruncate..." << std::endl;
  assert((bytesReceived == 36) && "Invalid message size for CmdTruncate");

  // must match client!
  double      *inputs   = (double *)(buffer + 4);
  const double t        = inputs[0];
  const double IN       = inputs[1];
  const double EN       = inputs[2];
  const double timestep = inputs[3];   // This is the otherwise planned timestep

  double retVal = 1e308;

  // return a number other than 1e308 to tactically stipulate a shorter
  // timestep.
  return retVal;
}

void handleCmdLogName(char *buffer, int bytesReceived, InstData *instData) {
  // syncOut << "[" << instData->portNbr << ":" << instData->instName
  //           << "] Processing CmdLogName..." << std::endl;
  assert((bytesReceived > 5) && "Invalid message size for CmdLogName");

  instData->logName.assign(buffer + 4, bytesReceived - 4);
}

void handleCmdInstName(char *buffer, int bytesReceived, InstData *instData) {
  // syncOut << "[" << instData->portNbr << ":" << instData->instName
  //           << "] Processing CmdInstName..." << std::endl;
  assert((bytesReceived > 5) && "Invalid message size for CmdInstName");

  instData->instName.assign(buffer + 4, bytesReceived - 4);
}

void handleCmdGain(char *buffer, int bytesReceived, InstData *instData) {
  // syncOut << "[" << instData->portNbr << ":" << instData->instName
  //           << "] Processing CmdGain..." << std::endl;
  assert((bytesReceived == 8) && "Invalid message size for CmdGain");

  int gainVal;
  memcpy(&gainVal, buffer + 4, sizeof(int));
  instData->gainVal = gainVal;
}

void handleCmdStepNbr(char *buffer, int bytesReceived, InstData *instData) {
  // syncOut << "[" << instData->portNbr << ":" << instData->instName
  //           << "] Processing CmdStepNbr..." << std::endl;
  assert((bytesReceived == 8) && "Invalid message size for CmdStepNbr");

  int stepNbr;
  memcpy(&stepNbr, buffer + 4, sizeof(int));
  instData->stepNbr = stepNbr;
}

void handleCmdDestroy(char *buffer, int bytesReceived, InstData *instData) {
  syncOut << "[" << instData->portNbr << ":" << instData->instName
          << "] Processing CmdDestroy..." << std::endl;
  assert((bytesReceived == 4) && "Invalid message size for CmdDestroy");

  // perform end of simulation step processing here...
  // for example, write end of step log entry
  syncOut << "[" << instData->portNbr << ":" << instData->instName
          << "] End of simulation step..." << std::endl;

  if (instData->postProcess) {
    // perform end of simulation processing here...
    // for example, write end of simulation log entry
    syncOut << "[" << instData->portNbr << ":" << instData->instName
            << "] End of simulation..." << std::endl;
  }
}

void handleCmdPostProcess(char *buffer, int bytesReceived, InstData *instData) {
  syncOut << "[" << instData->portNbr << ":" << instData->instName
          << "] Processing CmdPostProcess..." << std::endl;
  assert((bytesReceived == 4) && "Invalid message size for CmdPostProcess");

  // set flag to indicate end of simulation; see handleCmdDestroy...
  instData->postProcess = true;
}

// return true on success, false to indicate failure
bool handleCmdInitialize(char *buffer, int bytesReceived, InstData *instData) {
  syncOut << "[" << instData->portNbr << ":" << instData->instName
          << "] Processing CmdInitialize..." << std::endl;
  assert((bytesReceived == 4) && "Invalid message size for CmdInitialize");

  // perform any initialization needed with current state values...
  // for example, open a logfile and write initial entry (use instData->stepNbr
  // to determine whether to open for overwrite or append) -- logfile omitted
  // here for demo purposes
  instData->initialized = true;
  syncOut << "[" << instData->portNbr << ":" << instData->instName
          << "] Initialized.  Log File Name: " << instData->logName
          << "; Gain Value: " << instData->gainVal
          << "; Step Number: " << instData->stepNbr << std::endl;

  // return false to close socket on error
  return true;   // success
}

void handleCmdCloseConnection(
    char *buffer, int bytesReceived, InstData *instData) {
  syncOut << "[" << instData->portNbr << ":" << instData->instName
          << "] Processing CmdCloseConnection..." << std::endl;

  assert((bytesReceived == 4) && "Invalid message size for CmdCloseConnection");

  // this is the final opportunity to process before socket is closed
  // for example, close logfile, release other resources, write server message
}
