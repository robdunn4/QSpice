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
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils2.h"

class ItemLine : public ItemBase {
public:
  ItemLine() : ItemBase(QPI::LINE) {}
  ItemLine(const ItemLine &other) = default;

  // Construct with explicit member values
  ItemLine(const ArgPoint &pt1, const ArgPoint &pt2,
           const ArgLineWidth &lineWidth, const ArgLineType &lineType,
           const ArgColor &lineColor, const ArgLookupNdx &lookupNdx,
           const ArgPinNdx &pinNdx)
      : ItemBase(QPI::LINE), pt1(pt1), pt2(pt2), lineWidth(lineWidth),
        lineType(lineType), lineColor(lineColor), lookupNdx(lookupNdx),
        pinNdx(pinNdx) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx    pinNdx;
};

typedef std::shared_ptr<ItemLine> ItemLinePtr;
