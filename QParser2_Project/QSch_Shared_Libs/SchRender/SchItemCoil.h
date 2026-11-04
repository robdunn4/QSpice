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

  DrawKind drawKind() const override { return DrawKind::Coil; }

  QList<Point> controlPoints() const override;
  void         setControlPoint(int index, Point p) override;

  // The rotate code, as the element's orientation, so the editor's
  // turns and mirrors change it as QSpice does -- see codeAfterTurn().
  int  orientCode() const override { return item_->rotate.getValue(); }
  void setOrientCode(int code) override {
    prepareGeometryChange(); // an odd code tilts the box
    item_->rotate.setValue(code);
    update();
  }
  int orientAfterTurn(bool ccw) const override {
    return codeAfterTurn(orientCode(), ccw);
  }
  int orientAfterMirror() const override {
    return codeAfterMirror(orientCode());
  }

  // What QSpice writes for a coil's rotate code -- MEASURED 2026-11-04
  // (CoilEditTest.qsch, thirteen coils, Ctrl+R and Ctrl+M each):
  //
  //   Ctrl+R  the rotation part +2, the mirror bit kept -- mirrored or
  //           not (0->2, 3->5, 8->10, 11->13). A clockwise turn is
  //           taken as -2 (unmeasured).
  //   Ctrl+M  the mirror bit toggled and the rotation negated (1->15,
  //           2->14, 9->7, 10->6).
  //
  // The corners change too -- not a rigid turn; see the editor's
  // SchViewInteract::coilAfterXform().
  //
  // TO REVISIT: rect, ellipse, arc, three-point arc and zigzag carry the
  // same field and almost certainly follow the same rule in QSpice. The
  // editor still turns them by mapping their points and leaves their
  // code alone -- invisible for the symmetric shapes, likely not for the
  // others. Each needs a sheet like CoilEditTest.qsch.
  static int codeAfterTurn(int code, bool ccw) {
    const int step = ccw ? 2 : 6; // 6 == -2 (mod 8)
    return (code & 0x8) | ((code + step) & 0x7);
  }
  static int codeAfterMirror(int code) {
    return ((code ^ 0x8) & 0x8) | ((8 - (code & 0x7)) & 0x7);
  }

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

}   // namespace sch::render
