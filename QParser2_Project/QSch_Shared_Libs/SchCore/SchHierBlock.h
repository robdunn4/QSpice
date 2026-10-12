#pragma once

#include <QString>

#include "ItemTree.h"

// Hierarchical schematic blocks: a component whose symbol netlists as
// another schematic rather than as a SPICE device.
//
// A component is a block when its symbol's «type:» record is ABSENT or
// BLANK -- the blank first entry of QSpice's Symbol Type list. Its
// schematic is named by its FIRST STRING ATTRIBUTE (text record 1; record
// 0 is the designator) with ".qsch" appended. The name may be absolute or
// relative; relative names are taken from the folder of the schematic
// that holds the block.
//
// Records only: no scene, no dialogs. The programs report failures in
// their own words -- see sch::app::hierSchematicPath().
namespace sch::core::hier {

// Why a block's schematic could not be found, or Ok.
enum class Result {
  Ok,
  NotABlock, // not a component, or its symbol has a type
  NoName,    // no first string attribute, or a blank one
  NoFolder,  // a relative name, and the holding schematic is unsaved
  NotFound,  // the file does not exist
  NotAFile,  // the name is a folder
};

// Whether `compNode` (a «component» node) is a hierarchical block.
bool isBlock(const NodePtr &compNode);

// The block's schematic file name as written -- the first string
// attribute plus ".qsch" -- or empty if it has no such attribute. Not
// resolved against any folder.
QString schematicName(const NodePtr &compNode);

// The block's schematic as an absolute, cleaned path, in `path`, when the
// result is Ok. `docPath` is the file of the schematic holding the block;
// empty for an unsaved one. Whatever the result, `path` is the best path
// known, for the message -- the name as written when it cannot be
// resolved.
Result schematicPath(const NodePtr &compNode, const QString &docPath, QString &path);

} // namespace sch::core::hier
