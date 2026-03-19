//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemCmp.h"
#include "StrUtils.h"

ItemCmp::ItemCmp(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

// copy constructor
ItemCmp::ItemCmp(const ItemCmp &other)
    : ItemBase(other), pt1(other.pt1), rotate(other.rotate),
      stuffShort(other.stuffShort) {}

// clone method
ItemBasePtr ItemCmp::clone() const {
  return std::make_shared<ItemCmp>(*this);
}
void ItemCmp::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 3) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1        = ArgPoint(strList[0]);
  rotate     = ArgRot(strList[1]);
  stuffShort = ArgStuff(strList[2]);
}

std::string ItemCmp::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + rotate.toString() +
                    " " + stuffShort.toString();
  return str;
}

