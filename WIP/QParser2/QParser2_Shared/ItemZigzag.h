//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemZigzag.h -- Zigzag item class.
 *
 * Expecting: "zigzag" (P1) (P2) P3 P4 P5 P6 P7 P8
 * Example:   "zigzag (-80,180) (80,-180) 0 0 0 0x1000000 -1 -1"
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

class ItemZigzag : public ItemBase {
public:
  ItemZigzag(std::string typeStr, std::string argStr);
  ItemZigzag(const ItemZigzag &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgRot       rotation;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx    pinNdx;
};

typedef std::shared_ptr<ItemZigzag> ItemZigzagPtr;
