//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemLine.h"
#include "StrUtil.h"

bool ItemLine::parseItem(const std::string &argStr) {
  StrList strList = StrUtil::tokenize(argStr);

  if (strList.size() != 7) return false;

  try {
    pt1       = ArgPoint(strList[0]);
    pt2       = ArgPoint(strList[1]);
    lineWidth = ArgLineWidth(strList[2]);
    lineType  = ArgLineType(strList[3]);
    lineColor = ArgColor(strList[4]);
    lookupNdx = ArgLookupNdx(strList[5]);
    pinNdx    = ArgPinNdx(strList[6]);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemLine::toString() const {
  return std::string(getTypeStr()) + " " + pt1.toString() + " " +
         pt2.toString() + " " + lineWidth.toString() + " " +
         lineType.toString() + " " + lineColor.toString() + " " +
         lookupNdx.toString() + " " + pinNdx.toString();
}
