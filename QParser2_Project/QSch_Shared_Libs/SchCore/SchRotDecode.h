#pragma once
#include <cmath>

#include "ArgUtils.h" // Point

// Decodes the raw ArgRot integer per the confirmed bit-encoding (see
// project notes): mirrored = code & 0x8; rotationDegrees = (code & 0x7)
// * 45. Kept here (not in QParser2_Shared) since ArgRot itself is never
// modified by this project -- these are free functions operating on the
// plain int obtained via ArgRot::getValue()/operator int().
namespace sch::core::rotdecode {
inline bool   isMirrored(int code) { return (code & 0x8) != 0; }
inline double rotationDegrees(int code) { return (code & 0x7) * 45.0; }

// Reverse of rotationDegrees(): recovers the 0-7 rotation code from a
// degrees value that's known to be one of 0/45/90/.../315 (e.g. Qt's
// own QGraphicsItem::rotation(), when that rotation was set via
// rotationDegrees() in the first place, as CompSchItem does). Handles
// negative input correctly (C++'s % truncates toward zero for negative
// operands, so -45 % 8 would otherwise come out -1 instead of 7).
inline int rotationCode(double degrees) {
  int code = static_cast<int>(std::lround(degrees / 45.0)) % 8;
  if (code < 0) code += 8;
  return code;
}

// The transform QSpice actually applies for a rotation code.
//
// NOT a rigid rotation. MEASURED from files in which every rotation
// code appears with wires landing on the rotated pins: a symbol's pin
// at (0,200) ends up 283 units from the anchor under an ODD code, not
// 200. 283 is 200*sqrt(2), and the matrices below are rotation by
// n*45 degrees composed with a sqrt(2) scale on the odd codes.
//
// That is not a quirk -- it is what makes diagonal symbols usable. A
// true 45-degree rotation sends (0,200) to (141.42..., 141.42...),
// irrational coordinates that can never sit on any grid and so can
// never satisfy QSpice's exact-coincidence connection rule. These
// matrices have entries of 0 and +/-1 only, so they map the 100-unit
// lattice onto itself and every rotated pin stays exactly on grid.
//
// The arithmetic is entirely integer. sqrt(2) describes what the
// matrix does to lengths; it never appears in the computation, and
// nothing needs rounding.
//
// Consequence worth knowing: a symbol drawn at an odd code is 41%
// LARGER than the same symbol upright. QSpice draws it that way.
//
// MIRRORING is applied AFTER the rotation, negating x. Confirmed
// against a file where the same six components were mirrored: only
// this order reproduces the observed pins, and the asymmetric label
// positions agree independently. Negating y instead would put every
// label on the wrong side.
//
// Element order is (m11, m12, m21, m22) as a row-major 2x2, i.e.
//   x' = m11*x + m12*y
//   y' = m21*x + m22*y
struct RotMatrix {
  int m11;
  int m12;
  int m21;
  int m22;
};

inline RotMatrix rotMatrix(int code) {
  const int rot = code & 0x7;

  RotMatrix m{1, 0, 0, 1};

  switch (rot) {
  case 0: m = {1, 0, 0, 1}; break;
  case 1: m = {1, -1, 1, 1}; break;    // 45, scaled sqrt(2)
  case 2: m = {0, -1, 1, 0}; break;    // 90
  case 3: m = {-1, -1, 1, -1}; break;  // 135, scaled
  case 4: m = {-1, 0, 0, -1}; break;   // 180
  case 5: m = {-1, 1, -1, -1}; break;  // 225, scaled
  case 6: m = {0, 1, -1, 0}; break;    // 270
  case 7: m = {1, 1, -1, 1}; break;    // 315, scaled
  default: break;
  }

  // Mirror LAST, negating x -- which negates the first row, since that
  // row is what produces x'.
  if (isMirrored(code)) {
    m.m11 = -m.m11;
    m.m12 = -m.m12;
  }

  return m;
}

// Where a symbol-local point lands for a component at `pos` with
// rotation `code`: pos + M*local, all integer. The record-tree form of
// what SchItemComp's QTransform does in the scene -- the two must agree
// (SchNetModel is checked against the scene for exactly that).
inline Point apply(int code, const Point &pos, const Point &local) {
  const RotMatrix m = rotMatrix(code);
  return Point(pos.x + m.m11 * local.x + m.m12 * local.y,
               pos.y + m.m21 * local.x + m.m22 * local.y);
}

// Composes a quarter-turn onto an existing code.
//
// Not simply "code + 2": a MIRRORED element rotates the other way,
// because its transform is FlipX after the rotation and FlipX does not
// commute with rotation. Working it through, M2 * FlipX * M(r) equals
// FlipX * M(r-2), so a mirrored element's code goes DOWN as it turns
// counter-clockwise on screen.
inline int rotatedCode(int code, bool ccw) {
  const int mirror = code & 0x8;
  const int rot    = code & 0x7;

  const int step = ccw ? 2 : 6;                 // 6 == -2 (mod 8)
  const int turn = mirror ? (8 - step) % 8 : step;

  return mirror | ((rot + turn) & 0x7);
}

// Mirrors a code about a VERTICAL axis (x negated).
//
// Just the mirror bit: the stored transform already applies FlipX after
// the rotation, so flipping about x is exactly toggling that bit with
// the rotation untouched. CONFIRMED against a file in which every
// component was mirrored -- codes 7,1,5,3,2,0 became 15,9,13,11,10,8,
// which is bit 3 set with the rotation unchanged in every case.
inline int mirroredCodeX(int code) { return code ^ 0x8; }

// Mirrors about a HORIZONTAL axis (y negated).
//
// FlipY is FlipX composed with a half-turn, so this is the vertical
// mirror plus 180 degrees.
inline int mirroredCodeY(int code) {
  const int mirrored = code ^ 0x8;
  return (mirrored & 0x8) | (((mirrored & 0x7) + 4) & 0x7);
}

// True when the code scales by sqrt(2) -- the odd rotations.
inline bool isDiagonal(int code) { return (code & 0x1) != 0; }

} // namespace sch::core::rotdecode
