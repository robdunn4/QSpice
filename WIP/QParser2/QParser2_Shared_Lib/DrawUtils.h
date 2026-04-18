/*
 * Rect uses fully inclusive coordinates: left, top, right, and bottom are all
 * inclusive integer coordinates.  This matches QSpice's internal coordinate
 * system.  We assume x increases to the right and y increases upwards.
 * After normalization, left <= right and bottom <= top.
 *
 * Zero-width and zero-height Rects are valid QSpice elements (e.g., a point
 * or a line segment) and are never treated as empty or degenerate.
 */

//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * DrawUtils.height -- Supporting types for drawable schematic items.
 *
 * Depends on Point (from QArgUtils.height) since schematic coordinates are
 * already expressed as Point throughout the parsing layer.
 */
#pragma once
#include <algorithm>
#include <optional>
#include "QArgUtils.h" // for Point

// ============================================================================
// Rect
// Fully inclusive coordinates, y-up orientation.
// All four edges are inclusive: a point (x,y) is inside iff:
//   left <= x <= right  &&  bottom <= y <= top
// width  = right  - left
// height = top    - bottom
// We assume that x-values increase to the right and y-values increase upwards.
// ============================================================================
struct Rect {
  int left   = 0; // inclusive
  int top    = 0; // inclusive (highest y inside the rect)
  int right  = 0; // inclusive (rightmost x inside the rect)
  int bottom = 0; // inclusive (lowest y inside the rect)

  constexpr Rect() = default;

  constexpr Rect(int left, int top, int right, int bottom)
      : left(left), top(top), right(right), bottom(bottom) {
    normalize();
  }

  constexpr Rect(const Point &topLeft, const Point &bottomRight)
      : left(topLeft.x), top(topLeft.y), right(bottomRight.x),
        bottom(bottomRight.y) {
    normalize();
  }

  // Ensure left <= right and bottom <= top.
  constexpr Rect &normalize() noexcept {
    if (right < left) std::swap(left, right);
    if (top < bottom) std::swap(top, bottom);
    return *this;
  }

  constexpr Point topLeft()     const noexcept { return Point(left,  top);    }
  constexpr Point bottomRight() const noexcept { return Point(right, bottom); }
  constexpr Point topRight()    const noexcept { return Point(right, top);    }
  constexpr Point bottomLeft()  const noexcept { return Point(left,  bottom); }

  // Width and height in QSpice coordinates (zero is valid).
  constexpr int getWidth()  const noexcept { return right - left;   }
  constexpr int getHeight() const noexcept { return top   - bottom; }

  constexpr Rect &setWidth(int w)  noexcept { return setSize(w, getHeight()); }
  constexpr Rect &setHeight(int h) noexcept { return setSize(getWidth(), h);  }
  constexpr Rect &setSize(int width, int height) noexcept {
    // Clamp to non-negative; zero width/height is valid in QSpice.
    int ww = std::max(width, 0);
    int hh = std::max(height, 0);
    right  = left + ww;
    bottom = top  - hh;
    return *this;
  }
  constexpr Rect &setSize(Point size) noexcept {
    return setSize(size.x, size.y);
  }

  // Midpoint of inclusive coordinates.
  constexpr Point getCenter() const noexcept {
    return Point((left + right) / 2, (top + bottom) / 2);
  }

  // True if p falls within [left,right] x [bottom,top].
  constexpr bool contains(Point p) const noexcept {
    return p.x >= left && p.x <= right && p.y >= bottom && p.y <= top;
  }

  // True if r is fully contained within *this.
  [[nodiscard]] constexpr bool contains(Rect r) const noexcept {
    return r.left >= left && r.right <= right &&
           r.top  <= top  && r.bottom >= bottom;
  }

  // True if rectangles share at least one point (touching edges count).
  constexpr bool intersects(Rect r) const noexcept {
    return left <= r.right && r.left <= right &&
           bottom <= r.top && r.bottom <= top;
  }

  constexpr bool operator==(const Rect &) const noexcept = default;

  // Returns a new Rect expanded as needed to include p.
  [[nodiscard]] constexpr Rect merge(Point p) const noexcept {
    return Rect(std::min(left,  p.x), std::max(top,    p.y),
                std::max(right, p.x), std::min(bottom, p.y));
  }

  // Returns the bounding Rect of *this and r.
  [[nodiscard]] constexpr Rect merge(Rect r) const noexcept {
    return Rect(std::min(left,   r.left),  std::max(top,    r.top),
                std::max(right,  r.right), std::min(bottom, r.bottom));
  }

  // Returns the overlapping region of *this and r, or std::nullopt if they
  // do not intersect.
  [[nodiscard]] constexpr std::optional<Rect> intersection(Rect r) const noexcept {
    if (!intersects(r)) return std::nullopt;
    return Rect(std::max(left,  r.left),  std::min(top,    r.top),
                std::min(right, r.right), std::max(bottom, r.bottom));
  }

  // Moves the Rect to position (x, y) anchored at topLeft, preserving size.
  constexpr Rect &moveTo(int x, int y) noexcept {
    int w  = getWidth();
    int h  = getHeight();
    left   = x;
    top    = y;
    right  = left + w;
    bottom = top  - h;
    return *this;
  }
  constexpr Rect &moveTo(Point p) noexcept { return moveTo(p.x, p.y); }

  // Shifts the Rect by (dx, dy).
  constexpr Rect &moveBy(int dx, int dy) noexcept {
    left   += dx;  right  += dx;
    top    += dy;  bottom += dy;
    return *this;
  }
  constexpr Rect &moveBy(Point delta) noexcept {
    return moveBy(delta.x, delta.y);
  }

  // Union: returns smallest Rect containing both *this and r.
  [[nodiscard]] constexpr Rect operator|(Rect r) const noexcept {
    return merge(r);
  }

  // Intersection: returns overlapping Rect, or std::nullopt if none.
  [[nodiscard]] constexpr std::optional<Rect> operator&(Rect r) const noexcept {
    return intersection(r);
  }

};
