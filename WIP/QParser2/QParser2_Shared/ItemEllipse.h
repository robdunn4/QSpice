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
 */
#pragma once
#include "DrawIntf.h"
#include "QArgUtils2.h"
#include "ItemBase.h"

class ItemEllipse : public ItemBase, public DrawIntf {
public:
  ItemEllipse(std::string typeStr, std::string argStr);
  ItemEllipse(const ItemEllipse &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  // DrawIntf overrides
  [[nodiscard]] Rect  getBounds() const noexcept override;
  [[nodiscard]] Point getPosition() const noexcept override;
  void                setPosition(Point p) noexcept override;

  ArgPoint     pt1;
  ArgPoint     pt2;
  ArgRot       rotate;
  ArgLineWidth lineWidth;
  ArgLineType  lineType;
  ArgColor     lineColor;
  ArgColor     fillColor;
  ArgLookupNdx lookupNdx;
  ArgPinNdx    pinNdx;
};

typedef std::shared_ptr<ItemEllipse> ItemEllipsePtr;
