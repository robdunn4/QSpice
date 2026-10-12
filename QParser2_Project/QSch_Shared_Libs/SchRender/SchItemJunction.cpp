#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemJunction.h"

#include <QBrush>
#include <QPainter>
#include <QPen>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <utility>

#include <algorithm>
#include <cmath>

#include "RenderStyle.h"
#include "SchPen.h"

namespace sch::render {

namespace {

// The record is just a point -- no colour, no size -- so both come from
// QSpice's own settings. Junctions are drawn in the wire colour because
// that is what they are: a marked meeting of wires.
QColor junctionColor() { return RenderStyle::current().wire; }

// The smallest the dot is drawn, in device pixels, so a far zoomed-out
// view still shows a speck where wires meet.
constexpr double kMinRadiusPx = 1.0;

// While printing, the smallest radius in PRINTED line widths: a dot
// three lines across, so it reads as a dot over thickened wires.
constexpr double kPrintRadiusLineWidths = 1.5;


} // namespace

SchItemJunction::SchItemJunction(ItemJunctionPtr item, QGraphicsItem *parent)
    : SchItemBase(parent), item_(std::move(item)) {
  // Deliberately NOT selectable or movable, even at top level. A
  // junction has no independent existence: it is a consequence of where
  // wire endpoints are, and dragging one would assert otherwise.
  // Moving the wires is what moves the junction.
  setFlag(QGraphicsItem::ItemIsSelectable, false);
  setFlag(QGraphicsItem::ItemIsMovable, false);

  setZValue(kZJunction);
}

double SchItemJunction::dotRadius() {
  return radiusLineWidths * SchPen::widthUnits(0) * RenderStyle::current().junctionSize;
}

Rect SchItemJunction::getRect() const noexcept {
  const Point p = item_->pt1.getValue();
  const int   r = static_cast<int>(std::ceil(dotRadius()));
  return Rect(Point(p.x - r, p.y + r), Point(p.x + r, p.y - r));
}

Point SchItemJunction::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemJunction::moveTo(Point p) noexcept {
  prepareGeometryChange();
  item_->pt1.setValue(p);
}

QList<QPointF> SchItemJunction::connectionPoints() const {
  return {toQ(item_->pt1.getValue())};
}

void SchItemJunction::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem *, QWidget *) {
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setPen(Qt::NoPen);
  painter->setBrush(QBrush(junctionColor()));

  // Sized in SCENE units, so it scales with the drawing (it was fixed
  // in device pixels until 2026-10-04, which made it huge in a small
  // rendering -- the print page map -- and tiny in print). Only a far
  // zoomed-out view is held at a minimum speck.
  //
  // hypot of the transform's first row, not m11: m11 is scale*cos of
  // any rotation.
  const QTransform xform = painter->transform();
  const double     scale = std::hypot(xform.m11(), xform.m12());

  double radius = dotRadius();

  // PRINTING with a minimum line width: the wires around the dot may be
  // thickened to SchPen::printMinDevicePx, and a dot must stay clearly
  // wider than the lines it joins. A FLOOR, not a rescale: scaling the
  // whole radius by (minimum / nominal) made dots far too large, since
  // the nominal width at print scale is often a fraction of a pixel.
  if (scale > 0.0 && SchPen::printMinDevicePx > 0.0)
    radius = std::max(radius, kPrintRadiusLineWidths * SchPen::printMinDevicePx / scale);

  if (scale > 0.0) radius = std::max(radius, kMinRadiusPx / scale);

  painter->drawEllipse(toQ(item_->pt1.getValue()), radius, radius);
}

} // namespace sch::render
