//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemArc3P.h"
#include "StrUtil.h"

bool ItemArc::parseItem(const std::string &argStr) {
  StrList strList = StrUtil::tokenize(argStr);

  if (strList.size() != 8) return false;

  try {
    pt1       = ArgPoint(strList[0]);
    pt2       = ArgPoint(strList[1]);
    pt3       = ArgPoint(strList[2]);
    lineWidth = ArgLineWidth(strList[3]);
    lineType  = ArgLineType(strList[4]);
    lineColor = ArgColor(strList[5]);
    lookupNdx = ArgLookupNdx(strList[6]);
    pinNdx    = ArgPinNdx(strList[7]);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemArc::toString() const {
  return std::string(getTypeStr()) + " " + pt1.toString() + " " +
         pt2.toString() + " " + pt3.toString() + " " + lineWidth.toString() +
         " " + lineType.toString() + " " + lineColor.toString() + " " +
         lookupNdx.toString() + " " + pinNdx.toString();
}
