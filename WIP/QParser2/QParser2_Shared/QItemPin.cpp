#include "QItemPin.h"
#include "StrUtils.h"

QItemPin::QItemPin(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemPin::QItemPin(const QItemPin &other)
    : QItemBase(other), p1(other.p1), p2(other.p2), p3(other.p3), p4(other.p4),
      p5(other.p5), p6(other.p6), p7(other.p7), p8(other.p8) {}

QItemBasePtr QItemPin::clone() const {
  return std::make_shared<QItemPin>(*this);
}

void QItemPin::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  // we expect three subStrings
  if (strList.size() < 8 || strList.size() > 9) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  p1 = ArgPoint(strList[0]);
  p2 = ArgPoint(strList[1]);
  p3 = ArgFontSize(strList[2]);
  p4 = ArgRotAlign(strList[3]);
  p5 = ArgPinInfo(strList[4]);
  p6 = ArgColor(strList[5]);
  p7 = ArgInt(strList[6]);
  p8 = ArgString(strList[7]);
  p9 = strList.size() < 9 ? std::string() : strList[8];
}

std::string QItemPin::toString() const {
  std::string str = typeStr + " " + p1.toString() + " " + p2.toString() + " " +
                    p3.toString() + " " + p4.toString() + " " + p5.toString() +
                    " " + p6.toString() + " " + p7.toString() + " " +
                    p8.toString();
  if (p9.getValue().length()) str += " " + p9.toString();
  return str;
}
