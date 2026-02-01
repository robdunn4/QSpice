/*
 * QItemType.h -- Type item class.
 *
 * Expecting: "type:" P1
 * Example:   "type: V"
 *
 * Analysis:
 *   P1 --  Type string.  Optional?  Unquoted?
 */
#pragma once
#include "QArgUtils.h"
#include "QItemBase.h"

class QItemType : public QItemBase {
public:
  QItemType(std::string typeStr, std::string argStr);
  QItemType(const QItemType &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgString p1Str;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemType> QItemTypePtr;