#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemZigzag.h"

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

SchItemZigzag::SchItemZigzag(ItemZigzagPtr item, QGraphicsItem *parent)
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

Rect SchItemZigzag::getRect() const noexcept {
  // The bounds of the shape AS DRAWN, which an odd rotation code tilts
  // -- so not simply the two corner points. Selection and repaint both
  // read this, and a shape drawn outside its own bounds smears when
  // dragged.
  const SchBoxFrame::Frame frame = boxFrame();

  if (!frame.tilted())
    return Rect(item_->pt1.getValue(), item_->pt2.getValue());

  const QRectF bounds = frame.sceneBounds();

  return Rect(Point(static_cast<int>(std::lround(bounds.left())),
                    static_cast<int>(std::lround(bounds.top()))),
              Point(static_cast<int>(std::lround(bounds.right())),
                    static_cast<int>(std::lround(bounds.bottom()))));
}

Point SchItemZigzag::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemZigzag::moveTo(Point p) noexcept {
  prepareGeometryChange();
  const Point delta = p - getPosition();
  item_->pt1.setValue(item_->pt1.getValue() + delta);
  item_->pt2.setValue(item_->pt2.getValue() + delta);
}

QList<Point> SchItemZigzag::controlPoints() const {
  return {item_->pt1.getValue(), item_->pt2.getValue()};
}

void SchItemZigzag::setControlPoint(int index, Point p) {
  prepareGeometryChange();
  if (index == 0) item_->pt1.setValue(p);
  else if (index == 1) item_->pt2.setValue(p);
}

