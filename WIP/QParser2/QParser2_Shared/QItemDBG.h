//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemDBG.h -- Dummy class for debugging.  Simply uses the line text.
 */
#pragma once
#include "QItemBase.h"

class QItemDbg : public QItemBase {
public:
  QItemDbg(std::string typeStr, std::string argStr);
  QItemDbg(const QItemDbg &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;
};

typedef std::shared_ptr<QItemDbg> QItemDbgPtr;
