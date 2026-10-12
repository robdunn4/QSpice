#pragma once

#include <QList>
#include <QPair>
#include <QString>

#include <utility>

#include "ArgUtils.h" // Point
#include "ItemTree.h"
#include "SchNetModel.h"
#include "SchPointKey.h"

// The editing passes' DECISIONS -- records in, a list of edits out. No
// scene, no QGraphicsItem, no undo commands: SchConnect applies what
// these return. Split out of SchConnect (2026-10-07).
//
// Every plan describes each edit as a line of text, for the log should
// one fail to apply.
//
// The rules themselves are unchanged from the scene-era passes; see
// each function in the .cpp for the measurements behind them.
//
// No moc.
namespace sch::core::connplan {

using Seg = std::pair<PointKey, PointKey>;

// One edit of the wire and line passes: remove `remove`, add `add` --
// wires carrying `like`'s net name, or lines.
struct ReplaceOp {
  QList<NodePtr> remove;
  NodePtr        like;
  QList<Seg>     add;
  QString        text;
};

// A wire whose stored net name should change.
struct Rename {
  NodePtr wire;
  QString oldName;
  QString newName;
  QString text;
};

// A net label or ground whose alignment should change.
struct Realign {
  NodePtr net;
  int     oldAlign = 0;
  int     newAlign = 0;
  QString text;
};

// --- Wires and lines.

// Every wire cut where a connection point lands strictly inside it.
QList<ReplaceOp> planSplits(const NetModel &model);

// Every box-defined shape, wire or line under `root` whose two points
// coincide -- removed.
QList<ReplaceOp> planDegenerate(const NodePtr &root);

// Collinear LINES under `root` that overlap or touch, merged.
QList<ReplaceOp> planLines(const NodePtr &root);

// Collinear WIRES that overlap, merged into their union.
QList<ReplaceOp> planOverlaps(const NetModel &model);

// Wires meeting end to end in a straight line, with nothing else at the
// join, merged.
QList<ReplaceOp> planChains(const NetModel &model);

// A segment being drawn, cut at the pins on it; a span between two
// consecutive pins of one component (through its body) is dropped.
QList<QPair<Point, Point>> cutAtPins(const Point &from, const Point &to,
                                     const NetModel &model);

// --- Names and anchors.

// Net names for every wire: labels' names where a net has one (first
// in record order wins), N01, N02, ... otherwise.
QList<Rename> planNames(const NetModel &model);

// The alignment each of `touchedNets` (net-label records) should have,
// from the wires and taps at its anchor.
QList<Realign> planAnchors(const NetModel &model, const QList<NodePtr> &touchedNets);

// Grounds under `root` still in their unconnected orientation.
QList<NodePtr> unorientedGrounds(const NodePtr &root);

// Where an unconnected net item is anchored: a label at centre-bottom, a
// ground at centre-top.
int unconnectedAlign(bool ground);

// GND, EARTH or CHASSIS, ignoring case.
bool isGroundName(const QString &name);

} // namespace sch::core::connplan
