/*==============================================================================
 * SpiMaster.cpp -- Generic Spi master device.
 *============================================================================*/
// Note:  Compile with MS VC

#include <stdio.h>
#include <stdarg.h>
#include <thread>

#include "SpiIO.h"
#include "PinIO.h"

// versioning for messages
#define PROGRAM_NAME    "SpiMaster"
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
#undef EN
#undef SCLK
#undef MOSI
#undef MISO
#undef SS1
#undef SS2
#undef VCC

/*------------------------------------------------------------------------------
 * Components may use the uData array of ports/attributes passed by QSpice in
 * several places.  If the ports/attributes are changed, the array offsets
 * change.  For convenience, I #define it here so that later changes to
 * ports/attributes require code changes only here.
 *----------------------------------------------------------------------------*/
#define UDATA                                                                  \
  double  EN      = data[0].d;                                                 \
  double  MISO    = data[1].d;                                                 \
  double  VCC     = data[2].d;                                                 \
  int     SPIFREQ = data[3].i;                                                 \
  int     SPIMODE = data[4].i;                                                 \
  double &SCLK    = data[5].d;                                                 \
  double &MOSI    = data[6].d;                                                 \
  double &SS1     = data[7].d;                                                 \
  double &SS2     = data[8].d;

/*------------------------------------------------------------------------------
 * Per instance data structure.  Allocated in evalutation function.
 *----------------------------------------------------------------------------*/
struct InstData {
  PinIn        enPinIn;           // enable signal
  PinIn        misoPinIn;         // MISO pin
  PinOut       mosiPinOut;        // MOSI pin
  PinOut       sclkPinOut;        // SPI clock
  PinOut       ss1PinOut;         // slave select
  PinOut       ss2PinOut;         // slave select
  SerialBuffer sBuf;              // output buffer
  SpiModeTbl   spiMode;           // SPI mode for instance
  bool         adcRead = true;    // true=reading ADC, false = writing DAC
  double       sclkHalfCycleT;    // half SCLK cycle seconds
  double       sclkNextToggleT;   // next simulation time to toggle SCLK
};

/*------------------------------------------------------------------------------
 * Fwd decls
 *----------------------------------------------------------------------------*/
bool loadDataBuf(InstData &inst, uData *data);
void processDataBuf(InstData &inst, uData *data);

/*------------------------------------------------------------------------------
 * Constants
 *----------------------------------------------------------------------------*/
const double       eternity   = 1.7e308;   // end of the 'verse
const unsigned int SpiFreqDef = 10000;     // default speed if attribute invalid
const unsigned int SpiModeDef = 3;   // default SPI mode if attribute invalid

