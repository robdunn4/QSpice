#include "SchNetModel.h"

#include <memory>

#include "ItemCmp.h"
#include "ItemJunction.h"
#include "ItemNet.h"
#include "ItemPin.h"
#include "ItemTap.h"
#include "ItemWire.h"
#include "SchBus.h"
#include "SchRefDes.h"
#include "SchRotDecode.h"
#include "SchStringDecode.h"

namespace sch::core {

namespace {

PointKey keyOf(const Point &p) { return PointKey{p.x, p.y}; }

// A symbol-local point, placed for its component.
PointKey placed(int code, const Point &pos, const Point &local) {
  return keyOf(rotdecode::apply(code, pos, local));
}

// Groups points: a wire joins its two ends, nothing else joins. The
// same union-find the dump has always used.
class PointSets {
public:
  int add(const PointKey &key) {
    const QHash<PointKey, int>::const_iterator it = index.constFind(key);
    if (it != index.constEnd()) return it.value();
    const int id = static_cast<int>(parent.size());
    index.insert(key, id);
    parent.append(id);
    return id;
  }

  int find(int id) {
    while (parent[id] != id) {
      parent[id] = parent[parent[id]];
      id         = parent[id];
    }
    return id;
  }

  int rootOf(const PointKey &key) { return find(add(key)); }

