//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * QItemLib.h -- Library file item class.
 *
 * Expecting: "library file:" P1
 * Example:   "library file: xxx"
 *
 * Analysis:
 *   P1 --  String value.  Optional?  Non-quoted?
 */
#pragma once
#include "QArgUtils.h"
#include "QItemBase.h"

class QItemLib : public QItemBase {
public:
  QItemLib(std::string typeStr, std::string argStr);
  QItemLib(const QItemLib &other);
  QItemBasePtr clone() const override;

  void parseItem() override;
  std::string toString() const override;

  ArgString libText;
};

typedef std::shared_ptr<QItemLib> QItemLibPtr;
