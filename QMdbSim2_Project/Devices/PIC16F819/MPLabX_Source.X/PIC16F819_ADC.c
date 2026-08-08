//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework 
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/*
 * PIC16F819 adaptation of the PIC18F46K22 Charlieplex+ADC demo.
 */

#include <xc.h>

// Needed for __delay_us() below -- must match the INTOSC frequency actually
// selected via OSCCON<IRCF> in main(). 4 MHz chosen as a common/moderate
// INTOSC setting; VERIFY the IRCF bit pattern used in main() actually
// yields this frequency on this device (see header comment above).
#define _XTAL_FREQ 4000000

// -----------------------------------------------------------------------
// Configuration bits
// -----------------------------------------------------------------------
#pragma config FOSC    = INTOSCIO  // internal oscillator, I/O on RA6/RA7 -- VERIFY exact enum name (see header above)
#pragma config WDTE    = OFF       // demo loop never clears the WDT
#pragma config PWRTE   = ON        // Power-up Timer enabled -- VERIFY polarity (see header above)
#pragma config MCLRE   = ON        // RA5 functions as external MCLR (Reset), not GPIO
#pragma config BOREN   = OFF
#pragma config LVP     = OFF
#pragma config CPD     = OFF
#pragma config WRT     = OFF
#pragma config CP      = OFF

typedef unsigned char BYTE;

// -----------------------------------------------------------------------
// Manual startup init.  No ANSELx registers on this device -- analog vs.
// digital pin configuration is handled entirely via ADCON1's PCFG bits,
// set later in main() alongside the rest of the ADC setup.
// -----------------------------------------------------------------------
void PORTS_Initialize(void)
{
  TRISA = 0xFF;
  TRISB = 0xFF;

  PORTA = 0x00;
  PORTB = 0x00;
}

// Charlieplex now on RA1-RA3 (bits 1-3), NOT RA0-RA2 -- see header comment
// above re: why RA0 had to move to the ADC input instead.
BYTE cplxMask = 0b00001110;

typedef struct
{
  BYTE tris;
  BYTE port;
} BITSTATES;

// Same six states as the original, shifted left one bit to land on
// RA1-RA3 (bits 1-3) instead of RA0-RA2 (bits 0-2).
BITSTATES bitStates[] = {
    {0b1000, 0b0010}, // LED 1
    {0b1000, 0b0100}, // LED 2
    {0b0010, 0b0100}, // LED 3
    {0b0010, 0b1000}, // LED 4
    {0b0100, 0b0010}, // LED 5
    {0b0100, 0b1000}  // LED 6
};

typedef char STATENDX;
STATENDX     nextState = 0;
STATENDX     stateCnt = sizeof(bitStates)/sizeof(bitStates[0]);

// -----------------------------------------------------------------------
// ADC support: samples RA0/AN0 (VREF+ = AVDD, VREF- = AVSS, both internal)
// and mirrors the 8-bit result onto PORTB (RB0-RB7).  GO/DONE is bit 2 of
// ADCON0 on this device (differs from the PIC18F46K22 original, where it
// was bit 1) -- tested directly rather than via a header bitfield name,
// same rationale as the original: those names vary across device headers/
// compiler versions.
// -----------------------------------------------------------------------
#define ADC_GODONE_MASK 0b00000100

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

  // Select 4 MHz INTOSC -- must match _XTAL_FREQ above for __delay_us()
  // to be accurate. IRCF<2:0> bit pattern below is a common convention
  // across INTOSC-block classic PIC16 devices -- VERIFY against
  // pic16f819.h (see header comment above).
  OSCCONbits.IRCF = 0b110; // 110 = 4 MHz (0=31kHz ... 111=8MHz)

  // configuration should already set pins to high-impedance but just making
  // certain...
  clrCplx();

  // -----------------------------------------------------------------------
  // ADC output port (PORTB): drive the pins low *before* switching them to
  // outputs, so they never glitch high on the transition out of high-Z.
  // No LATx register on this device -- PORTB doubles as both the output
  // latch and the pin-state read, unlike the PIC18F46K22 original's LATD.
  // -----------------------------------------------------------------------
  PORTB = 0x00;
  TRISB = 0x00; // RB0-RB7 all outputs

  // ADCON1: ADFM = 0 (left-justified -- ADRESH alone is the 8-bit result),
  // ADCS2 = 0 (upper clock-select bit, combines with ADCON0<7:6> below),
  // PCFG<3:0> = 1110 -- only AN0 analog, AN1-AN4 digital, VREF+ = AVDD,
  // VREF- = AVSS. See the FLAGGED FOR VERIFICATION header comment above.
  ADCON1 = 0b00001110;

  // ADCON0: ADCS<1:0> = 11 (dedicated ADC RC oscillator, FRC -- independent
  // of FOSC, same rationale as the original), CHS<2:0> = 000 (AN0),
  // GO/DONE = 0, ADON = 1
  ADCON0 = 0b11000001;

  // Acquisition delay -- classic PIC16 mid-range has no ACQT-style
  // hardware acquisition delay (unlike the PIC18F46K22 original's
  // ADCON2<ACQT>); must be inserted manually before every GO/DONE trigger,
  // not just this first one. See header comment above re: the 20us figure.
  __delay_us(20);

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
      PORTB = ADRESH;

      // acquisition delay before every re-trigger -- see comment above the
      // first __delay_us(20) call.
      __delay_us(20);
      ADCON0 |= ADC_GODONE_MASK;
    }
  }
}