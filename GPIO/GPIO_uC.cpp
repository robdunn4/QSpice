/*==============================================================================
 * GPIO_uC.cpp -- Proof of Concept GPIO QSpice C-Block component.
 *
 * The code is intended to test mapping GPIO pins to/from something similar to a
 * Microchip PIC micro-controller's port, latch, and TRIS registers.
 *
 * To demonstrate a use-case where tri-state pins are reconfigured after
 * initialization, this code implements "Charlieplexing."  See
 * https://en.wikipedia.org/wiki/Charlieplexing for more information.
 *
 * Note:  Compiles with MSVC.  Does not compile with DMC.
 * Requires C++17 or newer.
 *============================================================================*/
#include <cstdlib>

/*------------------------------------------------------------------------------
 * Versioning Information
 *----------------------------------------------------------------------------*/
#define PROGRAM_NAME    "GPIO_uC"
#define PROGRAM_VERSION "v0.1"
#define PROGRAM_INFO    PROGRAM_NAME " " PROGRAM_VERSION

// must follow above versioning information
#include "Cblock.h"

#include "GPIO_Class.h"

/*------------------------------------------------------------------------------
 * Per-instance Data
 *----------------------------------------------------------------------------*/
struct InstData {
  bool         lastClk;
  unsigned int ctr;

  GPIO_Pin_Ptr p1;
  GPIO_Pin_Ptr p2;
  GPIO_Pin_Ptr p3;
  GPIO_Pin_Ptr p4;
};

/*------------------------------------------------------------------------------
 * UDATA() definition -- regenerate the template with QSpice and revise this
 * whenever ports/attributes change; make input/attribute parameters const&
 *----------------------------------------------------------------------------*/
#define UDATA(data)                                                            \
  const bool   &Clk   = data[0].b;  /* input */                                \
  const double &PIn1  = data[1].d;  /* input */                                \
  const double &PIn2  = data[2].d;  /* input */                                \
  const double &PIn3  = data[3].d;  /* input */                                \
  const bool   &PIn4  = data[4].b;  /* input */                                \
  const double &Vcc   = data[5].d;  /* input */                                \
  double       &POut1 = data[6].d;  /* output */                               \
  bool         &PCtl1 = data[7].b;  /* output */                               \
  double       &POut2 = data[8].d;  /* output */                               \
  bool         &PCtl2 = data[9].b;  /* output */                               \
  double       &POut3 = data[10].d; /* output */                               \
  bool         &PCtl3 = data[11].b; /* output */                               \
  bool         &POut4 = data[12].b; /* output */                               \
  bool         &PCtl4 = data[13].b; /* output */

/*------------------------------------------------------------------------------
 * Evaluation Function
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void gpio_uc(
    InstData **opaque, double t, uData data[]) {

  UDATA(data);

  InstData *inst = *opaque;

  if (!*opaque) {
    *opaque = new InstData;
    inst    = *opaque;

    if (!*opaque) {
      // terminate with prejudice
      msg("Memory allocation failure.  Terminating simulation.\n");
      exit(1);
    }

    // create pins
    inst->p1 = makeGpioPinPtr(PIn1, POut1, PCtl1, Vcc);
    inst->p2 = makeGpioPinPtr(PIn2, POut2, PCtl2, Vcc);
    inst->p3 = makeGpioPinPtr(PIn3, POut3, PCtl3, Vcc);
    inst->p4 = makeGpioPinPtr(PIn4, POut4, PCtl4, Vcc);

    // set p4 for output
    inst->p4->setTris(TRIS_OUT);

    // if important, output component parameters
    msg("Component loaded.\n");
  }

  // if not rising clock edge, quick out
  if (inst->lastClk == Clk) return;
  inst->lastClk = Clk;
  if (!Clk) return;

  // demonstrate tri-state on boolean p4
  switch (inst->ctr % 3) {
  case 0:
    inst->p4->setTris(TRIS_IN);
    break;
  case 1:
    inst->p4->setTris(TRIS_OUT);
    inst->p4->writePort(0);
    break;
  case 2:
    inst->p4->setTris(TRIS_OUT);
    inst->p4->writePort(1);
    break;
  default:
    msg("Error -- should not get here!\n");
  }

  // charlie-plex LEDs on GPIO pins 1-3
  // set GPIO 1-3 for high-impedance (inputs)
  inst->p1->setTris(TRIS_IN);
  inst->p2->setTris(TRIS_IN);
  inst->p3->setTris(TRIS_IN);

  // set plexing
  switch (inst->ctr) {
  case 0:
    inst->p1->setTris(TRIS_OUT);
    inst->p1->writePort(1);
    inst->p2->setTris(TRIS_OUT);
    inst->p2->writePort(0);
    break;
  case 1:
    inst->p1->setTris(TRIS_OUT);
    inst->p1->writePort(0);
    inst->p2->setTris(TRIS_OUT);
    inst->p2->writePort(1);
    break;
  case 2:
    inst->p2->setTris(TRIS_OUT);
    inst->p2->writePort(1);
    inst->p3->setTris(TRIS_OUT);
    inst->p3->writePort(0);
    break;
  case 3:
    inst->p2->setTris(TRIS_OUT);
    inst->p2->writePort(0);
    inst->p3->setTris(TRIS_OUT);
    inst->p3->writePort(1);
    break;
  case 4:
    inst->p1->setTris(TRIS_OUT);
    inst->p1->writePort(1);
    inst->p3->setTris(TRIS_OUT);
    inst->p3->writePort(0);
    break;
  case 5:
    inst->p1->setTris(TRIS_OUT);
    inst->p1->writePort(0);
    inst->p3->setTris(TRIS_OUT);
    inst->p3->writePort(1);
    break;
  default:
    msg("Error -- should not get here!\n");
  }

  inst->ctr++;
  inst->ctr %= 6;
}

/*------------------------------------------------------------------------------
 * Destroy()
 *----------------------------------------------------------------------------*/
extern "C" __declspec(dllexport) void Destroy(InstData &inst) {
  // free allocated memory
  delete &inst;
}
/*==============================================================================
 * End of Cblock.cpp
 *============================================================================*/
