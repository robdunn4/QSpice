//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemZigzag.h"
#include "StrUtils.h"

ItemBasePtr ItemZigzag::clone() const {
  return std::make_shared<ItemZigzag>(*this);
}

bool ItemZigzag::parseItem(const std::string &argStr) {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 8) return false;

  try {
    pt1       = ArgPoint(strList[0]);
    pt2       = ArgPoint(strList[1]);
    rotation  = ArgRot(strList[2]);
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

std::string ItemZigzag::toString() const {
  return std::string(getTypeStr()) + " " + pt1.toString() + " " +
         pt2.toString() + " " + rotation.toString() + " " +
         lineWidth.toString() + " " + lineType.toString() + " " +
         lineColor.toString() + " " + lookupNdx.toString() + " " +
         pinNdx.toString();
}
