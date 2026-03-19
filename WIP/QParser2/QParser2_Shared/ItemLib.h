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
#include "QArgUtils.h"
#include "ItemBase.h"

class ItemLib : public ItemBase {
public:
  ItemLib(std::string typeStr, std::string argStr);
  ItemLib(const ItemLib &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  ArgString libText;
};

typedef std::shared_ptr<ItemLib> ItemLibPtr;

