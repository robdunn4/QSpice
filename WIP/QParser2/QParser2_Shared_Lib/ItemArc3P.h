//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemArc3P.h -- Arc3P item class.
 *
 * Expecting: "arc3p" (P1) (P2) (P3) P4 P5 P6 P7 P8
 * Example:   "arc3p (0,0) (60,0) (30,0) 0 0 0x1000000 -1 -1"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Endpoint coordinate
 *   P4 -- Line width
 *   P5 -- Line type
 *   P6 -- Line color
 *   P7 -- Lookup index
 *   P8 -- Pin index
 *
 * Construction: use ItemArc::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils2.h"

class ItemArc;
typedef std::shared_ptr<ItemArc> ItemArcPtr;

class ItemArc : public ItemBaseT<ItemArc> {
public:
  static ItemArcPtr makePtr() {
    return ItemArcPtr(new ItemArc());
  }
  static ItemArcPtr
  makePtr(const ArgPoint &pt1, const ArgPoint &pt2, const ArgPoint &pt3,
          const ArgLineWidth &lineWidth, const ArgLineType &lineType,
          const ArgColor &lineColor, const ArgLookupNdx &lookupNdx,
          const ArgPinNdx &pinNdx) {
    return ItemArcPtr(new ItemArc(pt1, pt2, pt3, lineWidth, lineType,
                                  lineColor, lookupNdx, pinNdx));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgPoint     pt3;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx    pinNdx;

protected:
  ItemArc() : ItemBaseT(QPI::ARC3P) {}
  ItemArc(const ItemArc &other) = default;
  ItemArc(const ArgPoint &pt1, const ArgPoint &pt2, const ArgPoint &pt3,
          const ArgLineWidth &lineWidth, const ArgLineType &lineType,
          const ArgColor &lineColor, const ArgLookupNdx &lookupNdx,
          const ArgPinNdx &pinNdx)
      : ItemBaseT(QPI::ARC3P), pt1(pt1), pt2(pt2), pt3(pt3),
        lineWidth(lineWidth), lineType(lineType), lineColor(lineColor),
        lookupNdx(lookupNdx), pinNdx(pinNdx) {}

  friend class ItemBaseT<ItemArc>;
};
