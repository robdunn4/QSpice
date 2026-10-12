#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemTap.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPen>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cmath>
#include <cstdlib>
#include <utility>

#include "RenderStyle.h"
#include "SchBus.h"
#include "SchItemNet.h"
#include "SchPen.h"
#include "SchItemWire.h"
#include "SchScene.h"

namespace sch::render {

namespace {

// Click tolerance either side of the centreline, in schematic units --
// the same allowance a wire gets, which also covers the widest shape
// (a diagonal spindle is 14 units either side).
constexpr double kTapClickTolerance = 20.0;

// Whether p lies on segment a-b, ends included. Exact integer test:
// coincidence in QSpice is exact, never within a tolerance.
bool onSegment(const Point &p, const Point &a, const Point &b) {
  const qint64 dx = static_cast<qint64>(b.x) - a.x;
  const qint64 dy = static_cast<qint64>(b.y) - a.y;
  const qint64 rx = static_cast<qint64>(p.x) - a.x;
  const qint64 ry = static_cast<qint64>(p.y) - a.y;

  if (dx * ry - dy * rx != 0) return false; // not on the line

  const qint64 along = dx * rx + dy * ry;
  return along >= 0 && along <= dx * dx + dy * dy;
}

// The top-level wires -- symbol geometry has none, but the filter keeps
// this honest if that ever changes.
QList<SchItemWire *> topLevelWires(QGraphicsScene *scene) {
  QList<SchItemWire *> wires;
  for (QGraphicsItem *item : scene->items()) {
    if (item->parentItem() != nullptr) continue;
    if (SchItemWire *wire = dynamic_cast<SchItemWire *>(item))
      wires.append(wire);
  }
  return wires;
}

// The bus a point lies on -- at a bus wire's end OR anywhere along it,
// since a tap attaches mid-span without splitting the bus.
bool busAt(QGraphicsScene *scene, const Point &p, sch::core::BusName &bus) {
  for (SchItemWire *wire : topLevelWires(scene)) {
    const QList<Point> ends = wire->controlPoints();
    if (ends.size() < 2 || !onSegment(p, ends[0], ends[1])) continue;
    if (sch::core::parseBusName(wire->netName(), bus)) return true;
  }
  return false;
}

// The net at a tap's MEMBER end: an ordinary wire through or ending at
// the point, else a net label anchored there. QSpice's own sample has
// both -- Bus[1] is reached once by a wire and once by a bare label.
bool netAt(QGraphicsScene *scene, const Point &p, QString &net) {
  for (SchItemWire *wire : topLevelWires(scene)) {
    if (wire->isBus()) continue;

    const QList<Point> ends = wire->controlPoints();
    if (ends.size() < 2 || !onSegment(p, ends[0], ends[1])) continue;

    net = wire->netName();
    return true;
  }

  for (QGraphicsItem *item : scene->items()) {
    if (item->parentItem() != nullptr) continue;

    const SchItemNet *label = dynamic_cast<const SchItemNet *>(item);
    if (label == nullptr) continue;

    const Point at = label->getPosition();
    if (at.x != p.x || at.y != p.y) continue;

    net = label->netName();
    return true;
  }

  return false;
}

} // namespace

SchItemTap::SchItemTap(ItemTapPtr item, QGraphicsItem *parent)
    : SchItemBase(parent), item_(std::move(item)) {
  // Selected and moved whole, like a wire; a symbol has no taps.
  const bool topLevel = (parent == nullptr);
  setFlag(QGraphicsItem::ItemIsSelectable, topLevel);
  setFlag(QGraphicsItem::ItemIsMovable, topLevel);
  setZValue(kZGraphic);
}

Rect SchItemTap::getRect() const noexcept {
  // Everything either shape can reach: both ends, each pushed a tenth of
  // the tap's length out to either side.
  const Point p1 = item_->pt1.getValue();
  const Point p2 = item_->pt2.getValue();

  const double nx = -(p2.y - p1.y) * 0.1;
  const double ny = (p2.x - p1.x) * 0.1;

  double minX = qMin(p1.x, p2.x) - std::fabs(nx);
  double maxX = qMax(p1.x, p2.x) + std::fabs(nx);
  double minY = qMin(p1.y, p2.y) - std::fabs(ny);
  double maxY = qMax(p1.y, p2.y) + std::fabs(ny);

  return Rect(Point(static_cast<int>(std::floor(minX)),
                    static_cast<int>(std::ceil(maxY))),
              Point(static_cast<int>(std::ceil(maxX)),
                    static_cast<int>(std::floor(minY))));
}

Point SchItemTap::getPosition() const noexcept { return item_->pt1.getValue(); }

void SchItemTap::moveTo(Point p) noexcept {
  prepareGeometryChange();
  const Point delta = p - getPosition();
  item_->pt1.setValue(item_->pt1.getValue() + delta);
  item_->pt2.setValue(item_->pt2.getValue() + delta);
}

Point SchItemTap::end(int index) const {
  return (index == 0) ? item_->pt1.getValue() : item_->pt2.getValue();
}

void SchItemTap::setControlPoint(int index, Point p) {
  prepareGeometryChange();
  if (index == 0) item_->pt1.setValue(p);
  else if (index == 1) item_->pt2.setValue(p);
}

int SchItemTap::pivotIndex() const {
  const Point p1 = item_->pt1.getValue();
  const Point p2 = item_->pt2.getValue();

  if (p1.y == p2.y) return (p1.x <= p2.x) ? 0 : 1; // horizontal: left end

  return (p1.y > p2.y) ? 0 : 1; // otherwise the top-most (Y is up)
}

Point SchItemTap::turned45(Point v, bool ccw) {
  const int len = qMax(std::abs(v.x), std::abs(v.y));
  if (len == 0) return v;

  // The eight directions, counter-clockwise from east, Y up.
  static const int kDirs[8][2] = {{1, 0},  {1, 1},   {0, 1},  {-1, 1},
                                  {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};

  const int sx = (v.x > 0) - (v.x < 0);
  const int sy = (v.y > 0) - (v.y < 0);

  for (int i = 0; i < 8; ++i) {
    if (kDirs[i][0] != sx || kDirs[i][1] != sy) continue;

    const int j = (i + (ccw ? 1 : 7)) % 8;
    return Point(kDirs[j][0] * len, kDirs[j][1] * len);
  }

  return v; // not one of the eight -- left as it is
}

QList<QPointF> SchItemTap::connectionPoints() const {
  return {toQ(item_->pt1.getValue()), toQ(item_->pt2.getValue())};
}

QPainterPath SchItemTap::shape() const {
  QPainterPath path;
  path.moveTo(toQ(item_->pt1.getValue()));
  path.lineTo(toQ(item_->pt2.getValue()));

  QPainterPathStroker stroker;
  stroker.setWidth(kTapClickTolerance * 2.0);
  stroker.setCapStyle(Qt::RoundCap);
  return stroker.createStroke(path);
}

SchItemTap::State SchItemTap::state() const {
  if (scene() == nullptr) return State::Loose;

  const Point p1 = item_->pt1.getValue();
  const Point p2 = item_->pt2.getValue();

  // From the document's connectivity model where there is one: the same
  // rule, against the sheet's records instead of every item in the
  // scene. Record points, exactly as the scene search below uses them.
  if (SchScene *sc = SchScene::of(this)) {
    if (const sch::core::NetModel *model = sc->netModel()) {
      switch (model->tapState(sch::core::PointKey{p1.x, p1.y},
                              sch::core::PointKey{p2.x, p2.y})) {
      case sch::core::NetModel::TapState::Valid: return State::Valid;
      case sch::core::NetModel::TapState::Invalid: return State::Invalid;
      case sch::core::NetModel::TapState::Loose: return State::Loose;
      }
      return State::Loose;
    }
  }

  sch::core::BusName bus1;
  sch::core::BusName bus2;
  const bool on1 = busAt(scene(), p1, bus1);
  const bool on2 = busAt(scene(), p2, bus2);

  // A bus at BOTH ends is not a valid tap; neither is no bus at all.
  if (on1 == on2) return State::Loose;

  const sch::core::BusName &bus    = on1 ? bus1 : bus2;
  const Point       member = on1 ? p2 : p1;

  // Nothing at the member end: loose, not merely invalid.
  QString net;
  if (!netAt(scene(), member, net)) return State::Loose;

  return sch::core::isBusMember(net, bus) ? State::Valid : State::Invalid;
}

QPolygonF SchItemTap::outline() const {
  const State st = state();

  const Point p1 = item_->pt1.getValue();
  const Point p2 = item_->pt2.getValue();

  // SPINDLE: pointed at both ends, a tenth of the length either side,
  // tapering over the first and last fifth. Measured: a straight tap
  // tapers over 20 units to 20.5 wide.
  if (st == State::Loose) {
    const QPointF a  = toQ(p1);
    const QPointF d  = toQ(p2) - a;
    const QPointF n(-d.y(), d.x());

    QPolygonF poly;
    poly << a << a + d * 0.2 + n * 0.1 << a + d * 0.8 + n * 0.1 << a + d
         << a + d * 0.8 - n * 0.1 << a + d * 0.2 - n * 0.1;
    return poly;
  }

  // TRIANGLE: point at the member end, base across the bus end, a tenth
  // of the length either side. Measured: 21 units wide at the bus on a
  // straight tap, 30 on a diagonal.
  sch::core::BusName bus;
  const bool busAtFirst = busAt(scene(), p1, bus);

  const QPointF busEnd    = toQ(busAtFirst ? p1 : p2);
  const QPointF memberEnd = toQ(busAtFirst ? p2 : p1);
  const QPointF d         = busEnd - memberEnd;
  const QPointF n(-d.y(), d.x());

  QPolygonF poly;
  poly << memberEnd << busEnd + n * 0.1 << busEnd - n * 0.1;
  return poly;
}

void SchItemTap::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                       QWidget *) {
  const QColor color = (state() == State::Valid) ? RenderStyle::current().wire
                                                 : RenderStyle::current().busTapError;

  // A cosmetic outline in the same colour keeps the fine point of a
  // triangle visible when zoomed out, where the fill alone vanishes.
  QPen pen(color);
  pen.setCosmetic(true);
  SchPen::enforcePrintMin(pen, painter);
  painter->setPen(pen);
  painter->setBrush(color);

  painter->drawPolygon(outline());
}

} // namespace sch::render
