#pragma once
#include "ItemJunction.h"
#include "SchItemBase.h"

namespace sch::render {

// The blue dot drawn where three or more wire endpoints meet.
//
// Junctions are DERIVED, not authored: SchConnect::updtJunctions()
// creates and destroys them so that a junction exists at exactly those
// coordinates where three or more wire endpoints coincide. Nothing else
// should add or remove one -- see the invariant described there.
class SchItemJunction : public SchItemBase {
public:
  explicit SchItemJunction(ItemJunctionPtr item,
                           QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  bool           isElectrical() const override { return true; }
  QList<QPointF> connectionPoints() const override;

  // Radius of the drawn dot, in SCENE units: the dot scales with the
  // drawing, on screen, in print and in the print page map.
  //
  // radiusLineWidths times the default line thickness
  // (SchPen::widthUnits(0), i.e. CADlineWidth + 1 -- 2.5 units in a
  // stock installation), times QSpice's JunctionSize preference.
  static double dotRadius();

  // The multiplier, from our preferences
  // ([Display] JunctionRadiusLineWidths, default 10 -- 25 units).
  // Static because items have no route to the Prefs object; SchView
  // sets it.
  static void setRadiusLineWidths(double n) {
    if (n > 0.0) radiusLineWidths = n;
  }

protected:
  static inline double radiusLineWidths = 10.0;

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  ItemJunctionPtr item_;
};

} // namespace sch::render
