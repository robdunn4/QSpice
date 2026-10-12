#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemCoil.h"

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
#include "SchRotDecode.h"

namespace sch::render {

namespace {

constexpr double kPi = 3.14159265358979323846; // MSVC's <cmath> doesn't
                                               // define M_PI without
                                               // _USE_MATH_DEFINES

// Points per turn along the curve. The path is built in scene units and
// drawn with a cosmetic pen, so this fixes the smoothness in MODEL
// space, not device space -- generous enough that a coil zoomed to fill
// the viewport still reads as a smooth curve.
constexpr int kStepsPerTurn = 64;

} // namespace

SchItemCoil::SchItemCoil(ItemCoilPtr item, QGraphicsItem *parent)
    : SchItemBase(parent), item_(std::move(item)) {
  // Only truly top-level items are independently selectable/movable --
  // children of a SchItemComp act as part of their parent component.
  // Same rule as every other leaf wrapper.
  const bool topLevel = (parent == nullptr);
  setFlag(QGraphicsItem::ItemIsSelectable, topLevel);
  setFlag(QGraphicsItem::ItemIsMovable, topLevel);

  setZValue(kZGraphic);
}

Rect SchItemCoil::getRect() const noexcept {
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

Point SchItemCoil::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemCoil::moveTo(Point p) noexcept {
  prepareGeometryChange();
  const Point delta = p - getPosition();
  item_->pt1.setValue(item_->pt1.getValue() + delta);
  item_->pt2.setValue(item_->pt2.getValue() + delta);
}

QList<Point> SchItemCoil::controlPoints() const {
  return {item_->pt1.getValue(), item_->pt2.getValue()};
}

void SchItemCoil::setControlPoint(int index, Point p) {
  prepareGeometryChange();
  if (index == 0) item_->pt1.setValue(p);
  else if (index == 1) item_->pt2.setValue(p);
}

void SchItemCoil::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                        QWidget *) {
  QPen pen = SchPen::forElement(
      painter,
      tint(SchColorDecode::fromArgColor(item_->lineColor.getValue(),
                                        RenderStyle::current().graphicAnnotation)),
      item_->lineWidth.getValue(), item_->lineType.getValue());
  painter->setPen(pen);

  // Wound in the shape's OWN frame -- an odd rotation code tilts it 45
  // degrees, see SchBoxFrame. The winding maths below is unchanged and
  // works from the box, so the box is simply expressed in that frame:
  // centred on the origin, painter rotated to match. An even code makes
  // the transform identity and nothing moves.
  const SchBoxFrame::Frame frame = boxFrame();

  painter->save();
  painter->translate(frame.centre);
  painter->rotate(frame.degrees());

  const double left   = -frame.a;
  const double right  = frame.a;
  const double bottom = -frame.b;
  const double top    = frame.b;
  const double width  = right - left;
  const double height = top - bottom;

  // The coil runs along the box's LONGER dimension and oscillates
  // across the shorter one. Ties go to vertical, matching
  // SchItemZigzag's convention for the same ambiguity.
  //
  // The item's own rotate field: its MIRROR bit is applied below. The
  // rotation part (code & 0x7) still isn't -- orientation comes from
  // the box's aspect ratio, and no reference file exercises a coil with
  // a non-zero rotation to show what that field would add.
  const bool    vertical = (height >= width);
  const double  along    = vertical ? height : width;
  const double  across   = vertical ? width : height;
  const QPointF origin = vertical
                             ? QPointF((left + right) / 2.0, bottom)
                             : QPointF(left, (bottom + top) / 2.0);

  if (along < 1e-6 || across < 1e-6) {
    // Degenerate box -- nothing meaningful to wind. Draw the spine so
    // the element is still visible and selectable.
    painter->drawLine(QPointF(left, bottom), QPointF(right, top));
    painter->restore();
    return;
  }

  // --- Helix geometry -------------------------------------------------
  //
  // MEASURED against QSpice's own rendering (nine reference coils, box
  // aspect ratios 0.56 to 2.50), by fitting candidate models to the
  // extracted curve pixels. This parametric helix fits at 0.46-0.68 px
  // mean error with 81-93% of the drawn curve covered. An alternative
  // model -- N separate overlapping ellipses -- was tried first and is
  // WRONG: it fits 2-6x worse, and it predicts that every horizontal
  // turning point lies exactly on the centerline, whereas in the real
  // rendering the leftmost and rightmost points of each loop sit at the
  // same offset OFF the centerline. That offset is the helix's
  // signature: dx/dt = 0 gives sin(theta) = pitch / (2*pi*rx), a
  // constant, so all turning points share one height.
  //
  // Constants, both sharp optima rather than eyeballed:
  //   rx = across/4 -- beats across/3.6 and across/4.5 by 2-4x error.
  //   turns = loops + 0.5, where the whole-loop count grows at 1.5 loops
  //     per across-worth of length, not 1 (see the table below). The
  //     right turn count beats the runner-up by 3-6x on every sample.
  //
  // Loop count MEASURED 2026-10-07 (PointsLoopsTest.qsch, r = along/across):
  //     r      1.5  1.9  2  2.5  3  3.5  4  5  6  7
  //     loops   1    1   2   2   3   4   5  6  8  9
  // loops = floor(1.5*r + c) fits every sample for c in [-1.0, -0.75);
  // -0.875 is the middle of that window. The 1 -> 2 step then falls at
  // r = 1.917, between the measured 1.90 and 2.00. At least one loop.
  //
  // The half turn is what puts both ends of the curve on the
  // centerline at the box's ends -- which is presumably the point,
  // since that's where a wire has to meet the symbol.
  // The epsilon: in a tilted frame the box's sides come out of a 45-degree
  // rotation, so an exact ratio arrives as e.g. 2.9999999, and floor()
  // must not lose a loop to that.
  const double ratio = along / across;
  const double loops = std::max(1.0, std::floor(1.5 * ratio - 0.875 + 1e-6));
  const double turns = loops + 0.5;
  const double rx    = across / 4.0; // loop radius along the spine
  const double ry    = across / 2.0; // amplitude across it -- fills the box

  // Linear travel of the helix's axis. The cos term adds rx at each
  // end, so the drawn curve spans exactly the box.
  const double travel = along - 2.0 * rx;

  // Unit vectors along the spine and across it.
  //
  // Handedness is NOT arbitrary: mirroring a coil across its own spine
  // produces a visibly different drawing (the loops lean the other
  // way), and the reference rendering picks one. Measured both ways
  // against all nine samples -- the handedness below fits at 0.7-1.6
  // px, the mirrored one at 4.3-22.5 px.
  //
  // The direction of travel, by contrast, doesn't matter at all: with
  // a half-integer turn count the curve is point-symmetric, so
  // reversing the spine AND the cross axis together is the identity
  // (confirmed -- both variants score identically).
  //
  // Note what the two orientations actually are: the vertical coil is
  // the horizontal one TRANSPOSED (x and y swapped), not rotated 90
  // degrees. No single perpendicular formula covers both, because a
  // transpose is a reflection, not a rotation. Worth knowing if the
  // element's own rotate field is ever implemented -- rotating a
  // horizontal coil by 90 degrees will NOT reproduce what QSpice draws
  // for a tall box.
  //
  // CAVEAT: only one vertical reference sample exists (250x450), so
  // the transpose reading rests on a single data point. A tall coil
  // with an asymmetric turn count (aspect ratio >= 2, giving 2.5
  // turns) would confirm it properly.
  const QPointF alongVec = vertical ? QPointF(0.0, 1.0) : QPointF(1.0, 0.0);
  const QPointF crossVec = vertical ? QPointF(-1.0, 0.0) : QPointF(0.0, -1.0);

  // MEASURED against QSpice, from a file with all four combinations
  // (horizontal and vertical, rotate code 0 and code 8): a mirrored
  // coil is the unmirrored one REFLECTED ACROSS ITS OWN SPINE. Best
  // symmetry match by ink overlap was a vertical flip for the
  // horizontal pair (IoU 0.57, against 0.37 identical) and a horizontal
  // flip for the vertical pair (0.62, against 0.40) -- in both cases
  // the flip whose axis is the coil's long axis. That reverses the
  // helix's handedness, which the earlier reverse-engineering already
  // established is a real, visible property rather than a symmetry.
  //
  // NOTE the contrast with SchItemZigzag, which must IGNORE its
  // parent's mirror. Both are consistent under one rule: a primitive is
  // drawn per its OWN rotate field, and a parent component's mirror
  // moves elements around without re-orienting the shapes themselves.
  // The zigzag inside a mirrored resistor keeps its own rotate of 0 and
  // so draws unmirrored; these coils carry rotate = 8 themselves and so
  // draw mirrored. That rule is inferred from two elements, not
  // verified in general -- a symbol containing an element with a
  // non-zero rotate of its own would test it directly.
  //
  // WHICH SIDE the loops fall on -- MEASURED 2026-10-07, 19 of 19 QSpice
  // coils across three files (OddRotations_Claude_Gend.qsch, its
  // corners-up twin, RotationTests.qsch), fitted from the pixels:
  // the corners are SIGNED. With the box's diagonal pt1 -> pt2 expressed
  // in the frame (L = toFrame(pt2 - pt1)), the canonical drawing below
  // stands when Lx*Ly < 0 and is reflected across its spine when
  // Lx*Ly > 0. So the order the corners were written in matters -- the
  // same box drawn from the other corner pair loops the other way --
  // which is what the old code, working from the normalised box alone,
  // could not reproduce.
  const Point   p1   = item_->pt1.getValue();
  const Point   p2   = item_->pt2.getValue();
  const QPointF diag = frame.toFrame(QPointF(p2.x - p1.x, p2.y - p1.y));
  double crossSign = (diag.x() * diag.y() < 0.0) ? 1.0 : -1.0;

  // The MIRROR bit on top -- measured earlier (codes 0 and 8). Whether it
  // composes this way with the corner signs is unmeasured.
  if (sch::core::rotdecode::isMirrored(item_->rotate.getValue())) crossSign = -crossSign;

  const int steps = std::max(2, static_cast<int>(kStepsPerTurn * turns));

  QPainterPath path;
  for (int i = 0; i <= steps; ++i) {
    const double t = static_cast<double>(i) / steps;

    // Phase pi starts the curve at the near end, on the centerline.
    const double theta = 2.0 * kPi * turns * t + kPi;

    const double u = t * travel + rx * std::cos(theta) + rx;
    const double v = crossSign * ry * std::sin(theta);

    const QPointF pt = origin + alongVec * u + crossVec * v;
    if (i == 0) path.moveTo(pt);
    else path.lineTo(pt);
  }

  painter->drawPath(path);

  painter->restore();
}

} // namespace sch::render
