//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*                                                                             \
 * ItemJunction.h -- Junction item class.                                     \
 *                                                                             \
 * Expecting: "junction" (P1)                                                  \
 * Example:   "junction (1500,0)"                                              \
 *                                                                             \
 * Analysis:                                                                   \
 *   P1 -- Endpoint coordinate                                                 \
 */                                                                            \
#pragma once
#include "QArgUtils.h"
#include "ItemBase.h"

class ItemJunction : public ItemBase {
public:
  ItemJunction(std::string typeStr, std::string argStr);
  ItemJunction(const ItemJunction &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  ArgPoint pt1;
};

typedef std::shared_ptr<ItemJunction> ItemJunctionPtr;

