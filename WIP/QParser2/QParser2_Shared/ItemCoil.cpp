//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemCoil.h"
#include "StrUtils.h"

ItemCoil::ItemCoil(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

// copy constructor
ItemCoil::ItemCoil(const ItemCoil &other)
    : ItemBase(other), pt1(other.pt1), pt2(other.pt2), rotate(other.rotate),
      lineWidth(other.lineWidth), lineType(other.lineType),
      lineColor(other.lineColor), lookupNdx(other.lookupNdx),
      pinNdx(other.pinNdx) {}

// clone method
ItemBasePtr ItemCoil::clone() const {
  return std::make_shared<ItemCoil>(*this);
}

void ItemCoil::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 8) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1       = ArgPoint(strList[0]);
  pt2       = ArgPoint(strList[1]);
  rotate    = ArgRot(strList[2]);
  lineWidth = ArgLineWidth(strList[3]);
  lineType  = ArgLineType(strList[4]);
  lineColor = ArgColor(strList[5]);
  lookupNdx = ArgLookupNdx(strList[6]);
  pinNdx    = ArgPinNdx(strList[7]);
}

std::string ItemCoil::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() +
                    " " + rotate.toString() + " " + lineWidth.toString() + " " +
                    lineType.toString() + " " + lineColor.toString() + " " +
                    lookupNdx.toString() + " " + pinNdx.toString();
  return str;
}

