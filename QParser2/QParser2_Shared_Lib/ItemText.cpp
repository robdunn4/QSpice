//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemText.h"
#include "StrUtil.h"
#include <iostream>

bool ItemText::parseItem(const std::string &argStr) {
  StrList strList = StrUtil::tokenize(argStr);

  if (strList.size() != 8) return false;

  try {
    pt1         = ArgPoint(strList[0]);
    fontSize    = ArgFontSize(strList[1]);
    rotateAlign = ArgRotAlign(strList[2]);
    textFlags   = ArgTextFlags(strList[3]);
    textColor   = ArgColor(strList[4]);
    lookupNdx   = ArgLookupNdx(strList[5]);
    pinNdx      = ArgPinNdx(strList[6]);
    text        = ArgString(strList[7]);
  } catch (...) {
    return false;
  }

  // debugging/reverse-engineering... expecting only bits 0-1 used in p4
  // TODO:  Revisit...
  if (textFlags & ~0x03) return false;

  // reverse-engineering...
  if (pinNdx.getValue() != -1) {
    std::cout << "*** Parameter p7 has undecoded value (lookup index?): "
              << toString() << std::endl;
  }

  return true;
}

std::string ItemText::toString() const {
  return std::string(getTypeStr()) + " " + pt1.toString() + " " +
         fontSize.toString() + " " + rotateAlign.toString() + " " +
         textFlags.toString() + " " + textColor.toString() + " " +
         lookupNdx.toString() + " " + pinNdx.toString() + " " + text.toString();
}
