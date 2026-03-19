//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemDesc.h"
#include "StrUtils.h"

ItemDesc::ItemDesc(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemDesc::ItemDesc(const ItemDesc &other)
    : ItemBase(other), text(other.text) {}

ItemBasePtr ItemDesc::clone() const {
  return std::make_shared<ItemDesc>(*this);
}

void ItemDesc::parseItem() { text = ArgString(argStr); }

std::string ItemDesc::toString() const {
  std::string str = typeStr + " " + text.toString();
  StrUtils::trim(str);
  return str;
}

