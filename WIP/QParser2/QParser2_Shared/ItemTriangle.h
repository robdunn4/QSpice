//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemTriangle.h -- Triangle item class.
 *
 * Expecting: "arc3p" (P1) (P2) (P3) P4 P5 P6 P7 P8 P9
 * Example:   "triangle (0,0) (60,0) (30,0) 0 2 0xff0000 0x2000000 -1 -1"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Endpoint coordinate
 *   P4 -- Line widtth
 *   P5 -- Line type
 *   P6 -- Line color
 *   P7 -- Fill color
 *   P8 -- Lookup index
 *   P9 -- Pin index
 */

#pragma once
#include "QArgUtils2.h"
#include "ItemBase.h"

class ItemTriangle : public ItemBase {
public:
  ItemTriangle(std::string typeStr, std::string argStr);
  ItemTriangle(const ItemTriangle &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
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
};

typedef std::shared_ptr<ItemTriangle> ItemTrianglePtr;
