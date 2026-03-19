//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemEllipse.h"
#include "StrUtils.h"
#include <algorithm> // for std::min, std::max

ItemEllipse::ItemEllipse(std::string typeStr, std::string argStr)
    : ItemBase(typeStr, argStr) {}

ItemEllipse::ItemEllipse(const ItemEllipse &other)
    : ItemBase(other), DrawIntf(other), pt1(other.pt1), pt2(other.pt2),
      rotate(other.rotate), lineWidth(other.lineWidth),
      lineType(other.lineType), lineColor(other.lineColor),
      fillColor(other.fillColor), lookupNdx(other.lookupNdx),
      pinNdx(other.pinNdx) {}

ItemBasePtr ItemEllipse::clone() const {
  return std::make_shared<ItemEllipse>(*this);
}

void ItemEllipse::parseItem() {
  StrUtils::StrList strList = StrUtils::tokenize(argStr);

  if (strList.size() != 9) {
    std::string str = "Unexpected content in " + typeStr + " " + argStr;
    throw std::invalid_argument(str);
  }

  pt1       = ArgPoint(strList[0]);
  pt2       = ArgPoint(strList[1]);
  rotate    = ArgRot(strList[2]);
  lineWidth = ArgLineWidth(strList[3]);
  lineType  = ArgLineType(strList[4]);
  lineColor = ArgColor(strList[5]);
  fillColor = ArgColor(strList[6]);
  lookupNdx = ArgLookupNdx(strList[7]);
  pinNdx    = ArgPinNdx(strList[8]);
}

std::string ItemEllipse::toString() const {
  std::string str = typeStr + " " + pt1.toString() + " " + pt2.toString() +
                    " " + rotate.toString() + " " + lineWidth.toString() + " " +
                    lineType.toString() + " " + lineColor.toString() + " " +
                    fillColor.toString() + " " + lookupNdx.toString() + " " +
                    pinNdx.toString();
  return str;
}

// ── DrawIntf implementation
// ───────────────────────────────────────────────────

Rect ItemEllipse::getBounds() const noexcept {
  Point p1 = pt1.getValue();
  Point p2 = pt2.getValue();
  return Rect(std::min(p1.x, p2.x), std::min(p1.y, p2.y), std::max(p1.x, p2.x),
              std::max(p1.y, p2.y));
}

Point ItemEllipse::getPosition() const noexcept { return pt1.getValue(); }

void ItemEllipse::setPosition(Point p) noexcept {
  Point delta = p - pt1.getValue();
  pt1.setValue(p);
  pt2.setValue(pt2.getValue() + delta); // preserve shape size
}
