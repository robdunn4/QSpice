/*******************************************************************************
 * WavSrc.h -- QSpice C-Block component to read *.WAV file data as a circuit
 * signal source.
 *
 * Copyright © 2023 Robert Dunn.  Licensed for use under the GNU GPLv3.0.
 ******************************************************************************/
#ifndef WAVSRC_H_
#define WAVSRC_H_

#include <inttypes.h>

struct WavFileHeaderChunk {
  char    groupID[4];    // "RIFF"
  int32_t chunkSize;     // size
  char    riffType[4];   // "WAVE"
};
typedef WavFileHeaderChunk *pWavFileHeaderChunk;

#define FmtPCM    0x0001   // PCM
#define FmtIEEE   0x0003   // IEEE float
#define FmtALaw   0x0006   // 8-bit ITU-T G.711 A-law
#define FmtuLaw   0x0007   // 8-bit ITU-T G.711 �-law
#define FmtSubExt 0xffee   // 0xFFFE 	Determined by SubFormat

/*
 * chunk header -- used for format, data, and unused chunks
 */
struct WavChunkHeader {
  char     format[4];   // "fmt " or "data"
  uint32_t chunkSize;   // if fmt, 16, 18, or 40 are valid
};
typedef WavChunkHeader *pWavChunkHeader;

/*
 * format chumk data
 */
union WavFmtChunk {
  char buffer[40];   // max size
  struct {
    int16_t fmtCode;          // see above Fmtxxx defines
    int16_t nbrChannels;      // number of interleaved channels
    int32_t samplesPerSec;    // sample rate (blocks per second)
    int32_t avgBytesPerSec;   // data rate
    int16_t blkAlign;         // data block size
    int16_t bitsPerSample;    // bits per sample
    // note: 8-bit samples are unsigned and need to be normalized to signed by
    // subtracting 128 all others are signed... additional format info
    // ignored/unused
  };
};
typedef WavFmtChunk *pWavFmtChunk;

/*
 * for reading/parsing data chunks.  allocate buffer for chunk, read chunk,
 * index from struct dataXXX[]
 */
union WavDataChunk {
  char    buffer[1];
  int8_t  data8[1];
  int16_t data16[1];
  int32_t data32[1];
};
typedef WavDataChunk *pWavDataChunk;

#endif /* WAVSRC_H_ */
/*==============================================================================
 * EOF WavSrc.h
 *============================================================================*/
