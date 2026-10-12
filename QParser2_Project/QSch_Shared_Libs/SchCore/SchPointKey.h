#pragma once

#include <QHash>
#include <QPointF>

#include <cmath>

// Coordinates as connectivity sees them -- record-level, no scene, no
// QGraphicsItem. Split out of SchConnect.h (2026-10-07) so the
// connectivity model (NetModel) needs nothing from the editor.
//
// "Connected" means sharing EXACTLY the same coordinate -- there is no
// tolerance and there must not be one, because QSpice itself treats
// coincidence as exact. That's also why electrical geometry snaps to
// 100-unit boundaries: near-misses are silently not connections.
namespace sch::core {

// Grid that electrical geometry normally lands on. QSpice only treats
// items as connected when their coordinates match exactly, so anything
// off the grid its pins and wires use can never connect no matter how
// close it looks.
constexpr int kElectricalGrid = 100;

// The grid GRAPHICS snap to by default -- lines, boxes, arcs, text.
//
// The same 100 as the electrical grid, but by CHOICE rather than
// necessity: nothing connects to a graphic, so it could sit anywhere.
// Matching the electrical grid keeps annotation aligned with the wiring
// and with the displayed grid. Kept as its own name so the two can be
// separated if that ever stops being wanted.
constexpr int kGraphicGrid = 100;

// The FINE grid, for GRAPHICS only, while Shift is held.
//
// Symbol artwork sits on 10-unit coordinates, so this is the finest
// placement worth offering. Electrical items never use it: pins are
// always on the 100-unit grid, so wires, net anchors and components
// must be too, or nothing can connect to them.
constexpr int kFineGrid = 10;

// Wire endpoints required at a coordinate before it counts as a
// junction. Two wires meeting end-to-end is just a continuation and
// gets no dot; three or more is a junction.
constexpr int kJunctionMinEndpoints = 3;

// A coordinate, exactly. Integer scene units, which is what the model
// stores -- deliberately not QPointF, so equality is exact equality and
// not a float comparison that happens to work.
struct PointKey {
  int x = 0;
  int y = 0;

  bool operator==(const PointKey &other) const {
    return x == other.x && y == other.y;
  }
};

inline size_t qHash(const PointKey &key, size_t seed = 0) {
  return ::qHash(static_cast<qint64>(key.x) << 32 ^
                     static_cast<quint32>(key.y),
                 seed);
}

// The exact coordinate a scene position stands for.
//
// Rounded, not truncated: scene coordinates come back through Qt
// transforms as doubles, and a value that should be exactly 700 can
// arrive as 699.9999999. Truncation would turn that into 699 and lose
// the connection; rounding recovers the integer the model holds.
inline PointKey keyFor(const QPointF &scenePos) {
  PointKey key;
  key.x = static_cast<int>(std::lround(scenePos.x()));
  key.y = static_cast<int>(std::lround(scenePos.y()));
  return key;
}

} // namespace sch::core
