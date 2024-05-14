/*******************************************************************************
 * WavSrc.cpp -- QSpice C-Block component to read *.WAV file data as a circuit
 * signal source.
 *
 * 2024.04.29 - v0.2 added support for 24-bit PCM.
 * 2024.05.11 - v0.3 revised sample timing & normalization factor.
 *
 * Copyright © 2023-2024 Robert Dunn.  Licensed for use under the GNU GPLv3.0.
 ******************************************************************************/
// To build with Digital Mars C++ Compiler: dmc -mn -WD wavsrc.cpp kernel32.lib
// or open the component source in QSpice, right-click, compile...

#include "wavsrc.h"
#include <algorithm>   // for std::min/max
#include <limits.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define PROGRAM_NAME    "WavSrc"
#define PROGRAM_VERSION "v0.3"
#define PROGRAM_INFO    PROGRAM_NAME " " PROGRAM_VERSION

/*
 * standard QSpice template-generated parameter data structure
 */
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

/*
 * for convenience, put the uData offsets in one place.  including UDATA_DEFS
 * in functions doesn't add code overhead even when not used...
 */
#define UDATA_DEFS                                                             \
  double      Vref     = data[0].d;                                            \
  const char *filename = data[1].str;                                          \
  int         loops    = data[2].i;                                            \
  double      gain     = data[3].d;                                            \
  double     &CH1      = data[4].d;                                            \
  double     &CH2      = data[5].d;

// #undef pin names lest they collide with names in any header file(s) you might
// include. (could use namespaces if DMC.exe supports them?)
#undef CH1
#undef CH2
#undef Vref

/*******************************************************************************
 * constants & defines for convenience
 ******************************************************************************/
#define FileClosed 0
#define FileOpen   1
#define FileError  -1

const char *MsgBadRead   = "Unexpected error reading WAV file (\"%s\").\n";
const char *MsgBadFormat = "Unsupported WAV format in file \"%s\"\n";
const char *MsgBadOpen =
    "Unexpected error opening WAV file (\"%s\").  File not found or cannot be "
    "opened.\n)";

/*******************************************************************************
 * forward decls
 ******************************************************************************/
struct InstData;
void           initInst(InstData &, uData *);
void           getSample(InstData &, double, const char *);
typedef double SampleFunc(InstData &, const char *);
SampleFunc     getSample16;
SampleFunc     getSample24;

/*******************************************************************************
 * Per-instance data.  The QSpice template generator gives this structure a
 * unique name based on the C-Block mocule name for reasons that excape me.
 ******************************************************************************/
struct InstData {
  FILE       *file;             // file stream pointer for WAV data
  int         fileState;        // 0 = closed; -1 = error; 1 = open
  fpos_t      startOfData;      // file position of start of data for looping
  uint32_t    sampleCnt;        // # of samples read so far
  uint32_t    nbrSamples;       // # of samples in file
  int         bytesPerSample;   // bytes in each data sample
  double      lastCh1;   // last normalized value read/output of channel 1
  double      lastCh2;   // last normalized value read/output of channel 2
  double      nextSampleTime;   // time to fetch next sample
  double      nextSampleIncr;   // simulation time increment for next sample
  double      maxAmplitude;   // max input amplitude for normalization to +/-1.0
  double      sampleTimeIncr;   // 1 / sample frequency
  int         nbrChannels;      // number of channels per sample
  int         maxLoops;         // number of times to loop through samples
  int         loopCnt;          // number of loops so far
  double      gain;             // output gain to apply to normalized values
  SampleFunc *getSample;        // pointer to sampling function
};

/*------------------------------------------------------------------------------
 * msg() -- display message in QSpice Output window
 *----------------------------------------------------------------------------*/
