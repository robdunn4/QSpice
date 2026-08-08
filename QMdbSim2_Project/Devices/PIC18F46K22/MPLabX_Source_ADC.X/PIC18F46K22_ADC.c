//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework 
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/*
 * This PIC18F46K22 code implements a "Charlie-Plexing" output on pins RA0-RA2.
 * It also samples an analog voltage on RE0 (AN5) and mirrors the 8-bit ADC
 * result onto PORTD (RD0-RD7).  The ADC uses the internal AVDD/AVSS voltage
 * references; external VREF+ on RA3 is supported but currently disabled
 * (commented out) below.
 * This is a "proof of concept" to demonstrate minimal functionality to handle
 * tri-state pins and a free-running ADC read/output loop.
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
  ANSELE = 0x00; // RE0-RE2 only; RE3 (MCLR) has no ANSEL bit

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

// -----------------------------------------------------------------------
// ADC support: samples RE0/AN5 (VREF+ = RA3, VREF- = AVSS default) and
// mirrors the 8-bit result onto PORTD (RD0-RD7).  ADCON0<1> (GO/DONE) is
// tested directly rather than via a header bitfield name, since those
// names vary across device headers/compiler versions.
// -----------------------------------------------------------------------
#define ADC_GODONE_MASK 0b00000010

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

  // -----------------------------------------------------------------------
  // ADC output port (PORTD): drive the pins low *before* switching them to
  // outputs, so they never glitch high on the transition out of high-Z.
  // -----------------------------------------------------------------------
  LATD  = 0x00;
  TRISD = 0x00; // RD0-RD7 all outputs

  // -----------------------------------------------------------------------
  // ADC input (RE0/AN5) and external VREF+ (RA3): mark both pins analog.
  // RE0's TRIS bit is already an input (TRISE = 0x0F in PORTS_Initialize);
  // RA3's TRIS bit is already an input too (TRISA = 0xFF) and is never
  // touched by the Charlieplex logic (cplxMask only covers RA0-RA2).
  // -----------------------------------------------------------------------
  ANSELEbits.ANSE0 = 1; // RE0 = AN5, analog
  // ANSELAbits.ANSA3 = 1; // RA3 = VREF+, analog -- re-enable if using external VREF+

  // ADCON1: PVCFG = 00 (VREF+ = internal AVDD), NVCFG = 00 (VREF- = AVSS)
  ADCON1 = 0b00000000;
  // ADCON1 = 0b00000100; // PVCFG = 01 (VREF+ = external pin RA3) -- re-enable with the ANSA3 line above if using external VREF+

  // ADCON2: ADFM = 0 (left-justified -- ADRESH alone is the 8-bit result),
  // ACQT = 010 (4 TAD acquisition time), ADCS = 111 (dedicated ADC RC
  // oscillator, ~600kHz, independent of FOSC)
  ADCON2 = 0b00010111;

  // ADCON0: CHS<4:0> = 00101 (AN5), GO/DONE = 0, ADON = 1
  ADCON0 = 0b00010101;

  // trigger the first conversion
  ADCON0 |= ADC_GODONE_MASK;

  // main loop
  while (1)
  {
    setCplx(nextState);
    nextState++;
    nextState %= stateCnt;

    if (!(ADCON0 & ADC_GODONE_MASK))
    {
      // conversion complete -- ADFM=0 means ADRESH alone holds the 8-bit
      // result; mirror it onto the output port and start the next sample
      LATD = ADRESH;
      ADCON0 |= ADC_GODONE_MASK;
    }
  }
}
