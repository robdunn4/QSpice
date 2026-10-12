#pragma once

#include <QGraphicsView>
#include <QPoint>
#include <QPointF>
#include <QString>

#include "ItemTree.h"
#include "SchRenderSource.h"

class QMouseEvent;
class QPainter;
class QWheelEvent;

namespace sch::render {

class SchScene;

// How a view displays and navigates. A plain struct, so a program fills
// it from wherever it keeps its settings -- the editor from its Prefs,
// a viewer from its own .ini -- and nothing here knows which.
struct ViewOptions {
  bool   gridVisible         = true;
  qreal  gridMinDevicePixels = 20.0;   // a grid tier fades out below this spacing
  qreal  minZoomScale        = 0.02;   // pixels per schematic unit
  qreal  maxZoomScale        = 20.0;
  double newViewWidth        = 5000.0; // schematic units across an EMPTY view
};

// A read-only schematic view: the canvas, the grid, and navigation --
// zoom (wheel, keyboard, fit) and middle-drag panning. Nothing here
// changes the document.
//
// Used as it is by a viewer, and as the base of the editor's view, which
// adds the editing on top. The editor routes its own mouse and wheel
// events through its interaction code, which calls back into zoomBy()
// and wheelZoom() here, so both programs zoom by the same rules.
//
// The scene is a SchScene, created here, and the view is Y-flipped once
// at construction: QSpice's model Y increases upward, the scene's
// downward, so every item can use the model's coordinates directly.
class SchViewBase : public QGraphicsView, public SchRenderSource {
  Q_OBJECT

public:
  explicit SchViewBase(QWidget *parent = nullptr);
  ~SchViewBase() override;

  // --- Options.
  //
  // viewOptions() is what painting and zooming read, every time. The
  // default returns what setViewOptions() stored; a derived view may
  // instead answer from live settings of its own, so a change there
  // (View > Show Grid) needs no call here -- only a repaint.
  void                setViewOptions(const ViewOptions &opts);
  virtual ViewOptions viewOptions() const { return options; }

  // --- The document.
  //
  // The parsed model, if one was loaded from a file -- empty for a blank
  // new document. The scene's items hold shared references into the same
  // objects this tree owns, so it must live as long as they do.
  const ItemTree &tree() const noexcept { return tree_; }
  void            setTree(ItemTree tree);

  // The scene, as the SchScene it always is -- for its connectivity
  // model.
  SchScene *schScene() const;

  // Marks the scene's connectivity model out of date.
  void invalidateNetModel();

  // The file this document was loaded from; empty for an untitled one.
  const QString &filePath() const noexcept { return filePath_; }
  void           setFilePath(const QString &path) { filePath_ = path; }

  // --- Navigation.

  // Scales and pans so all content is visible. An empty scene is shown
  // at newViewWidth instead (applyNewViewZoom()).
  void zoomToFit();

  // Zooms an EMPTY view to show newViewWidth schematic units across,
  // centred on the origin.
  void applyNewViewZoom();

  // One step in or out, centred on the viewport -- the keyboard
  // equivalent of a wheel notch.
  void zoomIn() { zoomStep(true); }
  void zoomOut() { zoomStep(false); }

  // Scales by `factor`, clamped to the zoom limits, about the current
  // transformation anchor; then tops up the pan room.
  void zoomBy(qreal factor);

  // One wheel notch, toward the cursor. Accepts the event.
  void wheelZoom(QWheelEvent *event);

  // Guarantees scrollable room around whatever is visible.
  //
  // Panning works by moving the scrollbars, and QGraphicsView derives
  // their range from the SCENE RECT. Zoomed out far enough that the
  // whole scene rect fits the viewport, the range collapses to zero and
  // middle-drag silently stops working. This keeps a viewport-sized
  // margin on every side of the visible area.
  //
  // It only ever GROWS the rect: shrinking it mid-gesture would clamp the
  // scroll position and make the view jump under the cursor.
  // zoomToFit() recomputes it from scratch, which brings it back down.
  void ensurePanRoom();

  // Every element re-measures and repaints -- after a display setting
  // that changes text widths. Children included.
  void refreshTextDisplay();

  static constexpr double kZoomStepFactor = 1.15;

  // --- SchRenderSource: printing and export.
  //
  // A plain view excludes nothing; the editor overrides
  // isPrintExcluded() to leave out its transient items.
  QGraphicsScene *renderScene() const override { return scene(); }
  QBrush          renderBackground() const override { return backgroundBrush(); }
  bool            isPrintExcluded(const QGraphicsItem *) const override { return false; }
  QRectF          visibleSceneRect() const override; // the viewport
  QString         sourcePath() const override { return filePath_; }

signals:
  void cursorPosChanged(QPointF scenePos);

protected:
  // Called by setTree() after the tree and the scene have it.
  virtual void treeChanged() {}

  void zoomStep(bool zoomIn);

  // Read-only navigation: middle-drag pans, the wheel zooms. The editor
  // overrides all of these and does the same through its interaction
  // code.
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  // The canvas colour, then the grid (when shown).
  void drawBackground(QPainter *painter, const QRectF &rect) override;

  ViewOptions options;
  ItemTree    tree_;
  QString     filePath_;

  bool   panning = false;
  QPoint lastPanPos; // viewport coordinates
};

} // namespace sch::render
