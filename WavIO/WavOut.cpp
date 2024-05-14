/*******************************************************************************
 * WavOut.cpp -- QSpice C-Block component to write *.WAV file data from a
 * circuit signal.
 *
 * 2024.05.08 - v0.2 Added support for 24-bit PCM stereo.
 * 2024.05.11 - v0.3 Revised normalization factor.
 * 2024.05.12 - v0.3 Fixed max samples limit.
 *
 * Copyright © 2023-2024 Robert Dunn.  Licensed for use under the GNU GPLv3.0.
 ******************************************************************************/
// The code was compiled with Microsoft VC:
//   * Run from within "C:\Program Files\Microsoft Visual Studio\2022\
//     Community\VC\Auxiliary\Build\vcvars32.bat" command line environment
//   * cl /std:c++17 /EHsc /LD wavout.cpp /link /PDBSTRIPPED /out:wavout.dll
//

#include "wavout.h"
#include <cmath>
#include <stdarg.h>
#include <stdio.h>
#include <thread>

#define HIGH true
#define LOW  false

#define FILE_CLOSED 0
#define FILE_OPEN   1
#define FILE_ERROR  -1

#define PROGRAM_NAME    "WavOut"
#define PROGRAM_VERSION "v0.3"
#define PROGRAM_INFO    PROGRAM_NAME " " PROGRAM_VERSION

/*------------------------------------------------------------------------------
 * Standard QSpice type overlay for data passed in the uData parameter.
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
#define UDATA_DEFS                                                             \
  double      IN1        = data[0].d;                                          \
  double      IN2        = data[1].d;                                          \
  int         frequency  = data[2].i;                                          \
  const char *filename   = data[3].str;                                        \
  int         maxSamples = data[4].i;                                          \
  int         bitDepth   = data[5].i;                                          \
  double     &OUT1       = data[6].d;                                          \
  double     &OUT2       = data[7].d;                                          \
  double     &CLIP       = data[8].d;

/*------------------------------------------------------------------------------
 * constants
 *----------------------------------------------------------------------------*/
const char *msgFileError = "Error writing WAV file.\n";

// #undef pin names lest they collide with names in any header file(s) you might
// include.  (Wouldn't namespaces eliminate this issue?)
// #undef CLK
#undef IN1
#undef IN2
#undef OUT1
#undef OUT2
#undef CLIP

/*------------------------------------------------------------------------------
 * Per-instance data structure stuff...
 *----------------------------------------------------------------------------*/
struct DblVals {
  double CH1 = 0;
  double CH2 = 0;
};

struct IntVals {
  int32_t CH1 = 0;
  int32_t CH2 = 0;
};

// per instance data
struct InstData {
  // we'll just include entire WAV file header block here because we'll need
  // to fill in the blanks before finalizing the file
  WavHeader wavHeader;

  double  nextSample_t   = 0;             // next sample simulation time
  double  nextIncr_t     = 0;             // time increment for next sample
  double  sampleIncr_t   = 0;             // time between samples (1/Hz)
  FILE   *file           = nullptr;       // output file
  int     fileState      = FILE_CLOSED;   // 0=closed, 1=open, -1=error
  size_t  sampleCnt      = 0;             // number of samples
  int     sampleRate     = 0;             // frequency
  int     bitDepth       = 0;             // bits per sample (16 or 24)
  int     bytesPerSample = 0;             // bytes per sample
  DblVals lastIn;                         // last channel inputs (for debugging)
  DblVals lastOut;                        // last sample channel outputs
  IntVals lastBytes;                      // last sample values written to file
  int     maxSamples   = 0;               // 0=no limit
  double  lastClip     = 0;               // last clipping output
  bool    clipDetected = false;           // for end of sim warning
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
    // construct instance
    inst = *opaque = new InstData;
    if (!inst) {   // terminate with extreme prejudice
      msg(__LINE__, "Unable to allocate memory.  Terminating simulation.\n");
      std::exit(1);
    }

    // remaining initialization
    initInst(inst, t, data);
  }

  inst->lastIn.CH1 = IN1;
  inst->lastIn.CH2 = IN2;
  OUT1             = inst->lastOut.CH1;
  OUT2             = inst->lastOut.CH2;
  CLIP             = inst->lastClip;

  if (inst->fileState != FILE_OPEN) return;   // nothing to do

  // limit # of samples if maxSamples > 0
  if (inst->maxSamples && inst->sampleCnt > inst->maxSamples) {
    // set next sample at eternity to disable Trunc()?
    inst->nextSample_t = 1e308;
    return;
  }

  if (t >= inst->nextSample_t) {
    writeSamples(*inst);
    // note:  rounding errors will accumulate...
    inst->nextSample_t += inst->sampleIncr_t;
  }

  // adjust increment used in Trunc()
  inst->nextIncr_t = inst->nextSample_t - t;
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
 * Trunc() -- limit timestep to next sample timepoint
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Trunc(
    InstData *inst, double t, uData *data, double *timestep) {

  // calculate implied timestep
  double tstep = t - inst->nextSample_t;

  if (tstep > 0) *timestep = inst->nextIncr_t;
}

/*------------------------------------------------------------------------------
 * initInst() -- initialize, open output file, etc.
 *----------------------------------------------------------------------------*/
