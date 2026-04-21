//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemPin.h"
#include "StrUtils.h"

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
    // reparse to split pin label and optional net name if present
    pinLabel.splitQuoted(netName);
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
  // default net mame is double-quotes (empty string); only include if non-empty
  if (netName.getValue().length() > 2) str += " " + netName.toString();
  return str;
}
