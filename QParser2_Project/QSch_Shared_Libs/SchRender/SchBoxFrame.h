#pragma once

#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QTransform>

#include <cmath>
#include <numbers>

namespace sch::render {

// The frame a BOX-DEFINED shape lives in.
//
// rect, ellipse, coil, zigzag and the four-point arc each carry an
// ArgRot beside their two corner points, and it means the same thing in
// all of them: the corners are opposite corners of a box in a frame
// ROTATED by (code x 45) degrees. So an odd code tilts the shape 45
// degrees and changes its proportions.
//
// Only ODD versus EVEN matters. Any multiple of 90 degrees maps a box
// given by opposite corners onto itself, so codes 0, 2, 4 and 6 all look
// alike, and so do 1, 3, 5 and 7. Measured across all sixteen codes on
// the arc, where this was the field's one visible effect.
//
// READ but never written. QSpice renders odd codes yet offers no way to
// set one, and neither do we: Ctrl+R transforms an element's geometry
// rather than touching this field. Honouring it on load is what matters
// -- an odd-coded ellipse from QSpice used to draw here as an upright
// one, wrong shape and wrong size, with nothing to say so.
namespace SchBoxFrame {

struct Frame {
  QPointF centre;
  double  cosT = 1.0;
  double  sinT = 0.0;
  double  a    = 0.0; // half-width, in the frame
  double  b    = 0.0; // half-height

  bool tilted() const { return std::abs(sinT) > 1e-12; }

  QPointF fromFrame(const QPointF &v) const {
    return QPointF(v.x() * cosT - v.y() * sinT, v.x() * sinT + v.y() * cosT);
  }

  QPointF toFrame(const QPointF &v) const {
    return QPointF(v.x() * cosT + v.y() * sinT, -v.x() * sinT + v.y() * cosT);
  }

  // The shape's own rect, centred on the origin, drawn with the painter
  // already translated to centre and rotated by degrees().
  QRectF localRect() const { return QRectF(-a, -b, a * 2.0, b * 2.0); }

  double degrees() const {
    return std::atan2(sinT, cosT) * 180.0 / std::numbers::pi;
  }

  // Frame -> item coordinates: the painter's translate(centre) and
  // rotate(degrees()), as a transform.
  QTransform transform() const {
    QTransform t;
    t.translate(centre.x(), centre.y());
    t.rotate(degrees());
    return t;
  }

  // `local` (frame coordinates) in item coordinates.
  QPainterPath toItem(const QPainterPath &local) const {
    return transform().map(local);
  }

  // Where the shape sits in scene coordinates -- the bounding box of the
  // rotated rect, for hit-testing and repaint bounds.
  QRectF sceneBounds() const {
    double minX = 0.0, maxX = 0.0, minY = 0.0, maxY = 0.0;
    bool   first = true;

    for (const QPointF &corner :
         {QPointF(-a, -b), QPointF(a, -b), QPointF(a, b), QPointF(-a, b)}) {
      const QPointF p = centre + fromFrame(corner);

      if (first) {
        minX = maxX = p.x();
        minY = maxY = p.y();
        first = false;
        continue;
      }

      minX = std::min(minX, p.x());
      maxX = std::max(maxX, p.x());
      minY = std::min(minY, p.y());
      maxY = std::max(maxY, p.y());
    }

    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
  }
};

// Plain coordinates rather than the record's Point, so this header stays
// free of the parser's types and of SchItemBase's toQ().
//
// `oddDegrees` is the tilt an odd code gives the frame: +45 by default.
// For a shape symmetric under a quarter turn -- rect, ellipse -- the sign
// is invisible (frames at +45 and -45 have the same axes, a and b merely
// swapped).
//
// The default is RIGHT for everything but the coil -- CONFIRMED
// 2026-10-07 against QSpice: rects, ellipses, IMAGE rects (all eight
// codes, all four corner orders -- the picture neither turns with the
// code nor flips with the corner order) and both arcs match as they
// are. The coil alone follows a different, measured rule (code x 45,
// signed corners): see SchItemCoil::boxFrame(). Do NOT move that rule
// in here -- image rects would turn by 135 or 225 degrees and stop
// matching.
inline Frame frameFor(double x1, double y1, double x2, double y2,
                      int rotCode, double oddDegrees = 45.0) {
  Frame f;

  const double theta =
      (rotCode & 1) ? (oddDegrees * std::numbers::pi / 180.0) : 0.0;

  f.cosT = std::cos(theta);
  f.sinT = std::sin(theta);

  // The centre is the midpoint of the corners whatever the frame, so
  // there is no pivot to choose.
  f.centre = QPointF((x1 + x2) / 2.0, (y1 + y2) / 2.0);

  const QPointF diag = f.toFrame(QPointF(x2 - x1, y2 - y1));
  f.a = std::abs(diag.x()) / 2.0;
  f.b = std::abs(diag.y()) / 2.0;

  return f;
}

} // namespace SchBoxFrame

} // namespace sch::render
