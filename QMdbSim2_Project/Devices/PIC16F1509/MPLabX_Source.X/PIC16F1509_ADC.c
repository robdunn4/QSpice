//------------------------------------------------------------------------------
// This file is part of the QMdbSim2 project, a Microchip Simulator framework 
// for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, current
// sources, documentation, and demonstration code.
//------------------------------------------------------------------------------
/*
 * PIC16F1509 adaptation of the Charlieplex+ADC demo (most recently built for
 * PIC16F877A). Started from that file per instructions, but this device's
 * pin set forced two real redesigns rather than a simple port -- both
 * discovered directly from this project's own PIC16F1509.qpindef analysis
 * and this datasheet, not assumed from precedent:
 *
 * 1. CHARLIEPLEX MOVED FROM RA1-RA3 TO RA1/RA2/RA4. This device's RA3 is
 *    confirmed INPUT-ONLY (Table 1-2: "General purpose INPUT with IOC and
 *    WPU", no output driver at all, even with MCLR disabled) -- unlike the
 *    MCLR-shared pin on every other device this demo has targeted. The
 *    charlieplex scheme requires all three pins to drive both directions,
 *    so RA1-RA3 as used everywhere else simply cannot work here. Re-derived
 *    the six LED bit patterns for RA1(bit1)/RA2(bit2)/RA4(bit4) instead of
 *    the original RA1(bit1)/RA2(bit2)/RA3(bit3) -- see cplxMask and
 *    bitStates[] below.
 *
 * 2. ADC MIRROR MOVED FROM PORTB TO PORTC. This device's PORTB only
 *    implements 4 pins (RB4-RB7), not a full 8-bit port like every other
 *    device this demo has targeted -- there is nowhere to mirror an 8-bit
 *    ADC result 1:1 onto "PORTB" here. PORTC has the needed 8 ordinary
 *    bidirectional GPIO pins (RC0-RC7), none used elsewhere in this demo,
 *    so the mirror moved there instead.
 *
 * RA0/AN0 remains the ADC input, unchanged.
 *
 * A note on ICSPDAT/ICSPCLK (RA0/RA1 on this device) and the DEBUG bit:
 * on real hardware, enabling the in-circuit debugger reserves those pins
 * permanently for physical ICSP wire communication with the programmer/
 * debugger, which would conflict with using them as ADC input and a
 * charlieplex leg. DEBUG is not a user-settable Configuration Word
 * pragma at all on this device/XC8 combination -- it's managed by the
 * MPLAB X build configuration instead, which is why the line has been
 * removed below rather than set explicitly.
 *
 * That real-hardware caveat likely does not apply to this project's
 * actual use, though: QMdbSim2 drives the simulated device through
 * MPLAB X's debugger core (MDBCore) as its normal mode of operation, not
 * as an occasional debugging session on physical silicon over a real
 * ICSP cable -- Debug-mode builds are how this whole co-simulation
 * framework runs, not something to avoid. This file compiles and has
 * been confirmed to build under a Debug configuration; the physical-pin-
 * hijacking concern that legitimately applies to real hardware appears
 * not to carry over to MDBCore's simulated target. CLKOUTEN must still
 * be disabled regardless, since RA4/CLKOUT is a charlieplex leg here --
 * otherwise that pin would output FOSC/4 instead of being available as
 * GPIO.
 *
 * Oscillator: this device has an internal oscillator, but it tops out at
 * 16MHz (not 32MHz like the PIC16F1847/1827) -- there is no CPU-clock PLL
 * on this part at all (confirmed: no PLLEN field exists in either
 * Configuration Word). Configured for the internal oscillator's maximum,
 * 16MHz, as the closest equivalent to "run it as fast as possible
 * internally" given this device's real ceiling.
 *
 * On "high-impedance inputs at startup": same story as every enhanced
 * mid-range part in this project -- no Configuration Word bit controls
 * pin direction or analog/digital state. TRISx defaults to 0xFF (input)
 * at POR; ANSELA/ANSELC default to all-1 (analog, digital buffer
 * disabled) for every analog-capable bit, same protective POR philosophy
 * as the PIC16F1847/1827.
 */

#include <xc.h>

// Needed for __delay_us() below -- must match the oscillator frequency
// actually selected in main() (16MHz HFINTOSC -- this device's maximum;
// no PLL exists here to reach higher).
#define _XTAL_FREQ 16000000

// -----------------------------------------------------------------------
// Configuration bits (two-word CONFIG1/CONFIG2, same structure as the
// PIC16F1847/1827)
// -----------------------------------------------------------------------
// CONFIG1
#pragma config FOSC     = INTOSC   // internal oscillator
#pragma config WDTE     = OFF      // demo loop never clears the WDT
#pragma config PWRTE    = ON       // Power-up Timer enabled -- NOTE: bit polarity is inverted
                                    // (1=disabled, 0=enabled), same recurring caveat as every other
                                    // port in this project; PWRTE=ON correctly programs the bit to 0
                                    // (enabled) via the XC8 header's symbolic name -- VERIFY against
                                    // pic16f1509.h if not using XC8.
#pragma config MCLRE    = ON       // RA3 functions as MCLR. Does NOT affect output capability either
                                    // way -- RA3 has no output driver at all per Table 1-2, regardless
                                    // of this bit (see header comment above).
#pragma config CP       = OFF
#pragma config BOREN    = OFF
#pragma config CLKOUTEN = OFF      // REQUIRED: RA4 is a charlieplex leg in this port -- if CLKOUTEN
                                    // were on, RA4/CLKOUT would output FOSC/4 instead of being GPIO
