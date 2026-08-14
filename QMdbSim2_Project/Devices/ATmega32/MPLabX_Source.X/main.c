//------------------------------------------------------------------------------
// This file is part of the QMdbSim project, a Microchip Simulator framework for
// QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/*
 * This ATmega32 code implements a "Charlie-Plexing" output on pins PB0-PB2.
 * This is a "proof of concept" to demonstrate minimal functionality to handle
 * tri-state pins.
 *
 * Ported from the ATtiny85 version of this demo. PORTB/DDRB register names
 * and bit-manipulation logic are identical between the two devices (both
 * classic 8-bit AVR), so the only device-specific change required is the
 * FUSES block below.
 */

#include <avr/io.h>

// Internal Calibrated RC Oscillator, 8.0MHz, SUT=10 (6 CK + 65ms, "slowly
// rising power" startup) -- same clock target and startup-time class as the
// ATtiny85 original ("INTRCOSC_8MHZ_6CK_14CK_64MS"). Unlike the ATtiny85,
// the ATmega32 has no CKDIV8 fuse -- the 8MHz internal RC option (CKSEL =
// "0100") runs at 8MHz directly with no /8 prescale step to account for.
//
// The ATmega32 has only two fuse bytes (Low, High) -- no Extended fuse byte
// exists on this device (per Atmel 2503Q-AVR-02/11 Table 104/105), so the
// .extended member is omitted entirely rather than set to 0xFF as on the
// ATtiny85.
FUSES = {
	.low = 0xE4,  // LOW {BODLEVEL=DISABLED(unprog), BODEN=DISABLED(unprog),
	              //      SUT=10(slowly rising power), CKSEL=0100(8MHz Int. RC)}
	.high = 0xD9, // HIGH {OCDEN=DISABLED(unprog), JTAGEN=DISABLED(unprog,
	              //       not used by this demo -- PB0-PB2 only, no PORTC),
	              //       SPIEN=SET(programmed, SPI prog. enabled),
	              //       CKOPT=DISABLED(unprog, correct for Int. RC per
	              //       datasheet), EESAVE=CLEAR(unprog), BOOTSZ=max
	              //       (default, unused -- no bootloader), BOOTRST=CLEAR
	              //       (unprog, reset vector at 0x0000)}
};
LOCKBITS = 0xFF; // {LB=NO_LOCK}

typedef unsigned char BYTE;

BYTE cplxMask = 0b00000111;

typedef struct
{
  BYTE ddr;
  BYTE port;
} BITSTATES;

BITSTATES bitStates[] = {
    {0b011, 0b001}, // LED 1
    {0b011, 0b010}, // LED 2
    {0b110, 0b010}, // LED 3
    {0b110, 0b100}, // LED 4
    {0b101, 0b001}, // LED 5
    {0b101, 0b100}  // LED 6
};

typedef unsigned char STATENDX;
STATENDX     nextState = 0;
STATENDX     stateCnt = sizeof(bitStates)/sizeof(bitStates[0]);

void clrCplx()
{
  // disable all cplx outputs, i.e., go high-impedance on cplx pins
  BYTE reg = DDRB;
  reg &= ~cplxMask;
  DDRB = reg;
}

void setCplx(STATENDX ndx)
{
  // disable cplx outputs before changing port states
  clrCplx();

  // grab current states & mask off cplx bits
  BYTE ddr = DDRB & ~cplxMask;
  BYTE port = PORTB & ~cplxMask;
  
  // set cplx state
  ddr |= bitStates[ndx].ddr;
  port |= bitStates[ndx].port;

  // set cplx port values (high/low) before enabling output
  PORTB = port;
  DDRB = ddr;
}

int main(void)
{
  // configuration should already set pins to high-impedance but just making
  // certain...
  clrCplx();
  
  // main loop
  while (1)
  {
    setCplx(nextState);
    nextState++;
    nextState %= stateCnt;
  }
}
