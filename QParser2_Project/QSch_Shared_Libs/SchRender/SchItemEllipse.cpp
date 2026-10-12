#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchItemEllipse.h"

#include "SchPen.h"
#include "SchBoxFrame.h"

#include <QPainter>
#include <QPen>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <utility>

#include "RenderStyle.h"
#include "SchColorDecode.h"

namespace sch::render {

SchItemEllipse::SchItemEllipse(ItemEllipsePtr item, QGraphicsItem *parent)
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

  setZValue(kZFill);
}

Rect SchItemEllipse::getRect() const noexcept {
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

Point SchItemEllipse::getPosition() const noexcept {
  return item_->pt1.getValue();
}

void SchItemEllipse::moveTo(Point p) noexcept {
  prepareGeometryChange();
  const Point delta = p - getPosition();
  item_->pt1.setValue(item_->pt1.getValue() + delta);
  item_->pt2.setValue(item_->pt2.getValue() + delta);
}

QList<Point> SchItemEllipse::controlPoints() const {
  return {item_->pt1.getValue(), item_->pt2.getValue()};
}

void SchItemEllipse::setControlPoint(int index, Point p) {
  prepareGeometryChange();
  if (index == 0) item_->pt1.setValue(p);
  else if (index == 1) item_->pt2.setValue(p);
}

void SchItemEllipse::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                           QWidget *) {
  const QColor lineColor = SchColorDecode::fromArgColor(
      item_->lineColor.getValue(), RenderStyle::current().graphicAnnotation);

  QPen pen = SchPen::forElement(painter, tint(lineColor),
                                item_->lineWidth.getValue(),
                                item_->lineType.getValue());
  painter->setPen(pen);

  const QColor fill =
      SchColorDecode::fromArgFill(item_->fillColor.getValue(), lineColor);

  // Unstuffed: OUTLINE only -- see SchItemRect::paint().
  if (isUnstuffed()) painter->setBrush(Qt::NoBrush);
  else painter->setBrush(fill);

  // elementRect(), NOT boundingRect(). This is the element's own
  // geometry; boundingRect() is the REPAINT region and carries
  // SchItemBase::kPaintSlack plus the children's extent.
  //
  // CONFIRMED regression: drawing boundingRect() here inflated every
  // rect and ellipse by a slack width on each side -- invisible on
  // large shapes, glaring on the thin 4-unit bars inside the V symbol,
  // which came out roughly ten times too thick. It also painted right
  // at the edge of the reported bounds, so the cosmetic pen spilled
  // past them and the drag smearing came straight back.
  // Drawn in the shape's OWN frame -- see SchItemRect::paint().
  const SchBoxFrame::Frame frame = boxFrame();

  painter->save();
  painter->translate(frame.centre);
  painter->rotate(frame.degrees());

  painter->drawEllipse(frame.localRect());

  painter->restore();
}

QPainterPath SchItemEllipse::outlinePath() const {
  const SchBoxFrame::Frame frame = boxFrame();

  QPainterPath local;
  local.addEllipse(frame.localRect());
  return frame.toItem(local);
}

} // namespace sch::render