void initInst(InstData *inst, double t, uData *data) {
  // port/attribute offsets/definitions
  UDATA_DEFS;

  inst->fileState      = FILE_ERROR;   // default to failed
  inst->sampleRate     = frequency;
  inst->maxSamples     = maxSamples;
  inst->bitDepth       = bitDepth;
  inst->bytesPerSample = bitDepth / 8;
  inst->sampleIncr_t   = 1.0 / inst->sampleRate;

  if (bitDepth != 16 && bitDepth != 24) {
    msg(__LINE__, "Invalid bit depth specified.  Must be 16 or 24\n");
    return;
  }

  // open file for "create new" & binary read/write
  inst->file = fopen(filename, "w+b");
  if (!inst->file) {
    msg(__LINE__, "Unable to create/open WAV file \"%s\"\n.", filename);
    return;
  }

  msg(__LINE__,
      "Creating WAV file \"%s\", Bit Depth=%d, Sample Rate=%dHz, Max "
      "Samples=%d.\n",
      filename, bitDepth, frequency, inst->maxSamples);

  // populate known WAV header values
  inst->wavHeader.samplesPerSec = inst->sampleRate;
  inst->wavHeader.avgBytesPerSec =
      inst->sampleRate * inst->wavHeader.nbrChannels * inst->bytesPerSample;
  inst->wavHeader.bitsPerSample = bitDepth;

  // write file header info (will be written again when finalizing the file)
  // this positions the file for subsequently writing sample data
  if (!fwrite(&inst->wavHeader, sizeof(WavHeader), 1, inst->file)) {
    msg(__LINE__, msgFileError);
    return;
  }

  // so far, so good
  inst->fileState    = FILE_OPEN;
  inst->nextSample_t = 0;
  inst->nextIncr_t   = inst->sampleIncr_t;
}

/*------------------------------------------------------------------------------
 * finalizeFile() -- rewrite the header block with updated values (chunk sizes)
 * and close file.
 *----------------------------------------------------------------------------*/
void finalizeFile(InstData &inst) {
  // default to error state
  inst.fileState = FILE_ERROR;

  // we are finalizing so first flush and reposition to start of file
  if (fflush(inst.file) || fseek(inst.file, 0, SEEK_SET)) {
    msg(__LINE__, msgFileError);
    return;
  }

  // update header values before writing
  int dataSize =
      inst.sampleCnt * inst.wavHeader.nbrChannels * inst.bytesPerSample;
  inst.wavHeader.chunkSize     = sizeof(WavHeader) + dataSize - 8;
  inst.wavHeader.dataChunkSize = dataSize;

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
inline int32_t convToInt(InstData &inst, double val) {
  // detect & limit clipping
  inst.clipDetected = val > 1.0 || val < -1.0;
  if (inst.clipDetected) inst.lastClip = 1.0;
  val = std::min(std::max(val, -1.0), 1.0);

  // scale value to max 16- or 24-bit
  int32_t maxInt = inst.bytesPerSample == 2 ? 0x7fff : 0x7fffff;
  val *= (double)maxInt;

  // round to nearest integer
  return round(val);
}

// scale 16-bit int to double; inline for speed
inline double convToDbl16(int32_t val) {
  constexpr double factor = 1.0 / 0x8000;   // pre-calculate for speed
  return val * factor;
}

// scale 24-bit int to double; inline for speed
inline double convToDbl24(int32_t val) {
  constexpr double factor = 1.0 / 0x800000;   // pre-calculate for speed
  return val * factor;
}

/*------------------------------------------------------------------------------
 * writeSamples() -- write sample data to WAV file.
 *----------------------------------------------------------------------------*/
void writeSamples(InstData &inst) {
  inst.lastClip = 0.0;

  // convert to int for sample and back to double for output
  inst.lastBytes.CH1 = convToInt(inst, inst.lastIn.CH1);
  inst.lastBytes.CH2 = convToInt(inst, inst.lastIn.CH2);

  // use switch for possible future sample-type additions (e.g., 32-bit float)
  switch (inst.bytesPerSample) {
  case 2:
    inst.lastOut.CH1 = convToDbl16(inst.lastBytes.CH1);
    inst.lastOut.CH2 = convToDbl16(inst.lastBytes.CH2);
    break;
  case 3:
    inst.lastOut.CH1 = convToDbl24(inst.lastBytes.CH1);
    inst.lastOut.CH2 = convToDbl24(inst.lastBytes.CH2);
    break;
  default:
    // this shouldn't happen -- terminate with prejudice...
    msg(__LINE__, "Unexpected program fault.  Terminating simulation.\n");
    exit(1);
  }

  // map int to bytes for extracting 2- or 3-byte samples
  union {
    int32_t i;
    uint8_t b[4];
  } buf[2];

  buf[0].i = inst.lastBytes.CH1;
  buf[1].i = inst.lastBytes.CH2;

  // write the samples to the file
  if (fwrite(&buf[0].b[0], 1, inst.bytesPerSample, inst.file) !=
          inst.bytesPerSample ||
      fwrite(&buf[1].b[0], 1, inst.bytesPerSample, inst.file) !=
          inst.bytesPerSample) {
    msg(__LINE__, msgFileError);
    inst.fileState = FILE_ERROR;
  }

  // if (inst.fileState != FILE_ERROR) inst.sampleCnt++;
  if (inst.fileState == FILE_OPEN) inst.sampleCnt++;
  else
    msg(__LINE__, msgFileError);
}
/*==============================================================================
 * End of WavOut.cpp
 *============================================================================*/