//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemJunction.h"
#include "StrUtils.h"

bool ItemJunction::parseItem(const std::string &argStr) {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 1) return false;

  try {
    pt1 = ArgPoint(strList[0]);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemJunction::toString() const {
  return std::string(getTypeStr()) + " " + pt1.toString();
}
