//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemDBG.h"
#include "iostream"

// this is a dummy class to use during development -- it does no actual parsing
// and throws no execptions

ItemDbg::ItemDbg(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemDbg::ItemDbg(const ItemDbg &other) : ItemBase(other) {}

ItemBasePtr ItemDbg::clone() const {
  return std::make_shared<ItemDbg>(*this);
}

void ItemDbg::parseItem() {
  // show error message for debugging
  std::cout << "*** Unhandled record type in ItemDBG: " << toString()
            << std::endl;
}

std::string ItemDbg::toString() const {
  std::string str = typeStr;
  if (argStr.length()) str += " " + argStr;
  return str;
}

