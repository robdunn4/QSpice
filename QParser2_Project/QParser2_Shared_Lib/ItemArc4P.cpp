//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemArc4P.h"
#include "StrUtil.h"

bool ItemArc4P::parseItem(const std::string &argStr) {
  StrList strList = StrUtil::tokenize(argStr);

  if (strList.size() != 10) return false;

  try {
    pt1       = ArgPoint(strList[0]);
    pt2       = ArgPoint(strList[1]);
    pt3       = ArgPoint(strList[2]);
    pt4       = ArgPoint(strList[3]);
    rotate    = ArgRot(strList[4]);
    lineWidth = ArgLineWidth(strList[5]);
    lineType  = ArgLineType(strList[6]);
    lineColor = ArgColor(strList[7]);
    lookupNdx = ArgLookupNdx(strList[8]);
    pinNdx    = ArgPinNdx(strList[9]);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemArc4P::toString() const {
  return std::string(getTypeStr()) + " " + pt1.toString() + " " +
         pt2.toString() + " " + pt3.toString() + " " + pt4.toString() + " " +
         rotate.toString() + " " + lineWidth.toString() + " " +
         lineType.toString() + " " + lineColor.toString() + " " +
         lookupNdx.toString() + " " + pinNdx.toString();
}
