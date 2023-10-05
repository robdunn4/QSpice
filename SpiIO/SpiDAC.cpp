/*==============================================================================
 * SpiDAC.cpp -- Demonstration DAC SPI slave device.
 *============================================================================*/
// Note:  Compile with MS VC

#include <stdio.h>
#include <stdarg.h>
#include <thread>

#include "PinIO.h"
#include "SpiIO.h"

// versioning for messages
#define PROGRAM_NAME    "SpiDAC"
#define PROGRAM_VERSION "v0.1"
#define PROGRAM_INFO    PROGRAM_NAME " " PROGRAM_VERSION

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

/*------------------------------------------------------------------------------
 * uData -- union overlay for passed port/attribute data.
 *----------------------------------------------------------------------------*/
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
#undef CS
#undef SCLK
#undef VCC
#undef MOSI
#undef MISO
#undef DAC_OUT

/*------------------------------------------------------------------------------
 * Components may use the uData array of ports/attributes passed by QSpice in
 * several places.  If the ports/attributes are changed, the array offsets
 * change.  For convenience, I #define it here so that later changes to
 * ports/attributes require code changes only here.
 *----------------------------------------------------------------------------*/
#define UDATA(data)                                                            \
  double  CS      = data[0].d;                                                 \
  double  SCLK    = data[1].d;                                                 \
  double  VCC     = data[2].d;                                                 \
  double  MOSI    = data[3].d;                                                 \
  int     SPIMODE = data[4].i;                                                 \
  double &MISO    = data[5].d;                                                 \
  double &DAC_OUT = data[6].d;

/*------------------------------------------------------------------------------
 * Per instance data structure.  Allocated in evalutation function.
 *----------------------------------------------------------------------------*/
struct InstData {
  PinIn        sclkPinIn;    // system clock
  PinIn        csPinIn;      // chip select signal
  PinOut       misoPinOut;   // MISO pin
  PinIn        mosiPinIn;    // MOSI pin
  SerialBuffer sBuf;         // output buffer
  SpiModeTbl   spiMode;      // SPI mode for instance
  double       dacOutV;      // DAC output pin
};

/*------------------------------------------------------------------------------
 * Fwd decls
 *----------------------------------------------------------------------------*/
void loadDataBuf(InstData &inst, uData *data);
void bitReceived(InstData &inst, uData *data);
void processDataBuf(InstData &inst, uData *data);

/*------------------------------------------------------------------------------
 * Constants
 *----------------------------------------------------------------------------*/
const unsigned int SpiModeDef = 3;   // default SPI mode if attribute invalid

/*------------------------------------------------------------------------------
 * spidac() -- evaluation function called by QSpice.  This should not require
 * modification -- use loadDataBuf(), processDataBuf(), and bitReceived() to
 * implement SPI devices.
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void spidac(
    InstData **opaque, double t, uData *data) {
  UDATA(data);

  InstData *inst = *opaque;

  if (!inst) {
    // first time, VCC is 0.0V so delay until VCC is something valid...
    if (VCC == 0.0) return;

    *opaque = inst = new InstData();
    if (!inst) {   // terminate with prejudice
      msg(__LINE__, "Unable to allocate memory.  Terminating simulation.\n");
      std::terminate();
    }

    // get SPI mode component instance attribute
    if (SPIMODE < 0 || SPIMODE > 3) {
      msg(__LINE__,
          "SpiMode=%d is not valid.  Valid values are 0-3.  Using default "
          "mode=%d.\n",
          SPIMODE, SpiModeDef);
      SPIMODE = SpiModeDef;
    }
    inst->spiMode = spiModes[SPIMODE];

    // configure some pins
    inst->csPinIn    = PinIn(VCC, CS);
    inst->misoPinOut = PinOut(VCC, PinState::LOW);
    inst->mosiPinIn  = PinIn(VCC, MOSI);
    DAC_OUT          = 0.0;   // start at 0V

    // debug info
    msg(__LINE__, "SpiMode=%d.\n", SPIMODE);

    // for now, just return after initialization
    return;
  }

  // set PinIn states from inputs
  inst->csPinIn.setState(CS);
  inst->mosiPinIn.setState(MOSI);
  inst->sclkPinIn.setState(SCLK);

  if (inst->csPinIn.isHigh() && !inst->csPinIn.isRising()) { return; }

  // if just now enabled, set up for start of SPI I/O
  // note that SCLK must be in idle state because wasn't enabled
  if (inst->csPinIn.isFalling()) {
    // load data
    loadDataBuf(*inst, data);
  }

  // read MOSI
  if (inst->sclkPinIn.getEdge() == inst->spiMode.sclkInEdge) {
    inst->sBuf.setBitIn(inst->mosiPinIn.isHigh());
    if (inst->sBuf.isDone()) {
      // process data
      processDataBuf(*inst, data);

      // set MOSI to idle
      MISO = inst->misoPinOut.setIdle().getStateV();
    } else bitReceived(*inst, data);
    return;
  }

  // set MISO
  if (inst->csPinIn.isRising()) return;
  if (inst->sclkPinIn.getEdge() == inst->spiMode.sclkOutEdge ||
      inst->csPinIn.getEdge() == inst->spiMode.csOutEdge) {
    MISO = inst->misoPinOut.setState(inst->sBuf.getBitOut()).getStateV();
    return;
  }
}

/*------------------------------------------------------------------------------
 * loadDataBuf() -- called when data exchange begins.
 *
 * modify this function to load data to be sent.
 *----------------------------------------------------------------------------*/
void loadDataBuf(InstData &inst, uData *data) {
  UDATA(data);

  // send zeros
  inst.sBuf.startIO(0, 8);
}

/*------------------------------------------------------------------------------
 * bitReceived() -- called after each bit is received into buffer.
 *
 * modify this function if the SPI slave device needs to receive some bits
 * before it can determine what remaining bits to send.
 *----------------------------------------------------------------------------*/
void bitReceived(InstData &inst, uData *data) {
  UDATA(data);

  /* nothing to do for this device */
}

/*------------------------------------------------------------------------------
 * processDataBuf() -- called when data exchange ends (buffer is full).
 *
 * modify this function to do something with the data received.
 *----------------------------------------------------------------------------*/
void processDataBuf(InstData &inst, uData *data) {
  UDATA(data);

  // set output voltage (VCC-referenced)
  uint8_t vOut8 = (uint8_t)(inst.sBuf.getData());
  double  x     = vOut8;
  DAC_OUT = inst.dacOutV = x * VCC / 0xff;
}

/*------------------------------------------------------------------------------
 * Destroy() -- called by QSpice when simulation ends.
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Destroy(struct InstData *inst) {
  // delete per-instance data allocated in the evaluation function
  delete inst;
}

/*------------------------------------------------------------------------------
 * int DllMain() must exist and return 1 for a process to load the .DLL
 * See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain for more
 * information.
 *----------------------------------------------------------------------------*/
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}
/*==============================================================================
 * End of SpiDAC.cpp
 *============================================================================*/