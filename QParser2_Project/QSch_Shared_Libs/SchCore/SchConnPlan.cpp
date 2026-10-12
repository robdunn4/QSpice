#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchConnPlan.h"

#include <QHash>
#include <QPointF>
#include <QSet>
#include <QStringList>
#include <QtGlobal>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <cmath>
#include <utility>

#include "ItemCoil.h"
#include "ItemEllipse.h"
#include "ItemLine.h"
#include "ItemNet.h"
#include "ItemRect.h"
#include "ItemWire.h"
#include "ItemZigzag.h"
#include "SchAlign.h"
#include "SchStringDecode.h"

namespace sch::core::connplan {

bool isGroundName(const QString &name) {
  const QString n = name.trimmed();
  return n.compare(QStringLiteral("GND"), Qt::CaseInsensitive) == 0 ||
         n.compare(QStringLiteral("EARTH"), Qt::CaseInsensitive) == 0 ||
         n.compare(QStringLiteral("CHASSIS"), Qt::CaseInsensitive) == 0;
}

namespace {

// True if p lies strictly between the endpoints of the segment a-b.
//
// Exact integer arithmetic throughout -- no tolerance, matching how
// QSpice decides coincidence. The cross product being zero puts p on
// the infinite line; the dot-product bounds put it inside the segment;
// and the equality checks exclude the two endpoints themselves, which
// are already coincident and need no split.
bool isOnSpan(const PointKey &p, const PointKey &a, const PointKey &b) {
  if ((p.x == a.x && p.y == a.y) || (p.x == b.x && p.y == b.y)) return false;

  const qint64 cross = static_cast<qint64>(p.x - a.x) * (b.y - a.y) -
                       static_cast<qint64>(p.y - a.y) * (b.x - a.x);
  if (cross != 0) return false;

  const qint64 dot = static_cast<qint64>(p.x - a.x) * (b.x - a.x) +
                     static_cast<qint64>(p.y - a.y) * (b.y - a.y);
  if (dot <= 0) return false;

  const qint64 lenSq = static_cast<qint64>(b.x - a.x) * (b.x - a.x) +
                       static_cast<qint64>(b.y - a.y) * (b.y - a.y);
  return dot < lenSq;
}

PointKey keyOf(const Point &p) { return PointKey{p.x, p.y}; }

bool keyLess(const PointKey &a, const PointKey &b) {
  if (a.x != b.x) return a.x < b.x;
  return a.y < b.y;
}

QString ptText(const PointKey &p) { return QStringLiteral("(%1,%2)").arg(p.x).arg(p.y); }

// A segment as written, first end first.
QString dirSegText(const PointKey &a, const PointKey &b) {
  return ptText(a) + QLatin1Char('-') + ptText(b);
}

// A segment either way round, for results whose direction carries no
// meaning.
QString segText(PointKey a, PointKey b) {
  if (keyLess(b, a)) std::swap(a, b);
  return dirSegText(a, b);
}

QString splitText(const PointKey &a, const PointKey &b, const QList<Seg> &pieces) {
  QStringList out;
  for (const Seg &piece : pieces) out << dirSegText(piece.first, piece.second);
  return QStringLiteral("split %1 -> %2")
      .arg(dirSegText(a, b),
           out.isEmpty() ? QStringLiteral("nothing") : out.join(QLatin1Char(' ')));
}

QString mergeText(const QString &kind, QStringList parts, const PointKey &a,
                  const PointKey &b) {
  std::sort(parts.begin(), parts.end());
  return QStringLiteral("%1 {%2} -> %3").arg(kind, parts.join(QLatin1Char(' ')), segText(a, b));
}

QString quotedText(const QString &text) { return QLatin1Char('"') + text + QLatin1Char('"'); }

// --- Merging runs -------------------------------------------------------

// A straight run of one or more original segments (indices into the
// gathering's own list).
struct Run {
  PointKey   a;
  PointKey   b;
  QList<int> parts;
  bool       bus = false;
};

// Collapses COLLINEAR runs that overlap -- or, with `touching`, merely
// touch -- into one spanning their union. Resolved in memory before any
// command is made, so chains collapse fully. (mergeOverlappingWires
// and mergeLines, unchanged in their rules.)
void mergeOverlaps(QList<Run> &runs, bool touching) {
  bool mergedAny = true;
  while (mergedAny) {
    mergedAny = false;

    for (int i = 0; i < runs.size() && !mergedAny; ++i) {
      for (int j = i + 1; j < runs.size() && !mergedAny; ++j) {
        const qint64 dx1 = static_cast<qint64>(runs[i].b.x) - runs[i].a.x;
        const qint64 dy1 = static_cast<qint64>(runs[i].b.y) - runs[i].a.y;
        const qint64 dx2 = static_cast<qint64>(runs[j].b.x) - runs[j].a.x;
        const qint64 dy2 = static_cast<qint64>(runs[j].b.y) - runs[j].a.y;

        // Parallel, in exact integer arithmetic...
        if (dx1 * dy2 - dy1 * dx2 != 0) continue;

        // ...and on the SAME line, not merely parallel.
        const qint64 ox = static_cast<qint64>(runs[j].a.x) - runs[i].a.x;
        const qint64 oy = static_cast<qint64>(runs[j].a.y) - runs[i].a.y;
        if (dx1 * oy - dy1 * ox != 0) continue;

        // Every endpoint projected onto the shared direction: the scalar
        // ordering is the geometric ordering.
        const PointKey origin = runs[i].a;
        const qint64   len2   = dx1 * dx1 + dy1 * dy1;
        const qint64 pia = static_cast<qint64>(runs[i].a.x - origin.x) * dx1 +
                           static_cast<qint64>(runs[i].a.y - origin.y) * dy1;
        const qint64 pib = static_cast<qint64>(runs[i].b.x - origin.x) * dx1 +
                           static_cast<qint64>(runs[i].b.y - origin.y) * dy1;
        const qint64 pja = static_cast<qint64>(runs[j].a.x - origin.x) * dx1 +
                           static_cast<qint64>(runs[j].a.y - origin.y) * dy1;
        const qint64 pjb = static_cast<qint64>(runs[j].b.x - origin.x) * dx1 +
                           static_cast<qint64>(runs[j].b.y - origin.y) * dy1;

        const qint64 i0 = qMin(pia, pib);
        const qint64 i1 = qMax(pia, pib);
        const qint64 j0 = qMin(pja, pjb);
        const qint64 j1 = qMax(pja, pjb);

        // Wires: STRICT overlap -- a single shared point is touching,
        // which the end-to-end merge handles and which may be a
        // junction. Lines: touching counts, nothing connects to a line.
        if (touching && (j0 > i1 || j1 < i0)) continue;
        if (!touching && (j0 >= i1 || j1 <= i0)) continue;

        const qint64 lo = qMin(i0, j0);
        const qint64 hi = qMax(i1, j1);

        // t is a dot product against (dx1,dy1), so dividing by its
        // squared length recovers the parameter -- exactly, since every
        // end lies on the line through integer points.
        runs[i].a = PointKey{static_cast<int>(origin.x + (dx1 * lo) / len2),
                             static_cast<int>(origin.y + (dy1 * lo) / len2)};
        runs[i].b = PointKey{static_cast<int>(origin.x + (dx1 * hi) / len2),
                             static_cast<int>(origin.y + (dy1 * hi) / len2)};
        runs[i].parts.append(runs[j].parts);
        runs.removeAt(j);
        mergedAny = true;
      }
    }
  }
}

// Joins runs meeting END TO END in a straight continuation, where
// exactly two wire ends meet and nothing else connects. A tap end
// blocks an ordinary pair but not two bus segments. (mergeCollinearWires,
// unchanged in its rules.)
void mergeChains(QList<Run> &runs, const QSet<PointKey> &blocked,
                 const QSet<PointKey> &tapPoints) {
  bool mergedAny = true;
  while (mergedAny) {
    mergedAny = false;

    for (int i = 0; i < runs.size() && !mergedAny; ++i) {
      for (int j = i + 1; j < runs.size() && !mergedAny; ++j) {
        PointKey shared;
        PointKey farI;
        PointKey farJ;
        if (runs[i].a == runs[j].a) {
          shared = runs[i].a;
          farI   = runs[i].b;
          farJ   = runs[j].b;
        } else if (runs[i].a == runs[j].b) {
          shared = runs[i].a;
          farI   = runs[i].b;
          farJ   = runs[j].a;
        } else if (runs[i].b == runs[j].a) {
          shared = runs[i].b;
          farI   = runs[i].a;
          farJ   = runs[j].b;
        } else if (runs[i].b == runs[j].b) {
          shared = runs[i].b;
          farI   = runs[i].a;
          farJ   = runs[j].a;
        } else {
          continue;
        }

        if (blocked.contains(shared)) continue;

        const bool bothBus = runs[i].bus && runs[j].bus;
        if (tapPoints.contains(shared) && !bothBus) continue;

        // Exactly two wire endpoints here -- a third makes a junction.
        int endpointsHere = 0;
        for (const Run &run : runs) {
          if (run.a == shared) ++endpointsHere;
          if (run.b == shared) ++endpointsHere;
        }
        if (endpointsHere != 2) continue;

        // Collinear AND leaving in opposite directions: a straight
        // continuation, not a segment folded back over the other.
        const qint64 cross =
            static_cast<qint64>(farI.x - shared.x) * (farJ.y - shared.y) -
            static_cast<qint64>(farI.y - shared.y) * (farJ.x - shared.x);
        if (cross != 0) continue;

        const qint64 dot =
            static_cast<qint64>(farI.x - shared.x) * (farJ.x - shared.x) +
            static_cast<qint64>(farI.y - shared.y) * (farJ.y - shared.y);
        if (dot >= 0) continue;

        runs[i].a = farI;
        runs[i].b = farJ;
        runs[i].parts.append(runs[j].parts);
        runs.removeAt(j);
        mergedAny = true;
      }
    }
  }
}

// The merge edits from resolved runs: one per run of two or more parts.
// `segs` and `nodes` are the gathering's originals, by index.
QList<ReplaceOp> opsFromRuns(const QString &kind, const QList<Run> &runs,
                             const QList<Seg> &segs, const QList<NodePtr> &nodes) {
  QList<ReplaceOp> ops;
  for (const Run &run : runs) {
    if (run.parts.size() < 2) continue;

    ReplaceOp   op;
    QStringList parts;
    for (int index : run.parts) {
      parts << segText(segs[index].first, segs[index].second);
      op.remove << nodes[index];
    }
    op.like = nodes[run.parts.first()];
    op.add << Seg(run.a, run.b);
    op.text = mergeText(kind, parts, run.a, run.b);
    ops.append(op);
  }
  return ops;
}

// --- Splitting a wire at the points on its span ------------------------

// The pieces a wire from a to b becomes, cut at `cuts` (points strictly
// on its span) and dropping a span between two consecutive pins of the
// same component -- `owner` maps a pin's point to its component, -1 for
// none. (splitWiresAtConnections, unchanged in its rules.)
QList<Seg> piecesOf(const PointKey &a, const PointKey &b, QList<PointKey> cuts,
                    const QHash<PointKey, int> &owner) {
  // Ordered along the wire: distance from a is monotonic on a span.
  std::sort(cuts.begin(), cuts.end(), [&a](const PointKey &lhs, const PointKey &rhs) {
    const qint64 dl = static_cast<qint64>(lhs.x - a.x) * (lhs.x - a.x) +
                      static_cast<qint64>(lhs.y - a.y) * (lhs.y - a.y);
    const qint64 dr = static_cast<qint64>(rhs.x - a.x) * (rhs.x - a.x) +
                      static_cast<qint64>(rhs.y - a.y) * (rhs.y - a.y);
    return dl < dr;
  });

  QList<Seg> pieces;
  PointKey   from = a;
  for (const PointKey &cut : cuts) {
    const int fromOwner = owner.value(from, -1);
    if (fromOwner < 0 || fromOwner != owner.value(cut, -1)) pieces.append(Seg(from, cut));
    from = cut;
  }

  const int fromOwner = owner.value(from, -1);
  if (fromOwner < 0 || fromOwner != owner.value(b, -1)) pieces.append(Seg(from, b));

  return pieces;
}

// The cuts on one wire: every candidate point strictly on its span,
// deduplicated; a bus is cut only at non-tap points.
QList<PointKey> cutsOn(const PointKey &a, const PointKey &b, bool bus,
                       const QList<PointKey> &points, const QSet<PointKey> &nonTap) {
  QList<PointKey> cuts;
  for (const PointKey &p : points) {
    if (!isOnSpan(p, a, b)) continue;
    if (bus && !nonTap.contains(p)) continue; // a tap: no cut
    if (cuts.contains(p)) continue;
    cuts.append(p);
  }
  return cuts;
}

void addCandidate(QList<PointKey> &points, QSet<PointKey> &nonTap, const PointKey &p,
                  bool tap) {
  points.append(p);
  if (!tap) nonTap.insert(p);
}

} // namespace

// --- Cutting a drawn segment at pins -----------------------------------

namespace {

// The cut of a drawn segment at the pins on it, from a list of pins and
// their owners (-1 for none).
QList<QPair<Point, Point>> cutAtPinList(const Point &from, const Point &to,
                                        const QList<std::pair<Point, int>> &pins) {
  QList<QPair<Point, Point>> result;

  // Every pin exactly on the segment, with its owner and how far along
  // it sits.
  struct Cut {
    qint64 along; // squared distance from `from`, for ordering
    Point  at;
    int    owner;
  };

  QList<Cut> cuts;

  const qint64 dx = static_cast<qint64>(to.x) - from.x;
  const qint64 dy = static_cast<qint64>(to.y) - from.y;

  for (const std::pair<Point, int> &pin : pins) {
    const Point &at = pin.first;

    // On the line: zero cross product, in exact integer arithmetic.
    const qint64 px = static_cast<qint64>(at.x) - from.x;
    const qint64 py = static_cast<qint64>(at.y) - from.y;
    if (px * dy - py * dx != 0) continue;

    // ...and BETWEEN the ends rather than beyond them.
    const qint64 dot = px * dx + py * dy;
    if (dot < 0) continue;
    if (dot > dx * dx + dy * dy) continue;

    cuts.append(Cut{px * px + py * py, at, pin.second});
  }

  if (cuts.isEmpty()) {
    result.append(qMakePair(from, to));
    return result;
  }

  std::sort(cuts.begin(), cuts.end(),
            [](const Cut &a, const Cut &b) { return a.along < b.along; });

  // The ordered points the segment is divided at: its start, each cut,
  // its end. Duplicates collapse, so a pin exactly on an endpoint does
  // not produce a zero-length piece.
  QList<Point> points;
  QList<int>   owners; // owner of the pin at each point, or -1

  points.append(from);
  owners.append(-1);

  for (const Cut &cut : cuts) {
    if (cut.at.x == points.last().x && cut.at.y == points.last().y) {
      owners.last() = cut.owner; // the endpoint IS this pin
      continue;
    }
    points.append(cut.at);
    owners.append(cut.owner);
  }

  if (to.x != points.last().x || to.y != points.last().y) {
    points.append(to);
    owners.append(-1);
  }

  for (int i = 0; i + 1 < points.size(); ++i) {
    // A span between two CONSECUTIVE pins of the same component runs
    // through its body and is not wire.
    if (owners[i] >= 0 && owners[i] == owners[i + 1]) continue;
    result.append(qMakePair(points[i], points[i + 1]));
  }

  return result;
}

} // namespace

namespace {

// Every pin of the document, owners numbered by component.
QList<std::pair<Point, int>> recordPins(const NetModel &model) {
  QList<std::pair<Point, int>> pins;
  for (const NetModel::Pin &pin : model.pins())
    pins.append({Point(pin.at.x, pin.at.y), pin.comp});
  return pins;
}

} // namespace

// --- splitWiresAtConnections -------------------------------------------

QList<ReplaceOp> planSplits(const NetModel &model) {
  QList<PointKey>      points;
  QSet<PointKey>       nonTap;
  QHash<PointKey, int> owner;

  for (const NetModel::Wire &wire : model.wires()) {
    addCandidate(points, nonTap, wire.a, false);
    addCandidate(points, nonTap, wire.b, false);
  }
  for (const NetModel::Label &label : model.labels())
    addCandidate(points, nonTap, label.at, false);
  for (const NetModel::Tap &tap : model.taps()) {
    addCandidate(points, nonTap, tap.a, true);
    addCandidate(points, nonTap, tap.b, true);
  }
  for (const NetModel::Pin &pin : model.pins()) {
    addCandidate(points, nonTap, pin.at, false);
    if (pin.comp >= 0) owner.insert(pin.at, pin.comp);
  }

  QList<ReplaceOp> ops;
  for (const NetModel::Wire &wire : model.wires()) {
    const QList<PointKey> cuts = cutsOn(wire.a, wire.b, wire.bus, points, nonTap);
    if (cuts.isEmpty()) continue;

    // One wire in, N+1 out -- the original REMOVED rather than
    // shortened, so undo restores exactly one record. Every piece
    // dropped means the whole wire ran through one component.
    ReplaceOp op;
    op.remove << wire.node;
    op.like = wire.node;
    op.add  = piecesOf(wire.a, wire.b, cuts, owner);
    op.text = splitText(wire.a, wire.b, op.add);
    ops.append(op);
  }
  return ops;
}

// --- removeDegenerateShapes --------------------------------------------

namespace {

// A box-defined shape, wire or line record's two points and its kind;
// false for any other record.
bool cornersOf(const NodePtr &node, Point &p1, Point &p2, QString &word) {
  switch (node->getEnumID()) {
  case QPI::RECT: {
    const ItemRectPtr rec = std::static_pointer_cast<ItemRect>(node->item());
    if (rec == nullptr) return false;
    p1   = rec->pt1.getValue();
    p2   = rec->pt2.getValue();
    word = QStringLiteral("rect");
    return true;
  }
  case QPI::ELLIPSE: {
    const ItemEllipsePtr rec = std::static_pointer_cast<ItemEllipse>(node->item());
    if (rec == nullptr) return false;
    p1   = rec->pt1.getValue();
    p2   = rec->pt2.getValue();
    word = QStringLiteral("ellipse");
    return true;
  }
  case QPI::COIL: {
    const ItemCoilPtr rec = std::static_pointer_cast<ItemCoil>(node->item());
    if (rec == nullptr) return false;
    p1   = rec->pt1.getValue();
    p2   = rec->pt2.getValue();
    word = QStringLiteral("coil");
    return true;
  }
  case QPI::ZIGZAG: {
    const ItemZigzagPtr rec = std::static_pointer_cast<ItemZigzag>(node->item());
    if (rec == nullptr) return false;
    p1   = rec->pt1.getValue();
    p2   = rec->pt2.getValue();
    word = QStringLiteral("zigzag");
    return true;
  }
  case QPI::WIRE: {
    const ItemWirePtr rec = std::static_pointer_cast<ItemWire>(node->item());
    if (rec == nullptr) return false;
    p1   = rec->pt1.getValue();
    p2   = rec->pt2.getValue();
    word = QStringLiteral("wire");
    return true;
  }
  case QPI::LINE: {
    const ItemLinePtr rec = std::static_pointer_cast<ItemLine>(node->item());
    if (rec == nullptr) return false;
    p1   = rec->pt1.getValue();
    p2   = rec->pt2.getValue();
    word = QStringLiteral("line");
    return true;
  }
  default: return false;
  }
}

} // namespace

QList<ReplaceOp> planDegenerate(const NodePtr &root) {
  QList<ReplaceOp> ops;
  for (const NodePtr &child : root->children()) {
    Point   p1;
    Point   p2;
    QString word;
    if (!cornersOf(child, p1, p2, word)) continue;
    if (p1.x != p2.x || p1.y != p2.y) continue;

    ReplaceOp op;
    op.remove << child;
    op.text = QStringLiteral("remove %1 %2").arg(word, ptText(keyOf(p1)));
    ops.append(op);
  }
  return ops;
}

// --- mergeLines ----------------------------------------------------------

QList<ReplaceOp> planLines(const NodePtr &root) {
  QList<Seg>     segs;
  QList<NodePtr> nodes;
  QList<Run>     runs;
  for (const NodePtr &child : root->children()) {
    if (child->getEnumID() != QPI::LINE) continue;

    const ItemLinePtr rec = std::static_pointer_cast<ItemLine>(child->item());
    if (rec == nullptr) continue;

    const PointKey a = keyOf(rec->pt1.getValue());
    const PointKey b = keyOf(rec->pt2.getValue());
    if (a == b) continue;

    runs.append(Run{a, b, {static_cast<int>(segs.size())}, false});
    segs.append(Seg(a, b));
    nodes.append(child);
  }

  mergeOverlaps(runs, /*touching=*/true);
  return opsFromRuns(QStringLiteral("lines"), runs, segs, nodes);
}

// --- mergeOverlappingWires / mergeCollinearWires ------------------------

namespace {

void recordWireRuns(const NetModel &model, bool skipDegenerate, QList<Run> &runs,
                    QList<Seg> &segs, QList<NodePtr> &nodes) {
  for (const NetModel::Wire &wire : model.wires()) {
    if (skipDegenerate && wire.a == wire.b) continue;

    runs.append(Run{wire.a, wire.b, {static_cast<int>(segs.size())}, wire.bus});
    segs.append(Seg(wire.a, wire.b));
    nodes.append(wire.node);
  }
}

} // namespace

QList<ReplaceOp> planOverlaps(const NetModel &model) {
  QList<Run>     runs;
  QList<Seg>     segs;
  QList<NodePtr> nodes;
  recordWireRuns(model, /*skipDegenerate=*/true, runs, segs, nodes);
  mergeOverlaps(runs, /*touching=*/false);
  return opsFromRuns(QStringLiteral("overlap"), runs, segs, nodes);
}

QList<ReplaceOp> planChains(const NetModel &model) {
  // Pins and labels block; tap ends are tap points.
  QSet<PointKey> blocked;
  QSet<PointKey> tapPoints;

  for (const NetModel::Label &label : model.labels()) blocked.insert(label.at);
  for (const NetModel::Pin &pin : model.pins()) blocked.insert(pin.at);
  for (const NetModel::Tap &tap : model.taps()) {
    tapPoints.insert(tap.a);
    tapPoints.insert(tap.b);
  }

  QList<Run>     runs;
  QList<Seg>     segs;
  QList<NodePtr> nodes;
  recordWireRuns(model, /*skipDegenerate=*/false, runs, segs, nodes);
  mergeChains(runs, blocked, tapPoints);
  return opsFromRuns(QStringLiteral("join"), runs, segs, nodes);
}

QList<QPair<Point, Point>> cutAtPins(const Point &from, const Point &to,
                                     const NetModel &model) {
  if (from.x == to.x && from.y == to.y) return {qMakePair(from, to)};
  return cutAtPinList(from, to, recordPins(model));
}

namespace {

// The anchor for a net LABEL, from the directions its wires leave in.
//
// MEASURED against QSpice: 38 of 38 decidable labels across
// NetLabelAlignTests.qsch (22 labels, plain) and NetLabelTests.qsch (18,
// all ports -- a port's box does not change the choice). The rule:
//
//   1. Average the wire directions as unit vectors and point AWAY from
//      the result, snapped to the nearest AXIS, ties going vertical
//      (|y| >= |x|). Above, below, left or right -- never a corner. A
//      single diagonal wire, or an L-corner, puts the label straight
//      above or below.
//   2. If the directions CANCEL and there are exactly two -- a straight
//      line through the anchor -- the label goes on the line's upward
//      perpendicular: above a horizontal line, right of a vertical one,
//      and at the upward CORNER beside a diagonal one. The only case
//      that uses a corner.
//   3. Three or more cancelling (in practice four or six, in opposed
//      pairs): the first corner with NO wire leaving through it, in the
//      order up-right, up-left, down-right, down-left. If every corner
//      has a wire, alignment 0 -- centred on the anchor, which is what
//      QSpice writes for a diagonal cross.
//
// All 42 labels match: NetLabelAlignTests.qsch (24, including the
// four-way N26, the cross N27, and N29/N30 drawn to test rule 3) and
// NetLabelTests.qsch (18).
//
// Width plays no part. The chooser this replaces scored candidate boxes
// for overlap with the wires and preferred the averaged direction among
// all eight anchors; it matched only where the average happened to lie
// on an axis, and it skipped every straight-through case outright.
int labelAnchorFor(const QList<QPointF> &dirs) {
  QPointF sum(0.0, 0.0);
  for (const QPointF &d : dirs) {
    const double len = std::hypot(d.x(), d.y());
    if (len > 0.0) sum += d / len;
  }

  // Unit vectors from opposed wires cancel EXACTLY, so this is a
  // genuine zero test with room for rounding, not a threshold on "small".
  constexpr double kCancelled = 1e-9;

  if (std::hypot(sum.x(), sum.y()) > kCancelled) {
    const double awayX = -sum.x();
    const double awayY = -sum.y();

    if (std::fabs(awayY) >= std::fabs(awayX))
      return kAlignHCenter | (awayY > 0.0 ? kAlignVSouth : kAlignVNorth);

    return kAlignVCenter | (awayX > 0.0 ? kAlignHWest : kAlignHEast);
  }

  if (dirs.size() != 2) {
    // Rule 3. A corner is taken by a wire leaving exactly along it: equal
    // |dx| and |dy|, with matching signs.
    struct Corner {
      int sx;
      int sy;
      int align;
    };
    const Corner corners[] = {
        {+1, +1, kAlignHWest | kAlignVSouth},  // up-right
        {-1, +1, kAlignHEast | kAlignVSouth},  // up-left
        {+1, -1, kAlignHWest | kAlignVNorth},  // down-right
        {-1, -1, kAlignHEast | kAlignVNorth}}; // down-left

    for (const Corner &corner : corners) {
      bool taken = false;
      for (const QPointF &d : dirs) {
        if (!qFuzzyCompare(std::fabs(d.x()), std::fabs(d.y()))) continue;
        if (d.x() * corner.sx > 0.0 && d.y() * corner.sy > 0.0) taken = true;
      }
      if (!taken) return corner.align;
    }

    // Every corner occupied: centred on the anchor. Alignment 0 is
    // centre/centre, so this is a real placement, not "no answer".
    return 0;
  }

  // The line's UPWARD perpendicular; for a vertical line, whose
  // perpendicular is horizontal, the one to the RIGHT. Y is up.
  const QPointF d = dirs.first();
  double        px = -d.y();
  double        py = d.x();

  if (py < 0.0 || (py == 0.0 && px < 0.0)) {
    px = -px;
    py = -py;
  }

  // Snapped to the nearest of the eight directions: a horizontal line
  // gives straight up, a vertical one straight right, a 45-degree one
  // the corner between. 22.5 degrees either side of each axis, as for
  // the wire tool -- tan(22.5) = sqrt(2) - 1.
  constexpr double kTan22_5 = 0.41421356237309503;

  const double ax = std::fabs(px);
  const double ay = std::fabs(py);

  if (ax <= kTan22_5 * ay) return kAlignHCenter | kAlignVSouth; // above
  if (ay <= kTan22_5 * ax) return kAlignVCenter | kAlignHWest;  // right

  return kAlignVSouth | (px > 0.0 ? kAlignHWest : kAlignHEast); // corner
}

} // namespace

namespace {

// Disjoint sets over wire endpoints, keyed by coordinate.
class UnionFind {
public:
  int add(const PointKey &key) {
    const QHash<PointKey, int>::const_iterator it = index.constFind(key);
    if (it != index.constEnd()) return it.value();

    const int id = static_cast<int>(parent.size()); // qsizetype
    index.insert(key, id);
    parent.append(id);
    return id;
  }

