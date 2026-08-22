//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework 
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/*
 * PIC16F1827 adaptation of the PIC16F819 Charlieplex+ADC demo. Pin functions
 * are unchanged from the PIC16F819 version: RA0/AN0 is the ADC input,
 * RA1-RA3 drive the charlieplex LEDs, PORTB (RB0-RB7) mirrors the 8-bit
 * ADC result.
 *
 * This device turned out to be register-identical to the PIC16F1847 for
 * every register this file touches -- ADCON0, ADCON1, ANSELA/ANSELB,
 * WPUA, CONFIG1, and CONFIG2 all match layout and bit values exactly
 * (cross-checked directly against the DS41391D datasheet, not assumed).
 * This file is a straight device-name port of PIC16F1847_ADC.c with no
 * register-value changes.
 *
 * Internal oscillator is configured for 32MHz (up from 4MHz on the PIC16F819
 * original) via the enhanced mid-range core's 4x PLL -- see the OSCCON/
 * PLLEN comments in main() below.
 *
 * On "high-impedance inputs at startup": there is no Configuration Word bit
 * on this device that controls pin direction or analog/digital state --
 * TRISx and ANSELx are ordinary runtime SFRs, not config-word bits. The
 * good news is this device's Power-on Reset default already puts EVERY
 * analog-capable pin into TRIS=1 (input) AND ANSEL=1 (analog, digital
 * input buffer disabled) simultaneously -- a more thorough high-impedance
 * state at power-up than the PIC16F819 achieves via TRIS alone (that
 * device has no ANSELx register at all). PORTS_Initialize() below still
 * explicitly writes TRISA/TRISB, matching the original file's own
 * "should already be true but just making certain" style, but does NOT
 * touch ANSELA/ANSELB until the point in main() where pin function is
 * actually decided -- leaving them at their safe POR default in between.
 */

#include <xc.h>

// Needed for __delay_us() below -- must match the oscillator frequency
// actually selected in main() (32MHz via HFINTOSC + 4xPLL).
#define _XTAL_FREQ 32000000

// -----------------------------------------------------------------------
// Configuration bits
// -----------------------------------------------------------------------
// CONFIG1
#pragma config FOSC    = INTOSC    // internal oscillator, I/O on CLKIN(RA7); CLKOUTEN below frees RA6 too
#pragma config WDTE    = OFF       // demo loop never clears the WDT
#pragma config PWRTE   = ON        // Power-up Timer enabled -- NOTE: bit polarity is inverted (1=disabled,
                                   // 0=enabled), same as the PIC16F819 original; PWRTE=ON here correctly
                                   // programs the bit to 0 (enabled) via the XC8 header's symbolic name --
                                   // VERIFY against pic16f1827.h if not using XC8.
#pragma config MCLRE   = ON        // RA5 functions as external MCLR (Reset), not GPIO
#pragma config CP      = OFF
#pragma config CPD     = OFF
#pragma config BOREN   = OFF
#pragma config CLKOUTEN = OFF      // OFF = CLKOUT function disabled -> RA6 available as plain I/O
                                   // (matches the PIC16F819 original's INTOSCIO choice of freeing both
                                   // OSC pins for I/O, even though this demo doesn't drive RA6/RA7)
#pragma config IESO    = OFF       // no internal/external switchover -- always running from INTOSC
#pragma config FCMEN   = OFF       // no external clock to monitor

// CONFIG2
#pragma config WRT     = OFF
#pragma config STVREN  = ON        // reset on stack overflow/underflow -- cheap safety net
#pragma config BORV    = LO        // moot, since BOREN = OFF above
#pragma config DEBUG   = OFF       // ICSPCLK/ICSPDAT are general purpose I/O, not debugger-dedicated
#pragma config LVP     = OFF       // must be OFF for MCLRE above to take effect (see datasheet 4.4)
#pragma config PLLEN   = ON        // REQUIRED for 32MHz: latches the 4xPLL on permanently via the
                                   // Configuration Word (rather than the software SPLLEN bit in OSCCON) --
                                   // see main() for the matching IRCF/SCS setup

typedef unsigned char BYTE;

// -----------------------------------------------------------------------
// Manual startup init. TRISx is written explicitly even though it already
// matches the POR default (see header comment above) -- ANSELA/ANSELB are
// deliberately left untouched here, staying at their POR default (all
// analog-capable bits = 1) until main() decides actual pin function.
// -----------------------------------------------------------------------
void PORTS_Initialize(void)
{
  TRISA = 0xFF;
  TRISB = 0xFF;

  PORTA = 0x00;
  PORTB = 0x00;
}

// Charlieplex on RA1-RA3 (bits 1-3), same as the PIC16F819 version --
// unchanged, since RA0 (ADC input) and RA1-RA3 map identically on this
// device.
BYTE cplxMask = 0b00001110;

