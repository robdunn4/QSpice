/*******************************************************************************
 * WavOut.h -- QSpice C-Block component to write *.WAV file data from a circuit
 * signal.
 *
 * Copyright © 2023 Robert Dunn.  Licensed for use under the GNU GPLv3.0.
 ******************************************************************************/

#ifndef WAVOUT_H_
#define WAVOUT_H_

#include <inttypes.h>

#define FmtPCM    0x0001   // PCM
#define FmtIEEE   0x0003   // IEEE float
#define FmtALaw   0x0006   // 8-bit ITU-T G.711 A-law
#define FmtuLaw   0x0007   // 8-bit ITU-T G.711 �-law
#define FmtSubExt 0xffee   // 0xFFFE 	Determined by SubFormat

struct WavHeader {
  // wave file header chunk
  char     groupID[4] = {'R', 'I', 'F', 'F'};    // "RIFF"
  uint32_t chunkSize;                            // size of file less 8 bytes?
  char     riffType[4] = {'W', 'A', 'V', 'E'};   // "WAVE"

  // format chunk header
  char     fmtFormat[4] = {'f', 'm', 't', ' '};   // "fmt "
  uint32_t fmtChunkSize = 16;   // 16 for this 16-bit, PCM file

  // format chunk data
  int16_t fmtCode     = FmtPCM;   // FmtPCM
  int16_t nbrChannels = 2;        // number of interleaved channels (2)
  int32_t samplesPerSec;          // sample rate (blocks per second)
  int32_t avgBytesPerSec;         // data rate
  int16_t blkAlign      = 4;      // data block size (2)
  int16_t bitsPerSample = 16;     // bits per sample (16)

  // data chunk header
  char     dataFormat[4] = {'d', 'a', 't', 'a'};   // "data"
  uint32_t dataChunkSize;                          // size of samples to follow

  // file sample data would begin here...
};
typedef WavHeader *pWavHeader;

#endif /* WAVOUT_H_ */
/*==============================================================================
 * EOF WavOut.h
 *============================================================================*/