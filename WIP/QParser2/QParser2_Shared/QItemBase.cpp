#include "QItemBase.h"
#include "QItemArc3P.h"
#include "QItemCmp.h"
#include "QItemCoil.h"
#include "QItemDBG.h"
#include "QItemDesc.h"
#include "QItemEllipse.h"
#include "QItemJunction.h"
#include "QItemLib.h"
#include "QItemLine.h"
#include "QItemNet.h"
#include "QItemPin.h"
#include "QItemRect.h"
#include "QItemSch.h"
#include "QItemShort.h"
#include "QItemSym.h"
#include "QItemTap.h"
#include "QItemText.h"
#include "QItemTriangle.h"
#include "QItemType.h"
#include "QItemWire.h"
#include "QItemZigzag.h"
// #include <stdexcept>

// Constructor: sets basic fields only, NO parsing
QItemBase::QItemBase(std::string typeStr, std::string argStr)
    : typeStr(typeStr), argStr(argStr) {
  enumID = QItemTypes::getEnum(typeStr);
}

// Copy constructor: copies all base class members
QItemBase::QItemBase(const QItemBase &other)
    : typeStr(other.typeStr), argStr(other.argStr), enumID(other.enumID) {}

QItemBasePtr QItemBase::makeItem(std::string typeStr, std::string argStr) {
  QItemBasePtr item;

  switch (QItemTypes::getEnum(typeStr)) {
  case QPI::SCH:
    item = std::make_shared<QItemSch>(typeStr, argStr);
    break;

  case QPI::COMP:
    item = std::make_shared<QItemCmp>(typeStr, argStr);
    break;

  case QPI::SYM:
    item = std::make_shared<QItemSym>(typeStr, argStr);
    break;

  case QPI::TYPE:
    item = std::make_shared<QItemType>(typeStr, argStr);
    break;

  case QPI::DESC:
    item = std::make_shared<QItemDesc>(typeStr, argStr);
    break;

  case QPI::LIB:
    item = std::make_shared<QItemLib>(typeStr, argStr);
    break;

  case QPI::SHORTED:
    item = std::make_shared<QItemShort>(typeStr, argStr);
    break;

  case QPI::LINE:
    item = std::make_shared<QItemLine>(typeStr, argStr);
    break;

  case QPI::RECT:
    item = std::make_shared<QItemRect>(typeStr, argStr);
    break;

  case QPI::ELLIPSE:
    item = std::make_shared<QItemEllipse>(typeStr, argStr);
    break;

  case QPI::ARC3P:
    item = std::make_shared<QItemArc>(typeStr, argStr);
    break;

  case QPI::TEXT:
    item = std::make_shared<QItemText>(typeStr, argStr);
    break;

  case QPI::PIN:
    item = std::make_shared<QItemPin>(typeStr, argStr);
    break;

  case QPI::ZIGZAG:
    item = std::make_shared<QItemZigzag>(typeStr, argStr);
    break;

  case QPI::NET:
    item = std::make_shared<QItemNet>(typeStr, argStr);
    break;

  case QPI::JUNCTION:
    item = std::make_shared<QItemJunction>(typeStr, argStr);
    break;

  case QPI::WIRE:
    item = std::make_shared<QItemWire>(typeStr, argStr);
    break;

  case QPI::COIL:
    item = std::make_shared<QItemCoil>(typeStr, argStr);
    break;

  case QPI::TRIANGLE:
    item = std::make_shared<QItemTriangle>(typeStr, argStr);
    break;

  case QPI::TAP:
    item = std::make_shared<QItemTap>(typeStr, argStr);
    break;

  default:
    item = std::make_shared<QItemDbg>(typeStr, argStr);
    break;
  }

  // Parse AFTER construction
  item->parseItem();

  return item;
}