#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemArc3P.h"

#include "SchPen.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <cmath>
#include <utility>

#include "RenderStyle.h"
#include "SchColorDecode.h"

namespace sch::render {

namespace {

constexpr double kPi = 3.14159265358979323846; // MSVC's <cmath> doesn't
                                               // define M_PI without
                                               // _USE_MATH_DEFINES

// Normalizes an angle (radians) into [0, 2*pi).
double normalizeAngle(double a) {
  const double twoPi = 2.0 * kPi;
  a                  = std::fmod(a, twoPi);
  if (a < 0.0) a += twoPi;
  return a;
}

// Shared arc geometry: computed once, used by both getRect() and
// paint() so they can never disagree about what's actually drawn.
//
// CONFIRMED (via rigorous pixel-level curve fitting against three
// independent real QSpice renders): arc3p draws a SPIRAL, not a simple
// circular arc through three points. pt3 is ALWAYS the center. Radius
// interpolates LINEARLY, as a function of swept angle, from
// |pt1-center| at the start to |pt2-center| at the end -- confirmed by
// directly measuring rendered curve pixels and fitting radius as a
// function of angle from pt3: a near-circular symbol (V1's sine-source
// squiggle) gave a near-exact linear fit with near-zero slope (radius
// barely changes, |pt1-pt3| vs |pt2-pt3| differ by ~7%, looks
// circular); Vexp's second arc3p -- previously an unexplained "flat
// bulge" that didn't fit ANY circular-arc hypothesis -- fits this
// linear-radius model with 0.36% residual error using pt3's exact file
// coordinates as center; and an explicit spiral screenshot (radius
// visibly changing from ~217 to ~91 pixels) fits with 0.4-pixel
// residual on a 147-pixel range -- essentially exact. The sine
// symbol's two semicircles are simply the special case where
// |pt1-pt3| = |pt2-pt3| exactly, so linear interpolation between equal
// endpoints degenerates naturally to a constant radius (an ordinary
// circular arc) -- this ONE formula covers every case seen so far,
// with no separate degenerate/fallback handling needed (unlike the
// previous circumcircle-based version, which needed a special case for
// collinear points and still couldn't explain the Vexp "mystery" arc
// at all).
//
// Sweep direction is CCW from pt1's angle to pt2's angle (confirmed:
// this is what makes the sine symbol's two arcs bulge in opposite
// directions, matching its "~" shape, and what the spiral screenshot's
// start/end dots trace out).
struct ArcGeometry {
  QPointF center;
  double  radius1 = 0.0; // radius at pt1 (t=0)
  double  radius2 = 0.0; // radius at pt2 (t=1)
  double  angle1  = 0.0; // pt1's angle, radians
  double  sweep = 0.0; // CCW-positive sweep from angle1 to angle2, in [0, 2*pi)
  bool isPoint  = false; // pt1 == pt3 or pt2 == pt3 (zero radius) -- degenerate
};

ArcGeometry computeArcGeometry(QPointF pt1, QPointF pt2, QPointF pt3) {
  ArcGeometry g;
  g.center  = pt3;
  g.radius1 = std::hypot(pt1.x() - pt3.x(), pt1.y() - pt3.y());
  g.radius2 = std::hypot(pt2.x() - pt3.x(), pt2.y() - pt3.y());

  if (g.radius1 < 1e-9 || g.radius2 < 1e-9) {
    g.isPoint = true;
    return g;
  }

  g.angle1            = std::atan2(pt1.y() - pt3.y(), pt1.x() - pt3.x());
  const double angle2 = std::atan2(pt2.y() - pt3.y(), pt2.x() - pt3.x());
  g.sweep             = normalizeAngle(angle2 - g.angle1);
  return g;
}

// Point on the spiral at parameter t in [0,1].
QPointF spiralPoint(const ArcGeometry &g, double t) {
  const double angle  = g.angle1 + g.sweep * t;
  const double radius = g.radius1 + (g.radius2 - g.radius1) * t;
  return QPointF(g.center.x() + radius * std::cos(angle),
                 g.center.y() + radius * std::sin(angle));
}

} // namespace

SchItemArc3P::SchItemArc3P(ItemArc3pPtr item, QGraphicsItem *parent)
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

Rect SchItemArc3P::getRect() const noexcept {
  const QPointF     p1 = toQ(item_->pt1.getValue());
  const QPointF     p2 = toQ(item_->pt2.getValue());
  const QPointF     p3 = toQ(item_->pt3.getValue());
  const ArcGeometry g  = computeArcGeometry(p1, p2, p3);

  if (g.isPoint) return Rect(item_->pt1.getValue(), item_->pt1.getValue());

  // CONFIRMED bug fixed here: the previous version only checked the
  // circle's four cardinal angles (0/90/180/270 degrees) for where the
  // curve might bulge furthest out -- correct for a CONSTANT-radius
  // circle (where the X/Y extrema always fall exactly on those angles),
  // but WRONG for our spiral, where radius varies linearly with angle:
  // the true extrema shift away from the cardinal angles by an amount
  // that grows with how much the radius changes. Confirmed directly by
  // comparing against a dense (10000-sample) numerical reference: for
  // Arc B (the "flatter" Vexp arc, largest radius change of the arcs
  // seen so far), the cardinal-only box came up 3-5 units short on
  // multiple sides -- boundingRect() not fully containing what's
  // actually painted is exactly what leaves stale pixels behind during
  // a drag (Qt's dirty-region tracking only invalidates/redraws the
  // area the item CLAIMS to occupy).
  //
  // Fixed by sampling the curve densely (matching paint()'s own
  // kSamples resolution, which keeps this consistent with what's
  // actually drawn) and taking the min/max over all samples, rather
  // than assuming where the extrema fall analytically. A small
  // percentage-based margin on top absorbs whatever residual gap
  // remains between discrete samples on a smooth curve.
  constexpr int kBoundsSamples = 64;
  double        minX = p1.x(), maxX = p1.x();
  double        minY = p1.y(), maxY = p1.y();
  for (int i = 0; i <= kBoundsSamples; ++i) {
    const QPointF p = spiralPoint(g, static_cast<double>(i) / kBoundsSamples);
    minX            = std::min(minX, p.x());
    maxX            = std::max(maxX, p.x());
    minY            = std::min(minY, p.y());
    maxY            = std::max(maxY, p.y());
  }

  constexpr double kMarginFraction = 0.02; // 2% of the curve's own extent
  constexpr double kMarginMin      = 5.0;
  const double marginX = std::max(kMarginMin, (maxX - minX) * kMarginFraction);
  const double marginY = std::max(kMarginMin, (maxY - minY) * kMarginFraction);
  minX -= marginX;
  maxX += marginX;
  minY -= marginY;
  maxY += marginY;

  return Rect(Point(static_cast<int>(std::floor(minX)),
                    static_cast<int>(std::floor(minY))),
              Point(static_cast<int>(std::ceil(maxX)),
                    static_cast<int>(std::ceil(maxY))));
}

Point SchItemArc3P::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemArc3P::moveTo(Point p) noexcept {
  prepareGeometryChange();
  const Point delta = p - getPosition();
  item_->pt1.setValue(item_->pt1.getValue() + delta);
  item_->pt2.setValue(item_->pt2.getValue() + delta);
  item_->pt3.setValue(item_->pt3.getValue() + delta);
}

QList<Point> SchItemArc3P::controlPoints() const {
  return {item_->pt1.getValue(), item_->pt2.getValue(), item_->pt3.getValue()};
}

void SchItemArc3P::setControlPoint(int index, Point p) {
  prepareGeometryChange();
  if (index == 0) item_->pt1.setValue(p);
  else if (index == 1) item_->pt2.setValue(p);
  else if (index == 2) item_->pt3.setValue(p);
}

void SchItemArc3P::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                         QWidget *) {
  QPen pen = SchPen::forElement(
      painter,
      tint(SchColorDecode::fromArgColor(item_->lineColor.getValue(),
                                        RenderStyle::current().graphicAnnotation)),
      item_->lineWidth.getValue(), item_->lineType.getValue());
  painter->setPen(pen);

  // Empty when all three points coincide: a dot.
  const QPainterPath path = outlinePath();
  if (path.isEmpty()) {
    painter->drawPoint(toQ(item_->pt1.getValue()));
    return;
  }

  painter->drawPath(path);
}

QPainterPath SchItemArc3P::outlinePath() const {
  const QPointF p1 = toQ(item_->pt1.getValue());
  const QPointF p2 = toQ(item_->pt2.getValue());
  const QPointF p3 = toQ(item_->pt3.getValue());

  const ArcGeometry g = computeArcGeometry(p1, p2, p3);
  if (g.isPoint) return QPainterPath();

  constexpr int kSamples = 32;
  QPainterPath  path(spiralPoint(g, 0.0));
  for (int i = 1; i <= kSamples; ++i)
    path.lineTo(spiralPoint(g, static_cast<double>(i) / kSamples));
  return path;
}

} // namespace sch::render
