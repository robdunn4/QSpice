//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemLib.h"
#include "StrUtils.h"

ItemLib::ItemLib(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemLib::ItemLib(const ItemLib &other)
    : ItemBase(other), libText(other.libText) {}

ItemBasePtr ItemLib::clone() const {
  return std::make_shared<ItemLib>(*this);
}

void ItemLib::parseItem() { libText = ArgString(argStr); }

std::string ItemLib::toString() const {
  std::string str = typeStr + " " + libText.toString();
  StrUtils::trim(str);
  return str;
}

