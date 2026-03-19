//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*                                                                             \
 * ItemWire.h -- Wire item class.                                             \
 *                                                                             \
 * Expecting: "wire" (P1) (P2) P3                                              \
 * Example:   "wire (-1700,-300) (-1700,0) "Vin""                              \
 *                                                                             \
 * Analysis:                                                                   \
 *   P1 -- Endpoint coordinate                                                 \
 *   P2 -- Endpoint coordinate                                                 \
 *   P3 -- Net/port name                                                       \
 */                                                                            \
#pragma once
#include "QArgUtils.h"
#include "ItemBase.h"

class ItemWire : public ItemBase {
public:
  ItemWire(std::string typeStr, std::string argStr);
  ItemWire(const ItemWire &other);
  ItemBasePtr clone() const override;

  void        parseItem() override;
  std::string toString() const override;

  ArgPoint  pt1;
  ArgPoint  pt2;
  ArgString netName;
};

typedef std::shared_ptr<ItemWire> ItemWirePtr;

