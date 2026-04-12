//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * ItemLib.h -- Library file item class.
 *
 * Expecting: "library file:" P1
 * Example:   "library file: xxx"
 *
 * Analysis:
 *   P1 --  String value.  Optional?  Non-quoted?
 */
#pragma once
#include "ItemBase.h"
#include "QArgUtils.h"
#include <memory>

class ItemLib : public ItemBase {
public:
  ItemLib() : ItemBase(QPI::LIB) {}
  ItemLib(const ItemLib &other) = default;
  ItemLib(const ArgString &libText) : ItemBase(QPI::LIB), libText(libText) {}

  ItemBasePtr clone() const override;

  bool        parseItem(const std::string &argStr) override;
  std::string toString() const override;

  ArgString libText;
};

typedef std::shared_ptr<ItemLib> ItemLibPtr;
