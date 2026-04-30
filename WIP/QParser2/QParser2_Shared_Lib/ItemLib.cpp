//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemLib.h"
#include "StrUtils.h"

bool ItemLib::parseItem(const std::string &argStr) {
  try {
    libText = ArgLibString(argStr);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemLib::toString() const {
  std::string str = std::string(getTypeStr()) + " " + libText.toString();
  StrUtils::trim(str);
  return str;
}
