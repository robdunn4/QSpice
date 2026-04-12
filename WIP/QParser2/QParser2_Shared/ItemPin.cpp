//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemPin.h"
#include "StrUtils.h"

ItemBasePtr ItemPin::clone() const { return std::make_shared<ItemPin>(*this); }

bool ItemPin::parseItem(const std::string &argStr) {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() < 8 || strList.size() > 9) return false;

  try {
    pt1         = ArgPoint(strList[0]);
    pt2         = ArgPoint(strList[1]);
    fontSize    = ArgFontSize(strList[2]);
    rotateAlign = ArgRotAlign(strList[3]);
    pinInfo     = ArgPinInfo(strList[4]);
    textColor   = ArgColor(strList[5]);
    lookupNdx   = ArgLookupNdx(strList[6]);
    pinLabel    = ArgString(strList[7]);
    netName     = strList.size() < 9 ? std::string() : strList[8];
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemPin::toString() const {
  std::string str = std::string(getTypeStr()) + " " + pt1.toString() + " " +
                    pt2.toString() + " " + fontSize.toString() + " " +
                    rotateAlign.toString() + " " + pinInfo.toString() + " " +
                    textColor.toString() + " " + lookupNdx.toString() + " " +
                    pinLabel.toString();
  if (netName.getValue().length()) str += " " + netName.toString();
  return str;
}
