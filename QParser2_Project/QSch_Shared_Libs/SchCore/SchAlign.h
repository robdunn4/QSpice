#pragma once

// A record's text ALIGNMENT code: which point of the text sits on its
// anchor. Horizontal in bits 2-3, vertical in bits 0-1; 0 is centre on
// both. Record-level, no drawing -- split out of SchTextLayout.h
// (2026-10-07) so connectivity planning (net-label anchoring) needs no
// fonts.

namespace sch::core {

constexpr int kAlignNone = 0;

inline bool alignMarksHidden(int alignH, int alignV) {
  return alignH == 0 && alignV == 0;
}

constexpr int kAlignHWest   = 0b0100;
constexpr int kAlignHEast   = 0b1000;
constexpr int kAlignHCenter = 0b1100;
constexpr int kAlignVNorth  = 0b01;
constexpr int kAlignVSouth  = 0b10;
constexpr int kAlignVCenter = 0b11;

} // namespace sch::core
