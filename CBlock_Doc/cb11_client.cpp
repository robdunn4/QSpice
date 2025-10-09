//------------------------------------------------------------------------------
// CB11_Client.cpp -- Example client DLL code for C-Block Basics #11.
//------------------------------------------------------------------------------
// Full project code and documentation is available here:
//    https://github.com/robdunn4/QSpice
//
// This code compiles successfully using the Digital Mars C++ Compiler:
//    dmc -mn -WD cb11_client.cpp kernel32.lib
//
// It also compiles successfully with MSVC 2022 (Community Edition)
//

#include <cstring>
#include <malloc.h>

#define BUFFER_SIZE 1024   // message buffer size

extern "C" __declspec(dllexport) void (*Display)(const char *format, ...) = 0;
extern "C" __declspec(dllexport) void (*EXIT)(const char *format, ...)    = 0;
extern "C" __declspec(dllexport) const double *DegreesC                   = 0;

// externs copied from basic C++ non-client DLL
extern "C" __declspec(dllexport) const int         *StepNumber   = 0;
extern "C" __declspec(dllexport) const char *const *InstanceName = 0;

// Berkeley Socket API:
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

extern "C" __declspec(dllexport) unsigned int (*BerkeleySocket)(
    const char *hostPortServiceString, int Ninputs, int Noutputs,
    unsigned char **buffer) = 0;
extern "C" __declspec(dllexport) int (*SocketSend)(
    unsigned int socket, unsigned char *buffer, int count) = 0;
extern "C" __declspec(dllexport) int (*SocketRecv)(
    unsigned int socket, unsigned char *buffer, int count)                = 0;
extern "C" __declspec(dllexport) void (*SocketClose)(unsigned int socket) = 0;
extern "C" __declspec(dllexport) double *(*ConfigureBuffer)(
    unsigned char *buffer, int code) = 0;

union uData {
  bool                   b;
  char                   c;
  unsigned char          uc;
  short                  s;
  unsigned short         us;
  int                    i;
  unsigned int           ui;
  float                  f;
  double                 d;
  long long int          i64;
  unsigned long long int ui64;
  char                  *str;
  unsigned char         *bytes;
};

int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

const int Ninputs  = 2;
const int Noutputs = 1;

// this per-instance data is used by the QSocks framework; don't save component
// per-instance data here, put it in server code
struct InstData {
  unsigned int   ConnectSocket;
  unsigned char *buffer;
};

// define some structures to re-cast messages for convenience/clarity
struct IntBuf {
  int msgID;
  int intVal;
};
struct StrBuf {
  int  msgID;
  char strVal[BUFFER_SIZE - sizeof(StrBuf::msgID)];
};

/*
 * Evaluation function -- heavily modified from template
 */
