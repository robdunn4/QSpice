#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemComp.h"
#include "RenderStyle.h"
#include "SchColorDecode.h"
#include "SchStringDecode.h"
#include "SchItemPin.h"
#include "SchPen.h"
#include "SchItemWire.h"
#include "ItemSym.h"
#include "ItemType.h"

#include <QPainter>
#include <QTransform>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <cmath>
#include <utility>

#include "SchItemText.h"
#include "SchRotDecode.h"

namespace sch::render {

SchItemComp::SchItemComp(ItemCmpPtr item, QGraphicsItem *parent)
    : SchItemBase(parent), item_(std::move(item)) {
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemIsMovable, true);

  setPos(toQ(item_->pt1.getValue()));

  // The component's transform, from the MEASURED rotation matrices --
  // see sch::core::rotdecode::rotMatrix() for the evidence and why this is not
  // QTransform::rotate().
  //
  // Set as a single transform rather than setRotation() plus a mirror
  // scale, because the odd codes are not rotations: they carry a
  // sqrt(2) scale that Qt's rotation API cannot express. Everything
  // else follows from this one matrix -- child geometry, pin positions
  // in scene coordinates, hit-testing and bounds all flow through it.
  const int                   rotCode = item_->rotate.getValue();
  const sch::core::rotdecode::RotMatrix m     = sch::core::rotdecode::rotMatrix(rotCode);

  // QTransform's constructor takes (m11, m12, m21, m22, dx, dy) with
  // its own row/column convention: Qt maps x' = m11*x + m21*y, so the
  // matrix is passed TRANSPOSED relative to how rotMatrix() states it.
  setTransform(QTransform(m.m11, m.m21, m.m12, m.m22, 0.0, 0.0));

  // setRotation() is deliberately NOT used: it would compose with the
  // transform above rather than replace it.
  setRotation(0.0);
}

int SchItemComp::stuffCode() const { return item_->stuffShort.getValue(); }

void SchItemComp::setStuffCode(int code) {
  item_->stuffShort.setValue(code);
  update();
}

int SchItemComp::orientCode() const { return item_->rotate.getValue(); }

int SchItemComp::orientAfterTurn(bool ccw) const {
  return sch::core::rotdecode::rotatedCode(orientCode(), ccw);
}

int SchItemComp::orientAfterMirror() const {
  return sch::core::rotdecode::mirroredCodeX(orientCode());
}

void SchItemComp::setOrientCode(int code) {
  prepareGeometryChange();
  item_->rotate.setValue(code & 0xF);

  // The transform is derived entirely from the code, so re-deriving it
  // is all that is needed -- children, pins and bounds all follow.
  const sch::core::rotdecode::RotMatrix m = sch::core::rotdecode::rotMatrix(code);
  setTransform(QTransform(m.m11, m.m21, m.m12, m.m22, 0.0, 0.0));
}

Rect SchItemComp::getRect() const noexcept {
  // Deliberately NOT childrenBoundingRect() (Qt's built-in union of ALL
  // children): CONFIRMED by direct measurement that attribute-text
  // children (reference designator, value, etc.) sit well outside the
  // actual body geometry -- for R1 in Simple.qsch, the zigzag body's own
  // local bounding box is 160x360, but including its two text children
  // stretches that to 405x444. Since R1's component rotation is 90
  // degrees, that local axis becomes the ON-SCREEN vertical axis, so
  // the selection highlight (and rubber-band hit box) ended up much
  // taller than the visible resistor -- "correct horizontally, large
  // margins vertically" is exactly this effect. Excluding SchItemText
  // children fixes this without weakening hit-testing: clicking
  // directly on a label still selects the component correctly via its
  // own shape plus the walk-up-to-parent redirect in
  // SchViewInteract::handleMousePress, which doesn't consult this
  // bounding box at all -- only rubber-band selection (which typically
  // encloses the whole visible component anyway) and the highlight
  // visual are affected, both for the better.
  //
  // childrenBoundingRect() is otherwise in this item's own local
  // coordinate system (pre-this-item's-own-rotation) -- exactly what
  // boundingRect()'s contract expects, confirmed empirically (a
  // rotated+offset parent's childrenBoundingRect() reflects the
  // child's pre-rotation local position, not its rotated scene
  // position). mapRectToParent() on each non-text child's own
  // boundingRect() reproduces that same convention manually.
  QRectF cbr;
  for (QGraphicsItem *child : childItems()) {
    if (dynamic_cast<SchItemText *>(child) != nullptr) continue;
    // bodyRect(), not boundingRect() and not elementRect():
    //
    // boundingRect() carries SchItemBase::kPaintSlack, and unioning
    // that in would grow the component's own box -- and with it the
    // selection outline and rubber-band hit area -- by a slack width
    // per child, for geometry that isn't there.
    //
    // elementRect() covers everything a child PAINTS, which for a pin
    // includes its label. Pin labels can sit well outside the symbol's
    // outline, and counting them meant an exclusive band had to enclose
    // them before it would select the component -- so a band drawn
    // neatly around the visible symbol selected nothing. Selection
    // examines the core symbol geometry only.
    SchItemBase *schChild = dynamic_cast<SchItemBase *>(child);

    // An element with an EMPTY body contributes nothing -- an internal
    // pin, say. Skipped explicitly rather than unioned, because a null
    // rect maps to a point at the child's origin and would pull the
    // component's box toward it.
    const QRectF childBody =
        (schChild != nullptr) ? schChild->bodyRect() : child->boundingRect();
    if (childBody.isNull()) continue;

    const QRectF childRect = child->mapRectToParent(childBody);
    cbr                    = cbr.isNull() ? childRect : cbr.united(childRect);
  }

  if (cbr.isNull() || cbr.isEmpty()) {
    const Point p = item_->pt1.getValue();
    return Rect(p, p);
  }
  return Rect(Point(static_cast<int>(std::floor(cbr.left())),
                    static_cast<int>(std::floor(cbr.top()))),
              Point(static_cast<int>(std::ceil(cbr.right())),
                    static_cast<int>(std::ceil(cbr.bottom()))));
}

