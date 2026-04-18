//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemTap.h"
#include "StrUtils.h"

bool ItemTap::parseItem(const std::string &argStr) {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 2) return false;

  try {
    pt1 = ArgPoint(strList[0]);
    pt2 = ArgPoint(strList[1]);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemTap::toString() const {
  return std::string(getTypeStr()) + " " + pt1.toString() + " " +
         pt2.toString();
}
