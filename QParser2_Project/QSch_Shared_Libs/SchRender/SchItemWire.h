#pragma once
#include <QPen>
#include <string>
#include <QPainterPath>
#include "ItemWire.h"
#include <QString>

#include "SchItemBase.h"

class QPainter;

namespace sch::render {

class SchItemWire : public SchItemBase {
public:
  explicit SchItemWire(ItemWirePtr item, QGraphicsItem *parent = nullptr);

  Rect  getRect() const noexcept override;
  Point getPosition() const noexcept override;
  void  moveTo(Point p) noexcept override;

  // The stored net name in its RAW form -- quotes included, exactly as
  // the record holds it. Used when splitting a wire, so each new
  // segment inherits the original's net without the string making a
  // round trip through quoting (ItemWire's constructor quotes what it
  // is given, which would double-quote an already-quoted value).
  std::string netNameRaw() const;

  // The net this wire belongs to, as assigned by net extraction. Stored
  // raw: a net name is a plain identifier, so it needs none of the
  // quoting or encoding a text element does.
  QString netName() const;
  void    setNetName(const QString &name);

  // The pen a wire is drawn with.
  //
  // In ONE place because more than one thing draws wires: the wire
  // itself, and the live preview the wire tool shows while a segment is
  // being placed. The preview used its own dashed pen, which meant the
  // segment changed appearance at the moment it was committed -- and
  // meant wire styling, once it is configurable (width, style and
  // colour are all coming), would have had to be applied twice.
  static QPen wirePen();

  // The pen for drawing a wire with `painter`. On screen, wirePen().
  // While PRINTING or EXPORTING, the default LINE WIDTH (CADlineWidth),
  // in schematic units like a symbol's own lines -- so wires and symbol
  // line work come out alike on paper and in SVG -- never thinner than
  // the print minimum, with round caps so segments join cleanly.
  static QPen wirePen(const QPainter *painter);

  // A BUS: its net name has the form Name[n1:n2] -- see SchBus.h. Drawn
  // wide, never given junctions, never split by a tap.
  bool isBus() const;

  bool           isElectrical() const override { return true; }
  QList<QPointF> connectionPoints() const override;

  // Stroked outline with a tolerance band, so a one-pixel-wide wire can
  // actually be clicked. See the implementation.
  QPainterPath shape() const override;

  // Half-width of the clickable band around a wire, in scene units
  // (a fifth of the 100-unit electrical grid).
  static constexpr double kClickTolerance = 20.0;

  QList<Point> controlPoints() const override;
  void         setControlPoint(int index, Point p) override;

protected:
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  ItemWirePtr item_;
};

} // namespace sch::render