extern "C" __declspec(dllexport) void cb11_client(
    InstData **opaque, double t, uData *data) {
  double      IN      = data[0].d;     // input
  double      EN      = data[1].d;     // input
  int         Gain    = data[2].i;     // input parameter
  const char *LogName = data[3].str;   // input parameter
  const char *server  = data[4].str;   // input parameter
  double     &OUT     = data[5].d;     // output

  InstData *inst = *opaque;

  if (!inst) { /* begin one-time initialization section */
    inst = *opaque = (InstData *)calloc(1, sizeof(InstData));
    inst->ConnectSocket =
        BerkeleySocket(server, Ninputs, Noutputs, &inst->buffer);

    // replace the buffer allocated by BerkeleySocket() with a larger buffer
    free(inst->buffer);
    inst->buffer = (unsigned char *)calloc(1, BUFFER_SIZE);

    // send PORT_GAIN
    IntBuf *iBuf = (IntBuf *)inst->buffer;
    iBuf->msgID  = PORT_GAIN;
    iBuf->intVal = Gain;
    SocketSend(inst->ConnectSocket, inst->buffer, 8);
    if (SocketRecv(inst->ConnectSocket, inst->buffer, BUFFER_SIZE) != 4)
      EXIT("PORT_GAIN message returned invalid length.");

    // send PORT_STEPNBR
    iBuf->msgID  = PORT_STEPNBR;
    iBuf->intVal = *StepNumber;
    SocketSend(inst->ConnectSocket, inst->buffer, 8);
    if (SocketRecv(inst->ConnectSocket, inst->buffer, BUFFER_SIZE) != 4)
      EXIT("PORT_STEPNBR message returned invalid length.");

    // send PORT_INSTNAME; error checks for string length omitted
    StrBuf *sBuf = (StrBuf *)inst->buffer;
    int     len  = strlen(*InstanceName);
    sBuf->msgID  = PORT_INSTNAME;
    strcpy(sBuf->strVal, *InstanceName);
    SocketSend(inst->ConnectSocket, inst->buffer, 4 + len + 1);
    if (SocketRecv(inst->ConnectSocket, inst->buffer, BUFFER_SIZE) != 4)
      EXIT("PORT_INSTNAME message returned invalid length.");

    // send PORT_LOGNAME; error checks for string length omitted
    len         = strlen(LogName);
    sBuf->msgID = PORT_LOGNAME;
    strcpy(sBuf->strVal, LogName);
    SocketSend(inst->ConnectSocket, inst->buffer, 4 + len + 1);
    if (SocketRecv(inst->ConnectSocket, inst->buffer, BUFFER_SIZE) != 4)
      EXIT("PORT_LOGNAME message returned invalid length.");

    // send PORT_INITIALIZE (notification, no data)
    ConfigureBuffer(inst->buffer, PORT_INITIALZE);
    SocketSend(inst->ConnectSocket, inst->buffer, 4);
    if (SocketRecv(inst->ConnectSocket, inst->buffer, BUFFER_SIZE) != 4)
      EXIT("PORT_INITIALIZE message returned invalid length.");
  } /* end one-time initialization section */

  // send time-point and input port data to server
  double *vector = ConfigureBuffer(inst->buffer, PORT_EVALUATE);
  vector[0]      = t;
  vector[1]      = IN;
  vector[2]      = EN;
  SocketSend(inst->ConnectSocket, inst->buffer, 12 + 8 * Ninputs);

  // receive calculated output port data from server
  SocketRecv(inst->ConnectSocket, inst->buffer, 8 * Noutputs);
  OUT = ((double *)inst->buffer)[0];
}

/*
 * MaxExtStepSize() -- if not used in server code, remove this function to speed
 * up simulation
 */
extern "C" __declspec(dllexport) double MaxExtStepSize(
    InstData *inst, double t) {
  double *vector = ConfigureBuffer(inst->buffer, PORT_MAX_STEPSIZE);
  vector[0]      = t;

  SocketSend(inst->ConnectSocket, inst->buffer, 12);
  SocketRecv(inst->ConnectSocket, inst->buffer, 8);

  const double dt = ((double *)inst->buffer)[0];
  return dt > 0. ? dt : 1e308;
}

/*
 * Trunc() -- if not used in server code, remove this function to speed up
 * simulation
 */
extern "C" __declspec(dllexport) void Trunc(
    InstData *inst, double t, uData *data, double *timestep) {
  double IN = data[0].d;   // input
  double EN = data[1].d;   // input

  double *vector = ConfigureBuffer(inst->buffer, PORT_TRUNCATE);
  vector[0]      = t;
  vector[1]      = IN;
  vector[2]      = EN;
  vector[3]      = *timestep;

  SocketSend(inst->ConnectSocket, inst->buffer, 12 + 8 * Ninputs);
  SocketRecv(inst->ConnectSocket, inst->buffer, 8);

  const double dt = ((double *)inst->buffer)[0];
  if ((dt > 0.) && (*timestep > dt)) *timestep = dt;
}

/*
 * Destroy() -- modified
 */
extern "C" __declspec(dllexport) void Destroy(InstData *inst) {
  // send event message
  ConfigureBuffer(inst->buffer, PORT_DESTROY);
  SocketSend(inst->ConnectSocket, inst->buffer, 4);
  SocketRecv(inst->ConnectSocket, inst->buffer, 4);

  // send event message; do not wait for a response (there won't be one)
  ConfigureBuffer(inst->buffer, PORT_CLOSESERVER);
  SocketSend(inst->ConnectSocket, inst->buffer, 4);

  // close client socket and release allocated memory
  SocketClose(inst->ConnectSocket);
  free(inst->buffer);
  free(inst);
}

/*
 * PostProcess() -- added manually
 */
extern "C" __declspec(dllexport) void PostProcess(InstData *inst) {
  // send event message
  ConfigureBuffer(inst->buffer, PORT_POSTPROCESS);
  SocketSend(inst->ConnectSocket, inst->buffer, 4);
  SocketRecv(inst->ConnectSocket, inst->buffer, 4);
}
