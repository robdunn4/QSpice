//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemLine.h -- Line item class.
 *
 * Expecting: "line" (P1) (P2) P3 P4 P5 P6 P7
 * Example:   "line (0,-130) (0,-200) 0 0 0x1000000 -1 -1"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Line width
 *   P4 -- Line type
 *   P5 -- Line color
 *   P6 -- Lookup index
 *   P7 -- Pin index
 *
 * Construction: use ItemLine::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils2.h"

class ItemLine;
typedef std::shared_ptr<ItemLine> ItemLinePtr;

class ItemLine : public ItemBaseT<ItemLine> {
public:
  static ItemLinePtr makePtr() {
    return ItemLinePtr(new ItemLine());
  }
  static ItemLinePtr
  makePtr(const ArgPoint &pt1, const ArgPoint &pt2,
          const ArgLineWidth &lineWidth, const ArgLineType &lineType,
          const ArgColor &lineColor, const ArgLookupNdx &lookupNdx,
          const ArgPinNdx &pinNdx) {
    return ItemLinePtr(new ItemLine(pt1, pt2, lineWidth, lineType, lineColor,
                                    lookupNdx, pinNdx));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx    pinNdx;

protected:
  ItemLine() : ItemBaseT(QPI::LINE) {}
  ItemLine(const ItemLine &other) = default;
  ItemLine(const ArgPoint &pt1, const ArgPoint &pt2,
           const ArgLineWidth &lineWidth, const ArgLineType &lineType,
           const ArgColor &lineColor, const ArgLookupNdx &lookupNdx,
           const ArgPinNdx &pinNdx)
      : ItemBaseT(QPI::LINE), pt1(pt1), pt2(pt2), lineWidth(lineWidth),
        lineType(lineType), lineColor(lineColor), lookupNdx(lookupNdx),
        pinNdx(pinNdx) {}

  friend class ItemBaseT<ItemLine>;
};
