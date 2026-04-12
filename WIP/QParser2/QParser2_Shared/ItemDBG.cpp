//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemDBG.h"
#include <iostream>

// this is a dummy class to use during development -- it does no actual parsing
// and returns true so parsing proceeds past unknown lines.

ItemBasePtr ItemDbg::clone() const { return std::make_shared<ItemDbg>(*this); }

bool ItemDbg::parseItem(const std::string &argStr) {
  this->argStr = argStr;
  std::cout << "*** Unhandled record type in ItemDBG: " << toString()
            << std::endl;
  return true;
}

std::string ItemDbg::toString() const {
  std::string str = "dbg";
  if (!argStr.empty()) str += " " + argStr;
  return str;
}
