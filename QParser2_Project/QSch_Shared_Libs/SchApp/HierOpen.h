#pragma once

#include <QString>

#include "ItemTree.h"

class QWidget;

// Opening a hierarchical block's schematic, as both programs do it: the
// path found by sch::core::hier::schematicPath(), or a message saying why
// there is none. Each program then opens the path in its own way.
namespace sch::app {

// The block's schematic, absolute, in `path`; true if it exists. False
// after a warning box (parent `parent`) naming the problem and the path
// tried. `docPath` is the schematic holding the block, empty if unsaved.
bool hierSchematicPath(QWidget *parent, const NodePtr &compNode, const QString &docPath,
                       QString &path);

} // namespace sch::app
