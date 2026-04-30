//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemType.h -- Type item class.
 *
 * Expecting: "type:" P1
 * Example:   "type: V" -- a voltage source
 *            "type: "   -- a hierarchical block
 *
 * Analysis:
 *   P1 --  Type string.  (Optional, can be empty.  See note below.)
 *
 * Note:  The type record is optional.  If missing, the type is a hierarchical
 *        schematic block.  It can also be present with no value (not a blank,
 *        simply empty) which can happen after editing the symbol type in Symbol
 *        Properties.
 *
 * Construction: use ItemType::makePtr(...).  Constructors are protected.
 */
#pragma once
#include "ArgUtils.h"
#include "ItemBase.h"

class ItemType;
typedef std::shared_ptr<ItemType> ItemTypePtr;

class ItemType : public ItemBaseT<ItemType> {
public:
  static ItemTypePtr makePtr() { return ItemTypePtr(new ItemType()); }
  static ItemTypePtr makePtr(const ArgString &text) {
    return ItemTypePtr(new ItemType(text));
  }

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgString text;

protected:
  ItemType() : ItemBaseT(QPI::TYPE) {}
  ItemType(const ItemType &other) = default;
  ItemType(const ArgString &text) : ItemBaseT(QPI::TYPE), text(text) {}

  friend class ItemBaseT<ItemType>;
};