  // The set a coordinate belongs to, or -1 if no wire touches it.
  int rootFor(const PointKey &key) {
    const QHash<PointKey, int>::const_iterator it = index.constFind(key);
    return (it == index.constEnd()) ? -1 : find(it.value());
  }

  int find(int id) {
    while (parent[id] != id) {
      parent[id] = parent[parent[id]]; // halve the path as we go
      id         = parent[id];
    }
    return id;
  }

  void join(int a, int b) {
    const int ra = find(a);
    const int rb = find(b);
    if (ra != rb) parent[rb] = ra;
  }

private:
  QHash<PointKey, int> index;
  QList<int>           parent;
};

// "N01", "N02", ... skipping every name already taken by a label.
//
// Compared case-insensitively: QSpice treats net names that way, so a
// user label "n02" must stop the generator handing out "N02" as well.
class NetNamer {
public:
  explicit NetNamer(const QSet<QString> &reservedLower)
      : reserved(reservedLower) {}

  QString next() {
    for (;;) {
      const QString name = QStringLiteral("N%1").arg(++counter, 2, 10,
                                                     QLatin1Char('0'));
      if (reserved.contains(name.toLower())) continue;

      reserved.insert(name.toLower());
      return name;
    }
  }

private:
  QSet<QString> reserved;
  int           counter = 0;
};

} // namespace

// --- nameNets: the decision ---------------------------------------------

namespace {

struct NameWire {
  NodePtr  node;
  bool     ok = false; // has two ends
  PointKey a;
  PointKey b;
  QString  raw;  // as stored
  QString  name; // decoded, trimmed
};

struct NameLabel {
  bool     ok = false; // has an anchor
  PointKey at;
  QString  name; // decoded, trimmed
};

// Wires and labels in RECORD order -- order decides which label wins a
// net carrying several, and the sequence auto names are handed out in.
QList<Rename> decideNames(const QList<NameWire> &wires, const QList<NameLabel> &labels) {
  if (wires.isEmpty()) return {};

  // --- Group the wires ---
  UnionFind  sets;
  QList<int> wireSet;
  for (const NameWire &wire : wires) {
    if (!wire.ok) {
      wireSet.append(-1);
      continue;
    }

    const int a = sets.add(wire.a);
    const int b = sets.add(wire.b);
    sets.join(a, b);
    wireSet.append(a);
  }

  // --- Reserve every label's name, then let labels claim their sets ---
  QSet<QString> reserved;
  for (const NameLabel &label : labels)
    if (!label.name.isEmpty()) reserved.insert(label.name.toLower());

  QHash<int, QString> nameOfSet;
  for (const NameLabel &label : labels) {
    if (label.name.isEmpty() || !label.ok) continue;

    const int root = sets.rootFor(label.at);
    if (root < 0) continue; // a label on no wire at all

    // FIRST label wins: later ones find the set already named.
    if (!nameOfSet.contains(root)) nameOfSet.insert(root, label.name);
  }

  // --- Name what is left ---
  NetNamer namer(reserved);

  QList<Rename> out;
  for (int i = 0; i < wires.size(); ++i) {
    if (wireSet[i] < 0) continue;

    const int root = sets.find(wireSet[i]);
    if (!nameOfSet.contains(root)) nameOfSet.insert(root, namer.next());

    const QString want = nameOfSet.value(root);

    // Compared as STORED, not as decoded: `N01` and `"N01"` decode
    // alike, and a wire written unquoted by an older build would never
    // be repaired.
    const QString wantRaw =
        QString::fromStdString(ArgString(encodeQSpiceString(want)).quote().getValue());
    if (wires[i].raw == wantRaw) continue;

    Rename rename;
    rename.wire    = wires[i].node;
    rename.oldName = wires[i].name;
    rename.newName = want;
    rename.text    = QStringLiteral("rename %1 %2 -> %3")
                      .arg(segText(wires[i].a, wires[i].b), quotedText(wires[i].name),
                           quotedText(want));
    out.append(rename);
  }
  return out;
}

} // namespace

QList<Rename> planNames(const NetModel &model) {
  QList<NameWire> nameWires;
  for (const NetModel::Wire &wire : model.wires()) {
    NameWire in;
    in.node = wire.node;
    in.ok   = true;
    in.a    = wire.a;
    in.b    = wire.b;
    in.raw  = wire.netNameRaw;
    in.name = wire.netName;
    nameWires.append(in);
  }

  QList<NameLabel> nameLabels;
  for (const NetModel::Label &label : model.labels()) {
    NameLabel in;
    in.ok   = true;
    in.at   = label.at;
    in.name = label.netName;
    nameLabels.append(in);
  }

  return decideNames(nameWires, nameLabels);
}

// --- updtNetAnchors: the decision ---------------------------------------

namespace {

// The directions wires and taps leave each point in, and the wire spans
// a label can sit partway along.
struct AnchorDirs {
  QHash<PointKey, QList<QPointF>> atEnds;
  QList<QPair<QPointF, QPointF>>  spans;
};

// A wire (or, with `isWire` false, a TAP -- a direction at its ends
// only: QSpice anchored the Bus[1] label touching nothing but a tap
// exactly as if it were a wire).
void addDirs(AnchorDirs &dirs, const QPointF &a, const QPointF &b, bool isWire) {
  dirs.atEnds[keyFor(a)].append(b - a);
  dirs.atEnds[keyFor(b)].append(a - b);
  if (isWire) dirs.spans.append(qMakePair(a, b));
}

// Strictly BETWEEN a span's ends: both ways along the wire, which cancel
// when averaged and so put the label beside it.
QList<QPointF> midSpanDirs(const AnchorDirs &dirs, const QPointF &at) {
  for (const QPair<QPointF, QPointF> &span : dirs.spans) {
    const QPointF vec = span.second - span.first;
    const QPointF rel = at - span.first;

    const double cross = vec.x() * rel.y() - vec.y() * rel.x();
    if (std::fabs(cross) > 1e-6) continue; // not on the line

    const double along = vec.x() * rel.x() + vec.y() * rel.y();
    const double len2  = vec.x() * vec.x() + vec.y() * vec.y();
    if (along <= 0.0 || along >= len2) continue; // at or past an end

    return {vec, -vec};
  }
  return {};
}

} // namespace

// Where an unconnected net item is anchored: a label at CENTRE-BOTTOM
// (the text sits above its anchor), a ground at CENTRE-TOP (NORTH -- the
// symbol hangs DOWN from its anchor, the way a ground is drawn).
int unconnectedAlign(bool ground) {
  return kAlignHCenter | (ground ? kAlignVNorth : kAlignVSouth);
}

// Grounds still in their unconnected orientation. A ground connected by
// a gesture that did not move it -- a wire drawn to it, a wire dragged
// onto it -- is not "touched" by that gesture, yet has to turn to face
// the wire; one still in the default it got while unconnected has never
// been oriented, so turning it changes nothing anyone chose. Grounds
// already facing a wire are left alone, as labels are.
QList<NodePtr> unorientedGrounds(const NodePtr &root) {
  QList<NodePtr> out;
  if (root == nullptr) return out;

  for (const NodePtr &child : root->children()) {
    if (child->getEnumID() != QPI::NET) continue;

    const ItemNetPtr rec = std::static_pointer_cast<ItemNet>(child->item());
    if (rec == nullptr) continue;
    if (!isGroundName(decodeQSpiceString(rec->netName.getValue()))) continue;
    if (rec->rotateAlign.getAlign() != unconnectedAlign(true)) continue;

    out.append(child);
  }
  return out;
}

namespace {

// The alignment a label at `anchor` should have; `current` when it
// should be left as it is.
int anchorWanted(const AnchorDirs &all, const QPointF &anchor, bool ground, int current) {
  QList<QPointF> dirs = all.atEnds.value(keyFor(anchor));

  // No wire ENDS here, so try one running past.
  if (dirs.isEmpty()) dirs = midSpanDirs(all, anchor);

  // Nothing attached: back to the unconnected default -- reset, not left
  // with an anchor chosen for geometry that is gone.
  if (dirs.isEmpty()) return unconnectedAlign(ground);

  // The same axis snap for a label, plus the cancelling cases a ground
  // has no answer for -- see labelAnchorFor().
  if (!ground) return labelAnchorFor(dirs);

  // A ground: averaged as UNIT vectors, so a long wire doesn't outvote
  // a short one.
  QPointF sum(0.0, 0.0);
  for (const QPointF &d : dirs) {
    const double len = std::hypot(d.x(), d.y());
    if (len > 0.0) sum += d / len;
  }

  // Opposed wires give no direction to face; leave it as it is.
  if (qFuzzyIsNull(sum.x()) && qFuzzyIsNull(sum.y())) return current;

  const double awayX = -sum.x();
  const double awayY = -sum.y();

  // Cardinal directions only -- the graphic has no 45-degree variants --
  // ties going VERTICAL. Facing down puts the anchor on the symbol's TOP
  // edge, which is NORTH: the alignment names where the anchor sits.
  if (std::fabs(awayY) >= std::fabs(awayX))
    return kAlignHCenter | (awayY < 0.0 ? kAlignVNorth : kAlignVSouth);

  return kAlignVCenter | (awayX < 0.0 ? kAlignHEast : kAlignHWest);
}

void addRealign(QList<Realign> &out, const NodePtr &net, const PointKey &at, int current,
                int wanted) {
  if (wanted == current) return;

  Realign realign;
  realign.net      = net;
  realign.oldAlign = current;
  realign.newAlign = wanted;
  realign.text     = QStringLiteral("align %1 %2 -> %3").arg(ptText(at)).arg(current).arg(wanted);
  out.append(realign);
}

} // namespace

QList<Realign> planAnchors(const NetModel &model, const QList<NodePtr> &touchedNets) {
  // Every wire and tap.
  AnchorDirs dirs;
  for (const NetModel::Wire &wire : model.wires())
    addDirs(dirs, QPointF(wire.a.x, wire.a.y), QPointF(wire.b.x, wire.b.y), true);
  for (const NetModel::Tap &tap : model.taps())
    addDirs(dirs, QPointF(tap.a.x, tap.a.y), QPointF(tap.b.x, tap.b.y), false);

  // The touched labels, read from their RECORDS.
  QList<Realign> out;
  for (const NodePtr &net : touchedNets) {
    if (net == nullptr || net->getEnumID() != QPI::NET) continue;

    const ItemNetPtr rec = std::static_pointer_cast<ItemNet>(net->item());
    if (rec == nullptr) continue;

    const PointKey at      = keyOf(rec->pt1.getValue());
    const int      current = rec->rotateAlign.getAlign();
    const bool     ground  = isGroundName(decodeQSpiceString(rec->netName.getValue()));
    addRealign(out, net, at, current, anchorWanted(dirs, QPointF(at.x, at.y), ground, current));
  }
  return out;
}

} // namespace sch::core::connplan
