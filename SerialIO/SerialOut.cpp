/*==============================================================================
 * SerialOut.cpp -- Serial output device proof of concept.
 *============================================================================*/

// Note:  Compile with MS VC

#include <stdio.h>
#include <stdarg.h>
#include <thread>

#include "SerialBuf.h"

// versioning
#define PROGRAM_NAME    "SerialOut"
#define PROGRAM_VERSION "v0.1"
#define PROGRAM_INFO    PROGRAM_NAME " " PROGRAM_VERSION

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

// #undef pin names lest they collide with names in any header file(s) you might
// include.
#undef CLK
#undef CS
#undef DSO
#undef VCC
#undef TEST

// int DllMain() must exist and return 1 for a process to load the .DLL
// See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain for more
// information.
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

#define msleep(msecs)                                                          \
  std::this_thread::sleep_for(std::chrono::milliseconds(msecs))

void msg(int lineNbr, const char *fmt, ...) {
  msleep(30);
  fflush(stdout);
  fprintf(stdout, PROGRAM_INFO " (@%d) ", lineNbr);
  va_list args = {0};
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  fflush(stdout);
  msleep(30);
}

#define HIGH         true
#define LOW          false
#define EDGE_FALLING false
#define EDGE_RISING  true

struct InstData {
  bool             lastClk = LOW;
  bool             lastEn  = HIGH;
  bool             lastDSO = LOW;
  int              testCtr = 0;
  SerialOutBuffer8 sBuf;
};

unsigned int testData[] = {
    0x88, 0x11, 0x55, 0xaa, 0xde, 0xad, 0xbe, 0xef, 0x00};

extern "C" __declspec(dllexport) void serialout(
    InstData **opaque, double t, uData *data) {
  double  CLK  = data[0].d;   // input
  double  CS   = data[1].d;   // input
  double  VCC  = data[2].d;   // input
  double  TEST = data[3].d;   // input
  double &DSO  = data[4].d;   // output

  InstData *inst = *opaque;

  if (!*opaque) {
    *opaque = inst = new InstData();
    if (!inst) {   // terminate with prejudice
      msg(__LINE__, "Unable to allocate memory.  Terminating simulation.\n");
      std::terminate();
    }
  }

  /*------------------------------------------------------------------------------
   * Process:
   * (1) Wait for CS to go low.
   * (2) If already low and goes high, stop output.
   * (3) Latch test voltage on first enable (this is our test data).
   * (4) Write one bit on each falling clock edge.
   * (5) When buffer is empty, output low.
   *----------------------------------------------------------------------------*/

  const bool enableState  = LOW;            // active low
  const bool clockOutEdge = EDGE_FALLING;   // clock data out on falling edge

  double vRef     = VCC / 2;
  bool   enState  = (CS > vRef);
  bool   enabled  = enState == enableState;
  bool   clkState = CLK > vRef;

  DSO = inst->lastDSO * VCC;

  if (!enabled) {
    inst->lastEn  = enabled;
    inst->lastClk = clkState;
    inst->lastDSO = LOW;
    inst->sBuf.serialEnd();   // end if active
    return;
  }

  if (enabled != inst->lastEn) {
    inst->lastEn = enabled;
    // latch test data -- we'll use the testData defined above
    inst->sBuf.serialStart(testData[inst->testCtr]);
    if (inst->testCtr < sizeof(testData) / sizeof(testData[0])) inst->testCtr++;
  }

  // clock data out on configured edge of clock
  if (clkState == inst->lastClk) { return; }

  inst->lastClk = clkState;

  if (clkState != clockOutEdge) { return; }

  // send the data
  inst->lastDSO = inst->sBuf.serialOut();
  DSO           = inst->lastDSO * VCC;
}

extern "C" __declspec(dllexport) void Destroy(struct InstData *inst) {
  //   free(inst);
  delete inst;
}
