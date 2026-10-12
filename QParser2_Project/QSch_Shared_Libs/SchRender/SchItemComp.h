#pragma once
#include "ItemCmp.h"
#include "SchItemBase.h"

namespace sch::render {

// A component placement: position + rotation/mirror only. Contributes no
// visible geometry itself -- its children (the flattened contents of its
// nested symbol) are what actually get painted, positioned via Qt's own
// parent/child item transform composition.
class SchItemComp : public SchItemBase {
public:
  explicit SchItemComp(ItemCmpPtr item, QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;

  // Picking: Body anywhere within the symbol's body box -- hollow or
  // not, the symbol is what a click there means -- or on any of its own
  // elements outside it (a label, a pin's text).
  HitLevel hitLevel(const QPointF &localPos, double tolerance) const override;

  // How many pins the symbol has -- "stuff with jumper" applies only to
  // two-pin components.
  int  pinCount() const;

  int  stuffCode() const override;
  void setStuffCode(int code) override;

  int  orientCode() const override;
  void setOrientCode(int code) override;
  int  orientAfterTurn(bool ccw) const override;
  int  orientAfterMirror() const override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  bool           isElectrical() const override { return true; }
  QList<QPointF> connectionPoints() const override;

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  // True when this component is drawn from the stock resistor symbol.
  bool isResistor() const;

  // Pin 0 and pin 1 in this component's own coordinates, or false when
  // the symbol does not have two pins.
  bool firstTwoPins(Point &p0, Point &p1) const;

private:
  ItemCmpPtr item_;
};

} // namespace sch::render
