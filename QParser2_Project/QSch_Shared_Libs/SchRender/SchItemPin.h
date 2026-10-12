#pragma once
#include <QRectF>
#include "ItemPin.h"
#include "SchItemBase.h"

namespace sch::render {

// Renders only a small marker at the pin's connection point (pt1). Note
// pt2 is documented as the label offset, not a line endpoint -- the
// stub line QSpice draws for a pin's lead is a separate explicit LINE
// element in the file (see Simple.qsch), not part of PIN itself.
//
// STUB: the pin label (pinLabel/netName) isn't rendered yet.
class SchItemPin : public SchItemBase {
public:
  explicit SchItemPin(ItemPinPtr item, QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  bool           isElectrical() const override { return true; }

  // Scene-space extent of the pin's label, or a null rect when it has
  // none. Folded into getRect() so the element reports everything it
  // paints.
  QRectF labelRect() const;

  // The connection marker alone, without the label -- see
  // SchItemBase::bodyRect().
  QRectF bodyRect() const override;

  // Whether this pin's label is drawn at all. QSpice hides some -- see
  // the implementation for which and why.
  bool labelVisible() const;

  // Whether this pin should show the "nothing attached here" marker:
  // no net name, and the pin is not one of the implicit ones.
  bool showsUnconnectedMark() const;

  // A pin bound to a net INSIDE the symbol is not shown at all: no
  // marker, no label, and no contribution to the symbol's bounds.
  //
  // The AnalogLatch's inverted-Q pin is the case -- it carries a net
  // name of its own, and QSpice draws nothing for it. We were drawing
  // its marker and counting it in the component's box, which put a
  // stray dot outside the body and made the selection rectangle and
  // band-selection bounds larger than the symbol.
  bool isInternal() const;

  // Horizontal alignment as DRAWN. A mirrored parent swaps WEST and
  // EAST, so a label that sits inside the body upright still sits
  // inside it mirrored -- the same swap SchItemText does, and missing
  // here is why mirrored symbols hung their pin labels outside.
  int effectiveAlignH() const;
  int effectiveAlignV() const;
  QList<QPointF> connectionPoints() const override;

  QList<Point> controlPoints() const override;
  void         setControlPoint(int index, Point p) override;

  int  textColorRaw() const override { return item_->textColor.getValue(); }
  void setTextColorRaw(int raw) override {
    item_->textColor.setValue(raw);
    update();
  }

  // Drawn in HIGHLIGHT colour while its row is selected in the Symbol
  // Properties pane, so the pane and the drawing point at each other.
  // The net this pin is ON, for display.
  //
  // The pin's OWN netName wins when set: that is an internal connection
  // written into the symbol, and it overrides whatever is wired to the
  // pin -- which is also why such a pin draws no marker.
  //
  // Otherwise the name comes from CONNECTIVITY: the wire or label that
  // meets the pin's coordinate. Connecting a wire deliberately does not
  // write the pin record, so the record has nothing to report and the
  // scene has to be asked.
  QString connectedNetName() const;

  bool isHighlighted() const { return highlighted; }

  void setHighlighted(bool on) {
    if (highlighted == on) return;

    highlighted = on;

    // prepareGeometryChange() as well as update(): the highlight square
    // is larger than the dot it replaces, so the item's painted area
    // grows, and update() alone repaints only the OLD area -- which can
    // leave the new marker unpainted until something else forces a
    // redraw.
    prepareGeometryChange();
    update();
  }

protected:
  bool highlighted = false;


  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  ItemPinPtr item_;
};

} // namespace sch::render
