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
 *
 * Construction: use ItemWire::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ItemBase.h"
#include "ArgUtils.h"

class ItemWire;
typedef std::shared_ptr<ItemWire> ItemWirePtr;

class ItemWire : public ItemBaseT<ItemWire> {
public:
  static ItemWirePtr makePtr() { return ItemWirePtr(new ItemWire()); }
  static ItemWirePtr makePtr(const ArgPoint &pt1, const ArgPoint &pt2,
                             const ArgString &netName) {
    return ItemWirePtr(new ItemWire(pt1, pt2, netName));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgPoint  pt1;
  ArgPoint  pt2;
  ArgString netName;

protected:
  ItemWire() : ItemBaseT(QPI::WIRE) {}
  ItemWire(const ItemWire &other) = default;
  ItemWire(const ArgPoint &pt1, const ArgPoint &pt2, const ArgString &netName)
      : ItemBaseT(QPI::WIRE), pt1(pt1), pt2(pt2), netName(netName.quote()) {}

  friend class ItemBaseT<ItemWire>;
};
