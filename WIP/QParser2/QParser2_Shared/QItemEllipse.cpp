//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemEllipse.h"
#include "StrUtils.h"

QItemEllipse::QItemEllipse(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemEllipse::QItemEllipse(const QItemEllipse &other)
    : QItemBase(other), pt1(other.pt1), pt2(other.pt2), rotate(other.rotate),
      lineWidth(other.lineWidth), lineType(other.lineType),
      lineColor(other.lineColor), fillColor(other.fillColor), p8(other.p8),
      p9(other.p9) {}

QItemBasePtr QItemEllipse::clone() const {
  return std::make_shared<QItemEllipse>(*this);
}

void QItemEllipse::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 9) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1 = ArgPoint(strList[0]);
  pt2 = ArgPoint(strList[1]);
  rotate = ArgRot(strList[2]);
  lineWidth = ArgLineWidth(strList[3]);
  lineType = ArgLineType(strList[4]);
  lineColor = ArgColor(strList[5]);
  fillColor = ArgColor(strList[6]);
  p8 = ArgLookupNdx(strList[7]);
  p9 = ArgPinNdx(strList[8]);
}

std::string QItemEllipse::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() +
                    " " + rotate.toString() + " " + lineWidth.toString() + " " +
                    lineType.toString() + " " + lineColor.toString() + " " +
                    fillColor.toString() + " " + p8.toString() + " " +
                    p9.toString();
  return str;
}
