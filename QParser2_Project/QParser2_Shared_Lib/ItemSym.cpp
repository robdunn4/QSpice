//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemSym.h"
#include "StrUtil.h"

bool ItemSym::parseItem(const std::string &argStr) {
  // The name is the rest of the record, spaces and all: QSpice writes
  // "symbol TL071 symbol" for a symbol it generated from a model, and a
  // file holding one failed to open (2026-11-27). The name was taken as
  // one token before, and a second refused.
  try {
    text = ArgString(StrUtil::trim(argStr));
  }
  catch (...) {
    return false;
  }
  return true;
}

std::string ItemSym::toString() const {
  std::string str = std::string(getTypeStr()) + " " + text.toString();
  return StrUtil::trim(str);
}
