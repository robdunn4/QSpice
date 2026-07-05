//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "ItemBase.h"
#include "ItemArc3P.h"
#include "ItemCmp.h"
#include "ItemCoil.h"
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
  case QPI::SCH:      return ItemSch::makePtr();
  case QPI::COMP:     return ItemCmp::makePtr();
  case QPI::SYM:      return ItemSym::makePtr();
  case QPI::TYPE:     return ItemType::makePtr();
  case QPI::DESC:     return ItemDesc::makePtr();
  case QPI::LIB:      return ItemLib::makePtr();
  case QPI::SHORTED:  return ItemShort::makePtr();
  case QPI::NET:      return ItemNet::makePtr();
  case QPI::JUNCTION: return ItemJunction::makePtr();
  case QPI::WIRE:     return ItemWire::makePtr();
  case QPI::TAP:      return ItemTap::makePtr();
  case QPI::LINE:     return ItemLine::makePtr();
  case QPI::RECT:     return ItemRect::makePtr();
  case QPI::ELLIPSE:  return ItemEllipse::makePtr();
  case QPI::ARC3P:    return ItemArc::makePtr();
  case QPI::TRIANGLE: return ItemTriangle::makePtr();
  case QPI::COIL:     return ItemCoil::makePtr();
  case QPI::ZIGZAG:   return ItemZigzag::makePtr();
  case QPI::TEXT:     return ItemText::makePtr();
  case QPI::PIN:      return ItemPin::makePtr();
  case QPI::UNKNOWN:
  case QPI::COUNT:
  default:            return nullptr;
  }
}

ItemBasePtr ItemBase::makeItem(const std::string &typeStr) {
  QPI enumID = ItemTypes::getEnum(typeStr);
  if (enumID == QPI::UNKNOWN) return nullptr;
  return makeItem(enumID);
}
