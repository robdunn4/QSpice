//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemTriangle.h -- Triangle item class.
 *
 * Expecting: "triangle" (P1) (P2) (P3) P4 P5 P6 P7 P8 P9
 * Example:   "triangle (0,0) (60,0) (30,0) 0 2 0xff0000 0x2000000 -1 -1"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Endpoint coordinate
 *   P4 -- Line width
 *   P5 -- Line type
 *   P6 -- Line color
 *   P7 -- Fill color
 *   P8 -- Lookup index
 *   P9 -- Pin index
 *
 * Construction: use ItemTriangle::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ArgUtils2.h"
#include "ItemBase.h"

class ItemTriangle;
typedef std::shared_ptr<ItemTriangle> ItemTrianglePtr;

class ItemTriangle : public ItemBaseT<ItemTriangle> {
public:
  static ItemTrianglePtr makePtr() {
    return ItemTrianglePtr(new ItemTriangle());
  }
  static ItemTrianglePtr
  makePtr(const ArgPoint &pt1, const ArgPoint &pt2, const ArgPoint &pt3,
          const ArgLineWidth &lineWidth, const ArgLineType &lineType,
          const ArgColor &lineColor, const ArgColor &fillColor,
          const ArgLookupNdx &lookupNdx, const ArgPinNdx &pinNdx) {
    return ItemTrianglePtr(new ItemTriangle(pt1, pt2, pt3, lineWidth, lineType,
                                            lineColor, fillColor, lookupNdx,
                                            pinNdx));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgPoint     pt3;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgColor     fillColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx    pinNdx;

protected:
  ItemTriangle() : ItemBaseT(QPI::TRIANGLE) {}
  ItemTriangle(const ItemTriangle &other) = default;
  ItemTriangle(const ArgPoint &pt1, const ArgPoint &pt2, const ArgPoint &pt3,
               const ArgLineWidth &lineWidth, const ArgLineType &lineType,
               const ArgColor &lineColor, const ArgColor &fillColor,
               const ArgLookupNdx &lookupNdx, const ArgPinNdx &pinNdx)
      : ItemBaseT(QPI::TRIANGLE), pt1(pt1), pt2(pt2), pt3(pt3),
        lineWidth(lineWidth), lineType(lineType), lineColor(lineColor),
        fillColor(fillColor), lookupNdx(lookupNdx), pinNdx(pinNdx) {}

  friend class ItemBaseT<ItemTriangle>;
};
