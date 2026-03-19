//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemTap.h"
#include "StrUtils.h"

ItemTap::ItemTap(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemTap::ItemTap(const ItemTap &other)
    : ItemBase(other), pt1(other.pt1), pt2(other.pt2) {}

ItemBasePtr ItemTap::clone() const {
  return std::make_shared<ItemTap>(*this);
}

void ItemTap::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 2) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1 = ArgPoint(strList[0]);
  pt2 = ArgPoint(strList[1]);
}

std::string ItemTap::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString();
  return str;
}

