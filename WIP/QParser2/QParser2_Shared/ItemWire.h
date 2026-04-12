//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemWire.h -- Wire item class.
 *
 * Expecting: "wire" (P1) (P2) P3
 * Example:   "wire (-1700,-300) (-1700,0) "Vin""
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 *   P3 -- Net/port name
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils.h"

class ItemWire : public ItemBase {
public:
  ItemWire() : ItemBase(QPI::WIRE) {}
  ItemWire(const ItemWire &other) = default;
  ItemWire(const ArgPoint &pt1, const ArgPoint &pt2, const ArgString &netName)
      : ItemBase(QPI::WIRE), pt1(pt1), pt2(pt2), netName(netName) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint  pt1;
  ArgPoint  pt2;
  ArgString netName;
};

typedef std::shared_ptr<ItemWire> ItemWirePtr;
