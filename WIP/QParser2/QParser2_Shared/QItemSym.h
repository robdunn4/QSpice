//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemSym.h -- Symbol item class.
 *
 * Expecting: "symbol" P1
 * Example:   "symbol Vsin"
 *
 * Analysis:
 *   P1 -- Optional string.
 */
#pragma once
#include "QArgUtils.h"
#include "QItemBase.h"

class QItemSym : public QItemBase {
public:
  QItemSym(std::string typeStr, std::string argStr);
  QItemSym(const QItemSym &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgString text;
};

typedef std::shared_ptr<QItemSym> QItemSymPtr;