#pragma config IESO     = OFF      // no internal/external switchover -- always running from INTOSC
#pragma config FCMEN    = OFF      // no external clock to monitor

// CONFIG2
#pragma config WRT      = OFF
#pragma config STVREN   = ON       // reset on stack overflow/underflow -- cheap safety net
#pragma config BORV     = LO       // moot, since BOREN = OFF above
#pragma config LPBOR    = OFF      // moot for the same reason; low-power BOR not used
// DEBUG is not a user-settable pragma on this device/XC8 combination --
// it's managed by the MPLAB X build configuration instead, which is why
// no line is set here. See the header comment above for why this
// doesn't appear to conflict with RA0/RA1 usage in QMdbSim2's actual
// (simulated-target) execution model, unlike on real hardware.
#pragma config LVP      = OFF

typedef unsigned char BYTE;

// -----------------------------------------------------------------------
// Manual startup init. TRISx is written explicitly even though it already
// matches the POR default -- ANSELA/ANSELC are deliberately left
// untouched here, staying at their POR default (all analog-capable bits
// = 1) until main() decides actual pin function.
// -----------------------------------------------------------------------
void PORTS_Initialize(void)
{
  TRISA = 0xFF;
  TRISC = 0xFF;

  PORTA = 0x00;
  PORTC = 0x00;
}

// Charlieplex on RA1/RA2/RA4 (bits 1, 2, 4) -- NOT RA1-RA3, see header
// comment above. RA3 (bit 3) is deliberately skipped: input-only on this
// device.
BYTE cplxMask = 0b00010110;

typedef struct
{
  BYTE tris;
  BYTE port;
} BITSTATES;

// Re-derived for RA1(bit1)/RA2(bit2)/RA4(bit4) -- same six-state
// charlieplex pattern as every other version of this demo (each state
// drives two of the three legs, high-Zs the third), just remapped off
// the original's consecutive-bit assumption (bits 1,2,3) onto this
// device's usable set (bits 1,2,4).
BITSTATES bitStates[] = {
    {0b00010000, 0b00000010}, // LED 1: RA4 hi-Z, RA1 high, RA2 low
    {0b00010000, 0b00000100}, // LED 2: RA4 hi-Z, RA2 high, RA1 low
    {0b00000010, 0b00000100}, // LED 3: RA1 hi-Z, RA2 high, RA4 low
    {0b00000010, 0b00010000}, // LED 4: RA1 hi-Z, RA4 high, RA2 low
    {0b00000100, 0b00000010}, // LED 5: RA2 hi-Z, RA1 high, RA4 low
    {0b00000100, 0b00010000}  // LED 6: RA2 hi-Z, RA4 high, RA1 low
};

typedef char STATENDX;
STATENDX     nextState = 0;
STATENDX     stateCnt = sizeof(bitStates)/sizeof(bitStates[0]);

// -----------------------------------------------------------------------
// ADC support: samples RA0/AN0 (VREF+ = VDD, no negative reference option
// on this device -- always VSS) and mirrors the 8-bit result onto PORTC
// (RC0-RC7) -- moved from PORTB, see header comment above. ADFM=0
// (left-justified) puts the 8 MSBs in ADRESH alone, same approach as
// every other version of this demo.
//
// GO/DONE is bit 1 of ADCON0 on this device -- matches the PIC16F1847/
// PIC16F1827 bit position, NOT the PIC16F819/PIC16F877A's (bit 2).
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

  // 16MHz internal oscillator -- this device's maximum; no PLL exists to
  // go higher (see header comment above).
  OSCCONbits.IRCF = 0b1111; // 1111 = 16 MHz
  OSCCONbits.SCS  = 0b00;   // clock source per FOSC<2:0> (INTOSC), not overridden

  // configuration should already set pins to high-impedance but just making
  // certain...
  clrCplx();

  // -----------------------------------------------------------------------
  // ADC output port (PORTC): drive the pins low *before* switching them to
  // outputs, so they never glitch high on the transition out of high-Z.
  // -----------------------------------------------------------------------
  PORTC = 0x00;
  TRISC = 0x00;   // RC0-RC7 all outputs
  ANSELC = 0x00;  // RC0-RC7 all digital (POR default is analog on the 6 analog-capable
                   // bits -- RC4/RC5 have no ANSEL bit at all, always digital)

  // ANSELA: only RA0/AN0 analog; RA1/RA2/RA4 stay digital for the
  // charlieplex. RA3 has no ANSEL bit at all (no analog function). POR
  // default is all-1 (analog) on the implemented bits -- see header
  // comment above.
  ANSELA = 0b00000001;

  // ADCON1: ADFM = 0 (left-justified), ADCS<2:0> = 011 (dedicated FRC
  // clock, independent of FOSC), ADPREF<1:0> = 00 (VRPOS = VDD -- this
  // device has no negative-reference option, VSS is implicit)
  ADCON1 = 0b00110000;

  // ADCON0: CHS<4:0> = 00000 (AN0), GO/DONE = 0, ADON = 1
  ADCON0 = 0b00000001;

  // Acquisition delay -- no ACQT-style hardware acquisition delay on this
  // device; must be inserted manually before every GO/DONE trigger, not
  // just this first one. 20us carried forward from earlier versions of
  // this demo as a conservative margin -- VERIFY against this device's
  // actual minimum acquisition time (Section 15.0) if precise timing
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
      // conversion complete -- ADFM=0 means ADRESH alone holds the 8-bit
      // result; mirror it onto the output port and start the next sample
      PORTC = ADRESH;

      // acquisition delay before every re-trigger -- see comment above the
      // first __delay_us(20) call.
      __delay_us(20);
      ADCON0 |= ADC_GODONE_MASK;
    }
  }
}
