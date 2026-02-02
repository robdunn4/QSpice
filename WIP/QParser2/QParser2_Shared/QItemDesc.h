//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemDesc.h -- Description item class.
 *
 * Expecting: "description:" P1
 * Example:   "description: Independent Voltage Source"
 *
 * Analysis:
 *   P1 -- String value.  Optional?  Non-quoted?
 */
#pragma once
#include "QArgUtils.h"
#include "QItemBase.h"

class QItemDesc : public QItemBase {
public:
  QItemDesc(std::string typeStr, std::string argStr);
  QItemDesc(const QItemDesc &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgString text;
};

typedef std::shared_ptr<QItemDesc> QItemDescPtr;
