//------------------------------------------------------------------------------
// CB11_Server.cpp -- Example server EXE code for C-Block Basics #11.
//------------------------------------------------------------------------------
// Full project code and documentation is available here:
//    https://github.com/robdunn4/QSpice
//
// This code compiles successfully using the Digital Mars C++ Compiler:
//    dmc CB11_Server.cpp
//
// It also compiles successfully with MSVC 2022 (Community Edition)
//

#include <stdio.h>
#include <winsock2.h>

#define BUFFER_SIZE 1024   // message buffer size

// for logging convenience -- note: must have a second parameter even if not
// used (e.g., LOG("Something with no args", 0))
#define LOG(fmt, ...)                                                          \
  fprintf(inst.hLogFile, "Inst %s Port %d: " fmt "\n", inst.instName,          \
      inst.portNbr, __VA_ARGS__);

// for "printf()" convenience -- note: must have a second parameter even if not
// used (e.g., Display("Something with no args", 0))
#define Display(fmt, ...)                                                      \
  printf("Port %d: " fmt "\n", inst.portNbr, __VA_ARGS__);

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

const int Ninputs  = 2;   // not used
const int Noutputs = 1;

#ifdef __DMC__
#pragma comment(lib, "wsock32.lib")
#else
#pragma comment(lib, "Ws2_32.lib")
#endif

// adding per-instance data as global variable(s); these could be simple global
// variables but using struct for clarity and grouping
struct InstData {
  int   portNbr;       // server port number (for user messages & logging)
  int   gainVal;       // input Gain attribute (from client)
  int   stepNbr;       // current simulation step number (from client)
  char *logName;       // logfile path (from client)
  char *instName;      // component instance name (from client)
  FILE *hLogFile;      // logfile handle
  bool  postProcess;   // end simulation flag (from client event)
} inst = {0, 0, 0, 0, 0, 0, false};

// define some structures to re-cast messages for convenience/clarity
struct IntBuf {
  int msgID;
  int intVal;
};
struct StrBuf {
  int  msgID;
  char strVal[BUFFER_SIZE - sizeof(StrBuf::msgID)];
};

// #undef pin names lest they collide with names in any header file(s) you might
// include.
#undef IN
#undef OUT
#undef EN

/*
 * Evaluate() -- this is where user-defined evaluation function code goes; note
 * that input/output operations use a single buffer
 */
void Evaluate(char *buffer) {
  double *inputs  = (double *)(buffer + 4);
  double *outputs = (double *)buffer;

  const double t   = inputs[0];
  const double IN  = inputs[1];
  const double EN  = inputs[2];
  double      &OUT = outputs[0];

  // calculate/set output
  OUT = EN ? IN * inst.gainVal : 0;
}

/*
 * MaxStepSize() -- this is where user-defined MaxExtStepSize() code goes
 */
double MaxStepSize(char *buffer) {
  const double t      = *((double *)(buffer + 4));
  double       retVal = 1e308;   // set default

  // return a number other than 1e308 to stipulate a max timestep.
  // logic that might change retVal omitted for demo
  LOG("MaxStep():  t=%g, retVal=%g", t, retVal);
  return retVal;
}

/*
 * Truncate() -- this is where user-defined Trunc() code goes
 */
double Truncate(char *buffer) {
  double      *inputs = (double *)(buffer + 4);
  const double t      = inputs[0];
  const double IN     = inputs[1];
  const double EN     = inputs[2];
  const double timestep =
      inputs[3];   // This is the otherwise planned timestep.

  double retVal = 1e308;
  // return a number other than 1e308 to tactically stipulate a shorter
  // timestep.

  // logic that might change *timestep value omitted for demo
  LOG("Trunc():    t=%g, Gain=%d, EN=%d, IN=%g, timestep=%g, retVal=%g", t,
      inst.gainVal, EN, IN, timestep, retVal);
  return retVal;
}

