/*==============================================================================
 * SpiIO.h -- SPI code common to master & slave devices (serial buffer & SPI
 * modes).
 *============================================================================*/

#ifndef SPIIO_H
#define SPIIO_H

#include <cinttypes>
#include "PinIO.h"

void msg(int lineNbr, const char *fmt, ...);   // fwd decl for debugging

/*------------------------------------------------------------------------------
 * class SerialBuffer - a single buffer for both sending & recieving data.
 * buffer is hard-coded for 32 bits max but that could easily be increased to 64
 * bits.  data rotates out of the MSb and into the LSb.  the number of bits to
 * be sent in a transaction is set in the startIO() method.
 *----------------------------------------------------------------------------*/
class SerialBuffer {
public:
  // sets initial data to send and # of bits for transaction
  void startIO(uint32_t startData, uint8_t bits) {
    data      = startData;
    bitsTotal = bits;
    bitsIn    = 0;
    msbMask   = 1 << (bits - 1);
    data      = startData & ~(UINT32_MAX << bits);
    overFlow  = false;
  }

  void endIO() { /* nothing to do? */
  }

  // set the buffer data -- useful if output data isn't known when starting the
  // transaction.  the new buffer output data is shifted left by the number of
  // bits already sent and preserves any data already received in the low bits.
  void setData(uint32_t newData) {
    data = getData();            // clear bits not yet sent
    data |= newData << bitsIn;   // set unsent bits
  }

  // get the MSb to send -- no need to call if no output needed
  inline bool getBitOut() { return data & msbMask; }

  // rotate bit into LSb -- must call this between getBitOut() calls to advance
  // the data bits even if there's no data input
  void setBitIn(bool bit) {   // = false) {
    if (isDone()) {
      // this is an error state...
      overFlow = true;
      return;
    }
    data <<= 1;
    data |= bit ? 1 : 0;
    bitsIn++;
  }

  // get the current data received.  note that, if the transaction isn't
  // complete, only the received bits are returned.  this can be useful if
  // the first few bits received in the transaction determine what bits will
  // be sent in the remaining bits of the transaction.
  uint32_t getData() const {
    uint32_t mask = ~(UINT32_MAX << bitsIn);
    return data & mask;
  }

  inline bool    isDone() const { return bitsIn >= bitsTotal; }
  inline bool    isOverflow() const { return overFlow; }
  inline uint8_t getBitsIn() const { return bitsIn; }

protected:
  uint32_t data;
  uint32_t msbMask;
  uint8_t  bitsTotal;   // transaction size in bits
  uint8_t  bitsIn;      // # of bits input
  bool     overFlow = false;
};

/*------------------------------------------------------------------------------
 * The SPI configurations are easier to understand once you realize that they
 * always have the data shifted out before data is shifted in.
 *
 * SPI  | Clk Polarity/Phase
 * Mode | CPOL CPHA | Data is shifted out on              | Data is sampled on
 *   0  |   0    0  | Falling SCLK, and when CS activates | Rising SCLK
 *   1  |   0    1  | Rising SCLK                         | Falling SCLK
 *   2  |   1    0  | Rising SCLK, and when CS activates  | Falling SCLK
 *   3  |   1    1  | Falling SCLK                        | Rising SCLK
 *----------------------------------------------------------------------------*/

struct SpiModeTbl {
  PinState sclkIdle;
  PinEdge  csOutEdge;
  PinEdge  sclkOutEdge;
  PinEdge  sclkInEdge;
};

const SpiModeTbl spiModes[] = {
    // mode 0 - CPOL=0  | Falling SCLK, and when CS activates | Rising SCLK
    {
        PinState::LOW,      // sclkIdle
        PinEdge::FALLING,   // csOutEdge (CS hardcoded for active low)
        PinEdge::FALLING,   // sclkOutEdge
        PinEdge::RISING     // sclkInEdge
    },
    // mode 1 - CPOL=0 | Rising SCLK | Falling SCLK
    {
        PinState::LOW,     // sclkIdle
        PinEdge::IGNORE,   // csOutEdge
        PinEdge::RISING,   // sclkOutEdge
        PinEdge::FALLING   // sclkInEdge
    },
    // mode 2 - CPOL=1 | Rising SCLK, and when CS activates | Falling SCLK
    {
        PinState::HIGH,     // sclkIdle
        PinEdge::FALLING,   // csOutEdge (CS hardcoded for active low)
        PinEdge::RISING,    // sclkOutEdge
        PinEdge::FALLING    // sclkInEdge
    },
    // mode 3 - CPOL=1 | Falling SCLK | Rising SCLK
    {
        PinState::HIGH,     // sclkIdle
        PinEdge::IGNORE,    // csOutEdge
        PinEdge::FALLING,   // sclkOutEdge
        PinEdge::RISING     // sclkInEdge
    }};

#endif   // SPIIO_H
/*==============================================================================
 * End of SpiIO.h
 *============================================================================*/