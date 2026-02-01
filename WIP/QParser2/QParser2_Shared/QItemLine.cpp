#include "QItemLine.h"
#include "StrUtils.h"

QItemLine::QItemLine(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

// copy constructor
QItemLine::QItemLine(const QItemLine &other)
    : QItemBase(other), p1(other.p1), p2(other.p2), p3(other.p3), p4(other.p4),
      p5(other.p5), p6(other.p6), p7(other.p7) {}

// clone method
QItemBasePtr QItemLine::clone() const {
  return std::make_shared<QItemLine>(*this);
}

void QItemLine::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 7) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  p1 = ArgPoint(strList[0]);
  p2 = ArgPoint(strList[1]);
  p3 = ArgLineWidth(strList[2]);
  p4 = ArgLineType(strList[3]);
  p5 = ArgColor(strList[4]);
  p6 = ArgInt(strList[5]);
  p7 = ArgInt(strList[6]);
}

std::string QItemLine::toString() const {
  std::string str = typeStr + " " + p1.toString() + " " + p2.toString() + " " +
                    p3.toString() + " " + p4.toString() + " " + p5.toString() +
                    " " + p6.toString() + " " + p7.toString();
  return str;
}
