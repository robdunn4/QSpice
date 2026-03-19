//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemLine.h"
#include "StrUtils.h"

ItemLine::ItemLine(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

// copy constructor
ItemLine::ItemLine(const ItemLine &other)
    : ItemBase(other), pt1(other.pt1), pt2(other.pt2),
      lineWidth(other.lineWidth), lineType(other.lineType),
      lineColor(other.lineColor), lookupNdx(other.lookupNdx),
      pinNdx(other.pinNdx) {}

// clone method
ItemBasePtr ItemLine::clone() const {
  return std::make_shared<ItemLine>(*this);
}

void ItemLine::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 7) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1       = ArgPoint(strList[0]);
  pt2       = ArgPoint(strList[1]);
  lineWidth = ArgLineWidth(strList[2]);
  lineType  = ArgLineType(strList[3]);
  lineColor = ArgColor(strList[4]);
  lookupNdx = ArgLookupNdx(strList[5]);
  pinNdx    = ArgPinNdx(strList[6]);
}

std::string ItemLine::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() +
                    " " + lineWidth.toString() + " " + lineType.toString() +
                    " " + lineColor.toString() + " " + lookupNdx.toString() +
                    " " + pinNdx.toString();
  return str;
}

