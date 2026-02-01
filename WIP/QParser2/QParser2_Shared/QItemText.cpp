#include "QItemText.h"
#include "StrUtils.h"
#include <iostream>

QItemText::QItemText(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemText::QItemText(const QItemText &other)
    : QItemBase(other), p1(other.p1), p2(other.p2), p3(other.p3), p4(other.p4),
      p5(other.p5), p6(other.p6), p7(other.p7), p8(other.p8) {}

QItemBasePtr QItemText::clone() const {
  return std::make_shared<QItemText>(*this);
}

void QItemText::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 8) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  p1 = ArgPoint(strList[0]);
  p2 = ArgFontSize(strList[1]);
  p3 = ArgRotAlign(strList[2]);
  p4 = ArgInt(strList[3]);
  p5 = ArgColor(strList[4]);
  p6 = ArgInt(strList[5]);
  p7 = ArgInt(strList[6]);
  p8 = ArgString(strList[7]);

  // debugging/reverse-engineering... some GUI instances of text/comment have
  // zero in p3?
  // TODO:  Revisit...
  // if (!p3)
  //  std::cout << "*** Questionable content in parameter p3: " << typeStr
  // << " " << argStr << std::endl;

  // debugging/reverse-engineering... expecting only bits 0-1 used in p4
  // TODO:  Revisit...
  if (p4 & ~0x03) {
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
  std::string str = typeStr + " " + p1.toString() + " " + p2.toString() + " " +
                    p3.toString() + " " + p4.toString() + " " + p5.toString() +
                    " " + p6.toString() + " " + p7.toString() + " " +
                    p8.toString();
  return str;
}
