#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemArc4P.h"

#include "SchPen.h"

#include <QPainter>
#include <QPen>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cmath>
#include <numbers>
#include <utility>

#include "RenderStyle.h"
#include "SchColorDecode.h"

namespace sch::render {

namespace {

constexpr double kPi    = std::numbers::pi;
constexpr double kTwoPi = 2.0 * std::numbers::pi;

// A full ellipse is sampled at this many points; partial arcs get a
// proportional share, with a floor so short arcs stay smooth.
constexpr int kSamplesPerTurn = 96;
constexpr int kMinSamples     = 8;

} // namespace

SchItemArc4P::SchItemArc4P(ItemArc4pPtr item, QGraphicsItem *parent)
    : SchItemBase(parent), item_(std::move(item)) {
  const bool topLevel = (parent == nullptr);
  setFlag(QGraphicsItem::ItemIsSelectable, topLevel);
  setFlag(QGraphicsItem::ItemIsMovable, topLevel);

  setZValue(kZGraphic);
}

QPointF SchItemArc4P::Frame::toFrame(const QPointF &v) const {
  return QPointF(v.x() * cosT + v.y() * sinT, -v.x() * sinT + v.y() * cosT);
}

QPointF SchItemArc4P::Frame::fromFrame(const QPointF &v) const {
  return QPointF(v.x() * cosT - v.y() * sinT, v.x() * sinT + v.y() * cosT);
}

SchItemArc4P::Frame SchItemArc4P::frame() const {
  const QPointF p1 = toQ(item_->pt1.getValue());
  const QPointF p2 = toQ(item_->pt2.getValue());

  Frame f;

  // The box's frame is turned by 45 degrees for an ODD rotation code and
  // not at all for an even one. Any multiple of 90 maps a box given by
  // opposite corners onto itself, which is why only the low bit is ever
  // visible -- measured across all sixteen codes.
  const double theta = (item_->rotate.getValue() & 1) ? (kPi / 4.0) : 0.0;
  f.cosT             = std::cos(theta);
  f.sinT             = std::sin(theta);

  // The centre is the midpoint of the corners whatever the frame, so
  // there is no pivot to choose.
  f.centre           = (p1 + p2) / 2.0;
  const QPointF diag = f.toFrame(p2 - p1);

  f.a = std::abs(diag.x()) / 2.0;
  f.b = std::abs(diag.y()) / 2.0;
  return f;
}

SchItemBase::EditGuide SchItemArc4P::editGuide() const {
  EditGuide guide;

  const Frame f = frame();

  // The bounding box, drawn in the ellipse's own frame -- so for an odd
  // rotation it appears TILTED, which is the one place that otherwise
  // invisible property shows.
  const QPointF c0 = f.centre + f.fromFrame(QPointF(-f.a, -f.b));
  const QPointF c1 = f.centre + f.fromFrame(QPointF(f.a, -f.b));
  const QPointF c2 = f.centre + f.fromFrame(QPointF(f.a, f.b));
  const QPointF c3 = f.centre + f.fromFrame(QPointF(-f.a, f.b));

  guide.dashed.moveTo(c0);
  guide.dashed.lineTo(c1);
  guide.dashed.lineTo(c2);
  guide.dashed.lineTo(c3);
  guide.dashed.closeSubpath();

  // The rays, from the centre out to their handles. Drawn to the HANDLE,
  // not to the ellipse, because the handle is what is being dragged --
  // and seeing the ray keep its direction as the handle slides along it
  // is what shows that distance does not matter.
  guide.dashed.moveTo(f.centre);
  guide.dashed.lineTo(toQ(item_->pt3.getValue()));
  guide.dashed.moveTo(f.centre);
  guide.dashed.lineTo(toQ(item_->pt4.getValue()));

  guide.dots.append(f.centre);
  return guide;
}

QPainterPath SchItemArc4P::arcPath() const {
  const QPointF ps = toQ(item_->pt3.getValue());
  const QPointF pe = toQ(item_->pt4.getValue());

  const Frame   f      = frame();
  const QPointF centre = f.centre;
  const double  a      = f.a;
  const double  b      = f.b;

  const auto toFrame   = [&f](const QPointF &v) { return f.toFrame(v); };
  const auto fromFrame = [&f](const QPointF &v) { return f.fromFrame(v); };

  // A flat or empty box has no curve to draw. (A square box with an odd
  // code flattens this way, which is the dot QSpice shows.)
  constexpr double kEps = 1e-9;
  if (a < kEps || b < kEps) return QPainterPath();

  // The RAY's direction sets the angle; how far along it the point sits
  // does not. Taking the angle of the point after scaling by the
  // semi-axes gives the ellipse's parametric angle -- which is exactly
  // where the ray from the centre meets the ellipse.
  const auto paramAngle = [&](const QPointF &p) {
    const QPointF v = toFrame(p - centre);
    return std::atan2(v.y() / b, v.x() / a);
  };

  const double start = paramAngle(ps);
  const double end   = paramAngle(pe);

  // Counter-clockwise, in Y-up coordinates, from start to end.
  double sweep = std::fmod(end - start, kTwoPi);
  if (sweep < 0.0) sweep += kTwoPi;

  // Both rays the same draws nothing -- measured, and the opposite of
  // GDI's Arc(), which would draw the full ellipse.
  if (sweep < kEps || (kTwoPi - sweep) < kEps) return QPainterPath();

  const int samples = qMax(
      kMinSamples, static_cast<int>(std::ceil(kSamplesPerTurn * sweep / kTwoPi)));

  const auto pointAt = [&](double t) {
    return centre + fromFrame(QPointF(a * std::cos(t), b * std::sin(t)));
  };

  QPainterPath path(pointAt(start));
  for (int i = 1; i <= samples; ++i)
    path.lineTo(pointAt(start + sweep * static_cast<double>(i) / samples));

  return path;
}

Rect SchItemArc4P::getRect() const noexcept {
  const QPainterPath path = arcPath();

  // Nothing drawn: the box itself, so the element can still be found
  // and selected rather than vanishing entirely.
  const QRectF r = path.isEmpty()
                       ? QRectF(toQ(item_->pt1.getValue()),
                                toQ(item_->pt2.getValue()))
                             .normalized()
                       : path.boundingRect();

  return Rect(Point(static_cast<int>(std::floor(r.left())),
                    static_cast<int>(std::ceil(r.bottom()))),
              Point(static_cast<int>(std::ceil(r.right())),
                    static_cast<int>(std::floor(r.top()))));
}

Point SchItemArc4P::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemArc4P::moveTo(Point p) noexcept {
  prepareGeometryChange();

  // All four points move together, keeping their offsets from pt1 -- the
  // rays are directions from the centre, which moves with the box.
  const Point o = item_->pt1.getValue();
  const Point d(p.x - o.x, p.y - o.y);

  const auto shift = [&d](const Point &q) { return Point(q.x + d.x, q.y + d.y); };

  item_->pt1.setValue(p);
  item_->pt2.setValue(shift(item_->pt2.getValue()));
  item_->pt3.setValue(shift(item_->pt3.getValue()));
  item_->pt4.setValue(shift(item_->pt4.getValue()));
}

QList<Point> SchItemArc4P::controlPoints() const {
  return {item_->pt1.getValue(), item_->pt2.getValue(), item_->pt3.getValue(),
          item_->pt4.getValue()};
}

void SchItemArc4P::setControlPoint(int index, Point p) {
  prepareGeometryChange();

  if (index == 0) item_->pt1.setValue(p);
  else if (index == 1) item_->pt2.setValue(p);
  else if (index == 2) item_->pt3.setValue(p);
  else if (index == 3) item_->pt4.setValue(p);
}

void SchItemArc4P::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                         QWidget *) {
  const QPainterPath path = arcPath();
  if (path.isEmpty()) return;

  QPen pen = SchPen::forElement(
      painter,
      tint(SchColorDecode::fromArgColor(item_->lineColor.getValue(),
                                        RenderStyle::current().graphicAnnotation)),
      item_->lineWidth.getValue(), item_->lineType.getValue());
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);

  painter->drawPath(path);
}

} // namespace sch::render
