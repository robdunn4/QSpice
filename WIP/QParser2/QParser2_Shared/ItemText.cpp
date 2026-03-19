//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemText.h"
#include "StrUtils.h"
#include <iostream>

ItemText::ItemText(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemText::ItemText(const ItemText &other)
    : ItemBase(other), pt1(other.pt1), fontSize(other.fontSize),
      rotateAlign(other.rotateAlign), textFlags(other.textFlags),
      textColor(other.textColor), lookupNdx(other.lookupNdx),
      pinNdx(other.pinNdx), text(other.text) {}

ItemBasePtr ItemText::clone() const {
  return std::make_shared<ItemText>(*this);
}

void ItemText::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 8) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1         = ArgPoint(strList[0]);
  fontSize    = ArgFontSize(strList[1]);
  rotateAlign = ArgRotAlign(strList[2]);
  textFlags   = ArgTextFlags(strList[3]);
  textColor   = ArgColor(strList[4]);
  lookupNdx   = ArgLookupNdx(strList[5]);
  pinNdx      = ArgPinNdx(strList[6]);
  text        = ArgString(strList[7]);

  // debugging/reverse-engineering... expecting only bits 0-1 used in p4
  // TODO:  Revisit...
  if (textFlags & ~0x03) {
    std::string str =
        "Unexpected content in parameter p4: " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  // reverse-engineering...
  if (pinNdx.getValue() != -1) {
    std::cout << "*** Parameter p7 has undecoded value (lookup index?): "
              << typeStr << " " << argStr << std::endl;
  }
}

std::string ItemText::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + fontSize.toString() +
                    " " + rotateAlign.toString() + " " + textFlags.toString() +
                    " " + textColor.toString() + " " + lookupNdx.toString() +
                    " " + pinNdx.toString() + " " + text.toString();
  return str;
}

