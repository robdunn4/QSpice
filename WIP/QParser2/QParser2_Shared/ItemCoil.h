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
#include "QArgUtils2.h"
#include "ItemBase.h"

class ItemCoil : public ItemBase {
public:
  ItemCoil(std::string typeStr, std::string argStr);
  ItemCoil(const ItemCoil &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
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

