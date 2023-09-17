/*******************************************************************************
 * WavOut.cpp -- QSpice C-Block component to write *.WAV file data from a
 * circuit signal.
 *
 * Copyright © 2023 Robert Dunn.  Licensed for use under the GNU GPLv3.0.
 ******************************************************************************/
// The code was compiled with Microsoft VC:
//   * Run from within "C:\Program Files\Microsoft Visual Studio\2022\
//     Community\VC\Auxiliary\Build\vcvars32.bat" command line environment
//   * cl /std:c++17 /EHsc /LD wavout.cpp /link /PDBSTRIPPED /out:wavout.dll
//

#include <stdio.h>
#include <stdarg.h>

#include <cmath>
#include <thread>

#include "wavout.h"

#define HIGH true
#define LOW  false

#define FILE_CLOSED 0
#define FILE_OPEN   1
#define FILE_ERROR  -1

#define PROGRAM_NAME    "WavOut"
#define PROGRAM_VERSION "v0.1"
#define PROGRAM_INFO    PROGRAM_NAME " " PROGRAM_VERSION

/*------------------------------------------------------------------------------
 * Standard type overlay for data passed in the uData parameter.
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

// for convenience when ports/attributes are changed, generate a temporary C/C++
// template and copy uData offsets here (with trailing "/" continuation chars).
// #define UDATA_DEFS \
  // double      CLK        = data[0].d; \
  // double      IN1        = data[1].d; \
  // double      IN2        = data[2].d; \
  // int         frequency  = data[3].i; \
  // const char *filename   = data[4].str; \
  // int         maxSamples = data[5].i; \
  // double     &OUT1       = data[6].d; \ double     &OUT2       = data[7].d;
#define UDATA_DEFS                                                             \
  double      CLK        = data[0].d;                                          \
  double      IN1        = data[1].d;                                          \
  double      IN2        = data[2].d;                                          \
  int         frequency  = data[3].i;                                          \
  const char *filename   = data[4].str;                                        \
  int         maxSamples = data[5].i;                                          \
  double     &OUT1       = data[6].d;                                          \
  double     &OUT2       = data[7].d;                                          \
  double     &CLIP       = data[8].d;

/*------------------------------------------------------------------------------
 * constants
 *----------------------------------------------------------------------------*/
const char *msgFileError = "Error writing WAV file.\n";

// #undef pin names lest they collide with names in any header file(s) you might
// include.  (Wouldn't namespaces eliminate this issue?)
#undef CLK
#undef IN1
#undef IN2
#undef OUT1
#undef OUT2
#undef CLIP

/*------------------------------------------------------------------------------
 * Per-instance data structure stuff...
 *----------------------------------------------------------------------------*/
struct DblVals {
  double CH1;
  double CH2;
};

struct IntVals {
  int16_t CH1;
  int16_t CH2;
};

// per instance data
struct InstData {
  // we'll just include entire WAV file header block here because we'll need
  // to fill in the blanks before finalizing the file
  WavHeader wavHeader;

  bool    lastClk      = HIGH;          // last clock state (high/low)
  FILE   *file         = nullptr;       //
  int     fileState    = FILE_CLOSED;   // 0=closed, 1=open, -1=error
  size_t  sampleCnt    = 0;             // number of samples
  int     sampleRate   = 0;             // frequency
  DblVals lastIn       = {0.0, 0.0};    // last channel inputs (for debugging)
  DblVals lastOut      = {0.0, 0.0};    // last sample channel outputs
  IntVals lastBytes    = {0, 0};        // last sample values written to file
  int     maxSamples   = 0;             // 0=no limit
  double  lastClip     = 0.0;           // last clipping output
  bool    clipDetected = false;         // for end of sim warning
};

/*------------------------------------------------------------------------------
 * forward decls
 *----------------------------------------------------------------------------*/
void initInst(InstData *inst, double t, uData *data);
void finalizeFile(InstData &inst);
void writeSamples(InstData &inst);

/*------------------------------------------------------------------------------
 * msg() -- send text to QSpice Output window
 *----------------------------------------------------------------------------*/
// msleep() isn't available in standard libraries...
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
 * wavout() -- QSpice "evaluation function"
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void wavout(
    InstData **opaque, double t, uData *data) {
  // port/attribute offsets/definitions
  UDATA_DEFS;

  InstData *inst = *opaque;

  if (!inst) {
    // construct/save instance
    inst = *opaque = new InstData;
    if (!inst) {   // terminate with extreme prejudice
      msg(__LINE__, "Unable to allocate memory.  Terminating simulation.\n");
      std::exit(1);
    }

    initInst(inst, t, data);
  }

  // default output
  OUT1 = inst->lastOut.CH1;
  OUT2 = inst->lastOut.CH2;
  CLIP = inst->lastClip;

  if (inst->fileState != FILE_OPEN) return;   // nothing to do

  // get current clock state
  bool curClk = CLK > 0.5;

  // quick out if no edge transition
  if (curClk == inst->lastClk) return;

  // save new state
  inst->lastClk = curClk;

  // quick out if rising edge
  if (inst->lastClk) return;

  // falling edge stuff/sampling starts here...
  inst->lastIn.CH1 = IN1;
  inst->lastIn.CH2 = IN2;
  OUT1             = inst->lastOut.CH1;
  OUT2             = inst->lastOut.CH2;
  CLIP             = inst->lastClip;

  if (!inst->sampleCnt && inst->sampleCnt > inst->maxSamples) return;

  writeSamples(*inst);
}

