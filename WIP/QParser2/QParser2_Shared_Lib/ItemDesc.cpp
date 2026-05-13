//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemDesc.h"
#include "StrUtil.h"

bool ItemDesc::parseItem(const std::string &argStr) {
  try {
    text = ArgString(argStr);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemDesc::toString() const {
  std::string str = std::string(getTypeStr()) + " " + text.toString();
  StrUtil::trim(str);
  return str;
}
