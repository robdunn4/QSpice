#pragma once

#include <QPainterPath>
#include <QPolygonF>

#include "ItemTap.h"
#include "SchItemBase.h"

namespace sch::render {

// A BUS TAP: logically a short wire from a bus to one of its members,
// stored as its own «tap (x1,y1) (x2,y2)» record. Always 100 units long,
// or 141 on a diagonal.
//
// Its appearance says whether it is CORRECTLY used, and depends on what
// its two ends touch -- confirmed against BusStuff_QSpice.qsch:
//
//   VALID    one end on a bus wire, the other on a net that is one of
//            that bus's members: a thin triangle in the WIRE colour,
//            base on the bus, point at the member.
//   INVALID  one end on a bus, the other on a net that is NOT a member
//            (wrong name or index out of range): the same triangle in
//            the bus-tap ERROR colour.
//   LOOSE    no bus at either end, a bus at BOTH ends, or a bus at one
//            end and nothing at the other: a spindle in the error colour.
//
// Shapes are built from the tap's own vector d (end to end) and its
// perpendicular n, so a diagonal tap is automatically sqrt(2) larger --
// which is what QSpice draws, as for its 45-degree rotation codes.
//
// Not split by, and never splitting, a bus wire; never joining its
// member net to the bus net. Its ends ARE connection points for the
// member side: wires, labels, and label anchoring.
class SchItemTap : public SchItemBase {
public:
  explicit SchItemTap(ItemTapPtr item, QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  bool           isElectrical() const override { return true; }
  QList<QPointF> connectionPoints() const override;

  // The drawn shape, plus a click tolerance along the centreline so a
  // thin triangle's point is still easy to hit.
  QPainterPath shape() const override;

  // The two ends, in FILE order -- never reordered, so a file round-trips
  // byte for byte.
  Point end(int index) const;

  // Moves one end. controlPoints() stays EMPTY on purpose: a tap's shape
  // is fixed, so it offers no end handles and is never picked by an end.
  // This exists for EditControlPointCmd, which is how a turn is undone.
  void setControlPoint(int index, Point p) override;

  // The end a single tap turns and mirrors about: the LEFT end of a
  // horizontal tap, otherwise the TOP-most end. Not the midpoint, which
  // a 45-degree step would put off the 100-unit grid.
  int pivotIndex() const;

  // A tap's end-to-end vector after one step of a transform. The vector
  // is always one of the eight directions with components 0 or +/-L, and
  // a 45-degree turn moves it to the NEXT of those directions -- which
  // swaps a diagonal (L, L) for a straight (L, 0), changing its length
  // the way QSpice's 45-degree rotation codes do, so both ends stay on
  // the grid. A mirror negates one component.
  static Point turned45(Point v, bool ccw);

  enum class State { Loose, Valid, Invalid };

  // Read from the scene as it stands -- see the class comment.
  State state() const;

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

  // The outline for the current state, in scene coordinates.
  QPolygonF outline() const;

private:
  ItemTapPtr item_;
};

} // namespace sch::render
