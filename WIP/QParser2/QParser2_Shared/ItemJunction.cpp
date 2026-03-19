//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemJunction.h"
#include "StrUtils.h"

ItemJunction::ItemJunction(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemJunction::ItemJunction(const ItemJunction &other)
    : ItemBase(other), pt1(other.pt1) {}

ItemBasePtr ItemJunction::clone() const {
  return std::make_shared<ItemJunction>(*this);
}

void ItemJunction::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 1) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1 = ArgPoint(strList[0]);
}

std::string ItemJunction::toString() const {
  std::string str = typeStr + " " + pt1.toString();
  return str;
}