typedef struct
{
  BYTE tris;
  BYTE port;
} BITSTATES;

// Unchanged from the PIC16F819 version -- same six states, same bit
// positions (RA1-RA3).
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
// ADC support: samples RA0/AN0 (VREF+ = VDD, VREF- = VSS, both internal,
// via ADPREF/ADNREF below) and mirrors the upper 8 bits of the 10-bit
// result onto PORTB (RB0-RB7) -- same "ADRESH-only" approach as the
// PIC16F819 original; ADFM=0 (left-justified) puts the 8 MSBs in ADRESH
// alone, same as before, even though this device's ADC is 10-bit rather
// than 8-bit.
//
// GO/DONE is bit 1 of ADCON0 on this device -- matches the PIC18F46K22
// original's bit position, NOT the PIC16F819's (which was bit 2).
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

  // -----------------------------------------------------------------------
  // 32MHz internal oscillator setup, per datasheet Section 5.2.2.6:
  //   1. FOSC<2:0> = INTOSC in the Configuration Word (done above via
  //      #pragma config FOSC = INTOSC)
  //   2. PLLEN = 1 in the Configuration Word (done above) -- latches the
  //      4xPLL on permanently; this is the config-word route rather than
  //      the software SPLLEN bit in OSCCON, chosen so 32MHz is fixed at
  //      power-up rather than something firmware has to remember to
  //      enable
  //   3. IRCF<3:0> = 1110 selects the 8MHz HFINTOSC tap, which the
  //      already-enabled 4xPLL multiplies up to 32MHz
  //   4. SCS<1:0> = 00 so the system clock is the one FOSC<2:0> selects
  //      (INTOSC), not a software-forced override
  // -----------------------------------------------------------------------
  OSCCONbits.IRCF = 0b1110; // 8MHz HFINTOSC tap -> 32MHz after the 4xPLL
  OSCCONbits.SCS  = 0b00;   // clock source per FOSC<2:0> (INTOSC), not overridden

  // configuration should already set pins to high-impedance but just making
  // certain...
  clrCplx();

  // -----------------------------------------------------------------------
  // ADC output port (PORTB): drive the pins low *before* switching them to
  // outputs, so they never glitch high on the transition out of high-Z.
  // No LATx register access needed here -- PORTB doubles as both the
  // output latch and the pin-state read, same as the PIC16F819 original.
  // -----------------------------------------------------------------------
  PORTB = 0x00;
  TRISB = 0x00;   // RB0-RB7 all outputs
  ANSELB = 0x00;  // RB0-RB7 all digital (POR default is analog -- see header comment)

  // ANSELA: only RA0/AN0 analog; RA1-RA4 stay digital for the charlieplex
  // and for RA5 (MCLR, not affected by ANSELA) / RA6-RA7 (unused here).
  // POR default is all-1 (analog) -- see header comment above.
  ANSELA = 0b00000001;

  // NOTE: the datasheet's ADC setup example (Example 16-1) shows disabling
  // a weak pull-up on the analog pin ("BCF WPUA,0"), but that doesn't
  // apply on this device -- WPUA only implements bit 5 (WPUA5, for
  // RA5/MCLR); RA0-RA4/RA6/RA7 have no individually-controllable weak
  // pull-up at all. Nothing to do here.

  // ADCON1: ADFM = 0 (left-justified -- ADRESH alone is the upper 8 bits
  // of the 10-bit result), ADCS<2:0> = 011 (dedicated FRC clock,
  // independent of FOSC, same rationale as the PIC16F819 original),
  // ADNREF = 0 (VREF- = VSS), ADPREF<1:0> = 00 (VREF+ = VDD)
  ADCON1 = 0b00110000;

  // ADCON0: CHS<4:0> = 00000 (AN0), GO/DONE = 0, ADON = 1
  ADCON0 = 0b00000001;

  // Acquisition delay -- like the PIC16F819, this device has no ACQT-style
  // hardware acquisition delay (unlike the PIC18F46K22 original's
  // ADCON2<ACQT>); must be inserted manually before every GO/DONE trigger,
  // not just this first one. 20us carried forward from the PIC16F819
  // version as a conservative margin -- VERIFY against this device's
  // actual minimum acquisition time (Section 16.3) if precise timing
  // matters for your use case.
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
      // conversion complete -- ADFM=0 means ADRESH alone holds the upper
      // 8 bits of the result; mirror it onto the output port and start
      // the next sample
      PORTB = ADRESH;

      // acquisition delay before every re-trigger -- see comment above the
      // first __delay_us(20) call.
      __delay_us(20);
      ADCON0 |= ADC_GODONE_MASK;
    }
  }
}
