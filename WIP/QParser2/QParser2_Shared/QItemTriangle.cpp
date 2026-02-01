#include "QItemTriangle.h"
#include "StrUtils.h"

QItemTriangle::QItemTriangle(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemTriangle::QItemTriangle(const QItemTriangle &other)
    : QItemBase(other), p1(other.p1), p2(other.p2), p3(other.p3), p4(other.p4),
      p5(other.p5), p6(other.p6), p7(other.p7), p8(other.p8), p9(other.p9) {}

QItemBasePtr QItemTriangle::clone() const {
  return std::make_shared<QItemTriangle>(*this);
}

void QItemTriangle::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 9) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  p1 = ArgPoint(strList[0]);
  p2 = ArgPoint(strList[1]);
  p3 = ArgPoint(strList[2]);
  p4 = ArgLineWidth(strList[3]);
  p5 = ArgLineType(strList[4]);
  p6 = ArgColor(strList[5]);
  p7 = ArgColor(strList[6]);
  p8 = ArgInt(strList[7]);
  p9 = ArgInt(strList[8]);
}

std::string QItemTriangle::toString() const {
  std::string str = typeStr + " " + p1.toString() + " " + p2.toString() + " " +
                    p3.toString() + " " + p4.toString() + " " + p5.toString() +
                    " " + p6.toString() + " " + p7.toString() + " " +
                    p8.toString() + " " + p9.toString();
  return str;
}
