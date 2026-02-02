//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QItemRect.h"
#include "StrUtils.h"

QItemRect::QItemRect(std::string typeStr, std::string argStr)
    : QItemBase(typeStr, argStr) {}

QItemRect::QItemRect(const QItemRect &other)
    : QItemBase(other), pt1(other.pt1), pt2(other.pt2), rotate(other.rotate), lineWidth(other.lineWidth),
      lineType(other.lineType), lineColor(other.lineColor), fillColor(other.fillColor), p8(other.p8), isHierarchicalBlock(other.isHierarchicalBlock),
      p10(other.p10), imageData(other.imageData) {}

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

  pt1 = ArgPoint(strList[0]);
  pt2 = ArgPoint(strList[1]);
  rotate = ArgRot(strList[2]);
  lineWidth = ArgLineWidth(strList[3]);
  lineType = ArgLineType(strList[4]);
  lineColor = ArgColor(strList[5]);
  fillColor = ArgColor(strList[6]);
  p8 = ArgInt(strList[7]);
  isHierarchicalBlock = ArgInt(strList[8]);
  p10 = ArgInt(strList[9]);
  if (strList.size() == 11) imageData = ArgImage(strList[10]);
}

std::string QItemRect::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() + " " +
                    rotate.toString() + " " + lineWidth.toString() + " " + lineType.toString() +
                    " " + lineColor.toString() + " " + fillColor.toString() + " " +
                    p8.toString() + " " + isHierarchicalBlock.toString() + " " + p10.toString();
  // if image data present...
  if (imageData.getValue().length()) str += " " + imageData.toString();
  return str;
}
