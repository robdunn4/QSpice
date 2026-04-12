//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemType.h"
#include "StrUtils.h"

ItemBasePtr ItemType::clone() const {
  return std::make_shared<ItemType>(*this);
}

bool ItemType::parseItem(const std::string &argStr) {
  try {
    text = ArgString(argStr);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemType::toString() const {
  std::string str = std::string(getTypeStr()) + " " + text.toString();
  StrUtils::trim(str);
  return str;
}
