#pragma once
#include "ItemCoil.h"
#include "SchBoxFrame.h"
#include "SchItemBase.h"

namespace sch::render {

// pt1/pt2 are the corners of the coil's BOUNDING BOX, as with
// SchItemZigzag -- the record is field-identical to ItemZigzag.
//
// The coil is a 2D projection of a helix (a spring seen from the side),
// not a series of separate loops: one continuous parametric curve whose
// long-axis position advances linearly while a circular component is
// added, so successive turns overlap and cross. See paint() for the
// derivation of the constants, all of which were measured against
// QSpice's own rendering rather than chosen to look right.
class SchItemCoil : public SchItemBase {
public:
  explicit SchItemCoil(ItemCoilPtr item, QGraphicsItem *parent = nullptr);

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

  // The frame this shape draws in -- see SchBoxFrame. Read-only: nothing
  // in the UI writes an odd code.
  //
  // NOT SchBoxFrame's usual frame. MEASURED 2026-10-07 against QSpice
  // (19 of 19 coils, three files, all eight codes, three corner orders):
  //   - EVEN codes ignore the rotation entirely: the frame is the
  //     sheet's own (0 degrees) for codes 0, 2, 4 and 6 alike.
  //   - ODD codes tilt the frame by the FULL code x 45 degrees -- 45,
  //     135, 225 or 315 -- not by 45 for every odd code. For a rect or
  //     an ellipse the four are the same box; for a coil they are not,
  //     because which way the frame is turned decides which of the
  //     box's sides is "along" and which signed corner order applies
  //     (see paint()).
  SchBoxFrame::Frame boxFrame() const {
    const Point p1   = item_->pt1.getValue();
    const Point p2   = item_->pt2.getValue();
    const int   code = item_->rotate.getValue() & 0x7;

    return SchBoxFrame::frameFor(p1.x, p1.y, p2.x, p2.y, code, code * 45.0);
  }

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  ItemCoilPtr item_;
};

} // namespace sch::render
