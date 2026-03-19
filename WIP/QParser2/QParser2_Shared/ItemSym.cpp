//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemSym.h"
#include "StrUtils.h"

ItemSym::ItemSym(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemSym::ItemSym(const ItemSym &other)
    : ItemBase(other), text(other.text) {}

ItemBasePtr ItemSym::clone() const {
  return std::make_shared<ItemSym>(*this);
}

void ItemSym::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);
  if (strList.size() > 1) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  if (strList.size()) text = ArgString(strList[0]);
}

std::string ItemSym::toString() const {
  std::string str = StrUtils::trim(typeStr + " " + text.toString());
  return str;
}

