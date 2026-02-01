/*
 * QItemJunction.h -- Junction item class.
 *
 * Expecting: "junction" (P1)
 * Example:   "junction (1500,0)"
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 */                                                                            \
#pragma once
#include "QArgUtils.h"
#include "QItemBase.h"

class QItemJunction : public QItemBase {
public:
  QItemJunction(std::string typeStr, std::string argStr);
  QItemJunction(const QItemJunction &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgPoint p1;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemJunction> QItemJunctionPtr;
