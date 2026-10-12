//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemArc4P.h -- Arc4P item class.
 *
 * Expecting: "arc" (P1) (P2) (P3) (P4) P5 P6 P7 P8 P9
 * Example:
 "   "arc (100,-100) (-100,-50) (-100,-75) (100,-75) 0 0 0 0x1000000 -1 -1"
 *
 * Analysis:
 *   P1 -- Coordinate
 *   P2 -- Coordinate
 *   P3 -- Coordinate
 *   P4 -- Coordinate
 *   P5 -- Rotation
 *   P6 -- Line width
 *   P7 -- Line type
 *   P8 -- Line color
 *   P9 -- Lookup index
 *   P10 -- Pin index
 *
 * Construction: use ItemArc4P::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ArgUtils2.h"
#include "ItemBase.h"

class ItemArc4P;
typedef std::shared_ptr<ItemArc4P> ItemArc4pPtr;

class ItemArc4P : public ItemBaseT<ItemArc4P> {
public:
  static ItemArc4pPtr makePtr() { return ItemArc4pPtr(new ItemArc4P()); }
  static ItemArc4pPtr
  makePtr(const ArgPoint &pt1, const ArgPoint &pt2, const ArgPoint &pt3,
          const ArgPoint &pt4, const ArgRot &rotate,
          const ArgLineWidth &lineWidth, const ArgLineType &lineType,
          const ArgColor &lineColor, const ArgLookupNdx &lookupNdx,
          const ArgPinNdx &pinNdx) {
    return ItemArc4pPtr(new ItemArc4P(pt1, pt2, pt3, pt4, rotate, lineWidth,
                                      lineType, lineColor, lookupNdx, pinNdx));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgPoint     pt3;
  ArgPoint     pt4;
  ArgRot       rotate;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx    pinNdx;

protected:
  ItemArc4P() : ItemBaseT(QPI::ARC4P) {}
  ItemArc4P(const ItemArc4P &other) = default;
  ItemArc4P(const ArgPoint &pt1, const ArgPoint &pt2, const ArgPoint &pt3,
            const ArgPoint &pt4, const ArgRot &rotate,
            const ArgLineWidth &lineWidth, const ArgLineType &lineType,
            const ArgColor &lineColor, const ArgLookupNdx &lookupNdx,
            const ArgPinNdx &pinNdx)
      : ItemBaseT(QPI::ARC4P), pt1(pt1), pt2(pt2), pt3(pt3), pt4(pt4),
        rotate(rotate), lineWidth(lineWidth), lineType(lineType),
        lineColor(lineColor), lookupNdx(lookupNdx), pinNdx(pinNdx) {}

  friend class ItemBaseT<ItemArc4P>;
};
