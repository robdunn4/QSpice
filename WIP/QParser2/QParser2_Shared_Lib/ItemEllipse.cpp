//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemEllipse.h"
#include "StrUtil.h"

bool ItemEllipse::parseItem(const std::string &argStr) {
  StrList strList = StrUtil::tokenize(argStr);

  if (strList.size() != 9) return false;

  try {
    pt1       = ArgPoint(strList[0]);
    pt2       = ArgPoint(strList[1]);
    rotate    = ArgRot(strList[2]);
    lineWidth = ArgLineWidth(strList[3]);
    lineType  = ArgLineType(strList[4]);
    lineColor = ArgColor(strList[5]);
    fillColor = ArgColor(strList[6]);
    lookupNdx = ArgLookupNdx(strList[7]);
    pinNdx    = ArgPinNdx(strList[8]);
  } catch (...) {
    return false;
  }
  return true;
}

std::string ItemEllipse::toString() const {
  return std::string(getTypeStr()) + " " + pt1.toString() + " " +
         pt2.toString() + " " + rotate.toString() + " " + lineWidth.toString() +
         " " + lineType.toString() + " " + lineColor.toString() + " " +
         fillColor.toString() + " " + lookupNdx.toString() + " " +
         pinNdx.toString();
}

// ── DrawIntf implementation ────────────────────────────────────────────────

Rect ItemEllipse::getRect() const noexcept {
  return Rect(pt1.getValue(), pt2.getValue());
}

Point ItemEllipse::getPosition() const noexcept { return pt1.getValue(); }

void ItemEllipse::moveTo(Point p) noexcept { moveBy(p - pt1.getValue()); }

void ItemEllipse::moveBy(Point delta) noexcept {
  pt1.setValue(pt1.getValue() + delta);
  pt2.setValue(pt2.getValue() + delta);
}