  void join(const PointKey &a, const PointKey &b) {
    const int ra = find(add(a));
    const int rb = find(add(b));
    if (ra != rb) parent[rb] = ra;
  }

protected:
  QHash<PointKey, int> index;
  QList<int>           parent;
};

void addAll(QHash<PointKey, int> &counts, const QList<PointKey> &points, int delta) {
  for (const PointKey &p : points) counts[p] += delta;
}

} // namespace

NetModel::NetModel(const NodePtr &root) {
  if (root == nullptr) return;

  readSheet(root);
  findNets();
  findMarks();
}

QList<PointKey> NetModel::requiredJunctions() const {
  QList<PointKey> out;
  for (QHash<PointKey, int>::const_iterator it = wireEnds_.constBegin();
       it != wireEnds_.constEnd(); ++it)
    if (it.value() >= kJunctionMinEndpoints) out.append(it.key());
  return out;
}

void NetModel::readSheet(const NodePtr &root) {
  for (const NodePtr &child : root->children()) {
    switch (child->getEnumID()) {
    case QPI::WIRE: {
      const ItemWirePtr rec = std::static_pointer_cast<ItemWire>(child->item());
      if (rec == nullptr) break;
      Wire wire;
      wire.node    = child;
      wire.a       = keyOf(rec->pt1.getValue());
      wire.b       = keyOf(rec->pt2.getValue());
      wire.netName    = decodeQSpiceString(rec->netName.getValue()).trimmed();
      wire.netNameRaw = QString::fromStdString(rec->netName.getValue());
      wire.bus     = isBusName(wire.netName);
      wires_.append(wire);
      break;
    }
    case QPI::COMP: readComp(child); break;
    case QPI::NET: {
      const ItemNetPtr rec = std::static_pointer_cast<ItemNet>(child->item());
      if (rec == nullptr) break;
      labels_.append(Label{child, keyOf(rec->pt1.getValue()),
                           decodeQSpiceString(rec->netName.getValue()).trimmed()});
      break;
    }
    case QPI::TAP: {
      const ItemTapPtr rec = std::static_pointer_cast<ItemTap>(child->item());
      if (rec == nullptr) break;
      taps_.append(Tap{child, keyOf(rec->pt1.getValue()), keyOf(rec->pt2.getValue())});
      break;
    }
    case QPI::JUNCTION: {
      const ItemJunctionPtr rec = std::static_pointer_cast<ItemJunction>(child->item());
      if (rec == nullptr) break;
      junctions_.append(Junction{child, keyOf(rec->pt1.getValue())});
      break;
    }
    case QPI::PIN: { // a symbol document's own pins
      const ItemPinPtr rec = std::static_pointer_cast<ItemPin>(child->item());
      if (rec == nullptr) break;
      Pin pin;
      pin.node     = child;
      pin.at       = keyOf(rec->pt1.getValue());
      pin.label    = decodeQSpiceString(rec->pinLabel.getValue());
      pin.internal = !decodeQSpiceString(rec->netName.getValue()).isEmpty();
      pins_.append(pin);
      break;
    }
    default: break;
    }
  }
}

void NetModel::readComp(const NodePtr &compNode) {
  const ItemCmpPtr rec = std::static_pointer_cast<ItemCmp>(compNode->item());
  if (rec == nullptr) return;

  const int   compIndex = static_cast<int>(comps_.size());
  const int   code      = rec->rotate.getValue();
  const Point pos       = rec->pt1.getValue();

  Comp comp;
  comp.node   = compNode;
  comp.refdes = refdes::ofNode(compNode);

  const NodePtr symNode = compNode->findFirstOf(QPI::SYM);
  int           number  = 0;

  // Pins only: nothing else electrical may sit in a symbol (see Comp).
  if (symNode != nullptr) {
    for (const NodePtr &child : symNode->children()) {
      if (child->getEnumID() != QPI::PIN) continue;

      ++number; // counted even if the record is unreadable, as the dump counts
      const ItemPinPtr pinRec = std::static_pointer_cast<ItemPin>(child->item());
      if (pinRec == nullptr) continue;

      Pin pin;
      pin.node     = child;
      pin.at       = placed(code, pos, pinRec->pt1.getValue());
      pin.comp     = compIndex;
      pin.number   = number;
      pin.label    = decodeQSpiceString(pinRec->pinLabel.getValue());
      pin.internal = !decodeQSpiceString(pinRec->netName.getValue()).isEmpty();
      comp.pins.append(static_cast<int>(pins_.size()));
      pins_.append(pin);
    }
  }

  comps_.append(comp);
}

void NetModel::findNets() {
  for (const Wire &wire : wires_) {
    if (!wire.bus) {
      wireEnds_[wire.a]++;
      wireEnds_[wire.b]++;
    }
  }

  PointSets sets;
  for (const Wire &wire : wires_) sets.join(wire.a, wire.b);

  // Members are placed against a POINT; groups are read off once every
  // wire has joined its ends.
  QHash<int, Net> byRoot;
  QHash<int, int> memberCount;
  QSet<int>       hasWire;

  for (int i = 0; i < wires_.size(); ++i) {
    const int root = sets.rootOf(wires_[i].a);
    byRoot[root].wires.append(i);
    byRoot[root].wireNames.insert(wires_[i].netName);
    memberCount[root]++;
    hasWire.insert(root);
  }
  for (int i = 0; i < pins_.size(); ++i) {
    const int root = sets.rootOf(pins_[i].at);
    byRoot[root].pins.append(i);
    memberCount[root]++;
  }
  for (int i = 0; i < labels_.size(); ++i) {
    const int root = sets.rootOf(labels_[i].at);
    byRoot[root].labels.append(i);
    if (!labels_[i].netName.isEmpty()) byRoot[root].labelNames.insert(labels_[i].netName);
    memberCount[root]++;
  }
  for (int i = 0; i < taps_.size(); ++i) {
    const PointKey ends[2] = {taps_[i].a, taps_[i].b};
    for (int e = 0; e < 2; ++e) {
      const int root = sets.rootOf(ends[e]);
      byRoot[root].tapEnds.append(i * 2 + e);
      memberCount[root]++;
    }
  }

  for (QHash<int, Net>::const_iterator it = byRoot.constBegin(); it != byRoot.constEnd(); ++it) {
    if (!hasWire.contains(it.key()) && memberCount.value(it.key()) < 2) continue;
    nets_.append(it.value());
  }
}

void NetModel::findMarks() {
  // Every electrical point, with multiplicity: wire and tap ends, label
  // anchors, and every pin, component and sheet alike. Junctions are
  // not counted -- derived from the wires, they would make every
  // junction look like a connection to itself.
  QHash<PointKey, int> &all = connPoints_;
  for (const Wire &wire : wires_) addAll(all, {wire.a, wire.b}, 1);
  for (const Label &label : labels_) addAll(all, {label.at}, 1);
  for (const Tap &tap : taps_) addAll(all, {tap.a, tap.b}, 1);
  for (const Pin &pin : pins_) addAll(all, {pin.at}, 1);

  for (const Pin &pin : pins_) pinNodes_.insert(pin.node.get());

  // A pin is marked when nothing but itself is at its point. Another pin
  // there -- of the same component or another -- connects it, as it
  // always has.
  for (int index = 0; index < pins_.size(); ++index) {
    const Pin &pin = pins_[index];
    if (pin.internal) continue;
    if (all.value(pin.at) - 1 > 0) continue; // -1: the pin's own entry

    markedPins_.insert(pin.node.get());
    if (pin.comp >= 0) unconnected_.append(index); // component pins only
  }

  // A wire end: not the wire itself. Buses are recorded too, for the
  // display, but left out of looseEnds() -- as on screen, an unselected
  // bus shows no marks.
  for (const Wire &wire : wires_) {
    addAll(all, {wire.a, wire.b}, -1);
    const bool looseA = all.value(wire.a) <= 0;
    const bool looseB = all.value(wire.b) <= 0;
    addAll(all, {wire.a, wire.b}, 1);

    wireLoose_.insert(wire.node.get(), (looseA ? 1 : 0) | (looseB ? 2 : 0));
    if (wire.bus) continue;
    if (looseA) looseEnds_.append(wire.a);
    if (looseB) looseEnds_.append(wire.b);
  }
}

bool NetModel::pinMarked(const NodePtr &pinNode, const PointKey &at) const {
  if (pinNodes_.contains(pinNode.get())) return markedPins_.contains(pinNode.get());

  // Not in the document: marked if the document has nothing there.
  return connPoints_.value(at) <= 0;
}

bool NetModel::wireEndLoose(const NodePtr &wireNode, int end, const PointKey &at) const {
  const QHash<const TreeNode *, int>::const_iterator it = wireLoose_.constFind(wireNode.get());
  if (it != wireLoose_.constEnd()) return (it.value() & (end == 0 ? 1 : 2)) != 0;

  return connPoints_.value(at) <= 0;
}

namespace {

// Whether p lies on segment a-b, ends included. Exact integer test, as
// SchItemTap has always used.
bool onSegment(const PointKey &p, const PointKey &a,
               const PointKey &b) {
  const qint64 dx = static_cast<qint64>(b.x) - a.x;
  const qint64 dy = static_cast<qint64>(b.y) - a.y;
  const qint64 rx = static_cast<qint64>(p.x) - a.x;
  const qint64 ry = static_cast<qint64>(p.y) - a.y;

  if (dx * ry - dy * rx != 0) return false; // not on the line

  const qint64 along = dx * rx + dy * ry;
  return along >= 0 && along <= dx * dx + dy * dy;
}

// The bus at a point -- at a bus wire's end OR anywhere along it, since
// a tap attaches mid-span without splitting the bus. The first in record
// order.
bool busAt(const QList<NetModel::Wire> &wires, const PointKey &p,
           BusName &bus) {
  for (const NetModel::Wire &wire : wires) {
    if (!onSegment(p, wire.a, wire.b)) continue;
    if (parseBusName(wire.netName, bus)) return true;
  }
  return false;
}

// The net at a tap's MEMBER end: an ordinary wire through or ending at
// the point, else a net label anchored there. QSpice's own sample has
// both -- Bus[1] is reached once by a wire and once by a bare label.
bool netAt(const QList<NetModel::Wire> &wires, const QList<NetModel::Label> &labels,
           const PointKey &p, QString &net) {
  for (const NetModel::Wire &wire : wires) {
    if (wire.bus || !onSegment(p, wire.a, wire.b)) continue;
    net = wire.netName;
    return true;
  }
  for (const NetModel::Label &label : labels) {
    if (!(label.at == p)) continue;
    net = label.netName;
    return true;
  }
  return false;
}

} // namespace

NetModel::TapState NetModel::tapState(const PointKey &p1, const PointKey &p2) const {
  BusName bus1;
  BusName bus2;
  const bool on1 = busAt(wires_, p1, bus1);
  const bool on2 = busAt(wires_, p2, bus2);

  // A bus at BOTH ends is not a valid tap; neither is no bus at all.
  if (on1 == on2) return TapState::Loose;

  const BusName &bus    = on1 ? bus1 : bus2;
  const PointKey   &member = on1 ? p2 : p1;

  // Nothing at the member end: loose, not merely invalid.
  QString net;
  if (!netAt(wires_, labels_, member, net)) return TapState::Loose;

  return isBusMember(net, bus) ? TapState::Valid : TapState::Invalid;
}

QString NetModel::netNameAt(const PointKey &at) const {
  for (const Wire &wire : wires_) {
    if (!(wire.a == at) && !(wire.b == at)) continue;
    if (!wire.netName.isEmpty()) return wire.netName;
  }
  for (const Label &label : labels_) {
    if (!(label.at == at)) continue;
    if (!label.netName.isEmpty()) return label.netName;
  }
  return QString();
}

} // namespace sch::core
