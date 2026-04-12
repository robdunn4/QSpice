//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemTriangle.h"
#include "StrUtils.h"

ItemBasePtr ItemTriangle::clone() const {
  return std::make_shared<ItemTriangle>(*this);
}

bool ItemTriangle::parseItem(const std::string &argStr) {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 9) return false;

  try {
    pt1       = ArgPoint(strList[0]);
    pt2       = ArgPoint(strList[1]);
    pt3       = ArgPoint(strList[2]);
    lineWidth = ArgLineWidth(strList[3]);
    lineType  = ArgLineType(strList[4]);
    lineColor = ArgColor(strList[5]);
    fillColor = ArgColor(strList[6]);
    lookupNdx = ArgLookupNdx(strList[7]);
    pinNdx    = ArgPinNdx(strList[8]);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemTriangle::toString() const {
  return std::string(getTypeStr()) + " " + pt1.toString() + " " +
         pt2.toString() + " " + pt3.toString() + " " + lineWidth.toString() +
         " " + lineType.toString() + " " + lineColor.toString() + " " +
         fillColor.toString() + " " + lookupNdx.toString() + " " +
         pinNdx.toString();
}
