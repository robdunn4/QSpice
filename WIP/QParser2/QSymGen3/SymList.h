//-----------------------------------------------------------------------------
// This file is part of the the QSymGen3 command-line tool contained in the
// QParser2 project.  You can find the complete project here:
// https://github.com/robdunn4/QSpice/
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

  // Populates *this with the QSymGen3 code snippet lines.
  // The caller passes *this as a block substitution to TemplateProcessor
  // rather than writing it directly to disk.
  bool makeCppSnippet(const PinDefList &pinList,
                      std::ostream     &errStrm = nullStream);

protected:
};
