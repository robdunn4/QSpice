//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemText.h"
#include "StrUtils.h"
#include <iostream>

QItemText::QItemText(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemText::QItemText(const QItemText &other)
    : QItemBase(other), pt1(other.pt1), fontSize(other.fontSize), rotateAlign(other.rotateAlign), commentVisible(other.commentVisible),
      textColor(other.textColor), p6(other.p6), p7(other.p7), text(other.text) {}

QItemBasePtr QItemText::clone() const {
  return std::make_shared<QItemText>(*this);
}

void QItemText::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 8) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1 = ArgPoint(strList[0]);
  fontSize = ArgFontSize(strList[1]);
  rotateAlign = ArgRotAlign(strList[2]);
  commentVisible = ArgInt(strList[3]);
  textColor = ArgColor(strList[4]);
  p6 = ArgInt(strList[5]);
  p7 = ArgInt(strList[6]);
  text = ArgString(strList[7]);

  // debugging/reverse-engineering... expecting only bits 0-1 used in p4
  // TODO:  Revisit...
  if (commentVisible & ~0x03) {
    std::string str =
        "Unexpected content in parameter p4: " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  // reverse-engineering...
  if (p7.getValue() != -1) {
    std::cout << "*** Parameter p7 has undecoded value (lookup index?): "
              << typeStr << " " << argStr << std::endl;
  }
}

std::string QItemText::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + fontSize.toString() + " " +
                    rotateAlign.toString() + " " + commentVisible.toString() + " " + textColor.toString() +
                    " " + p6.toString() + " " + p7.toString() + " " +
                    text.toString();
  return str;
}
