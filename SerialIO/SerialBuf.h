#ifndef SerialBuf_H
#define SerialBuf_H

#include <inttypes.h>

class SerialOutBuffer8 {
public:
  // set buffer value
  void serialStart(uint8_t val) {
    buf       = val;
    bitCnt    = 0;
    underflow = false;
  }

  // get next buffer bit
  bool serialOut() {
    if (underflow) return false;
    bool outBit = buf & 0x80;
    buf <<= 1;
    underflow |= ++bitCnt > bufBits;
    return outBit;
  }

  // end of transmission
  void serialEnd() { /* nothing to do? */
  }

  // may be useful later
  bool isUnderflow() { return underflow; }

protected:
  const uint8_t bufBits   = 8;
  uint8_t       buf       = 0;
  uint8_t       bitCnt    = 0;
  bool          underflow = true;
};

#if 0
class SerialInBuffer8 {
public:
  void serialStart() {
    buf = bitCnt = 0;
    overflow     = false;
  }

  void serialIn(bool bit) {
    buf <<= 1;
    buf |= bit ? 1 : 0;
    overflow |= bool(++bitCnt > bufBits);
  }

  void serialEnd() { /* nothing to do? */
  }

  bool isOverflow() { return overflow; }

protected:
  const uint8_t bufBits  = 8;
  uint8_t       buf      = 0;
  uint8_t       bitCnt   = 0;
  bool          overflow = true;
};
#endif

#endif   // SerialBuf_H
