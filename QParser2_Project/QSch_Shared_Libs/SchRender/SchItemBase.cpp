#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemBase.h"

#include <QPainterPathStroker>

#include "RenderStyle.h"
#include "SchAlign.h"
#include "SchColorDecode.h"
#include "SchPen.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QStyleOptionGraphicsItem>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cmath>

namespace sch::render {

bool SchItemBase::showResistorOrient_ = false;

QColor SchItemBase::doNotStuffColor_ = QColor(0xAA, 0xAA, 0xAA);

double SchItemBase::jumperWidthPx_ = 3.0;

bool SchItemBase::isUnstuffed() const {
  // Walks UP: a symbol's lines and labels are children of the
  // component, and it is the component that carries the flag.
  for (const QGraphicsItem *item = this; item != nullptr;
       item = item->parentItem()) {
    const SchItemBase *schItem = dynamic_cast<const SchItemBase *>(item);
    if (schItem == nullptr) continue;

    const int stuff = schItem->stuffCode();
    if (stuff > 0) return true;  // 1 do not stuff, 2 jumper
    if (stuff == 0) return false; // a component, and it IS stuffed
  }

  return false;
}

QRectF SchItemBase::boundingRect() const {
  const QRectF own = elementRect().adjusted(-kPaintSlack, -kPaintSlack,
                                            kPaintSlack, kPaintSlack);

  // United with the children's extent. For a component this is what
  // covers its attribute text (reference designator, value), which sits
  // outside the body geometry -- arbitrarily far outside, since nothing
  // constrains where a symbol places its labels.
  //
  // CONFIRMED necessary: dragging a resistor with labels above and
  // below left them smeared across the canvas, even with honest
  // per-element slack and BoundingRectViewportUpdate. Relying on Qt to
  // invalidate each child's own area independently was not enough in
  // practice.
  //
  // Note what this deliberately does NOT change: getRect() /
  // elementRect() still EXCLUDE attribute text, so the selection
  // outline and hit-testing stay tight around the visible body -- the
  // reason that exclusion exists (a rotated resistor's highlight was
  // 405x444 instead of 160x360) is untouched. Repaint coverage and
  // selection geometry are separate concerns and now have separate
  // rects.
  //
  // Permanent rather than applied only during a drag: a bounding rect
  // that changed mid-gesture would mean prepareGeometryChange() on
  // every mouse-move -- the call whose ordering caused the BSP crash --
  // and would make the reported geometry depend on what the user
  // happens to be doing. The cost of carrying it all the time is a
  // slightly larger invalidation region, which is exactly what's wanted
  // here anyway.
  const QRectF kids = childrenBoundingRect();
  return kids.isNull() ? own : own.united(kids);
}

double SchItemBase::parentScale() const {
  const QGraphicsItem *parent = parentItem();
  if (parent == nullptr) return 1.0;

  // From the DETERMINANT, whose magnitude is the area scale -- so the
  // linear scale is its square root. That holds whatever combination of
  // rotation, scale and mirroring produced the transform, which the
  // individual matrix elements do not.
  const double det = std::fabs(parent->transform().determinant());
  if (det <= 0.0) return 1.0;

  return std::sqrt(det);
}

QTransform SchItemBase::labelBaseXform(double ownRotDeg) const {
  QTransform xform;
  xform.rotate(ownRotDeg);

  // Cancel the parent's SCALE: a diagonal symbol is drawn 41% larger,
  // its labels are not (see parentScale()).
  const double pscale = parentScale();
  if (pscale > 0.0 && !qFuzzyCompare(pscale, 1.0)) xform.scale(1.0 / pscale, 1.0 / pscale);

  // Cancel the parent's mirror, so the glyphs do not read backwards.
  if (parentMirrored()) xform.scale(-1.0, 1.0);

  // Counteract the view's Y-flip, just for text.
  xform.scale(1.0, -1.0);

  return xform;
}

bool SchItemBase::labelTurned(double ownRotDeg) const {
  // Symbol labels only. Free text on the schematic is drawn as its own
  // record says.
  if (parentItem() == nullptr) return false;

  // The label's READING direction -- the layout frame's +x -- as it
  // lands in the scene. The scene is Y-up and the view flips only Y, so
  // a direction pointing left in the scene points left on screen too:
  // the text would read right to left, upside down.
  const QTransform toScene = labelBaseXform(ownRotDeg) * sceneTransform();
  const QPointF    dir     = toScene.map(QPointF(1.0, 0.0)) - toScene.map(QPointF(0.0, 0.0));

  const double len = std::hypot(dir.x(), dir.y());
  if (len <= 0.0) return false;

  // Strictly leftward: vertical labels are left alone.
  constexpr double kVerticalTolerance = 1e-6;
  return dir.x() < -kVerticalTolerance * len;
}

QTransform SchItemBase::labelXform(double ownRotDeg) const {
  QTransform xform = labelBaseXform(ownRotDeg);

  // Half a turn about the anchor. It commutes with every other step --
  // it is the negation of both axes -- so where it goes in the chain
  // does not matter.
  if (labelTurned(ownRotDeg)) xform.scale(-1.0, -1.0);

  return xform;
}

int SchItemBase::labelAlignH(int alignH, double /*ownRotDeg*/) const {
  // The MIRROR cancellation unflips the text box along with the glyphs,
  // so a west-anchored label would extend the wrong way. CONFIRMED
  // against a mirrored component (code 12).
  //
  // NOT the readability turn: QSpice turns the box about its anchor and
  // lets it move to the other side. Swapping to keep it in place put
  // R10, R15 and R16 of PolarityTest.qsch on top of the resistor -- the
  // side they were on before the turn was the wrong one too.
  if (!parentMirrored()) return alignH;

  if (alignH == sch::core::kAlignHWest) return sch::core::kAlignHEast;
  if (alignH == sch::core::kAlignHEast) return sch::core::kAlignHWest;
  return alignH; // centred is unaffected
}

int SchItemBase::labelAlignV(int alignV, double /*ownRotDeg*/) const {
  // Unchanged: a mirror is left-right, and the readability turn moves
  // the box with it (see labelAlignH()).
  return alignV;
}

bool SchItemBase::parentMirrored() const {
  const QGraphicsItem *parent = parentItem();
  if (parent == nullptr) return false;

  // Determinant, not m11() < 0. A reflection is exactly what a negative
  // determinant means, whatever combination of rotation and mirroring
  // produced it -- whereas m11() is scale*cos(rotation), which happens
  // to work only because SchItemComp keeps rotation in setRotation()
  // and the mirror alone in transform(). That's an invariant this class
  // shouldn't have to depend on, and the same m11()-is-the-scale
  // assumption already produced a real bug in the selection highlight.
  return parent->transform().determinant() < 0.0;
}

QPainterPath SchItemBase::shape() const {
  QRectF rect = elementRect();

  // Never zero-area. An axis-aligned line, wire or rect has a rect with
  // no height or no width, and a zero-area QPainterPath behaves
  // unpredictably in Qt's ContainsItemShape / IntersectsItemShape
  // tests -- which is how rubber-band selection came to miss elements
  // lying entirely inside the band, with no obvious pattern to which.
  //
  // kMinShapeSize is far below the 100-unit electrical grid, so this
  // cannot make the wrong element easier to hit; it only stops the path
  // being degenerate.
  if (rect.width() < kMinShapeSize)
    rect.adjust(-kMinShapeSize / 2.0, 0.0, kMinShapeSize / 2.0, 0.0);
  if (rect.height() < kMinShapeSize)
    rect.adjust(0.0, -kMinShapeSize / 2.0, 0.0, kMinShapeSize / 2.0);

  QPainterPath path;
  path.addRect(rect);
  return path;
}

void SchItemBase::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                        QWidget *) {
  QPen pen(QColor(180, 60, 180), 0, Qt::DashLine); // TODO: placeholder only
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);
  painter->drawRect(elementRect());
}

