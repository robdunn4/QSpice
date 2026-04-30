//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#pragma once
#include <ItemLib.h>
#include <ItemTreeIO.h>
#include <QString>

class QParserIO {
public:
  QParserIO()  = default;
  ~QParserIO() = default;

  // load a *.qsym file into an ItemTree, returning zero on success
  int load(const QString &fileName);

  // save an ItemTree to a *.qsym file, returning zero on success
  int save(const QString &fileName);

  // find first ItemLib entry and return pointer
  ItemLibPtr getLibItem() const;

protected:
  ItemTree tree;
};
