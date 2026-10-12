#pragma once
#include "ItemZigzag.h"
#include "SchBoxFrame.h"
#include "SchItemBase.h"

namespace sch::render {

// pt1/pt2 are the corners of the zigzag's BOUNDING BOX (confirmed
// against a real file -- see paint() for details), not the waveform's
// start/end points. The waveform runs between the midpoints of the
// box's two short edges, amplitude = half the box's short dimension --
// both derived from confirmed geometry, not guessed. Still an
// APPROXIMATION in one respect: the peak count (6) is an arbitrary
// choice, and the item's own "rotation" field isn't applied (only the
// parent component's rotation affects it so far, same known gap as
// Rect/Ellipse).
class SchItemZigzag : public SchItemBase {
public:
  explicit SchItemZigzag(ItemZigzagPtr item, QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  QList<Point> controlPoints() const override;
  void         setControlPoint(int index, Point p) override;

  int  lineWidth() const override { return item_->lineWidth.getValue(); }
  void setLineWidth(int w) override {
    prepareGeometryChange();
    item_->lineWidth.setValue(w);
    update();
  }

  int  lineType() const override { return item_->lineType.getValue(); }
  void setLineType(int t) override {
    item_->lineType.setValue(t);
    update();
  }

  int  lineColorRaw() const override { return item_->lineColor.getValue(); }
  void setLineColorRaw(int raw) override {
    item_->lineColor.setValue(raw);
    update();
  }

  // The frame this shape draws in -- see SchBoxFrame. Read-only: an odd
  // code tilts the shape 45 degrees, and nothing in the UI writes one.
  SchBoxFrame::Frame boxFrame() const {
    const Point p1 = item_->pt1.getValue();
    const Point p2 = item_->pt2.getValue();

    return SchBoxFrame::frameFor(p1.x, p1.y, p2.x, p2.y,
                                 item_->rotate.getValue());
  }

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  ItemZigzagPtr item_;
};

} // namespace sch::render