void SchItemZigzag::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                          QWidget *) {
  // save/restore because the mirror cancellation below alters the
  // painter's transform.
  painter->save();
  QPen pen = SchPen::forElement(
      painter,
      tint(SchColorDecode::fromArgColor(item_->lineColor.getValue(),
                                        RenderStyle::current().graphicAnnotation)),
      item_->lineWidth.getValue(), item_->lineType.getValue());
  painter->setPen(pen);

  // pt1/pt2 are the corners of the zigzag's BOUNDING BOX, not the start
  // and end of the waveform itself. CONFIRMED against Simple.qsch: R1's
  // zigzag box is (-80,180)-(80,-180) -- 360 tall, 160 wide -- and its
  // pin stub lines connect at (0,180) and (0,-180), i.e. the midpoints
  // of the box's top and bottom edges, not at either corner. So the
  // waveform runs between the midpoints of the box's two SHORT edges
  // (the long axis is the spine), oscillating between the two LONG
  // edges (amplitude = half the short dimension).
  //
  // Vertical-vs-horizontal still comes from the box's aspect ratio; the
  // ROTATION field is applied as the frame the box is expressed in --
  // odd codes tilt it 45 degrees, see SchBoxFrame. The painter is
  // already inside a save() from the mirror handling above.
  const SchBoxFrame::Frame frame = boxFrame();

  painter->translate(frame.centre);
  painter->rotate(frame.degrees());

  const double left   = -frame.a;
  const double right  = frame.a;
  const double bottom = -frame.b;
  const double top    = frame.b;
  const double width  = right - left;
  const double height = top - bottom;

  QPointF start, end;
  double  amplitude;
  if (height >= width) {
    const double centerX = (left + right) / 2.0;
    start                = QPointF(centerX, top);
    end                  = QPointF(centerX, bottom);
    amplitude            = width / 2.0;
  } else {
    const double centerY = (top + bottom) / 2.0;
    start                = QPointF(left, centerY);
    end                  = QPointF(right, centerY);
    amplitude            = height / 2.0;
  }

  // CONFIRMED, by rendering the same file in QSpice and here: QSpice
  // draws the zigzag IDENTICALLY for all eight rotate codes -- the
  // mirror bit has no effect on it. Measured peak sequences (four
  // codes each, mirrored and not) were identical per orientation in
  // QSpice, while ours came out reversed for every mirrored code.
  //
  // This is not the mirror bit being misread: the same file's LABELS
  // do mirror (R5's text record sits at x=+130 yet QSpice draws it to
  // the LEFT of the symbol). So a mirrored component mirrors its text
  // placement but not this primitive. The likeliest explanation is
  // that QSpice derives the zigzag's shape from its bounding box after
  // transformation, in a canonical orientation, so the phase can never
  // flip.
  //
  // Cancelled here by flipping about the box's own centre before
  // drawing. The box is symmetric about that centre, so this changes
  // only the phase -- nothing moves. The axis doesn't matter: flipping
  // about x versus y differ by a 180-degree rotation, and a 180-degree
  // rotation maps this zigzag onto itself (which is also why codes 0
  // and 4 are indistinguishable in both renderings).
  //
  // NOT applied to arcs or coils, which are also bounding-box-derived
  // primitives and plausibly behave the same way -- but no mirrored
  // reference file exercises them yet, and this should be measured
  // rather than assumed.
  if (parentMirrored()) {
    const QPointF center((left + right) / 2.0, (bottom + top) / 2.0);
    painter->translate(center);
    painter->scale(-1.0, 1.0);
    painter->translate(-center);
  }

  const QPointF spine  = end - start;
  const double  length = std::hypot(spine.x(), spine.y());
  if (length < 1e-6) {
    painter->drawPoint(start);
    painter->restore();
    return;
  }

  // Unit vectors along and perpendicular to the spine.
  const QPointF along(spine.x() / length, spine.y() / length);
  const QPointF perp(-along.y(), along.x());

  // CONFIRMED (per spec): for a horizontal resistor, the zigzag should
  // show exactly two peaks above the centerline and two below -- not
  // the asymmetric 3-and-2 that six segments produced. This constant
  // is the number of straight segments in the path (segments-1
  // intermediate turning points, alternating sides), not a literal
  // peak count -- five segments gives four intermediate points
  // (+1,-1,+1,-1), which is exactly two peaks per side, symmetric, and
  // ends back on the centerline before reaching "end" (matching the
  // straight pin-stub leads on either side).
  //
  // That count is for a box about 2:1, and it is NOT a constant.
  // MEASURED 2026-10-07 against QSpice (CoilZigzag_AllCodes.qsch): the
  // same 600x300 box drawn at an odd code -- whose tilted sides come out
  // exactly 3:1 -- has SIX peaks, against four for the upright 2:1 box
  // and for the 360x160 (2.25:1) resistor.
  //
  // MEASURED 2026-10-07 (PointsLoopsTest.qsch, r = long side / short side),
  // peak pairs (one above plus one below the centerline):
  //     r      1.5  2  2.25  2.5  3  3.5  4  5  6  7  8
  //     pairs   2   2   2     2   3   3   4  4  5  6  7
  // pairs = floor(0.75*r + c) fits every sample for c in [1.0, 1.125);
  // 1.0625 is the middle of that window. Never fewer than two pairs.
  // The epsilon is for the tilted case, where an exact ratio arrives as
  // e.g. 2.9999999 and floor() must not lose a pair to that.
  const double longSide  = std::max(width, height);
  const double shortSide = std::min(width, height);
  const int    pairs =
      (shortSide > 1e-6)
          ? std::max(2, static_cast<int>(
                            std::floor(0.75 * (longSide / shortSide) + 1.0625 + 1e-6)))
          : 2;
  const int segments = 2 * pairs + 1;

  // Spacing along the spine is NOT even: the first segment (centerline
  // to first peak) and last segment (last peak to centerline) only rise
  // by one amplitude (A), while each internal peak-to-peak segment
  // rises by two amplitudes (2A, crossing from +A to -A or vice versa).
  // Evenly-spaced t values (i/segments) gave the first/last
  // segments the same horizontal run as internal ones despite half the
  // rise -- exactly half the slope, visibly shallower. Giving the
  // first/last segments half the horizontal run of an internal segment
  // (confirmed algebraically: run proportional to rise) makes every
  // segment's slope identical. General form for `segments`
  // segments (segments-1 intermediate peaks): peak i sits at
  // t = (2i-1) / (2*(segments-1)).
  QPainterPath path(start);
  for (int i = 1; i < segments; ++i) {
    const double t =
        static_cast<double>(2 * i - 1) / (2.0 * (segments - 1));
    const QPointF onSpine = start + along * (length * t);
    const double  side    = (i % 2 == 1) ? 1.0 : -1.0;
    path.lineTo(onSpine + perp * (amplitude * side));
  }
  path.lineTo(end);

  painter->drawPath(path);
  painter->restore();
}

} // namespace sch::render
