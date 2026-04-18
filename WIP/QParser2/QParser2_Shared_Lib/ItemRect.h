//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
// clang-format off
/*
 * ItemRect.h -- Rectangle item class.
 *
 * Expecting: "rect" (P1) (P2) P3 P4 P5 P6 P7 P8 P9 P10
 * Example:   "rect (-25,77) (25,73) 0 0 0 0x1000000 0x3000000 -1 0 -1"
 *
 * With graphic: "rect" (P1) (P2) P3 P4 P5 P6 P7 P8 P9 P10 P11
 * Example:      "rect (3018,68) (3082,132) 0 0 2 0xff0000 0x5000000 -1 0 -1
 *                89504e470d0a1a0a...0000014f0000"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Rotation
 *   P4 -- Line width
 *   P5 -- Line type
 *   P6 -- Line color
 *   P7 -- Fill color
 *   P8 -- Lookup index
 *   P9 -- Is 0 normally; 1 if hierarchical block???
 *   P10 -- Pin index
 *   P11 -- Appears to be used only for a graphic image, a string of hex values
 *          with no embedded spaces/newlines.  Not included if no graphic.  Given 
 *          that this may be a very long block of text without embedded spaces, it
 *          overrun default buffers....
 *
 * Construction: use ItemRect::makePtr(...).  Constructors are protected.
 */
// clang-format on
#pragma once
#include "ItemBase.h"
#include "QArgUtils2.h"

class ItemRect;
typedef std::shared_ptr<ItemRect> ItemRectPtr;

class ItemRect : public ItemBaseT<ItemRect> {
public:
  static ItemRectPtr makePtr() { return ItemRectPtr(new ItemRect()); }
  static ItemRectPtr
  makePtr(const ArgPoint &pt1, const ArgPoint &pt2, const ArgRot &rotate,
          const ArgLineWidth &lineWidth, const ArgLineType &lineType,
          const ArgColor &lineColor, const ArgColor &fillColor,
          const ArgLookupNdx &lookupNdx, const ArgInt &isHierarchicalBlock,
          const ArgPinNdx &pinNdx, const ArgImage &imageData = ArgImage()) {
    return ItemRectPtr(new ItemRect(pt1, pt2, rotate, lineWidth, lineType,
                                    lineColor, fillColor, lookupNdx,
                                    isHierarchicalBlock, pinNdx, imageData));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgRot       rotate;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgColor     fillColor;
  ArgLookupNdx lookupNdx;
  ArgInt       isHierarchicalBlock;
  ArgPinNdx    pinNdx;
  ArgImage     imageData; // image bitmap data

protected:
  ItemRect() : ItemBaseT(QPI::RECT) {}
  ItemRect(const ItemRect &other) = default;
  ItemRect(const ArgPoint &pt1, const ArgPoint &pt2, const ArgRot &rotate,
           const ArgLineWidth &lineWidth, const ArgLineType &lineType,
           const ArgColor &lineColor, const ArgColor &fillColor,
           const ArgLookupNdx &lookupNdx, const ArgInt &isHierarchicalBlock,
           const ArgPinNdx &pinNdx, const ArgImage &imageData)
      : ItemBaseT(QPI::RECT), pt1(pt1), pt2(pt2), rotate(rotate),
        lineWidth(lineWidth), lineType(lineType), lineColor(lineColor),
        fillColor(fillColor), lookupNdx(lookupNdx),
        isHierarchicalBlock(isHierarchicalBlock), pinNdx(pinNdx),
        imageData(imageData) {}

  friend class ItemBaseT<ItemRect>;
};
