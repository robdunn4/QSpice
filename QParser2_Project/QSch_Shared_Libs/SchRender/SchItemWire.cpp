#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemWire.h"
#include "SchPen.h"

#include "SchStringDecode.h"

#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPen>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <QGraphicsScene>
#include <cmath>
#include "SchBus.h"
#include "SchPointKey.h"
#include "SchScene.h"
#include "RenderStyle.h"

#include <utility>

namespace sch::render {

namespace {

// A bus wire's drawn width, in SCHEMATIC units -- it scales with zoom,
// unlike an ordinary wire, which stays a fixed 2 pixels.
//
// FITTED to two measurements of QSpice's rendering: 28.2 units with
// BUSwidthMultiplier 2, 56.0 with 5. Proportional to the multiplier does
// not fit (14.1 and 11.2 per step); proportional to multiplier + 1 does,
// at 9.33 per step -- the same "+1" form as the line-width rule. Where
// 9.33 comes from is unknown; CADlineWidth was 1.5 throughout, so it may
// yet play a part.
constexpr double kBusWidthPerStep = 9.33;

inline double busWidthUnits() {
  return kBusWidthPerStep * (RenderStyle::current().busWidthMultiplier + 1.0);
}

} // namespace


SchItemWire::SchItemWire(ItemWirePtr item, QGraphicsItem *parent)
    : SchItemBase(parent), item_(std::move(item)) {
  // Only truly top-level items are independently selectable/movable --
  // children of a SchItemComp (the leaf elements making up a symbol's
  // geometry) act as part of their parent component, not as independent
  // objects. Matches standard EDA convention: clicking or rubber-band-
  // selecting any part of a placed component selects/moves the whole
  // component, not one internal element.
  const bool topLevel = (parent == nullptr);
  setFlag(QGraphicsItem::ItemIsSelectable, topLevel);
  setFlag(QGraphicsItem::ItemIsMovable, topLevel);

  setZValue(kZGraphic);
}

Rect SchItemWire::getRect() const noexcept {
  const Rect line(item_->pt1.getValue(), item_->pt2.getValue());
  if (!isBus()) return line;

  // A BUS is as wide as it is drawn, so the selection outline, the band
  // test and the repaint bounds all follow the visible bar rather than
  // its centreline. Rounded up, so nothing drawn falls outside.
  const int half = static_cast<int>(std::ceil(busWidthUnits() / 2.0));
  return Rect(line.left - half, line.top + half, line.right + half,
              line.bottom - half);
}

Point SchItemWire::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemWire::moveTo(Point p) noexcept {
  prepareGeometryChange();
  const Point delta = p - getPosition();
  item_->pt1.setValue(item_->pt1.getValue() + delta);
  item_->pt2.setValue(item_->pt2.getValue() + delta);
}

QString SchItemWire::netName() const {
  return sch::core::decodeQSpiceString(item_->netName.getValue()).trimmed();
}

bool SchItemWire::isBus() const { return sch::core::isBusName(netName()); }

void SchItemWire::setNetName(const QString &name) {
  // Before the change: a rename can make the wire a bus, or stop it
  // being one, and a bus's bounds are much wider.
  prepareGeometryChange();

  // QUOTED, as QSpice writes it: every wire in a QSpice file carries a
  // quoted name, since QSpice names every net. Storing the bare string
  // -- which this did at first -- produced records QSpice would not read
  // back.
  //
  // The same quoting a net label's editor applies, and the reason both
  // go through ArgString rather than building the text by hand.
  item_->netName.setValue(
      ArgString(sch::core::encodeQSpiceString(name)).quote().getValue());
  update();
}

std::string SchItemWire::netNameRaw() const {
  return item_->netName.getValue();
}

QList<QPointF> SchItemWire::connectionPoints() const {
  return {toQ(item_->pt1.getValue()), toQ(item_->pt2.getValue())};
}

QPainterPath SchItemWire::shape() const {
  // A wire is a line, and SchItemBase::shape() is the element RECT --
  // which for an axis-aligned wire has zero height or width, so there
  // is essentially nothing to click. Stroke the line instead, with a
  // tolerance band around it.
  //
  // The band is in scene units and therefore shrinks on screen as you
  // zoom out; that's deliberate, since a zoom-dependent shape() would
  // have to be recomputed (and the item re-indexed) on every zoom step.
  // kClickTolerance is sized against the 100-unit electrical grid: wide
  // enough to hit comfortably, far narrower than the spacing between
  // two adjacent wires, so it can never make the wrong wire easier to
  // hit than the right one.
  QPainterPath path;
  path.moveTo(toQ(item_->pt1.getValue()));
  path.lineTo(toQ(item_->pt2.getValue()));

  QPainterPathStroker stroker;
  // At least the bus's own width, so the whole drawn bus is clickable.
  stroker.setWidth(isBus() ? qMax(kClickTolerance * 2.0, busWidthUnits())
                           : kClickTolerance * 2.0);
  stroker.setCapStyle(Qt::RoundCap);
  return stroker.createStroke(path);
}

QList<Point> SchItemWire::controlPoints() const {
  return {item_->pt1.getValue(), item_->pt2.getValue()};
}

void SchItemWire::setControlPoint(int index, Point p) {
  prepareGeometryChange();
  if (index == 0) item_->pt1.setValue(p);
  else if (index == 1) item_->pt2.setValue(p);
}

namespace {

// Half-size of the loose-end box, in DEVICE pixels -- the same size the
// pin marker uses, since it means the same thing.
constexpr double kLooseEndBoxPx = 3.0;

} // namespace

QPen SchItemWire::wirePen() {
  // TODO: no net-name-based colouring/highlighting yet, and width and
  // style are not configurable yet either -- CADlineWidth is read from
  // the registry but not applied. When they are, this is the one place
  // that changes.
  QPen pen(RenderStyle::current().wire);
  pen.setCosmetic(true);
  pen.setStyle(Qt::SolidLine);
  SchPen::enforcePrintMin(pen);
  return pen;
}

QPen SchItemWire::wirePen(const QPainter *painter) {
  // On screen a wire is a constant one-pixel line (wirePen()). That is a
  // SCREEN convention: on paper and in SVG it came out at the minimum
  // line width, visibly thinner than every symbol's lines.
  if (SchPen::printMinDevicePx <= 0.0 && !SchPen::vectorExport) return wirePen();

  QPen pen = SchPen::forElement(painter, RenderStyle::current().wire, 0, 0);
  pen.setCapStyle(Qt::RoundCap);
  pen.setJoinStyle(Qt::RoundJoin);
  return pen;
}

void SchItemWire::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                        QWidget *) {
  QPen pen = wirePen(painter);
  SchPen::toGeometric(pen, painter);
  painter->setPen(pen);

  const QPointF a = toQ(item_->pt1.getValue());
  const QPointF b = toQ(item_->pt2.getValue());

  // A BUS is drawn wide, in schematic units so it scales with zoom, with
  // round ends -- which also rounds the corner where two bus segments
  // meet, as QSpice draws it. See SchBus.h.
  if (isBus()) {
    QPen busPen(RenderStyle::current().wire, busWidthUnits());
    busPen.setCapStyle(Qt::RoundCap);
    busPen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(busPen);
  }

  painter->drawLine(a, b);

  // A BUS shows no dangling-end markers unless it is selected: a bus
  // routinely ends in open space, its members leaving through taps, so
  // marking its ends would flag every bus in the schematic.
  if (isBus() && !isSelected()) return;

  // A dangling END gets the same "nothing attached here" box a pin
  // does. A wire ending in mid-air is almost always unfinished rather
  // than intended, and the alternative is discovering it at netlist
  // time.
  //
  // Sized in DEVICE pixels so it stays legible at any zoom: hypot of
  // the transform's first row, not m11, which is scale*cos of any
  // rotation.
  const QTransform xform = painter->transform();
  const double     scale = std::hypot(xform.m11(), xform.m12());
  const double     half =
      (scale > 0.0) ? (kLooseEndBoxPx / scale) : kLooseEndBoxPx;

  QPen boxPen(Qt::black);
  boxPen.setCosmetic(true);
  SchPen::enforcePrintMin(boxPen, painter);
  painter->setPen(boxPen);
  painter->setBrush(RenderStyle::current().unconnectedPin);

  // From the document's connectivity model -- see
  // SchItemPin::showsUnconnectedMark(). A scene that is not a document's
  // has none, and shows no marks. (Wires inside symbols no longer exist:
  // SchRecordRules removes them.)
  SchScene *sc = SchScene::of(this);
  if (sc == nullptr) return;

  const sch::core::NetModel *model = sc->netModel();
  if (model == nullptr) return;

  const QPointF ends[2] = {a, b};
  for (int i = 0; i < 2; ++i) {
    const QPointF &end     = ends[i];
    const QPointF  scenePt = mapToScene(end);
    if (!model->wireEndLoose(node(), i, sch::core::keyFor(scenePt))) continue;

    painter->drawRect(
        QRectF(end.x() - half, end.y() - half, half * 2, half * 2));
  }
}

} // namespace sch::render