/*------------------------------------------------------------------------------
 * DllMain() -- required DLL entry point, return 1 on success
 *----------------------------------------------------------------------------*/
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

/*------------------------------------------------------------------------------
 * Destroy() -- end of simulation calls this for cleanup
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Destroy(InstData *inst) {
  // finalize the WAV file
  finalizeFile(*inst);

  // release the per-instance memory
  delete inst;
}

/*------------------------------------------------------------------------------
 * initInst() -- initialize, open output file, etc.  much is hardcoded for
 * two-channel, 16-bit PCM.
 *----------------------------------------------------------------------------*/
void initInst(InstData *inst, double t, uData *data) {
  // port/attribute offsets/definitions
  UDATA_DEFS;

  inst->lastClk    = HIGH;
  inst->sampleRate = frequency;
  inst->maxSamples = maxSamples;

  // open file
  inst->fileState = FILE_ERROR;   // default

  inst->file = fopen(filename, "w+b");   // "b" for binary mode!!!
  if (!inst->file) {
    msg(__LINE__, "Unable to create/open WAV file \"%s\"\n.", filename);
    return;
  }

  msg(__LINE__, "Creating WAV file \"%s\" with %d samples/second.\n", filename,
      frequency);

  inst->wavHeader.samplesPerSec = frequency;
  inst->wavHeader.avgBytesPerSec =
      inst->sampleRate * inst->wavHeader.nbrChannels * sizeof(int16_t);

  // write file header info (will be written again when finalizing the file)
  // this positions the file for subsequently writing sample data
  if (!fwrite(&inst->wavHeader, sizeof(WavHeader), 1, inst->file)) {
    msg(__LINE__, msgFileError);
    return;
  }

  // so far, so good
  inst->fileState = FILE_OPEN;
}

/*------------------------------------------------------------------------------
 * finalizeFile() -- rewrite the header block with updated values (chunk sizes)
 * and close file.
 *----------------------------------------------------------------------------*/
void finalizeFile(InstData &inst) {
  // default to error
  inst.fileState = FILE_ERROR;

  // we are finalizing so first flush (belt + suspenders?) and reposition to
  // start of file
  if (fflush(inst.file) || fseek(inst.file, 0, SEEK_SET)) {
    msg(__LINE__, msgFileError);
    return;
  }

  // update header values before writing
  int dataSize = inst.sampleCnt * sizeof(uint16_t) * inst.wavHeader.nbrChannels;
  inst.wavHeader.chunkSize     = sizeof(WavHeader) + dataSize - 8;
  inst.wavHeader.dataChunkSize = dataSize;   //???

  // write the final file header info
  if (fwrite(&inst.wavHeader, 1, sizeof(WavHeader), inst.file) !=
      sizeof(WavHeader)) {
    msg(__LINE__, msgFileError);
    return;
  }

  // close the file
  msg(__LINE__, "Closing WAV file.  %d samples written.\n", inst.sampleCnt);
  if (inst.clipDetected) msg(__LINE__, "Warning:  Clipping was detected.\n");
  fclose(inst.file);
  inst.fileState = FILE_CLOSED;
}

/*------------------------------------------------------------------------------
 * convToInt() & convToDbl() -- convert sample data between doubles (on QSpice
 * side) and ints (WAV file side) with input clipping and rounding
 *----------------------------------------------------------------------------*/
inline int16_t convToInt(InstData &inst, double val) {
  // clip to max input value
  if (val > 1.0) {
    inst.clipDetected = true;
    inst.lastClip     = 1.0;
    val               = 1.0;
  } else if (val < -1.0) {
    inst.clipDetected = true;
    inst.lastClip     = 1.0;
    val               = -1.0;
  }

  // scale value to max int
  val *= (double)0x7fff;

  // round to nearest integer
  return (int16_t)round(val);
}

inline double convToDbl(int16_t val) {
  // this function needs to be fast so pre-calculate a multiplication factor
  // to avoid slow division
  constexpr double factor = 1.0 / 0x7fff;
  return val * factor;
}

/*------------------------------------------------------------------------------
 * writeSamples() -- write sample data to WAV file
 *----------------------------------------------------------------------------*/
void writeSamples(InstData &inst) {
  // convert inputs to 16-bit sample values
  inst.lastClip      = 0.0;
  inst.lastBytes.CH1 = convToInt(inst, inst.lastIn.CH1);
  inst.lastBytes.CH2 = convToInt(inst, inst.lastIn.CH2);

  // convert back to double for output
  inst.lastOut.CH1 = convToDbl(inst.lastBytes.CH1);
  inst.lastOut.CH2 = convToDbl(inst.lastBytes.CH2);

  // write the samples to the file
  if (fwrite(&inst.lastBytes, 1, sizeof(inst.lastBytes), inst.file) !=
      sizeof(inst.lastBytes)) {
    msg(__LINE__, msgFileError);
    inst.fileState = FILE_ERROR;
    return;
  }

  inst.sampleCnt++;
}
/*==============================================================================
 * End of WavOut.cpp
 *============================================================================*/