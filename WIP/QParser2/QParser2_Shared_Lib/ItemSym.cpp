//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemSym.h"
#include "StrUtil.h"

bool ItemSym::parseItem(const std::string &argStr) {
  StrList strList = StrUtil::tokenize(argStr);
  if (strList.size() > 1) return false;

  try {
    if (strList.size()) text = ArgString(strList[0]);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemSym::toString() const {
  std::string str = std::string(getTypeStr()) + " " + text.toString();
  return StrUtil::trim(str);
}