/*------------------------------------------------------------------------------
 * spimaster() -- evaluation function called by QSpice.  This should not require
 * modification -- use loadDataBuf() and processDataBuf() to implement SPI
 * devices.
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void spimaster(
    InstData **opaque, double t, uData *data) {
  UDATA;

  InstData *inst = *opaque;

  if (!inst) {
    // first time, VCC is 0.0V so delay until VCC is something valid...
    if (VCC == 0.0) return;

    *opaque = inst = new InstData();
    if (!inst) {   // terminate with prejudice
      msg(__LINE__, "Unable to allocate memory.  Terminating simulation.\n");
      std::terminate();
    }

    // set up SPI SCLK freqency from attribute
    if (SPIFREQ < 1) {
      msg(__LINE__, "SpiFreq=%d is not valid.  Using default frequency=%dHz.\n",
          SPIFREQ, SpiFreqDef);
      SPIFREQ = SpiFreqDef;
    }

    inst->sclkHalfCycleT  = (1.0 / SPIFREQ) / 2.0;   // seconds per half cycle
    inst->sclkNextToggleT = eternity;

    // set up SPI mode from attribute
    if (SPIMODE < 0 || SPIMODE > 3) {
      msg(__LINE__,
          "SpiMode=%d is not valid.  Valid values are 0-3.  Using default "
          "mode=%d.\n",
          SPIMODE, SpiModeDef);
      SPIMODE = SpiModeDef;
    }
    inst->spiMode = spiModes[SPIMODE];

    // set up PinIn instances
    inst->enPinIn   = PinIn(VCC, EN);
    inst->misoPinIn = PinIn(VCC, MISO);

    // initialize PinOut instances
    SCLK = (inst->sclkPinOut = PinOut(VCC, inst->spiMode.sclkIdle)).getStateV();
    MOSI = (inst->mosiPinOut = PinOut(VCC, PinState::LOW)).getStateV();
    SS1  = (inst->ss1PinOut = PinOut(VCC, PinState::HIGH)).getStateV();
    SS2  = (inst->ss2PinOut = PinOut(VCC, PinState::HIGH)).getStateV();

    // debug info
    msg(__LINE__, "SpiFreq=%dHz, SpiMode=%d.\n", SPIFREQ, SPIMODE);
  }

  // set PinIn states from inputs
  inst->enPinIn.setState(EN);
  inst->misoPinIn.setState(MISO);

  if (inst->enPinIn.isRising()) {
    // interrupt any processing that might be under way & reset outputs to idle
    // TODO

    // we're really done -- set stuff to idle
    // stop SCLK
    inst->sclkNextToggleT = eternity;
    SCLK                  = inst->sclkPinOut.setIdle().getStateV();

    // disable slaves
    SS1 = inst->ss1PinOut.setHigh().getStateV();
    SS2 = inst->ss2PinOut.setHigh().getStateV();

    // set MOSI to idle
    MOSI = inst->mosiPinOut.setIdle().getStateV();

    inst->sBuf.endIO();
  }

  if (inst->enPinIn.isHigh()) { return; }

  // if just now enabled, set up for start of SPI I/O
  // note that SCLK must be in idle state because wasn't enabled
  if (inst->enPinIn.isFalling()) {
    // load data
    bool haveData = loadDataBuf(*inst, data);
    if (!haveData) return;

    // enable slave device
    if (inst->adcRead) {
      SS1 = inst->ss1PinOut.setLow().getStateV();

      // TODO -- remove?
      SS2 = inst->ss2PinOut.setHigh().getStateV();
    } else {
      SS2 = inst->ss2PinOut.setLow().getStateV();

      // TODO -- remove?
      SS1 = inst->ss1PinOut.setHigh().getStateV();
    }

    MOSI = inst->mosiPinOut.setState(inst->sBuf.getBitOut()).getStateV();

    // next sim time to toggle SCLK
    inst->sclkNextToggleT = t + inst->sclkHalfCycleT;
  }

  // toggle SCLK now?
  if (t < inst->sclkNextToggleT) return;

  SCLK = inst->sclkPinOut.toggleState().getStateV();

  // next sim time to toggle SCLK
  inst->sclkNextToggleT = t + inst->sclkHalfCycleT;

  // set MOSI
  if (inst->sclkPinOut.getEdge() == inst->spiMode.sclkOutEdge ||
      inst->ss1PinOut.getEdge() == inst->spiMode.csOutEdge) {

    if (inst->sBuf.isDone()) {
      // we're really done -- set stuff to idle
      // stop SCLK
      inst->sclkNextToggleT = eternity;
      SCLK                  = inst->sclkPinOut.setIdle().getStateV();

      // disable slave devices
      SS1 = inst->ss1PinOut.setHigh().getStateV();
      SS2 = inst->ss2PinOut.setHigh().getStateV();

      // set MOSI to idle
      MOSI = inst->mosiPinOut.setIdle().getStateV();
      return;
    }
    MOSI = inst->mosiPinOut.setState(inst->sBuf.getBitOut()).getStateV();
  }

  // read MISO
  if (inst->sclkPinOut.getEdge() == inst->spiMode.sclkInEdge) {
    inst->sBuf.setBitIn(inst->misoPinIn.isHigh());
    if (inst->sBuf.isDone()) {
      // process data
      processDataBuf(*inst, data);

      // swap read ADC / write DAC
      inst->adcRead = !inst->adcRead;
    }
  }
}

/*------------------------------------------------------------------------------
 * MaxExtStepSize() -- set timestep to SPI clock frequency
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) double MaxExtStepSize(InstData *inst) {
  return inst->sclkHalfCycleT;
}

/*------------------------------------------------------------------------------
 * Trunc() -- force simulation to trigger on timed SPI clock edge
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Trunc(
    InstData *inst, double t, uData *data, double *timestep) {
  UDATA;

  double ttol = 1e-9;

  if (t < inst->sclkNextToggleT) ttol = inst->sclkNextToggleT - t;
  *timestep = ttol;
}

/*------------------------------------------------------------------------------
 * loadDataBuf() -- called when data exchange begins.
 *
 * modify this function to load data to be sent.
 *----------------------------------------------------------------------------*/
bool loadDataBuf(InstData &inst, uData *data) {
  UDATA(data);

  // if we're reading the ADC, just send zeros; otherwise copy data from ADC
  // read back into buffer for DAC write
  uint8_t bitsToSend = inst.adcRead ? 0 : (uint8_t)inst.sBuf.getData();
  inst.sBuf.startIO(bitsToSend, 8);
  return true;
}

/*------------------------------------------------------------------------------
 * processDataBuf() -- called when data exchange ends (buffer is full).
 *
 * modify this function to do something with the data received.
 *----------------------------------------------------------------------------*/
void processDataBuf(InstData &inst, uData *data) {
  UDATA(data);

  /* nothing to do for this device */
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
 * End of SpiMaster.cpp
 *============================================================================*/