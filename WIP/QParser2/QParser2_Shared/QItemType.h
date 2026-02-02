//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
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

  ArgString text;
};

typedef std::shared_ptr<QItemType> QItemTypePtr;