//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include "NullStream.h"
#include "PinDefs.h"
#include "StrList.h"

class SymList : public StrList {
public:
  SymList() {}

  bool makeSymbol(const PinDefList &pinList, int rectWidth,
                  std::ostream &errStrm = nullStream);

  bool makeSchematic(const PinDefList &pinList,
                     std::ostream     &errStrm = nullStream);

protected:
};
