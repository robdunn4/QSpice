#pragma once
#include "ItemArc4P.h"
#include "SchItemBase.h"

#include <QPainterPath>

namespace sch::render {

// Renders QSpice's four-point «arc» -- distinct from «arc3p», and with
// semantics QSpice does not document. Everything here was established
// by test files; see ItemArc4P.h for the record layout.
//
//   pt1, pt2  opposite corners of the ellipse's bounding box, in a frame
//             rotated by (rotate x 45) degrees -- so an ODD rotation
//             tilts the ellipse 45 degrees, and only the low bit shows
//   pt3, pt4  start and end RAYS from the box centre; distance along the
//             ray is ignored
//
// Drawn COUNTER-CLOCKWISE in QSpice's Y-up coordinates from the start ray
// to the end ray. Start and end on the same ray draw NOTHING -- unlike
// Windows GDI's Arc(), which would draw the whole ellipse.
class SchItemArc4P : public SchItemBase {
public:
  explicit SchItemArc4P(ItemArc4pPtr item, QGraphicsItem *parent = nullptr);

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

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  // The dashed bounding box (tilted with an odd rotation), a dot at the
  // centre the rays start from, and dashed rays out to the two ray
  // handles.
  EditGuide editGuide() const override;

private:
  // The ellipse's frame, shared by the drawing and the edit guide so the
  // two cannot disagree about where the ellipse is.
  struct Frame {
    QPointF centre;
    double  cosT = 1.0;
    double  sinT = 0.0;
    double  a    = 0.0; // semi-axes, in the frame
    double  b    = 0.0;

    QPointF toFrame(const QPointF &v) const;
    QPointF fromFrame(const QPointF &v) const;
  };

  Frame frame() const;

  // The arc as a polyline, in item coordinates. Empty when there is
  // nothing to draw -- a degenerate box, or both rays the same.
  QPainterPath arcPath() const;

  // Picking: the arc as drawn -- see SchItemBase::hitLevel().
  QPainterPath outlinePath() const override { return arcPath(); }

  ItemArc4pPtr item_;
};

} // namespace sch::render
