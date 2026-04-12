//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemCoil.h -- Coil item class (seems identical to Zigzagitem class).
 *
 * Expecting: "coil" (P1) (P2) P3 P4 P5 P6 P7 P8
 * Example:   "coil (450,-1400) (1100,-1700) 0 0 2 0xff0000 -1 -1"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Rotation
 *   P4 -- Line width
 *   P5 -- Line type
 *   P6 -- Line color
 *   P7 -- Lookup index
 *   P8 -- Pin index
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils2.h"

class ItemCoil : public ItemBase {
public:
  ItemCoil() : ItemBase(QPI::COIL) {}
  ItemCoil(const ItemCoil &other) = default;

  // Construct with explicit member values
  ItemCoil(const ArgPoint &pt1, const ArgPoint &pt2, const ArgRot &rotate,
           const ArgLineWidth &lineWidth, const ArgLineType &lineType,
           const ArgColor &lineColor, const ArgLookupNdx &lookupNdx,
           const ArgPinNdx &pinNdx)
      : ItemBase(QPI::COIL), pt1(pt1), pt2(pt2), rotate(rotate),
        lineWidth(lineWidth), lineType(lineType), lineColor(lineColor),
        lookupNdx(lookupNdx), pinNdx(pinNdx) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgRot       rotate;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx    pinNdx;
};

typedef std::shared_ptr<ItemCoil> ItemCoilPtr;
