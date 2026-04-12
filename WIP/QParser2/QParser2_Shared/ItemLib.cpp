//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemLib.h"
#include "StrUtils.h"

ItemBasePtr ItemLib::clone() const { return std::make_shared<ItemLib>(*this); }

bool ItemLib::parseItem(const std::string &argStr) {
  try {
    libText = ArgString(argStr);
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