SchItemBase::HitLevel SchItemBase::hitLevel(const QPointF &localPos,
                                            double tolerance) const {
  const QPainterPath outline = outlinePath();

  if (outline.isEmpty()) {
    const QRectF probe(localPos.x() - tolerance, localPos.y() - tolerance,
                       tolerance * 2.0, tolerance * 2.0);
    return shape().intersects(probe) ? HitLevel::Firm : HitLevel::None;
  }

  // The stroke as wide as it is drawn, plus the tolerance either side.
  const double halfPen =
      (lineWidth() >= 0) ? SchPen::widthUnits(lineWidth()) / 2.0 : 0.0;

  QPainterPathStroker stroker;
  stroker.setWidth((halfPen + tolerance) * 2.0);
  stroker.setCapStyle(Qt::RoundCap);
  stroker.setJoinStyle(Qt::RoundJoin);
  if (stroker.createStroke(outline).contains(localPos)) return HitLevel::Firm;

  // Only a shape with a fill field HAS an interior; an open path (a
  // line, an arc) would otherwise be closed by contains().
  if (fillColorRaw() < 0 || !outline.contains(localPos)) return HitLevel::None;

  return interiorHollow() ? HitLevel::Inside : HitLevel::Weak;
}

bool SchItemBase::interiorHollow() const {
  const int raw = fillColorRaw();
  if (raw < 0) return true;

  // Unstuffed: drawn as an outline -- see SchItemRect::paint().
  if (isUnstuffed()) return true;

  // An image is content.
  const int mode = SchColorDecode::colorMode(raw);
  if (mode >= 0x05 && mode <= 0x0B) return false;

  // Decided by the colour as RESOLVED, so a knockout made any way --
  // mode 0x02, an RGB fill or a solid-fill preference that happens to
  // match the canvas -- reads as empty, and follows the theme.
  const QColor fill = SchColorDecode::fromArgFill(raw, QColor());
  if (fill.alpha() == 0) return true;

  return fill.rgb() == RenderStyle::current().background.rgb();
}

} // namespace sch::render
