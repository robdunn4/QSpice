#include "QItemRect.h"
#include "StrUtils.h"

QItemRect::QItemRect(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemRect::QItemRect(const QItemRect &other)
    : QItemBase(other), p1(other.p1), p2(other.p2), p3(other.p3), p4(other.p4),
      p5(other.p5), p6(other.p6), p7(other.p7), p8(other.p8), p9(other.p9),
      p10(other.p10), p11(other.p11) {}

QItemBasePtr QItemRect::clone() const {
  return std::make_shared<QItemRect>(*this);
}

void QItemRect::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  // we expect eleven subStrings
  if (strList.size() < 10 || strList.size() > 11) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  p1 = ArgPoint(strList[0]);
  p2 = ArgPoint(strList[1]);
  p3 = ArgRot(strList[2]);
  p4 = ArgLineWidth(strList[3]);
  p5 = ArgLineType(strList[4]);
  p6 = ArgColor(strList[5]);
  p7 = ArgColor(strList[6]);
  p8 = ArgInt(strList[7]);
  p9 = ArgInt(strList[8]);
  p10 = ArgInt(strList[9]);
  if (strList.size() == 11) p11 = ArgImage(strList[10]);
}

std::string QItemRect::toString() const {
  std::string str = typeStr + " " + p1.toString() + " " + p2.toString() + " " +
                    p3.toString() + " " + p4.toString() + " " + p5.toString() +
                    " " + p6.toString() + " " + p7.toString() + " " +
                    p8.toString() + " " + p9.toString() + " " + p10.toString();
  // if image data present...
  if (p11.getValue().length()) str += " " + p11.toString();
  return str;
}
