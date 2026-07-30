//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework 
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/*
 * This PIC18F46K22 code implements a "Charlie-Plexing" output on pins RA0-RA2.
 * This is a "proof of concept" to demonstrate minimal functionality to handle
 * tri-state pins.
 */

#include <xc.h>

// -----------------------------------------------------------------------
// Configuration bits
// -----------------------------------------------------------------------
#pragma config FOSC     = INTIO67   // internal oscillator, RA6/RA7 as I/O
#pragma config PLLCFG   = OFF
#pragma config PRICLKEN = ON
#pragma config FCMEN    = OFF
#pragma config IESO     = OFF
#pragma config PWRTEN   = OFF
#pragma config BOREN    = OFF
#pragma config BORV     = 250
#pragma config WDTEN    = OFF       // demo loop never clears the WDT
#pragma config WDTPS    = 32768
#pragma config MCLRE    = EXTMCLR
#pragma config STVREN   = ON
#pragma config LVP      = OFF
#pragma config XINST    = OFF

typedef unsigned char BYTE;

// -----------------------------------------------------------------------
// Manual startup init (replaces SYSTEM_Initialize()).
// -----------------------------------------------------------------------
void PORTS_Initialize(void)
{
  TRISA = 0xFF;
  TRISB = 0xFF;
  TRISC = 0xFF;
  TRISD = 0xFF;
  TRISE = 0x0F;   // only RE0-RE3 implemented (RE3 is input-only: MCLR/VPP/RE3)

  ANSELA = 0x00;
  ANSELB = 0x00;
  ANSELC = 0x00;
  ANSELD = 0x00;
  ANSELE = 0x00;

  LATA = 0x00;
  LATB = 0x00;
  LATC = 0x00;
  LATD = 0x00;
  LATE = 0x00;
}

BYTE cplxMask = 0b00000111;

typedef struct
{
  BYTE tris;
  BYTE port;
} BITSTATES;

BITSTATES bitStates[] = {
    {0b100, 0b001}, // LED 1
    {0b100, 0b010}, // LED 2
    {0b001, 0b010}, // LED 3
    {0b001, 0b100}, // LED 4
    {0b010, 0b001}, // LED 5
    {0b010, 0b100}  // LED 6
};

typedef char STATENDX;
STATENDX     nextState = 0;
STATENDX     stateCnt = sizeof(bitStates)/sizeof(bitStates[0]);

void clrCplx()
{
  // disable all cplx outputs, i.e., go high-impedance on cplx pins
  BYTE reg = TRISA;
  reg |= cplxMask;
  TRISA = reg;
}

void setCplx(STATENDX ndx)
{
  // disable cplx outputs before changing port states
  clrCplx();

  // grab current states & mask off cplx bits
  BYTE tris = TRISA & ~cplxMask;
  BYTE port = PORTA & ~cplxMask;

  // set cplx state
  tris |= bitStates[ndx].tris;
  port |= bitStates[ndx].port;

  // set cplx port values (high/low) before enabling output
  PORTA = port;
  TRISA = tris;
}

int main(void)
{
  // manual initialization (no MCC available for this target)
  PORTS_Initialize();

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
