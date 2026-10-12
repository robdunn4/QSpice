#pragma once

#include <QPainter>
#include <QPen>
#include <QTransform>

#include <cmath>

#include "RenderStyle.h"

namespace sch::render {

// Line width and style for graphic elements, as QSpice draws them.
//
// Both were MEASURED against QSpice's own rendering; neither is
// documented. See the checkpoint for the test schematics.
//
//   thickness = (w + 1) SCHEMATIC UNITS, where w is the record's
//   lineWidth -- except that lineWidth 0 means "default", and
//   substitutes CADlineWidth from the registry (1.5 unless changed).
//
// So the thickness is in schematic units and GROWS as the view zooms
// in, unlike the cosmetic pens everything used before. Confirmed by
// measuring widths 1-7 at several zooms -- 2,3,4,5,6,7,8 px at 0.956
// units/px, exact at every step -- and by setting CADlineWidth to 5,
// where the default line then matched width 5 exactly in the same
// image.
//
// Line TYPE is Windows' pen style, value for value: 0 solid, 1 dash,
// 2 dot, 3 dash-dot, 4 dash-dot-dot. QSpice offers no other values.
namespace SchPen {

// While PRINTING: the thinnest line allowed, in the PRINTER's device
// pixels (the "minimum printed line width" converted for the device by
// SchPrint). 0 -- the screen -- means no minimum beyond Qt's own.
//
// Needed because a 1-pixel line on screen is a 1-dot line on paper --
// 0.04 mm at 600 dpi, all but invisible. Single-threaded painting; set
// and cleared around each printed page.
inline double printMinDevicePx = 0.0;

// While EXPORTING VECTORS (SVG): no cosmetic pens. Qt writes a cosmetic
// pen as a NON-SCALING stroke -- a fixed number of the viewer's SCREEN
// pixels at any zoom -- so each is turned into an ordinary pen of the
// same width on the page. Set and cleared around the export.
inline bool vectorExport = false;

// The scale `painter` draws at, device pixels per local unit: hypot of
// the transform's first row, not m11, which is scale*cos of a rotation.
inline double painterScale(const QPainter *painter) {
  if (painter == nullptr) return 1.0;
  const QTransform xform = painter->transform();
  return std::hypot(xform.m11(), xform.m12());
}

// A cosmetic pen as a GEOMETRIC one of the same device width, for the
// transform `painter` has NOW -- only while vectorExport is set.
inline void toGeometric(QPen &pen, const QPainter *painter) {
  if (!vectorExport || !pen.isCosmetic()) return;

  const double scale = painterScale(painter);
  if (scale <= 0.0) return;

  // Width 0 is Qt's "one device pixel".
  const double width = (pen.widthF() <= 0.0) ? 1.0 : pen.widthF();
  pen.setCosmetic(false);
  pen.setWidthF(width / scale);
}

// A COSMETIC pen (width in device pixels -- wires, outlines) widened to
// the print minimum while printing. Every cosmetic pen that draws part
// of the DOCUMENT goes through this.
inline void enforcePrintMin(QPen &pen) {
  if (printMinDevicePx <= 0.0 || !pen.isCosmetic()) return;

  // Width 0 is Qt's "one device pixel".
  const double width = (pen.widthF() <= 0.0) ? 1.0 : pen.widthF();
  if (width < printMinDevicePx) pen.setWidthF(printMinDevicePx);
}

// The same, then -- when exporting vectors -- made geometric for
// `painter`'s CURRENT transform. Call it where the pen is about to be
// used, so the transform is the one it will draw with.
inline void enforcePrintMin(QPen &pen, const QPainter *painter) {
  enforcePrintMin(pen);
  toGeometric(pen, painter);
}

// Dash patterns, in multiples of the pen width, as QPen expects.
//
// Qt's own DotLine is a dash ONE pen-width long. With a round cap that
// becomes a capsule TWO widths long -- a short dash, not a dot. A
// ZERO-length dash with a round cap draws as a circle, which is what a
// dot should be, so the dotted styles are spelled out here rather than
// using Qt's built-ins.
//
// The dash lengths match Qt's own (4 on, 2 off) so type 1 is unchanged.
inline void applyStyle(QPen &pen, int lineType) {
  // Not exactly zero: a zero-length dash is degenerate and some paint
  // engines drop it entirely. A hair above zero renders as the cap
  // alone, which is the circle wanted.
  constexpr double kDot = 0.001;

  switch (lineType) {
  case 1:
    pen.setStyle(Qt::DashLine);
    return;

  case 2:
    pen.setStyle(Qt::CustomDashLine);
    pen.setDashPattern({kDot, 3.0});
    return;

  case 3:
    pen.setStyle(Qt::CustomDashLine);
    pen.setDashPattern({4.0, 2.0, kDot, 2.0});
    return;

  case 4:
    pen.setStyle(Qt::CustomDashLine);
    pen.setDashPattern({4.0, 2.0, kDot, 2.0, kDot, 2.0});
    return;

  default:
    pen.setStyle(Qt::SolidLine);
    return;
  }
}

// The plain style, where the pattern does not matter (hit-testing and
// anything that only wants to know solid from patterned).
inline Qt::PenStyle styleFor(int lineType) {
  switch (lineType) {
  case 1: return Qt::DashLine;
  case 2: return Qt::DotLine;
  case 3: return Qt::DashDotLine;
  case 4: return Qt::DashDotDotLine;
  default: return Qt::SolidLine;
  }
}

// Thickness in schematic units.
inline double widthUnits(int lineWidth) {
  const double w =
      (lineWidth == 0) ? RenderStyle::current().cadLineWidth : double(lineWidth);

  return w + 1.0;
}

// Builds the pen for an element, given the painter it will draw with.
//
// The painter matters because of the MINIMUM: a line thinner than one
// device pixel would fade out or vanish when the view is zoomed far
// out, so below that it falls back to a cosmetic pen, which Qt always
// draws one pixel wide. QSpice's thin lines never disappear either.
//
// Note the transform includes any enclosing COMPONENT's scale, so a
// symbol's lines thicken with the symbol -- including the sqrt(2) of a
// 45-degree rotation.
inline QPen forElement(const QPainter *painter, const QColor &color,
                       int lineWidth, int lineType) {
  QPen pen(color);
  applyStyle(pen, lineType);
  pen.setJoinStyle(Qt::MiterJoin);

  // ROUND caps on the patterned styles, so each dash and dot is
  // rounded rather than a hard-ended bar -- which is what QSpice draws,
  // and what makes a dotted line read as dots at all once it is thick.
  //
  // A solid line keeps FLAT caps: round ones would push it half a
  // pen-width past each endpoint, so a thick line would overshoot its
  // own geometry and corners would bulge.
  pen.setCapStyle((lineType == 0) ? Qt::FlatCap : Qt::RoundCap);

  const double units = widthUnits(lineWidth);

  double scale = 1.0;
  if (painter != nullptr) {
    const QTransform xform = painter->transform();
    scale = std::hypot(xform.m11(), xform.m12());
  }

  // Printing: never thinner than the print minimum, kept in schematic
  // units so it stays a scaled pen like the rest.
  if (printMinDevicePx > 0.0 && scale > 0.0 &&
      scale * units < printMinDevicePx) {
    pen.setCosmetic(false);
    pen.setWidthF(printMinDevicePx / scale);
    return pen;
  }

  if (scale * units < 1.0) {
    pen.setCosmetic(true);
    pen.setWidth(0);
    toGeometric(pen, painter);
    return pen;
  }

  pen.setCosmetic(false);
  pen.setWidthF(units);
  return pen;
}

} // namespace SchPen

} // namespace sch::render
