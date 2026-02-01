#include "QItemNet.h"
#include "StrUtils.h"

QItemNet::QItemNet(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemNet::QItemNet(const QItemNet &other)
    : QItemBase(other), p1(other.p1), p2(other.p2), p3(other.p3), p4(other.p4),
      p5(other.p5) {}

QItemBasePtr QItemNet::clone() const {
  return std::make_shared<QItemNet>(*this);
}

void QItemNet::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 5) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  p1 = ArgPoint(strList[0]);
  p2 = ArgFontSize(strList[1]);
  p3 = ArgRotAlign(strList[2]);
  p4 = ArgInt(strList[3]);
  p5 = strList[4];
  p6 = std::string();

  // reparse for net description if present
  size_t pos = strList[4].find_first_of(" ");
  if (pos != std::string::npos) {
    p5 = strList[4].substr(0, pos);
    p6 = strList[4].substr(pos + 1);
  }
}

std::string QItemNet::toString() const {
  std::string str = typeStr + " " + p1.toString() + " " + p2.toString() + " " +
                    p3.toString() + " " + p4.toString() + " " + p5.toString();
  if (p6.getValue().length()) str += " " + p6.toString();
  return str;
}
