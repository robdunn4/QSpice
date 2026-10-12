#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include "ItemTree.h"

// Which records may sit where -- and the removal of any that may not.
//
// The file format will carry anything inside anything; QSpice's editors
// will not create these, and neither will we. A file that has them was
// edited by hand, written by another tool, or comes from a QSpice we
// have not seen. Such records are OMITTED -- detached from the tree
// before any item is built -- and the user is told.
//
// The rules (Robert, 2026-10-07):
//   - Inside a SYMBOL (a .qsym, the copy every component carries, a
//     symbol being placed or pasted): no wire (bus included), bus tap,
//     net label, junction, symbol, component or schematic.
//   - In a SCHEMATIC: no pin at the top level, no symbol outside a
//     component, and no schematic anywhere but the root.
//   - Inside a COMPONENT: no schematic, no component, and no symbol
//     beyond the first.
//
// Every entry point applies them: opening a file, the symbol libraries
// (built-in .qsch files and .qsym folders), paste, and drag-and-drop.
//
// The library (QParser2) is NOT involved: it reads files faithfully,
// and what the editor accepts is the editor's business.
//
// Records and text only -- no widgets.
// The dialogs are in SchOmittedDlg.
//
// No moc.
namespace sch::core::recordrules {

// What the records are going INTO, when that is not plain from the
// tree itself: a pasted fragment arrives under a «schematic» root
// whichever kind of document it is pasted into.
enum class Dest { Schematic, Symbol };

// One record removed.
struct Omitted {
  QString kind;   // "wire", "bus", "pin", ... -- singular
  QString where;  // "inside the symbol of R3", "at the top level"
  QString record; // the record as written, for the log
};

// Removes every record under `node` that may not be where it is, and
// says what went. `node` may be a «schematic» (read per `dest`), a
// «symbol» (always symbol rules) or a «component».
QList<Omitted> dropForbidden(const NodePtr &node, Dest dest);

// "2 x wire inside the symbol of R3", one line per kind and place.
QStringList summary(const QList<Omitted> &omitted);

// Every omitted record, one qWarning line each, headed by `source`.
void log(const QString &source, const QList<Omitted> &omitted);


// --- Symbol libraries.
//
// They load at startup, many files at once, so their reports are
// collected here and shown in ONE dialog when loading is done
// (SchOmittedDlg::warnLibraryNotes()), rather than a dialog per file.

void           noteLibrary(const QString &fileName, const QList<Omitted> &omitted);
QStringList    takeLibraryNotes();

} // namespace sch::core::recordrules
