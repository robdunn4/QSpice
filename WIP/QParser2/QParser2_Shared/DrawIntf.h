//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
/*
 * DrawIntf.h -- Interface for drawable schematic items.
 *
 * Drawable Item* classes inherit from both ItemBase and DrawIntf.
 * Non-drawable items (e.g., ItemDesc, ItemLib) inherit ItemBase only.
 *
 * This class has no knowledge of QSpice parsing internals -- it defines
 * only the visual/spatial contract.
 */
#pragma once
#include "DrawUtils.h" // for Point and Rect

class DrawIntf {
public:
  virtual ~DrawIntf() = default;

  // Non-assignable; ItemBase owns copy/clone semantics
  DrawIntf &operator=(const DrawIntf &) = delete;

  // ── Spatial ──────────────────────────────────────────────────────────────

  /// Axis-aligned bounding box in schematic (canvas) coordinates.
  [[nodiscard]] virtual Rect getBounds() const noexcept = 0;

  /// Centre of bounding box. Default derives from getBounds().
  [[nodiscard]] virtual Point getCenter() const noexcept;

  /// True if bounding box contains p. Override for non-rectangular shapes.
  [[nodiscard]] virtual bool contains(Point p) const noexcept;

  /// True if bounding box overlaps r.
  [[nodiscard]] virtual bool intersects(Rect r) const noexcept;

  // ── Transform ────────────────────────────────────────────────────────────

  /// Reference (anchor) position -- typically pt1 in the parsed item.
  [[nodiscard]] virtual Point getPosition() const noexcept = 0;

  /// Reposition the item. Concrete class updates its ArgPoint members.
  /// Implementations should preserve shape size (move pt1 and pt2 together).
  virtual void setPosition(Point p) noexcept = 0;

  /// Shift by delta. Default calls setPosition(getPosition() + delta).
  virtual void move(Point delta) noexcept;

  // ── Hit testing ──────────────────────────────────────────────────────────

  /// Pixel-precise hit test. Default delegates to contains().
  /// Override for hollow shapes (ellipse outlines, etc.).
  [[nodiscard]] virtual bool hitTest(Point p) const noexcept;

  // ── Visibility ───────────────────────────────────────────────────────────

  /// Whether the item is currently rendered. Default: true.
  [[nodiscard]] virtual bool isVisible() const noexcept;

  /// Show/hide the item.
  virtual void setVisible(bool vis) noexcept;

  // ── Rendering (signature deferred until canvas type is decided) ──────────
  // virtual void draw(...) = 0;

protected:
  DrawIntf()                 = default;
  DrawIntf(const DrawIntf &) = default; // for derived class copy constructors

private:
  bool visible_ = true;
};
