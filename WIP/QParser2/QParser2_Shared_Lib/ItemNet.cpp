//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemNet.h"
#include "StrUtils.h"

bool ItemNet::parseItem(const std::string &argStr) {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 5) return false;

  try {
    pt1         = ArgPoint(strList[0]);
    fontSize    = ArgFontSize(strList[1]);
    rotateAlign = ArgRotAlign(strList[2]);
    netType     = ArgInt(strList[3]);
    netName     = strList[4];
    netDesc     = std::string();
    // reparse for net description if present
    netName.splitQuoted(netDesc);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemNet::toString() const {
  std::string str = std::string(getTypeStr()) + " " + pt1.toString() + " " +
                    fontSize.toString() + " " + rotateAlign.toString() + " " +
                    netType.toString() + " " + netName.toString();
  if (netDesc.getValue().length()) str += " " + netDesc.toString();
  return str;
}
