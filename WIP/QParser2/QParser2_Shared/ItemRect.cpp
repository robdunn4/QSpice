//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemRect.h"
#include "StrUtils.h"

ItemRect::ItemRect(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemRect::ItemRect(const ItemRect &other)
    : ItemBase(other), pt1(other.pt1), pt2(other.pt2), rotate(other.rotate),
      lineWidth(other.lineWidth), lineType(other.lineType),
      lineColor(other.lineColor), fillColor(other.fillColor),
      lookupNdx(other.lookupNdx),
      isHierarchicalBlock(other.isHierarchicalBlock), pinNdx(other.pinNdx),
      imageData(other.imageData) {}

ItemBasePtr ItemRect::clone() const {
  return std::make_shared<ItemRect>(*this);
}

void ItemRect::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  // we expect eleven subStrings
  if (strList.size() < 10 || strList.size() > 11) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

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
}

std::string ItemRect::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() +
                    " " + rotate.toString() + " " + lineWidth.toString() + " " +
                    lineType.toString() + " " + lineColor.toString() + " " +
                    fillColor.toString() + " " + lookupNdx.toString() + " " +
                    isHierarchicalBlock.toString() + " " + pinNdx.toString();
  // if image data present...
  if (imageData.getValue().length()) str += " " + imageData.toString();
  return str;
}

