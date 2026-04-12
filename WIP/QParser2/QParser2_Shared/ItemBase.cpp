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

ItemBasePtr ItemBase::makeItem(QPI enumID) {
  switch (enumID) {
  case QPI::SCH:      return std::make_shared<ItemSch>();
  case QPI::COMP:     return std::make_shared<ItemCmp>();
  case QPI::SYM:      return std::make_shared<ItemSym>();
  case QPI::TYPE:     return std::make_shared<ItemType>();
  case QPI::DESC:     return std::make_shared<ItemDesc>();
  case QPI::LIB:      return std::make_shared<ItemLib>();
  case QPI::SHORTED:  return std::make_shared<ItemShort>();
  case QPI::NET:      return std::make_shared<ItemNet>();
  case QPI::JUNCTION: return std::make_shared<ItemJunction>();
  case QPI::WIRE:     return std::make_shared<ItemWire>();
  case QPI::TAP:      return std::make_shared<ItemTap>();
  case QPI::LINE:     return std::make_shared<ItemLine>();
  case QPI::RECT:     return std::make_shared<ItemRect>();
  case QPI::ELLIPSE:  return std::make_shared<ItemEllipse>();
  case QPI::ARC3P:    return std::make_shared<ItemArc>();
  case QPI::TRIANGLE: return std::make_shared<ItemTriangle>();
  case QPI::COIL:     return std::make_shared<ItemCoil>();
  case QPI::ZIGZAG:   return std::make_shared<ItemZigzag>();
  case QPI::TEXT:     return std::make_shared<ItemText>();
  case QPI::PIN:      return std::make_shared<ItemPin>();
  default:            return std::make_shared<ItemDbg>();
  }
}

ItemBasePtr ItemBase::makeItem(const std::string &typeStr) {
  QPI enumID = ItemTypes::getEnum(typeStr);
  if (enumID == QPI::UNKNOWN) return nullptr;
  return makeItem(enumID);
}
