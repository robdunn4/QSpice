//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemCmp.h"
#include "StrUtils.h"

bool ItemCmp::parseItem(const std::string &argStr) {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 3) return false;

  try {
    pt1        = ArgPoint(strList[0]);
    rotate     = ArgRot(strList[1]);
    stuffShort = ArgStuff(strList[2]);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemCmp::toString() const {
  return std::string(getTypeStr()) + " " + pt1.toString() + " " +
         rotate.toString() + " " + stuffShort.toString();
}
