#pragma once

// A record's text ALIGNMENT code: which point of the text sits on its
// anchor. Horizontal in bits 2-3, vertical in bits 0-1; 0 is center on
// both. Record-level, no drawing -- split out of SchTextLayout.h
// (2026-10-07) so connectivity planning (net-label anchoring) needs no
// fonts.

namespace sch::core {

constexpr int kAlignNone = 0;

inline bool alignMarksHidden(int alignH, int alignV) {
  return alignH == 0 && alignV == 0;
}

// Whether a symbol's TEXT record is hidden by its alignment: 0/0 on
// ATTRIBUTE text (comment flag clear) only.
//
// Plain text in a symbol -- comment flag set -- is drawn whatever its
// alignment. CONFIRMED (2026-11-02): QSpice draws an NE5532 op amp's
// "+ve", "-ve", "+" and "-" labels, all 0/0 with the flag set
// (TripleCompositeAmplifierwithCommon-emitterOutputStage.qsch). The
// attribute half remains OBSERVED, MECHANISM UNCONFIRMED.
inline bool alignHidesText(bool comment, int alignH, int alignV) {
  return !comment && alignMarksHidden(alignH, alignV);
}

constexpr int kAlignHWest   = 0b0100;
constexpr int kAlignHEast   = 0b1000;
constexpr int kAlignHCenter = 0b1100;
constexpr int kAlignVNorth  = 0b01;
constexpr int kAlignVSouth  = 0b10;
constexpr int kAlignVCenter = 0b11;

} // namespace sch::core
