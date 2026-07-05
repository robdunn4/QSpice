//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QParserIO.h"

// return zero on success
int QParserIO::load(const QString &fileName) {
  try {
    // parse input file into an ItemTree
    ItemTreeIO::ParseResult res = ItemTreeIO::readFile(fileName.toStdString());
    if (res.error.length()) {
      return -1;
    }
    tree = res.tree;

    if (tree.empty()) {
      return -2;
    }

    // if not a symbol, quick out
    QPI rootType = tree.root()->getEnumID();
    if (rootType != QPI::SYM) {
      return -3;
    }
  } catch (...) {
    return -4;
  }

  return 0;
}

// TODO: unimplemented
int QParserIO::save(const QString &fileName) { return 0; }

// get ItemLib pointer
ItemLibPtr QParserIO::getLibItem() const {
  NodePtr node = tree.root();
  node         = node->findFirstOf(QPI::LIB);

  if (!node) return nullptr;

  return std::dynamic_pointer_cast<ItemLib>(node->item());
}
