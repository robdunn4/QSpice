//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemSch.h -- Schematic item class.
 *
 * Expecting: "schematic"
 *
 * Analysis:
 *   No parameters?
 */
#pragma once
#include "QItemBase.h"

class QItemSch : public QItemBase {
public:
  QItemSch(std::string typeStr, std::string argStr);
  QItemSch(const QItemSch &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;
};

typedef std::shared_ptr<QItemSch> QItemSchPtr;
