//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemWire.h"
#include "StrUtils.h"

ItemWire::ItemWire(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemWire::ItemWire(const ItemWire &other)
    : ItemBase(other), pt1(other.pt1), pt2(other.pt2), netName(other.netName) {
}

ItemBasePtr ItemWire::clone() const {
  return std::make_shared<ItemWire>(*this);
}

void ItemWire::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 3) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1     = ArgPoint(strList[0]);
  pt2     = ArgPoint(strList[1]);
  netName = ArgString(strList[2]);
}

std::string ItemWire::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() +
                    " " + netName.toString();
  return str;
}

