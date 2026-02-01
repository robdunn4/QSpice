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

  ArgString p1Str;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemSym> QItemSymPtr;
