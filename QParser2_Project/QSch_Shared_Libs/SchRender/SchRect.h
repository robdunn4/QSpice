#pragma once
#include <algorithm>

#include "ArgUtils.h"

namespace sch::render {

// Own replacement for QParser2_Shared's Rect (formerly in DrawUtils.h,
// removed along with DrawIntf). Point itself is unaffected -- still in
// ArgUtils.h -- only Rect needed a home here instead.
//
// Same semantics our code has always assumed: Y-up, fully inclusive
// integer coordinates (left <= right, bottom <= top), matching QSpice's
// own coordinate system.
struct Rect {
  int left   = 0;
  int top    = 0;
  int right  = 0;
  int bottom = 0;

  constexpr Rect() = default;

  constexpr Rect(int left, int top, int right, int bottom)
      : left(left), top(top), right(right), bottom(bottom) {
    normalize();
  }

  // Corners in either order -- normalize() sorts them out.
  constexpr Rect(const Point &a, const Point &b)
      : left(a.x), top(a.y), right(b.x), bottom(b.y) {
    normalize();
  }

  constexpr Rect &normalize() noexcept {
    if (right < left) std::swap(left, right);
    if (top < bottom) std::swap(top, bottom);
    return *this;
  }

  constexpr int getWidth() const noexcept { return right - left; }
  constexpr int getHeight() const noexcept { return top - bottom; }

  constexpr Point getCenter() const noexcept {
    return Point((left + right) / 2, (top + bottom) / 2);
  }

  // Returns a new Rect expanded as needed to include p.
  [[nodiscard]] constexpr Rect merge(Point p) const noexcept {
    return Rect(std::min(left, p.x), std::max(top, p.y), std::max(right, p.x),
                std::min(bottom, p.y));
  }

  // Returns the bounding Rect of *this and r -- e.g. for a future
  // "component selection box = union of its children" enhancement.
  [[nodiscard]] constexpr Rect merge(Rect r) const noexcept {
    return Rect(std::min(left, r.left), std::max(top, r.top),
                std::max(right, r.right), std::min(bottom, r.bottom));
  }
};

} // namespace sch::render
