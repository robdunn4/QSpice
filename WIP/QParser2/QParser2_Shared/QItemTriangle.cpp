//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemTriangle.h"
#include "StrUtils.h"

QItemTriangle::QItemTriangle(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemTriangle::QItemTriangle(const QItemTriangle &other)
    : QItemBase(other), pt1(other.pt1), pt2(other.pt2), pt3(other.pt3), lineWidth(other.lineWidth),
      lineType(other.lineType), lineColor(other.lineColor), fillColor(other.fillColor), p8(other.p8), p9(other.p9) {}

QItemBasePtr QItemTriangle::clone() const {
  return std::make_shared<QItemTriangle>(*this);
}

void QItemTriangle::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 9) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1 = ArgPoint(strList[0]);
  pt2 = ArgPoint(strList[1]);
  pt3 = ArgPoint(strList[2]);
  lineWidth = ArgLineWidth(strList[3]);
  lineType = ArgLineType(strList[4]);
  lineColor = ArgColor(strList[5]);
  fillColor = ArgColor(strList[6]);
  p8 = ArgInt(strList[7]);
  p9 = ArgInt(strList[8]);
}

std::string QItemTriangle::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() + " " +
                    pt3.toString() + " " + lineWidth.toString() + " " + lineType.toString() +
                    " " + lineColor.toString() + " " + fillColor.toString() + " " +
                    p8.toString() + " " + p9.toString();
  return str;
}
