//------------------------------------------------------------------------------
// This file is part of the QMdbSim project, a Microchip Simulator framework for
// QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/*
 * This dsPIC33FJ128MC802 code implements a "Charlie-Plexing" output on pins
 * RA0-RA2.  This is a "proof of concept" to demonstrate minimal functionality
 * to handle tri-state pins.  Ported from the ATtiny85 demo (PB0-PB2).
 */

#include <xc.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Configuration bits
//
// Clock:  Boot on the internal FRC oscillator (7.37MHz), then switch to FRC
// with PLL in code.  PLL settings:  N1 = 2, M = 43, N2 = 2.
//   Fosc = 7.37MHz / 2 * 43 / 2 = 79.23MHz
//   Fcy  = Fosc / 2             = 39.61MHz (nominal 40 MIPS)
// The dsPIC33F executes 1 instruction cycle per 2 oscillator clocks, so the
// SIMCLK in the QSpice schematic should be Fcy (39.61MHz).
//------------------------------------------------------------------------------

// FOSCSEL
#pragma config FNOSC = FRC          // Start on FRC; switch to FRCPLL in code
#pragma config IESO = OFF           // Two-speed oscillator start-up disabled

// FOSC
#pragma config POSCMD = NONE        // Primary oscillator off (RA2/OSC1 is I/O)
#pragma config OSCIOFNC = ON        // OSC2 pin is digital I/O (RA3)
#pragma config IOL1WAY = OFF        // Allow multiple peripheral pin remaps
#pragma config FCKSM = CSECMD       // Clock switching on, fail-safe monitor off

// FWDT
#pragma config FWDTEN = OFF         // Watchdog timer disabled

// FPOR
#pragma config FPWRT = PWR1         // Power-up timer off (default is 128ms)

// FICD
#pragma config JTAGEN = OFF         // JTAG disabled

//------------------------------------------------------------------------------
// Clock setup
//------------------------------------------------------------------------------
void initClock()
{
  // configure PLL for Fosc = 79.23MHz from the 7.37MHz FRC
  CLKDIVbits.PLLPRE = 0;    // N1 = 2
  CLKDIVbits.PLLPOST = 0;   // N2 = 2
  PLLFBD = 41;              // M = 43

  // initiate clock switch to FRC with PLL (NOSC = 0b001)
  __builtin_write_OSCCONH(0x01);
  __builtin_write_OSCCONL(OSCCON | 0x01);

  // On real hardware, wait for the switch and PLL lock here:
  //   while (OSCCONbits.COSC != 0b001);
  //   while (OSCCONbits.LOCK != 1);
  // Omitted since MDBCS doesn't require it.
}

//------------------------------------------------------------------------------
// Charlie-Plexing on RA0-RA2
//------------------------------------------------------------------------------

typedef uint16_t WORD;

WORD cplxMask = 0b0000000000000111;

// PIC TRIS is the inverse of AVR DDR:  0 = output, 1 = input (high-Z).
typedef struct
{
  WORD tris;
  WORD lat;
} BITSTATES;

BITSTATES bitStates[] = {
    {0b100, 0b001}, // LED 1
    {0b100, 0b010}, // LED 2
    {0b001, 0b010}, // LED 3
    {0b001, 0b100}, // LED 4
    {0b010, 0b001}, // LED 5
    {0b010, 0b100}  // LED 6
};

typedef int STATENDX;
STATENDX     nextState = 0;
STATENDX     stateCnt = sizeof(bitStates)/sizeof(bitStates[0]);

void clrCplx()
{
  // disable all cplx outputs, i.e., go high-impedance on cplx pins
  WORD reg = TRISA;
  reg |= cplxMask;
  TRISA = reg;
}

void setCplx(STATENDX ndx)
{
  // disable cplx outputs before changing port states
  clrCplx();

  // grab current states & mask off cplx bits
  WORD tris = TRISA & ~cplxMask;
  WORD lat = LATA & ~cplxMask;

  // set cplx state
  tris |= bitStates[ndx].tris;
  lat |= bitStates[ndx].lat;

  // set cplx latch values (high/low) before enabling output
  LATA = lat;
  TRISA = tris;
}

int main(void)
{
  initClock();

  // RA0/RA1 are AN0/AN1 and default to analog; make them digital
  AD1PCFGL |= 0b0000000000000011;

  // reset should already set pins to high-impedance but just making
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
