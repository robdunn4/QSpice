//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemShort.h"

bool ItemShort::parseItem(const std::string &argStr) {
  try {
    bShorted = ArgBool(argStr);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemShort::toString() const {
  return std::string(getTypeStr()) + " " + bShorted.toString();
}
