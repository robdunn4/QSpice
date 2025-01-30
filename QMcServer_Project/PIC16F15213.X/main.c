/*
 * Test code for Microprocessor project.  This code implements a "Charlie-
 * Plexing" output on pins RA0-RA2.  This is for "proof of concept" to 
 * demonstrate minimal functionality to handle tri-state pins.
 */

#include "mcc_generated_files/system/system.h"

typedef unsigned char BYTE;

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
  // MPLabX-generated initialization
  SYSTEM_Initialize();

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