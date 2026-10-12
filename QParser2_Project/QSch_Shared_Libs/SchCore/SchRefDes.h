#pragma once

#include <QList>
#include <QSet>
#include <QString>

#include "ItemTree.h"

// Reference designators: R1, C7, Q3 -- allocated when a component is
// placed, so two parts never arrive with the same name.
//
// The rules, measured against QSpice and confirmed:
//
//   PREFIX comes from the placeholder with its trailing digits
//   stripped. The library's own number is IGNORED: a symbol whose
//   placeholder reads R5 still becomes R1 in an empty schematic. Every
//   built-in placeholder is its file's hotkey letter plus a digit --
//   M1..M4 for MOSFETs whose types are MN/MP -- so the prefix is the
//   letter, and the same rule handles a symbol with no digits at all
//   ("RLoad" -> "RLoad1").
//
//   NUMBER is the LOWEST FREE one for that prefix, not one past the
//   highest: with R1 and R3 placed, the next resistor is R2.
//
//   CASE-INSENSITIVE, as net names are: "r1" and "R1" are the same
//   designator.
//
//   PER DOCUMENT. A duplicate is not prevented -- QSpice allows one and
//   reports it at netlist time -- so this only decides what a NEW
//   component is called.
namespace sch::core::refdes {

// Records only: no scene, no QGraphicsItem. The editor maps items to their nodes and repaints what
// changed -- see SchViewInteract::beginPlacement().

// A component's designator RECORD: text record 0 inside its symbol
// (positional, as every attribute is -- record 1 is the value). Null if
// it has none.
NodePtr refDesNode(const NodePtr &compNode);

// The designator's non-numeric stem: "R5" -> "R", "RLoad" -> "RLoad".
QString prefixOf(const QString &refdes);

// A component's designator as written ("R1"), from its RECORD; empty
// if it has none.
QString ofNode(const NodePtr &compNode);

// Every designator of the components directly under the sheet root,
// lowercased, for collision testing. Content being placed is not in the
// tree, so it is not counted.
QSet<QString> existing(const NodePtr &root);

// The lowest free designator with this prefix, which is also RESERVED
// in `taken` so a run of allocations does not hand out the same name
// twice.
QString nextFree(const QString &prefix, QSet<QString> &taken);

// Names one component against `taken` (and reserves the name there),
// writing it into the component's designator record. Returns the new
// name, or empty if nothing changed -- no designator, a cleared one, or
// already the lowest free name.
QString assign(const NodePtr &compNode, QSet<QString> &taken);

} // namespace sch::core::refdes