void msg_(int lineNbr, const char *fmt, ...) {
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

#define msg(...) msg_(__LINE__, __VA_ARGS__)

/*******************************************************************************
 * QSpice-defined entry points
 ******************************************************************************/
// int DllMain() must exist and return 1 for a process to load the .DLL
// See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain for more
// information.
int __stdcall DllMain(void *module, unsigned int reason, void *reserved) {
  return 1;
}

/*------------------------------------------------------------------------------
 * wavsrc() -- QSpice "evaluation function."
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void wavsrc(
    InstData **opaque, double t, uData *data) {
  UDATA_DEFS;

  InstData *inst = *opaque;

  // allocate per-instance data if not already allocated
  if (!inst) {
    // allocate & clear the memory block
    *opaque = inst = (InstData *)calloc(1, sizeof(InstData));
    if (!inst) {   // terminate with extreme prejudice
      msg("Unable to allocate instance memory.  Terminating simulation...\n");
      exit(1);
    }

    // initialize the instance data (open/parse file through header stuff)
    initInst(*inst, data);
  }

  // if the current sample has "expired", get next sample
  if (t >= inst->nextSampleTime) getSample(*inst, t, filename);

  // set component's out port values to current sample values
  CH1 = (inst->lastCh1 * gain) + Vref;
  CH2 = (inst->lastCh2 * gain) + Vref;
}

/*------------------------------------------------------------------------------
 * MaxExtStepSize()
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) double MaxExtStepSize(InstData *inst) {
  double stepSize = 1e308;   // heat death of the universe?

  // if file is open, set to sample-time increment
  if (inst->fileState == FileOpen) stepSize = inst->sampleTimeIncr;

  return stepSize;
}

/*------------------------------------------------------------------------------
 * Trunc() -- force simulation sampling time to match wav sample frequency
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Trunc(
    InstData *inst, double t, union uData *data, double *timestep) {
  UDATA_DEFS;

  if (t < inst->nextSampleTime) *timestep = inst->nextSampleIncr;
}

/*------------------------------------------------------------------------------
 * Destroy() -- clean up upon end of simulation
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Destroy(InstData *inst) {
  msg("Closing WAV file.\n");
  fclose(inst->file);
  free(inst);
}

/*******************************************************************************
 * WavSrc component functions
 ******************************************************************************/
/*------------------------------------------------------------------------------
 * initInst() - opens the WAV file, parses through fmt chunk, and stops at
 * beginning of sample data.  initializes instance data for first data read.
 *----------------------------------------------------------------------------*/
void initInst(InstData &inst, uData *data) {
  UDATA_DEFS;

  // default instance file state to file error
  inst.fileState = FileError;

  msg("Reading WAV file \"%s\", loops=%d, gain=%f\n", filename, loops, gain);

  // open the WAV file
  if (!(bool)(inst.file = fopen(filename, "rb"))) {
    msg(MsgBadOpen, filename);
    return;
  }

  // read file header info
  size_t             bytes;
  WavFileHeaderChunk fileHdr;

  bytes = fread(&fileHdr, 1, sizeof(fileHdr), inst.file);
  if (bytes != sizeof(fileHdr)) {
    fclose(inst.file);
    msg(MsgBadRead, filename);
    return;
  }

  // check header for supported file type
  if (memcmp(fileHdr.groupID, "RIFF", 4) ||
      memcmp(fileHdr.riffType, "WAVE", 4)) {
    fclose(inst.file);
    msg(MsgBadFormat, filename);
    return;
  }

  // grab next chunk header -- should be a format chunk...
  WavChunkHeader chunkHdr;
  bytes = fread(&chunkHdr, 1, sizeof(chunkHdr), inst.file);
  if (bytes != sizeof(chunkHdr)) {
    msg(MsgBadRead, filename);
    fclose(inst.file);
    return;
  }

  // verify that it is a format chunk
  if (memcmp(chunkHdr.format, "fmt ", 4)) {
    msg(MsgBadFormat, filename);
    fclose(inst.file);
    return;
  }

  // verify that the format chunk size is acceptable/expected
  switch (chunkHdr.chunkSize) {
  case 16:
  case 18:
  case 40:
    break;
  default:
    msg(MsgBadFormat, filename);
    fclose(inst.file);
    return;
  }

  // read/save format chunk data in instance data
  WavFmtChunk fmtChunk;
  bytes = fread(&fmtChunk, 1, chunkHdr.chunkSize, inst.file);
  if (bytes != chunkHdr.chunkSize) {
    msg(MsgBadRead, filename);
    fclose(inst.file);
    return;
  }

  // validate allowed format -- only PCM is supported for now
  if (fmtChunk.fmtCode != FmtPCM) {
    msg(MsgBadFormat, filename);
    fclose(inst.file);
    return;
  }

  // only mono or stereo allowed
  if (fmtChunk.nbrChannels > 2) {
    msg(MsgBadFormat, filename);
    fclose(inst.file);
    return;
  }

  // set amplitude nomalization for bit depth -- commented code remains for
  // possible component enhancements
  switch (fmtChunk.bitsPerSample) {
    //	case 8: maxAmplitude = 0x7F; break;
  case 16:
    // inst.maxAmplitude   = 0x7fff;
    inst.maxAmplitude   = 0x8000;
    inst.getSample      = getSample16;
    inst.bytesPerSample = 2;
    break;
    //	case 20: inst.maxAmplitude = 0x07FFFF; break;
  case 24:
    // inst.maxAmplitude   = 0x7FFFFF;
    inst.maxAmplitude   = 0x800000;
    inst.getSample      = getSample24;
    inst.bytesPerSample = 3;
    break;
    //	case 32: inst.maxAmplitude = 0x7FFFFFFF; break;
  default:
    msg(MsgBadFormat, filename);
    fclose(inst.file);
    return;
  }

  // finally, get the data chunk (should be next)
  bytes = fread(&chunkHdr, 1, sizeof(chunkHdr), inst.file);
  if (bytes != sizeof(chunkHdr)) {
    msg(MsgBadRead, filename);
    fclose(inst.file);
    return;
  }

  // if not "data", not expected/handled
  if (memcmp(chunkHdr.format, "data", 4)) {
    msg(MsgBadFormat, filename);
    fclose(inst.file);
    return;
  }

  // save values in instance data
  inst.nbrChannels = fmtChunk.nbrChannels;
  inst.nbrSamples = chunkHdr.chunkSize / inst.bytesPerSample / inst.nbrChannels;
  inst.sampleTimeIncr = 1.0 / fmtChunk.samplesPerSec;
  inst.nextSampleTime = 0.0;
  inst.nextSampleIncr = inst.sampleTimeIncr;
  inst.maxLoops = loops < 1 ? INT_MAX : loops;   // technically not infinity
  inst.lastCh1 = inst.lastCh2 = 0.0;
  inst.gain                   = gain;

  // in theory, the file is positioned at the start of the sample data.  save
  // the position for looping...
  if (fgetpos(inst.file, &inst.startOfData)) {
    msg(MsgBadRead, filename);
    fclose(inst.file);
    inst.lastCh1 = inst.lastCh2 = 0.0;
    return;
  }

  // in theory, we're ready to start reading samples
  inst.fileState = FileOpen;

  // msg("Using WAV file=\"%s\", loops=%d, gain=%f\n", filename, loops, gain);
  msg("WAV Metadata: # of Channels=%d, Bit Depth=%d, Sample Rate=%dHz, # of "
      "Samples=%d\n",
      inst.nbrChannels, inst.bytesPerSample * 8, fmtChunk.samplesPerSec,
      inst.nbrSamples);
}

/*------------------------------------------------------------------------------
 * getData() - gets the next sample(s) from the file.
 *----------------------------------------------------------------------------*/
void getSample(InstData &inst, double t, const char *filename) {
  // default sample values
  inst.lastCh1 = inst.lastCh2 = 0.0;

  if (inst.fileState != FileOpen) return;

  // have we reached the end of data?
  if (inst.sampleCnt >= inst.nbrSamples) {
    // a loop finished
    inst.loopCnt++;
    inst.sampleCnt = 0;

    // do we have more loops to do?
    if (inst.loopCnt >= inst.maxLoops) {
      fclose(inst.file);
      inst.fileState = FileClosed;
      // we won't be reading again anytime soon
      inst.nextSampleTime = inst.nextSampleIncr = 1e308;
      return;
    }

    // reposition file to start of data for looping
    if (fsetpos(inst.file, &inst.startOfData)) {
      fclose(inst.file);
      inst.fileState = FileClosed;
      return;
    }
  }

  // get first channel sample
  inst.lastCh1 = inst.lastCh2 = inst.getSample(inst, filename);

  // if stereo, get the other channel sample
  if (inst.nbrChannels == 2) inst.lastCh2 = inst.getSample(inst, filename);

  // calculate next sample time adjusting for loop count
  inst.nextSampleTime = ((inst.loopCnt * inst.nbrSamples) + ++inst.sampleCnt) *
      inst.sampleTimeIncr;
  inst.nextSampleIncr = inst.nextSampleTime - t;
}

/*------------------------------------------------------------------------------
 * getSample16() - gets the next 16-bit value from the file and normalizes it
 * to +/-1.0.
 *----------------------------------------------------------------------------*/
// TODO:  Combine getSample16() & getSample24() and eliminate function
// indirection?
double getSample16(InstData &inst, const char *filename) {
  int16_t sampleVal;

  int bytes = fread(&sampleVal, 1, sizeof(int16_t), inst.file);
  if (bytes < sizeof(sampleVal)) {
    inst.fileState = FileError;
    msg(MsgBadRead, filename);
    return 0.0;
  }

  // normalize & limit
  double retVal = sampleVal / inst.maxAmplitude;
  retVal        = std::max(std::min(retVal, 1.0), -1.0);

  return retVal;
}

/*------------------------------------------------------------------------------
 * getSample24() - gets the next 24-bit value from the file and normalizes it
 * to +/-1.0.
 *----------------------------------------------------------------------------*/
double getSample24(InstData &inst, const char *filename) {
  // PCM data is in Intel native/little-endian format... twiddle the bits...
  union {
    int32_t i32;
    int8_t  b[4];
  } buf;

  // read 24-bit sample bytes
  if (fread(&buf, 1, 3, inst.file) != 3) {
    inst.fileState = FileError;
    msg(MsgBadRead, filename);
    return 0.0;
  }

  // set high byte for sign extension
  buf.b[3] = buf.b[2] & 0x80 ? 0xff : 0x00;

  // normalize & limit
  double retVal = buf.i32 / inst.maxAmplitude;
  retVal        = std::max(std::min(retVal, 1.0), -1.0);

  return retVal;
}
/*==============================================================================
 * EOF WavSrc.cpp
 *============================================================================*/
