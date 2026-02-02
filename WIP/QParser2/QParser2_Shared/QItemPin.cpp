//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemPin.h"
#include "StrUtils.h"

QItemPin::QItemPin(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemPin::QItemPin(const QItemPin &other)
    : QItemBase(other), pt1(other.pt1), pt2(other.pt2), fontSize(other.fontSize), rotateAlign(other.rotateAlign),
      pinInfo(other.pinInfo), textColor(other.textColor), p7(other.p7), pinLabel(other.pinLabel) {}

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

  pt1 = ArgPoint(strList[0]);
  pt2 = ArgPoint(strList[1]);
  fontSize = ArgFontSize(strList[2]);
  rotateAlign = ArgRotAlign(strList[3]);
  pinInfo = ArgPinInfo(strList[4]);
  textColor = ArgColor(strList[5]);
  p7 = ArgInt(strList[6]);
  pinLabel = ArgString(strList[7]);
  netName = strList.size() < 9 ? std::string() : strList[8];
}

std::string QItemPin::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() + " " +
                    fontSize.toString() + " " + rotateAlign.toString() + " " + pinInfo.toString() +
                    " " + textColor.toString() + " " + p7.toString() + " " +
                    pinLabel.toString();
  if (netName.getValue().length()) str += " " + netName.toString();
  return str;
}