/*
 * Destroy() -- this is the custom handler for the PORT_DESTROY msg; user code
 * for end-of-step events goes here
 */
void Destroy() {
  // write end of step msg to log
  LOG("End of Step %d", inst.stepNbr);

  // if done, write end of simulation msg to log
  if (inst.postProcess) { LOG("End of Simulation", 0); }

  // close logfile & release allocated strings
  fclose(inst.hLogFile);
  free(inst.instName);
  free(inst.logName);

  // not required but belt & braces...
  inst.hLogFile = 0;
  inst.instName = 0;
  inst.logName  = 0;
}

/*
 * PostProcess() -- this is the custom handler for the PORT_POSTPROCESS msg;
 * user code for end-of-simulation events goes here; this is called immediately
 * before Destroy()
 */
void PostProcess() {
  // set flag used in Destroy()
  inst.postProcess = true;
}

// Initialize() -- this is the custom handler for the PORT_INITIALIZE msg;
// client code should send this after sending initial values; returns false
// if anything fails
bool Initialize() {
  // open logfile for append or overwrite
  const char *fMode = inst.stepNbr > 1 ? "a" : "w";
  inst.hLogFile     = fopen(inst.logName, fMode);

  // if error...
  if (!inst.hLogFile) {
    Display("Unable to open logfile (\"%s\")", inst.logName);
    return false;
  }

  // write initial log entries; error handling omitted
  // if (inst.stepNbr == 1)
  //   LOG("Inst %s Port %d: Beginning Simulation...", inst.instName,
  //       inst.portNbr);
  // LOG("Inst %s Port %d: Beginning Step %d", inst.instName, inst.portNbr,
  //     inst.stepNbr);
  if (inst.stepNbr == 1) LOG("Beginning Simulation...", 0);

  LOG("Beginning Step %d", inst.stepNbr);

  return true;
}

// CloseServer() -- this is the custom handler for the PORT_CLOSESERVER msg;
// this is a last chance to do any user-defined finalization before the
// server program exits
void CloseServer() {
  // nothing to do here...
}

// ComposeSockAddr() -- unaltered template function
void ComposeSockAddr(unsigned short port, struct sockaddr *addr) {
  memset(addr, 0, sizeof(addr));
  addr->sa_family  = 2;
  addr->sa_data[0] = ((unsigned char *)&port)[1];
  addr->sa_data[1] = ((unsigned char *)&port)[0];
}

// sendMsg() -- moving repetitive template code in message loop that sends a
// message here; server exits ungracefully on error
void sendMsg(SOCKET socket, const char *buffer, int len) {
  if (SOCKET_ERROR == send(socket, buffer, len, 0)) {
    Display("Socket send() failed with error: %d.  Closing server...",
        WSAGetLastError());
    closesocket(socket);
    WSACleanup();
    exit(-1);
  }
}

