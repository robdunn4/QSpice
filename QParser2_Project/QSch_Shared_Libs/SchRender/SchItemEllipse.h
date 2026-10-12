#pragma once
#include "ItemEllipse.h"
#include "SchBoxFrame.h"
#include "SchItemBase.h"

namespace sch::render {

class SchItemEllipse : public SchItemBase {
public:
  explicit SchItemEllipse(ItemEllipsePtr item, QGraphicsItem *parent = nullptr);

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

  int  fillColorRaw() const override { return item_->fillColor.getValue(); }
  void setFillColorRaw(int raw) override {
    item_->fillColor.setValue(raw);
    update();
  }

  // Picking: the ellipse as drawn -- see SchItemBase::hitLevel().
  QPainterPath outlinePath() const override;

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
  ItemEllipsePtr item_;
};

} // namespace sch::render
