//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemSch.h"

ItemSch::ItemSch(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemSch::ItemSch(const ItemSch &other) : ItemBase(other) {}

ItemBasePtr ItemSch::clone() const {
  return std::make_shared<ItemSch>(*this);
}

void ItemSch::parseItem() {
  if (!argStr.empty()) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }
}

std::string ItemSch::toString() const { return typeStr; }

