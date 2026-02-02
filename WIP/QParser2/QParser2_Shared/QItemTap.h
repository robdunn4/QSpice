//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
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

  ArgPoint pt1;
  ArgPoint pt2;
};

typedef std::shared_ptr<QItemTap> QItemTapPtr;
