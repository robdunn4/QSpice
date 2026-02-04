//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemLine.h"
#include "StrUtils.h"

QItemLine::QItemLine(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

// copy constructor
QItemLine::QItemLine(const QItemLine &other)
    : QItemBase(other), pt1(other.pt1), pt2(other.pt2),
      lineWidth(other.lineWidth), lineType(other.lineType),
      lineColor(other.lineColor), p6(other.p6), p7(other.p7) {}

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

  pt1 = ArgPoint(strList[0]);
  pt2 = ArgPoint(strList[1]);
  lineWidth = ArgLineWidth(strList[2]);
  lineType = ArgLineType(strList[3]);
  lineColor = ArgColor(strList[4]);
  p6 = ArgLookupNdx(strList[5]);
  p7 = ArgPinNdx(strList[6]);
}

std::string QItemLine::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() +
                    " " + lineWidth.toString() + " " + lineType.toString() +
                    " " + lineColor.toString() + " " + p6.toString() + " " +
                    p7.toString();
  return str;
}
