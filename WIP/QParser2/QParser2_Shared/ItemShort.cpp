//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemShort.h"

ItemShort::ItemShort(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemShort::ItemShort(const ItemShort &other)
    : ItemBase(other), bShorted(other.bShorted) {}

ItemBasePtr ItemShort::clone() const {
  return std::make_shared<ItemShort>(*this);
}

void ItemShort::parseItem() { bShorted = ArgBool(argStr); }

std::string ItemShort::toString() const {
  return typeStr + " " + bShorted.toString();
}

