//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * DrawUtils.h -- Supporting types for drawable schematic items.
 *
 * Depends on Point (from QArgUtils.h) since schematic coordinates are
 * already expressed as Point throughout the parsing layer.
 */
#pragma once
#include "QArgUtils.h" // for Point

// ============================================================================
// Rect
// Coordinates are integers; right and bottom are exclusive (like Win32 RECT).
// ============================================================================
struct Rect {
  int left{};
  int top{};
  int right{};  // exclusive
  int bottom{}; // exclusive

  constexpr Rect() = default;

  constexpr Rect(int left, int top, int right, int bottom)
      : left(left), top(top), right(right), bottom(bottom) {}

  constexpr Rect(const Point &topLeft, const Point &bottomRight)
      : left(topLeft.x), top(topLeft.y), right(bottomRight.x),
        bottom(bottomRight.y) {}

  constexpr int  width() const noexcept { return right - left; }
  constexpr int  height() const noexcept { return bottom - top; }
  constexpr bool empty() const noexcept {
    return width() <= 0 || height() <= 0;
  }

  constexpr Point center() const noexcept {
    return Point(left + width() / 2, top + height() / 2);
  }

  /// True if p falls within [left,right) x [top,bottom)
  constexpr bool contains(Point p) const noexcept {
    return p.x >= left && p.x < right && p.y >= top && p.y < bottom;
  }

  /// True if rectangles overlap (touching edges do NOT count)
  constexpr bool intersects(Rect r) const noexcept {
    return left < r.right && r.left < right && top < r.bottom && r.top < bottom;
  }

  constexpr bool operator==(const Rect &) const noexcept = default;
};