// main() -- server entry point
int main(int argc, char *argv[]) {
  WSADATA         wsaData = {0};
  struct sockaddr addr    = {0};

  // replace default template buffer size
  //   char buffer[36] = {0};
  char buffer[BUFFER_SIZE] = {0};

  // make port # available for debugging/logging
  //   if (2 == argc) ComposeSockAddr(atoi(argv[1]), &addr);
  inst.portNbr = atoi(argv[1]);
  if (2 == argc) ComposeSockAddr(inst.portNbr, &addr);
  else {
    printf("Usage: %s <port>", argv[0]);
    return -1;
  }

  // Initialize Winsock
  if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
    printf("WSAStartup(%d) failed", MAKEWORD(2, 2));
    return -1;
  }
  // Create SOCKET to listen for client connections.
  SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ListenSocket == INVALID_SOCKET) {
    printf("socket() failed with error : %d", WSAGetLastError());
    WSACleanup();
    return -1;
  }
  // Setup the TCP listening socket
  if (SOCKET_ERROR == bind(ListenSocket, &addr, sizeof(addr))) {
    printf("bind() failed with error : %d", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    return -1;
  }
  if (SOCKET_ERROR == listen(ListenSocket, SOMAXCONN)) {
    printf("listen() failed with error : %d", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    return -1;
  }
  // Accept a client socket
  SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
  if (ClientSocket == INVALID_SOCKET) {
    printf("accept() failed with error: %d", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    return -1;
  }

  closesocket(ListenSocket);   // No longer need listening socket
  // main processing loop -- receive until the client shuts down the
  // connection or PORT_SOCKETCLOSE msg; modified to make msg length available
  // for error checking/debugging while (recv(ClientSocket, buffer,
  // sizeof(buffer), 0) > 0) {
  int  msgLen;
  bool closeSocket = false;

  while (recv(ClientSocket, buffer, sizeof(buffer), 0) > 0) {
    switch (*((int *)buffer)) {
    case PORT_EVALUATE: {
      Evaluate(buffer);
      sendMsg(ClientSocket, buffer, 8 * Noutputs);
      break;
    }

    case PORT_MAX_STEPSIZE: {
      const double dt = MaxStepSize(buffer);
      sendMsg(ClientSocket, (char *)&dt, 8);
      break;
    }

    case PORT_TRUNCATE: {
      const double dt = Truncate(buffer);
      sendMsg(ClientSocket, (char *)&dt, 8);
      break;
    }

    /* custom messages begin here */
    case PORT_GAIN: {
      IntBuf &iBuf = (IntBuf &)buffer;
      inst.gainVal = iBuf.intVal;

      // just return msgID
      sendMsg(ClientSocket, buffer, 4);

      break;
    }

    case PORT_STEPNBR: {
      IntBuf &iBuf = (IntBuf &)buffer;
      inst.stepNbr = iBuf.intVal;

      // just return msgID
      sendMsg(ClientSocket, buffer, 4);

      break;
    }

    case PORT_LOGNAME: {
      StrBuf &sBuf = (StrBuf &)buffer;

      // if received more than once, keep only last value
      if (inst.logName) free(inst.logName);
      inst.logName = strdup(sBuf.strVal);

      // just return msgID
      sendMsg(ClientSocket, buffer, 4);

      break;
    }

    case PORT_INSTNAME: {
      StrBuf &sBuf = (StrBuf &)buffer;

      if (inst.instName) free(inst.instName);
      inst.instName = strdup(sBuf.strVal);

      // just return msgID
      sendMsg(ClientSocket, buffer, 4);

      break;
    }

    case PORT_DESTROY: {
      // call user code
      Destroy();

      // just return msgID
      sendMsg(ClientSocket, buffer, 4);

      break;
    }

    case PORT_POSTPROCESS: {
      // call user code
      PostProcess();

      // just return msgID
      sendMsg(ClientSocket, buffer, 4);

      break;
    }

    case PORT_INITIALZE: {
      // call user code; on error exit ungracefully...
      if (!Initialize()) { return -1; }

      // just return msgID
      sendMsg(ClientSocket, buffer, 4);

      break;
    }

    case PORT_CLOSESERVER: {
      // set shutdown flag
      closeSocket = true;

      // call user code a final time
      CloseServer();

      // exit message loop; do not send a response
      break;
    }

    default: {
      // exit ungracefully...
      Display("Unhandled message in server (%d).  Closing server...",
          *((int *)buffer));
      return -1;
    }
    }

    // exit message loop
    if (closeSocket) break;
  }

  // if msgLen == 0, then the client closed the connection; otherwise,
  // there was a more serious error
  if (msgLen < 0) {
    Display(
        "Socket connection lost unexpectely with error %d.", WSAGetLastError());
  }

  if (SOCKET_ERROR == shutdown(ClientSocket, SD_SEND)) {
    Display("Socket shutdown() failed with error: %d.", WSAGetLastError());
    closesocket(ClientSocket);
    WSACleanup();
    return -1;
  }

  closesocket(ClientSocket);
  WSACleanup();

  return 0;
}
