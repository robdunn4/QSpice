//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemSch.h"

ItemBasePtr ItemSch::clone() const { return std::make_shared<ItemSch>(*this); }

bool ItemSch::parseItem(const std::string &argStr) {
  // schematic line takes no arguments
  return argStr.empty();
}

std::string ItemSch::toString() const { return std::string(getTypeStr()); }
