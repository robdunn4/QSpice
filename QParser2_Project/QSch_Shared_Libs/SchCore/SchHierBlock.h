#pragma once

#include <QString>
#include <QStringList>

#include <functional>
#include <string>

#include <ItemTree.h>

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
  NotABlock,   // not a component, or its symbol has a type
  NoName,      // no first string attribute, or a blank one
  NoFolder,    // a relative name, and the holding schematic is unsaved
  NotFound,    // the file does not exist
  NotAFile,    // the name is a folder
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
Result schematicPath(
    const NodePtr &compNode, const QString &docPath, QString &path);

// --- Parents: the schematics whose blocks open a given one ---
//
// QSpice keeps no record of which schematic a child belongs to, so a
// child's parents are FOUND, by searching where one would sit.

// The schematics in `childPath`'s folder and the folder above it with a
// block whose schematic (schematicPath()) is `childPath`: absolute,
// cleaned paths, sorted. A file is parsed only if its bytes contain the
// child's name, so a folder of unrelated schematics is quick; files that
// cannot be read or parsed are skipped.
QStringList findParents(const QString &childPath);

// The names of a schematic's PORTS -- top-level net labels with the port
// type; grounds never are -- each once (compared without regard to
// case), as written, in the order found.
QStringList portNames(const NodePtr &root);

// A new parent for the schematic `childRoot`, saved as `childPath`: the
// complete text of a schematic file to be saved as `parentPath`, holding
// one block that opens the child. Empty if the child has no ports.
//
// The block, laid out rather than piled up as QSpice's own is:
//   - one pin per port, on the left or right edge by the side the port's
//     label points to (text to the left of its point: the left edge);
//     a label pointing neither way goes by where it sits in the child
//   - top to bottom in the child's order, 100 apart on the electrical
//     grid, with 100 to spare above and below
//   - wide enough for both columns of pin names, `textWidth` giving a
//     name's width in schematic units at font size 1; at least 200
//   - the designator X1 above the box; the child's name below it,
//     relative to `parentPath`'s folder ("child", "sub\child")
//   - QSpice's own box colors and record forms
std::string
parentSchematicText(const NodePtr &childRoot, const QString &childPath,
                    const QString                                &parentPath,
                    const std::function<double(const QString &)> &textWidth);

}   // namespace sch::core::hier
