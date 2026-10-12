#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QRectF>
#include <QSet>
#include <QSize>
#include <QString>
#include <QTransform>

#include <utility>

class QGraphicsItem;
class QPainter;

namespace sch::render { class SchRenderSource; }

namespace sch::output {

// A schematic as a LAYERED SVG, for Inkscape -- File > Export as SVG
// (SchPrint::exportSvg()).
//
// Qt's QSvgGenerator writes one flat stream of styled paths: it has no
// idea where one element ends and the next begins. So each element is
// painted ON ITS OWN into a small generator of its own -- all of them
// sharing one size, resolution and view box, so their coordinates line
// up -- and the drawing part of each result is spliced into a file
// assembled here, inside groups and layers of our own:
//
//   Background         one canvas-colour rect, locked
//   Graphics           annotation shapes and images
//   Symbols            a group per component: id/label = refdes, a
//                      <title>, data-refdes / data-value / data-symbol
//   Wires              a sub-group per NET, holding its wires and taps
//   Net labels/ports   a group per label, data-net
//   Junctions          a group per dot, data-net
//   Text               free text, comments, directives
//
// Layers are Inkscape's (inkscape:groupmode="layer"); a sodipodi
// namedview sets mm as the document unit, so it opens at true size.
// Elements keep their stacking within a layer.
//
// No moc.
class SchSvgExport {
public:
  // The whole schematic of `view` at `mmPer1000` (paper length of 1000
  // units), lines no thinner than `minLineMm`, to `path`. `creator` is
  // the program's name, for the file's description. False on failure;
  // nothing is left behind.
  static bool write(const QString &path, const sch::render::SchRenderSource *view, const QString &title,
                    double mmPer1000, double minLineMm, const QString &creator);

protected:
  enum class Layer { Graphics, Symbols, Wires, Labels, Junctions, Text };

  SchSvgExport(const sch::render::SchRenderSource *view, const QRectF &source, double pxPerUnit);

  bool build(const QString &title, double mmPer1000, double minLineMm,
             const QString &creator, QByteArray &out);

  // One element, painted alone; the drawing part of the generator's
  // output, or empty.
  QByteArray piece(QGraphicsItem *item) const;

  // The element and its children, each in its own stacking order.
  void paintTree(QPainter &painter, QGraphicsItem *item) const;

  // An id unique in this file, from `wanted` made XML-safe.
  QString uniqueId(const QString &wanted);

  // `piece` inside <g> with these attributes.
  static QByteArray group(const QByteArray &body,
                          const QList<std::pair<QString, QString>> &attrs,
                          const QString &title = QString());

  static Layer   layerOf(const QGraphicsItem *item);
  static QString kindLabel(const QGraphicsItem *item);

  // Net names for elements that carry none themselves (taps, junctions),
  // from the wires that meet them.
  void    mapWireNets();
  QString netAt(const QGraphicsItem *item) const;

  static QString pointKey(const QPointF &p);
  static QString esc(const QString &text);

  const sch::render::SchRenderSource *view = nullptr;
  QRectF     source;      // scene area drawn
  double     pxPerUnit = 0.0;
  QSize      sizePx;
  QRectF     viewBox;
  QTransform page;        // scene -> drawing, Y flipped

  QSet<QString>           usedIds;
  QHash<QString, QString> wireNetAt; // pointKey -> net name
};

} // namespace sch::output
