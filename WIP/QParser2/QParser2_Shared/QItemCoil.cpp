//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemCoil.h"
#include "StrUtils.h"

QItemCoil::QItemCoil(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

// copy constructor
QItemCoil::QItemCoil(const QItemCoil &other)
    : QItemBase(other), pt1(other.pt1), pt2(other.pt2), rotate(other.rotate),
      lineWidth(other.lineWidth), lineType(other.lineType),
      lineColor(other.lineColor), p7(other.p7), p8(other.p8) {}

// clone method
QItemBasePtr QItemCoil::clone() const {
  return std::make_shared<QItemCoil>(*this);
}

void QItemCoil::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 8) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1 = ArgPoint(strList[0]);
  pt2 = ArgPoint(strList[1]);
  rotate = ArgRot(strList[2]);
  lineWidth = ArgLineWidth(strList[3]);
  lineType = ArgLineType(strList[4]);
  lineColor = ArgColor(strList[5]);
  p7 = ArgLookupNdx(strList[6]);
  p8 = ArgPinNdx(strList[7]);
}

std::string QItemCoil::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() +
                    " " + rotate.toString() + " " + lineWidth.toString() + " " +
                    lineType.toString() + " " + lineColor.toString() + " " +
                    p7.toString() + " " + p8.toString();
  return str;
}
