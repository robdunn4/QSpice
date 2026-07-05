//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemEllipse.h -- Ellipse item class.
 *
 * Expecting: "ellipse" (P1) (P2) P3 P4 P5 P6 P7 P8 P9
 * Example:   "ellipse (-130,130) (130,-130) 0 0 0 0x1000000 0x1000000 -1 -1"
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
 *   P9 -- Pin index
 *
 * Construction: use ItemEllipse::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ArgUtils2.h"
#include "DrawIntf.h"
#include "ItemBase.h"

class ItemEllipse;
typedef std::shared_ptr<ItemEllipse> ItemEllipsePtr;

class ItemEllipse : public ItemBaseT<ItemEllipse>, public DrawIntf {
public:
  static ItemEllipsePtr makePtr() { return ItemEllipsePtr(new ItemEllipse()); }
  static ItemEllipsePtr
  makePtr(const ArgPoint &pt1, const ArgPoint &pt2, const ArgRot &rotate,
          const ArgLineWidth &lineWidth, const ArgLineType &lineType,
          const ArgColor &lineColor, const ArgColor &fillColor,
          const ArgLookupNdx &lookupNdx, const ArgPinNdx &pinNdx) {
    return ItemEllipsePtr(new ItemEllipse(pt1, pt2, rotate, lineWidth, lineType,
                                          lineColor, fillColor, lookupNdx,
                                          pinNdx));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  // DrawIntf overrides
  [[nodiscard]] Rect  getRect() const noexcept override;
  [[nodiscard]] Point getPosition() const noexcept override;
  void                moveTo(Point p) noexcept override;
  void                moveBy(Point p) noexcept override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgRot       rotate;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgColor     fillColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx    pinNdx;

protected:
  ItemEllipse() : ItemBaseT(QPI::ELLIPSE) {}
  ItemEllipse(const ItemEllipse &other) = default;
  ItemEllipse(const ArgPoint &pt1, const ArgPoint &pt2, const ArgRot &rotate,
              const ArgLineWidth &lineWidth, const ArgLineType &lineType,
              const ArgColor &lineColor, const ArgColor &fillColor,
              const ArgLookupNdx &lookupNdx, const ArgPinNdx &pinNdx)
      : ItemBaseT(QPI::ELLIPSE), pt1(pt1), pt2(pt2), rotate(rotate),
        lineWidth(lineWidth), lineType(lineType), lineColor(lineColor),
        fillColor(fillColor), lookupNdx(lookupNdx), pinNdx(pinNdx) {}

  friend class ItemBaseT<ItemEllipse>;
};
