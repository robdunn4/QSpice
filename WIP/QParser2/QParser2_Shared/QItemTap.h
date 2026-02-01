/*
 * QItemTap.h -- Bus tap item class.
 *
 * Expecting: "tap" (P1) (P2)
 * Example:   "tap (-1700,-300) (-1700,0)
 *
 * Analysis:
 *   P1 -- Endpoint coordinate
 *   P2 -- Endpoint coordinate
 */
#pragma once
#include "QArgUtils.h"
#include "QItemBase.h"

class QItemTap : public QItemBase {
public:
  QItemTap(std::string typeStr, std::string argStr);
  QItemTap(const QItemTap &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgPoint p1;
  ArgPoint p2;

protected:
  // Add schematic-specific member variables as needed
};

typedef std::shared_ptr<QItemTap> QItemTapPtr;
