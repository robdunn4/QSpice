#pragma once

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>

#include <utility>

#include "ArgUtils.h"   // Point
#include "ItemTree.h"
#include "SchPointKey.h"

// A sheet's connectivity, computed from the RECORD TREE alone -- no
// scene, no QGraphicsItem.
//
// Step 2 of the connectivity rework built it and checked it against the
// scene-based code. Step 3: the DISPLAY reads it -- pin and wire-end
// marks, tap state, a pin's net name, and the counts junctions are
// derived from -- through the copy SchScene caches. Step 5: the editing
// passes plan from it. Step 5b (2026-10-07) retired the scene-based
// connectivity code and its Debug comparisons: this is now THE rule.
//
// Every rule here began as a COPY of the scene code's rule, quirks
// included -- see each. Those quirks are now ours to change, but each
// one matched QSpice or was kept on purpose, so change it knowingly.
//
// The DOCUMENT only: a placement preview or a paste not yet dropped is
// not in the tree, so it neither connects nor is connected. Queries
// about such an element (its record is not in the model) are answered
// against the document by point.
//
// Coordinates are integer scene units. A component's symbol content is
// placed with rotdecode::apply(), the integer form of the transform
// SchItemComp gives its children.
//
// Elements are the sheet root's direct children; a component's
// electrical content is its SYM record's children. Built once, from a
// snapshot -- rebuild after any change. No moc.
namespace sch::core {

class NetModel {
public:
  // A bus tap's appearance -- see SchItemTap.
  enum class TapState { Loose, Valid, Invalid };

  struct Wire {
    NodePtr  node;
    PointKey a;
    PointKey b;
    QString  netName;    // decoded, trimmed
    QString  netNameRaw; // as stored, quotes and all
    bool     bus = false;
  };

  struct Pin {
    NodePtr  node;
    PointKey at;
    int      comp   = -1; // index into comps(); -1 for a sheet pin
    int      number = 0;  // 1-based among the symbol's PIN records
    QString  label;       // decoded, as written
    bool     internal = false; // carries its own net name
  };

  // A component. Its symbol's only ELECTRICAL content is pins: a
  // symbol may not hold wires, taps, net labels or junctions, and any
  // that a file brings are removed before the tree is used
  // (SchRecordRules).
  struct Comp {
    NodePtr    node;
    QString    refdes; // empty if none
    QList<int> pins;   // indices into pins()
  };

  struct Label {
    NodePtr  node;
    PointKey at;
    QString  netName; // decoded, trimmed
  };

  struct Tap {
    NodePtr  node;
    PointKey a;
    PointKey b;
  };

  struct Junction {
    NodePtr  node;
    PointKey at;
  };

  // A connected group: a wire, or two or more members touching. A lone
  // pin is not a net.
  struct Net {
    QList<int>    wires;     // wires() (buses included)
    QList<int>    pins;      // pins() -- component and sheet pins
    QList<int>    labels;    // labels()
    QList<int>    tapEnds;   // taps() index * 2 + end (0 = a, 1 = b)
    QSet<QString> wireNames; // as the wires carry them
    QSet<QString> labelNames;
  };

  explicit NetModel(const NodePtr &root);

  // --- The elements.
  const QList<Wire>     &wires() const { return wires_; }
  const QList<Pin>      &pins() const { return pins_; }
  const QList<Comp>     &comps() const { return comps_; }
  const QList<Label>    &labels() const { return labels_; }
  const QList<Tap>      &taps() const { return taps_; }
  const QList<Junction> &junctions() const { return junctions_; }

  // --- The results.
  const QList<Net> &nets() const { return nets_; }

  // Non-bus wire endpoints at each point. kJunctionMinEndpoints or more
  // needs a dot.
  const QHash<PointKey, int> &wireEndCounts() const { return wireEnds_; }
  QList<PointKey>             requiredJunctions() const;

  // Pins showing the unconnected mark: not internal, and nothing else
  // electrical at the point (SchItemPin::showsUnconnectedMark()).
  // Component pins only. Indices into pins().
  const QList<int> &unconnectedPins() const { return unconnected_; }

  // Ends of non-bus wires with nothing else electrical there, one entry
  // per end.
  const QList<PointKey> &looseEnds() const { return looseEnds_; }

  // --- Display queries. Each takes the element's record and, for one
  // not in the model, the point to judge it by.

  // Whether a pin shows the unconnected mark. The caller has already
  // excluded internal pins, which show nothing.
  bool pinMarked(const NodePtr &pinNode, const PointKey &at) const;

  // Whether end `end` (0 = pt1, 1 = pt2) of a wire has nothing else
  // electrical at it. Buses too -- a SELECTED bus shows its ends.
  bool wireEndLoose(const NodePtr &wireNode, int end, const PointKey &at) const;

  // A tap's state from its two ends: a bus at one end -- at a bus
  // wire's end or anywhere along it -- and at the other a net that is,
  // or is not, one of that bus's members.
  TapState tapState(const PointKey &p1, const PointKey &p2) const;

  // The net name at a point: the first wire ending there that carries
  // one, else the first net label anchored there. Empty if neither.
  QString netNameAt(const PointKey &at) const;

protected:
  void readSheet(const NodePtr &root);
  void readComp(const NodePtr &compNode);
  void findNets();
  void findMarks();

  QList<Wire>     wires_;
  QList<Pin>      pins_;
  QList<Comp>     comps_;
  QList<Label>    labels_;
  QList<Tap>      taps_;
  QList<Junction> junctions_;

  QList<Net>           nets_;
  QHash<PointKey, int> wireEnds_;
  QList<int>           unconnected_;
  QList<PointKey>      looseEnds_;

  // Every electrical point of the document -- wire and tap ends, label
  // anchors, pins -- with multiplicity.
  QHash<PointKey, int> connPoints_;

  // By record: which pins show the mark (component and sheet pins), and
  // each wire's loose ends (bit 0 = pt1, bit 1 = pt2).
  QSet<const TreeNode *>       markedPins_;
  QHash<const TreeNode *, int> wireLoose_;
  QSet<const TreeNode *>       pinNodes_;
};

} // namespace sch::core
