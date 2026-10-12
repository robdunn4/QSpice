#pragma once

#include <QBrush>
#include <QRectF>
#include <QString>

class QGraphicsItem;
class QGraphicsScene;
class QPainter;

namespace sch::render {

// Something a schematic can be printed or exported FROM: a scene of
// items, a background, and which items are not part of the document.
//
// Printing and SVG export (SchPrint, SchSvgExport) and the Print dialog
// take one of these rather than the editor's view, so they work the same
// for the editor (SchView implements it) and for a program with no
// editor at all (SchDocSource). Goes to SchRender at the library split.
//
// Five things a source supplies; three are built on them, here, once.
// No QObject, no moc.
class SchRenderSource {
public:
  virtual ~SchRenderSource() = default;

  // --- Supplied by each source.

  // The items to draw.
  virtual QGraphicsScene *renderScene() const = 0;

  // What fills the page behind the items -- the canvas colour.
  virtual QBrush renderBackground() const = 0;

  // Not part of the document: for the editor, transient items and edit
  // decorations and anything hanging from them. Public for the SVG
  // export, which walks the elements itself.
  virtual bool isPrintExcluded(const QGraphicsItem *item) const = 0;

  // The area a viewer currently shows, in scene units -- the Print
  // dialog's "visible area" mode. A source with no viewer returns the
  // whole drawing.
  virtual QRectF visibleSceneRect() const = 0;

  // The document's file, for header/footer fields and as the default
  // place to save an export; empty for an untitled document.
  virtual QString sourcePath() const = 0;

  // --- Built on the above.

  // Every printable element's extent, in scene units. Empty for an empty
  // document.
  QRectF printableSceneRect() const;

  // Whether any printable element touches `sceneRect` -- for skipping
  // blank pages.
  bool hasPrintableIn(const QRectF &sceneRect) const;

  // Draws the scene area `source` into `target` on `painter`: background
  // and elements as on screen -- Y-flip included -- but no grid, no
  // selection highlights, and none of the excluded items. Vector output.
  // `target` must have `source`'s aspect ratio; the caller fits and
  // centres it. Without `withBackground`, nothing is drawn behind the
  // elements: the paper shows through.
  void renderSceneForPrint(QPainter *painter, const QRectF &target,
                           const QRectF &source, bool withBackground = true) const;
};

} // namespace sch::render
