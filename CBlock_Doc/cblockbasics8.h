/*******************************************************************************
 * CBlockBasics8.h -- Shared Per-Instance and Binary Data Struture Declarations
 * for C-Block Basics #8 project.
 *
 * See https://github.com/robdunn4/QSpice/ for additional documentation/code.
 ******************************************************************************/

#ifndef CBLOCKBASICS8_H
#define CBLOCKBASICS8_H

#include <cstring>

// *** Per-instance data structure ***
struct InstData {
  int simPtCnt;   // # of simulation points evaluted
  int instNbr;    // generated in order of instance creation
  int stepNbr;    // step # (from QSpice)
  int stepVal;    // from StepVal input attribute
  int totSteps;   // total number of expected steps (from QSpice)

  double lastT;      // often useful
  double lastVin;    // often useful
  double lastVout;   // often useful
};

// *** Binary Log File ***
// pattern to find start of records in file hex dumps (optional)
const char BinDataMarker[4] = {'\xDE', '\xAD', '\xBE', '\xEF'};

// arbitrary max length for inst name in log (beware!)
const int MaxInstNameLen = 8;

struct BinData {
  // start of data block marker for viewing hex dumps (optional)
  char blkMarker[sizeof(BinDataMarker)];

  char instName[MaxInstNameLen + 1];   // from InstanceName (null-terminated)

  InstData instData;   // copy of per-instance data

  // constructors
  BinData() {
    memcpy(blkMarker, BinDataMarker, sizeof(blkMarker));
    memset(instName, 0, sizeof(instName));
    memset(&instData, 0, sizeof(instData));
  }

  BinData(const InstData *inst, const char *instName) : instData(*inst) {
    memcpy(blkMarker, BinDataMarker, sizeof(blkMarker));
    memset(this->instName, 0, sizeof(instName));
    strncpy(this->instName, instName, MaxInstNameLen);
  }
};

#endif   // CBLOCKBASICS8_H

/*******************************************************************************
 * EOF CBlockBasics8.h
 ******************************************************************************/
