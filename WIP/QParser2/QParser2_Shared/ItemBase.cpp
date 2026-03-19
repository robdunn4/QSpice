//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemBase.h"
#include "ItemArc3P.h"
#include "ItemCmp.h"
#include "ItemCoil.h"
#include "ItemDBG.h"
#include "ItemDesc.h"
#include "ItemEllipse.h"
#include "ItemJunction.h"
#include "ItemLib.h"
#include "ItemLine.h"
#include "ItemNet.h"
#include "ItemPin.h"
#include "ItemRect.h"
#include "ItemSch.h"
#include "ItemShort.h"
#include "ItemSym.h"
#include "ItemTap.h"
#include "ItemText.h"
#include "ItemTriangle.h"
#include "ItemType.h"
#include "ItemWire.h"
#include "ItemZigzag.h"

// Constructor: sets basic fields only, NO parsing
ItemBase::ItemBase(std::string typeStr, std::string argStr)
    : typeStr(typeStr), argStr(argStr) {
  enumID = ItemTypes::getEnum(typeStr);
}

// Copy constructor: copies all base class members
ItemBase::ItemBase(const ItemBase &other)
    : typeStr(other.typeStr), argStr(other.argStr), enumID(other.enumID) {}

ItemBasePtr ItemBase::makeItem(std::string typeStr, std::string argStr) {
  ItemBasePtr item;

  switch (ItemTypes::getEnum(typeStr)) {
  case QPI::SCH:
    item = std::make_shared<ItemSch>(typeStr, argStr);
    break;

  case QPI::COMP:
    item = std::make_shared<ItemCmp>(typeStr, argStr);
    break;

  case QPI::SYM:
    item = std::make_shared<ItemSym>(typeStr, argStr);
    break;

  case QPI::TYPE:
    item = std::make_shared<ItemType>(typeStr, argStr);
    break;

  case QPI::DESC:
    item = std::make_shared<ItemDesc>(typeStr, argStr);
    break;

  case QPI::LIB:
    item = std::make_shared<ItemLib>(typeStr, argStr);
    break;

  case QPI::SHORTED:
    item = std::make_shared<ItemShort>(typeStr, argStr);
    break;

  case QPI::LINE:
    item = std::make_shared<ItemLine>(typeStr, argStr);
    break;

  case QPI::RECT:
    item = std::make_shared<ItemRect>(typeStr, argStr);
    break;

  case QPI::ELLIPSE:
    item = std::make_shared<ItemEllipse>(typeStr, argStr);
    break;

  case QPI::ARC3P:
    item = std::make_shared<ItemArc>(typeStr, argStr);
    break;

  case QPI::TEXT:
    item = std::make_shared<ItemText>(typeStr, argStr);
    break;

  case QPI::PIN:
    item = std::make_shared<ItemPin>(typeStr, argStr);
    break;

  case QPI::ZIGZAG:
    item = std::make_shared<ItemZigzag>(typeStr, argStr);
    break;

  case QPI::NET:
    item = std::make_shared<ItemNet>(typeStr, argStr);
    break;

  case QPI::JUNCTION:
    item = std::make_shared<ItemJunction>(typeStr, argStr);
    break;

  case QPI::WIRE:
    item = std::make_shared<ItemWire>(typeStr, argStr);
    break;

  case QPI::COIL:
    item = std::make_shared<ItemCoil>(typeStr, argStr);
    break;

  case QPI::TRIANGLE:
    item = std::make_shared<ItemTriangle>(typeStr, argStr);
    break;

  case QPI::TAP:
    item = std::make_shared<ItemTap>(typeStr, argStr);
    break;

  default:
    item = std::make_shared<ItemDbg>(typeStr, argStr);
    break;
  }

  // Parse AFTER construction
  item->parseItem();

  return item;
}
