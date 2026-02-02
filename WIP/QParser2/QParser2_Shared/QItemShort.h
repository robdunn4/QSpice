//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemShort.h -- Shorted Pins item class.
 *
 * Expecting: "shorted pins:" P1
 * Example:   "shorted pins: false"
 *
 * Analysis:
 *   P1 --  boolean shorted pin state ("true" or "false")
 */
#pragma once
#include "QArgUtils.h"
#include "QItemBase.h"

class QItemShort : public QItemBase {
public:
  QItemShort(std::string typeStr, std::string argStr);
  QItemShort(const QItemShort &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgBool bShorted;
};

typedef std::shared_ptr<QItemShort> QItemShortPtr;