QList<QPointF> SchItemComp::connectionPoints() const {
  // A component connects through its pins, which are child items. Their
  // points are in each child's coordinates, so they get mapped into
  // this item's own -- the caller then maps once more to the scene.
  QList<QPointF> points;
  for (QGraphicsItem *child : childItems()) {
    const SchItemBase *schChild = dynamic_cast<const SchItemBase *>(child);
    if (schChild == nullptr) continue;
    for (const QPointF &p : schChild->connectionPoints())
      points.append(child->mapToParent(p));
  }
  return points;
}

Point SchItemComp::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemComp::moveTo(Point p) noexcept {
  item_->pt1.setValue(p);
  setPos(toQ(p));
}

bool SchItemComp::isResistor() const {
  if (node() == nullptr) return false;

  const NodePtr symNode = node()->findFirstOf(QPI::SYM);
  if (symNode == nullptr) return false;

  // By the symbol's TYPE record, not its name. «symbol X» is only what
  // the symbol is called -- a resistor symbol could be named anything --
  // while «type: R» is what makes it a resistor.
  if (const NodePtr typeNode = symNode->findFirstOf(QPI::TYPE)) {
    const ItemTypePtr type =
        std::static_pointer_cast<ItemType>(typeNode->item());
    if (type != nullptr)
      return sch::core::decodeQSpiceString(type->text.getValue()).trimmed() ==
             QStringLiteral("R");
  }

  // No type record: fall back to the name, which for QSpice's stock
  // resistor is the same "R". Better than never marking one at all.
  const ItemSymPtr sym = std::static_pointer_cast<ItemSym>(symNode->item());
  if (sym == nullptr) return false;

  return sch::core::decodeQSpiceString(sym->text.getValue()).trimmed() ==
         QStringLiteral("R");
}

int SchItemComp::pinCount() const {
  int n = 0;
  for (const QGraphicsItem *child : childItems())
    if (dynamic_cast<const SchItemPin *>(child) != nullptr) ++n;

  return n;
}

bool SchItemComp::firstTwoPins(Point &p0, Point &p1) const {
  // In RECORD order, which is what pin index means here -- the same
  // positional convention as ArgPinNdx and the refdes.
  QList<const SchItemPin *> pins;

  for (const QGraphicsItem *child : childItems()) {
    if (const SchItemPin *pin = dynamic_cast<const SchItemPin *>(child))
      pins.append(pin);
  }

  if (pins.size() < 2) return false;

  const QList<Point> a = pins[0]->controlPoints();
  const QList<Point> b = pins[1]->controlPoints();
  if (a.isEmpty() || b.isEmpty()) return false;

  p0 = a.first();
  p1 = b.first();
  return true;
}

