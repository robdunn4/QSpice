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
 *
 * Construction: use ItemZigzag::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ArgUtils2.h"
#include "ItemBase.h"

class ItemZigzag;
typedef std::shared_ptr<ItemZigzag> ItemZigzagPtr;

class ItemZigzag : public ItemBaseT<ItemZigzag> {
public:
  static ItemZigzagPtr makePtr() { return ItemZigzagPtr(new ItemZigzag()); }
  static ItemZigzagPtr
  makePtr(const ArgPoint &pt1, const ArgPoint &pt2, const ArgRot &rotation,
          const ArgLineWidth &lineWidth, const ArgLineType &lineType,
          const ArgColor &lineColor, const ArgLookupNdx &lookupNdx,
          const ArgPinNdx &pinNdx) {
    return ItemZigzagPtr(new ItemZigzag(pt1, pt2, rotation, lineWidth, lineType,
                                        lineColor, lookupNdx, pinNdx));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgRot       rotation;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx    pinNdx;

protected:
  ItemZigzag() : ItemBaseT(QPI::ZIGZAG) {}
  ItemZigzag(const ItemZigzag &other) = default;
  ItemZigzag(const ArgPoint &pt1, const ArgPoint &pt2, const ArgRot &rotation,
             const ArgLineWidth &lineWidth, const ArgLineType &lineType,
             const ArgColor &lineColor, const ArgLookupNdx &lookupNdx,
             const ArgPinNdx &pinNdx)
      : ItemBaseT(QPI::ZIGZAG), pt1(pt1), pt2(pt2), rotation(rotation),
        lineWidth(lineWidth), lineType(lineType), lineColor(lineColor),
        lookupNdx(lookupNdx), pinNdx(pinNdx) {}

  friend class ItemBaseT<ItemZigzag>;
};
