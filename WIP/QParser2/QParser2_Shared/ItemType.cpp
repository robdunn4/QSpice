//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemType.h"
#include "StrUtils.h"

ItemType::ItemType(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemType::ItemType(const ItemType &other)
    : ItemBase(other), text(other.text) {}

ItemBasePtr ItemType::clone() const {
  return std::make_shared<ItemType>(*this);
}

void ItemType::parseItem() { text = ArgString(argStr); }

std::string ItemType::toString() const {
  std::string str = typeStr + " " + text.toString();
  StrUtils::trim(str);
  return str;
}

