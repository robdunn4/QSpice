//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemTriangle.h"
#include "StrUtils.h"

ItemTriangle::ItemTriangle(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemTriangle::ItemTriangle(const ItemTriangle &other)
    : ItemBase(other), pt1(other.pt1), pt2(other.pt2), pt3(other.pt3),
      lineWidth(other.lineWidth), lineType(other.lineType),
      lineColor(other.lineColor), fillColor(other.fillColor),
      lookupNdx(other.lookupNdx), pinNdx(other.pinNdx) {}

ItemBasePtr ItemTriangle::clone() const {
  return std::make_shared<ItemTriangle>(*this);
}

void ItemTriangle::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 9) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1       = ArgPoint(strList[0]);
  pt2       = ArgPoint(strList[1]);
  pt3       = ArgPoint(strList[2]);
  lineWidth = ArgLineWidth(strList[3]);
  lineType  = ArgLineType(strList[4]);
  lineColor = ArgColor(strList[5]);
  fillColor = ArgColor(strList[6]);
  lookupNdx = ArgLookupNdx(strList[7]);
  pinNdx    = ArgPinNdx(strList[8]);
}

std::string ItemTriangle::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() +
                    " " + pt3.toString() + " " + lineWidth.toString() + " " +
                    lineType.toString() + " " + lineColor.toString() + " " +
                    fillColor.toString() + " " + lookupNdx.toString() + " " +
                    pinNdx.toString();
  return str;
}

