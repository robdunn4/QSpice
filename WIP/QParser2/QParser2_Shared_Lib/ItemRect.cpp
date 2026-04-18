//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemRect.h"
#include "StrUtils.h"

bool ItemRect::parseItem(const std::string &argStr) {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  // we expect ten or eleven subStrings
  if (strList.size() < 10 || strList.size() > 11) return false;

  try {
    pt1                 = ArgPoint(strList[0]);
    pt2                 = ArgPoint(strList[1]);
    rotate              = ArgRot(strList[2]);
    lineWidth           = ArgLineWidth(strList[3]);
    lineType            = ArgLineType(strList[4]);
    lineColor           = ArgColor(strList[5]);
    fillColor           = ArgColor(strList[6]);
    lookupNdx           = ArgLookupNdx(strList[7]);
    isHierarchicalBlock = ArgInt(strList[8]);
    pinNdx              = ArgPinNdx(strList[9]);
    if (strList.size() == 11) imageData = ArgImage(strList[10]);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemRect::toString() const {
  std::string str = std::string(getTypeStr()) + " " + pt1.toString() + " " +
                    pt2.toString() + " " + rotate.toString() + " " +
                    lineWidth.toString() + " " + lineType.toString() + " " +
                    lineColor.toString() + " " + fillColor.toString() + " " +
                    lookupNdx.toString() + " " +
                    isHierarchicalBlock.toString() + " " + pinNdx.toString();
  // if image data present...
  if (imageData.getValue().length()) str += " " + imageData.toString();
  return str;
}