void SchItemComp::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                        QWidget *) {
  // --- Stuffed with a JUMPER ---
  //
  // The part is not fitted; a link is soldered across instead. So the
  // symbol is drawn grey like any unstuffed component -- each element
  // tints itself -- and a wire is drawn between its two pins.
  //
  // Wire-coloured, NOT grey: it is a real connection, and drawn with
  // the same pen SchItemWire uses so the two cannot look different.
  // This is also why the grey is applied per element rather than by
  // tinting the whole component: a subtree tint would take the jumper
  // with it.
  //
  // TWO-PIN ONLY. A jumper across three pins has no meaning, and
  // QSpice only offers it for two.
  if (stuffCode() == 2 && pinCount() == 2) {
    Point a, b;
    if (firstTwoPins(a, b)) {
      // The wire's own pen, WIDENED: same colour and style, so it still
      // reads as a connection, but heavier so it reads as a deliberate
      // link rather than a wire that happens to run past.
      //
      // Cosmetic, so the width is in device pixels and holds at any
      // zoom -- the same treatment wires get.
      QPen pen = SchItemWire::wirePen();
      pen.setCosmetic(true);
      pen.setWidthF(jumperWidthPx());
      pen.setCapStyle(Qt::RoundCap);
      SchPen::enforcePrintMin(pen, painter);

      painter->setPen(pen);
      painter->setBrush(Qt::NoBrush);
      painter->drawLine(toQ(a), toQ(b));
    }
  }

  // The component itself is invisible -- see the class comment -- apart
  // from one addition of our own, and that only when asked for.
  if (!showResistorOrient() || !isResistor()) return;

  Point p0, p1;
  if (!firstTwoPins(p0, p1)) return;

  // A small "+" beside pin 0, marking the polarity convention.
  //
  // Placed in the component's OWN frame, relative to the line from pin
  // 0 to pin 1, so it follows the symbol through every rotation and
  // mirror without being computed for each one. Think of the resistor
  // laid out horizontally with pin 0 at the left: the marker sits just
  // right of that pin and just below the axis.
  const double dx = p1.x - p0.x;
  const double dy = p1.y - p0.y;

  const double len = std::hypot(dx, dy);
  if (len <= 0.0) return;

  // Along the body, and perpendicular to it. For a left-to-right axis
  // the perpendicular points DOWN in model coordinates, which is the
  // "slightly below" of the description.
  const QPointF axis(dx / len, dy / len);
  QPointF       perp(axis.y(), -axis.x());

  // The SIDE is a screen convention -- the same side of the drawn line
  // from pin 0 to pin 1 at every orientation -- but this is drawn in the
  // symbol's own frame, which a mirror reflects. Reflect the side back,
  // or a mirrored resistor shows its marker at the right end on the
  // wrong side (PolarityTest.qsch, R9-R16).
  if (sch::core::rotdecode::isMirrored(orientCode())) perp = -perp;

  // Deliberately small and close in: the marker is a hint about which
  // pin is which, not a feature of the drawing, and it should read as
  // belonging to that pin rather than floating beside the body.
  constexpr double kAlongPin  = 35.0; // in from the pin
  constexpr double kOffAxis   = 28.0; // off the axis
  constexpr double kArmLength = 9.0;

  const QPointF centre(p0.x + axis.x() * kAlongPin + perp.x() * kOffAxis,
                       p0.y + axis.y() * kAlongPin + perp.y() * kOffAxis);

  // Drawn along the symbol's own axes rather than the screen's, so the
  // "+" tilts with a 45-degree symbol instead of staying upright and
  // looking detached from it.
  const QPointF arm1 = axis * kArmLength;
  const QPointF arm2 = perp * kArmLength;

  painter->save();

  QPen pen(SchColorDecode::fromArgColor(0x01000000,
                                        RenderStyle::current().graphicAnnotation));
  pen.setCosmetic(true);
  SchPen::enforcePrintMin(pen, painter);
  painter->setPen(pen);

  painter->drawLine(centre - arm1, centre + arm1);
  painter->drawLine(centre - arm2, centre + arm2);

  painter->restore();
}

SchItemBase::HitLevel SchItemComp::hitLevel(const QPointF &localPos,
                                            double tolerance) const {
  const QRectF body = elementRect().adjusted(-tolerance, -tolerance, tolerance,
                                             tolerance);
  if (body.contains(localPos)) return HitLevel::Body;

  for (const QGraphicsItem *child : childItems()) {
    const SchItemBase *schChild = dynamic_cast<const SchItemBase *>(child);
    if (schChild == nullptr || !schChild->isVisible()) continue;

    const HitLevel level =
        schChild->hitLevel(child->mapFromParent(localPos), tolerance);
    if (level >= HitLevel::Weak) return HitLevel::Body;
  }

  return HitLevel::None;
}

} // namespace sch::render
