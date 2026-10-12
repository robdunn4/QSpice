#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchViewBase.h"

#include <QColor>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QScrollBar>
#include <QTimer>
#include <QTransform>
#include <QWheelEvent>
#include <QtGlobal>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "RenderStyle.h"
#include "SchItemBase.h"
#include "SchScene.h"

namespace sch::render {
namespace {

// The grid is FIXED, in three tiers: small dots every 10 units (the
// graphics fine grid), larger dots every 100 (the electrical grid), and
// a small "+" every 1000. It used to coarsen with zoom in a 1-2-5
// progression, which made it useless as a ruler -- a dot could mean 10,
// 50 or 500 units depending on the zoom, so it could not show whether
// something sat on the electrical grid. Now each mark always means the
// same spacing, and a tier FADES OUT as it gets too dense instead of
// changing meaning.
constexpr qreal kFineGridSpacing  = 10.0;
constexpr qreal kMinorGridSpacing = 100.0;
constexpr qreal kMajorGridSpacing = 1000.0;

// Opacity for a tier whose points are `gapPixels` apart on screen:
// hidden at or below minPixels, fully shown from twice that, linear
// between. Zero means "do not draw at all", which also bounds the cost
// -- a tier is never asked to draw points closer than minPixels apart.
qreal gridTierOpacity(qreal gapPixels, qreal minPixels) {
  if (gapPixels <= minPixels) return 0.0;
  if (gapPixels >= 2.0 * minPixels) return 1.0;
  return (gapPixels - minPixels) / minPixels;
}

// A small "+" at every grid point, its arms `deviceArm` pixels long
// whatever the zoom -- converted to scene units the same way the dots'
// radius is, and drawn with a cosmetic pen so the strokes stay one
// pixel wide.
void drawGridCrosses(QPainter *painter, const QRectF &rect, qreal spacing,
                     const QColor &color, qreal deviceArm, qreal deviceScale) {
  const qreal arm = deviceArm / deviceScale;

  QPen pen(color, 1);
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);

  // Widened by an arm on every side: a cross centred just outside the
  // repainted area still reaches into it, and skipping it would leave
  // half a cross behind after a partial repaint.
  const QRectF area = rect.adjusted(-arm, -arm, arm, arm);

  const qreal left = qFloor(area.left() / spacing) * spacing;
  const qreal top  = qFloor(area.top() / spacing) * spacing;

  for (qreal x = left; x < area.right(); x += spacing)
    for (qreal y = top; y < area.bottom(); y += spacing) {
      painter->drawLine(QPointF(x - arm, y), QPointF(x + arm, y));
      painter->drawLine(QPointF(x, y - arm), QPointF(x, y + arm));
    }
}

void drawGridDots(QPainter *painter, const QRectF &rect, qreal spacing,
                  const QColor &color, qreal deviceRadius, qreal deviceScale) {
  // QPainter::drawPoint()'s size with a cosmetic pen is inconsistent
  // across paint backends -- in practice it tends to render as little
  // more than a single pixel regardless of pen width, which made the
  // dots effectively invisible. An explicit filled circle gives a
  // reliable size: deviceRadius is in device pixels, converted to scene
  // units via the current scale so it stays constant on screen.
  const qreal sceneRadius = deviceRadius / deviceScale;

  painter->setPen(Qt::NoPen);
  painter->setBrush(color);

  const qreal left = qFloor(rect.left() / spacing) * spacing;
  const qreal top  = qFloor(rect.top() / spacing) * spacing;

  for (qreal x = left; x < rect.right(); x += spacing)
    for (qreal y = top; y < rect.bottom(); y += spacing)
      painter->drawEllipse(QPointF(x, y), sceneRadius, sceneRadius);
}

// `from`, moved `towardBackground` of the way to `background`.
QColor blendColor(const QColor &from, const QColor &background, double towardBackground) {
  const double keep = 1.0 - towardBackground;
  return QColor(static_cast<int>(from.red() * keep + background.red() * towardBackground),
                static_cast<int>(from.green() * keep + background.green() * towardBackground),
                static_cast<int>(from.blue() * keep + background.blue() * towardBackground));
}

} // namespace

SchViewBase::SchViewBase(QWidget *parent) : QGraphicsView(parent) {
  setScene(new SchScene(this));

  setMouseTracking(true);
  setRenderHint(QPainter::Antialiasing, true);

  // Selection and panning are done by hand -- here, or by the editor's
  // interaction code -- never by QGraphicsView's own drag modes.
  setDragMode(QGraphicsView::NoDrag);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

  // QSpice's configured canvas colour, so a schematic looks the same
  // here as it does there.
  setBackgroundBrush(RenderStyle::current().background);

  // QGraphicsView's default is Qt::NoFocus, and then no key event ever
  // arrives.
  setFocusPolicy(Qt::StrongFocus);

  // Qt's default (SmartViewportUpdate) repaints only the regions items
  // report as dirty, derived from each item's boundingRect(). That makes
  // it strictly dependent on every element's boundingRect() covering
  // what its paint() draws -- which is the case, after the cosmetic-pen
  // slack (SchItemBase::kPaintSlack) and childrenBoundingRect() for a
  // component's labels.
  //
  // FullViewportUpdate and BoundingRectViewportUpdate were both tried
  // and stepped back from: they hide dishonest bounds rather than fix
  // them, and Full measurably lagged a drag by redrawing the whole grid
  // on every mouse move. Smearing during a drag means some element has
  // started painting outside its bounds again; the fix is that
  // element's rect, not a blunter update mode.
  setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);

  // QParser2 (and QSpice) use Y-up coordinates; QGraphicsScene is Y-down.
  // Flipped once here, before any zoom, so every item can use the
  // model's raw coordinates as its local coordinates -- confirmed by
  // rendering a real file (higher model Y renders higher on screen).
  scale(1.0, -1.0);

  // Placeholder extent until content is loaded and the scene rect is
  // derived from it.
  scene()->setSceneRect(-2000.0, -2000.0, 4000.0, 4000.0);

  // Deferred: the viewport has not been laid out yet and may be 0x0,
  // which would make fitInView() compute a degenerate transform.
  QTimer::singleShot(0, this, &SchViewBase::zoomToFit);
}

SchViewBase::~SchViewBase() = default;

void SchViewBase::setViewOptions(const ViewOptions &opts) {
  options = opts;
  viewport()->update();
}

void SchViewBase::setTree(ItemTree tree) {
  tree_ = std::move(tree);
  schScene()->setRoot(tree_.root());
  treeChanged();
}

SchScene *SchViewBase::schScene() const { return static_cast<SchScene *>(scene()); }

void SchViewBase::invalidateNetModel() {
  if (SchScene *sc = schScene()) sc->invalidateNetModel();
}

QRectF SchViewBase::visibleSceneRect() const {
  return mapToScene(viewport()->rect()).boundingRect();
}

// ---------------------------------------------------------------------------
// Zoom
// ---------------------------------------------------------------------------

void SchViewBase::applyNewViewZoom() {
  const double units = viewOptions().newViewWidth;

  const int viewportWidth = viewport()->width();
  if (viewportWidth <= 0 || units <= 0.0) return;

  const double pixelsPerUnit = viewportWidth / units;

  // Set outright rather than scaled from the current transform, so the
  // result does not depend on anything done to the view before. Keeps
  // the Y-flip.
  QTransform xform;
  xform.scale(pixelsPerUnit, -pixelsPerUnit);
  setTransform(xform);

  // Room to pan around the whole visible area, then centred on the
  // origin, where new content lands. The placeholder scene rect is
  // narrower than the default width, and a scene rect smaller than the
  // view would clamp centerOn().
  const double halfW = units / 2.0;
  const double halfH = halfW * viewport()->height() / viewportWidth;
  scene()->setSceneRect(-halfW, -halfH, units, halfH * 2.0);

  ensurePanRoom();
  centerOn(0.0, 0.0);
}

void SchViewBase::zoomToFit() {
  if (scene() == nullptr) return;

  const QRectF contentRect = scene()->itemsBoundingRect();

  // Nothing to fit: a NEW document. Open at the configured width instead
  // of whatever scale 1 happens to show.
  if (contentRect.isEmpty()) {
    applyNewViewZoom();
    return;
  }

  // The scene RECT -- not itemsBoundingRect() -- governs the scrollbar
  // range and how far panning and centerOn() can move. Left at the
  // placeholder, content larger than it could not be scrolled to, and
  // fitInView()'s centring was clamped. Reset here on every fit, with a
  // generous margin (half the content's own size each side).
  const qreal sceneMarginX = contentRect.width() * 0.5;
  const qreal sceneMarginY = contentRect.height() * 0.5;
  scene()->setSceneRect(
      contentRect.adjusted(-sceneMarginX, -sceneMarginY, sceneMarginX, sceneMarginY));

  constexpr qreal kMargin = 20.0;
  fitInView(contentRect.adjusted(-kMargin, -kMargin, kMargin, kMargin), Qt::KeepAspectRatio);

  // The rect set above is the only place it ever shrinks; topped up here
  // so the fitted view still has pan room on all sides.
  ensurePanRoom();
}

void SchViewBase::zoomBy(qreal factor) {
  const ViewOptions opts      = viewOptions();
  const qreal       currScale = transform().m11();

  const qreal targetScale = currScale * factor;
  if (targetScale < opts.minZoomScale) factor = opts.minZoomScale / currScale;
  if (targetScale > opts.maxZoomScale) factor = opts.maxZoomScale / currScale;

  // With AnchorUnderMouse this zooms toward the cursor.
  scale(factor, factor);

  // Zooming out shrinks the visible area's share of the scene rect.
  ensurePanRoom();
}

void SchViewBase::zoomStep(bool zoomIn) {
  // Anchored on the VIEWPORT CENTRE rather than the cursor, unlike the
  // wheel: a keyboard zoom has no pointer position to zoom toward, and
  // wherever the mouse happens to rest is not where the user is looking.
  const QGraphicsView::ViewportAnchor saved = transformationAnchor();
  setTransformationAnchor(QGraphicsView::AnchorViewCenter);

  zoomBy(zoomIn ? kZoomStepFactor : (1.0 / kZoomStepFactor));

  setTransformationAnchor(saved);
}

void SchViewBase::wheelZoom(QWheelEvent *event) {
  zoomBy(event->angleDelta().y() > 0 ? kZoomStepFactor : (1.0 / kZoomStepFactor));
  event->accept();
}

void SchViewBase::ensurePanRoom() {
  if (scene() == nullptr) return;

  const QRectF visible = visibleSceneRect();
  if (visible.isEmpty()) return;

  // One full viewport of slack on each side: enough that a single drag
  // cannot run out of room, without making the scrollbar thumb tiny.
  const QRectF needed =
      visible.adjusted(-visible.width(), -visible.height(), visible.width(), visible.height());

  const QRectF current = scene()->sceneRect();
  if (current.contains(needed)) return;

  scene()->setSceneRect(current.united(needed));
}

void SchViewBase::refreshTextDisplay() {
  if (scene() == nullptr) return;

  for (QGraphicsItem *item : scene()->items())
    if (SchItemBase *schItem = dynamic_cast<SchItemBase *>(item)) schItem->displayChanged();

  viewport()->update();
}

// ---------------------------------------------------------------------------
// Read-only navigation
// ---------------------------------------------------------------------------

void SchViewBase::mousePressEvent(QMouseEvent *event) {
  ensurePanRoom();

  if (event->button() != Qt::MiddleButton) {
    QGraphicsView::mousePressEvent(event);
    return;
  }

  panning    = true;
  lastPanPos = event->pos();
  viewport()->setCursor(Qt::ClosedHandCursor);
  event->accept();
}

void SchViewBase::mouseMoveEvent(QMouseEvent *event) {
  emit cursorPosChanged(mapToScene(event->pos()));

  if (!panning) {
    QGraphicsView::mouseMoveEvent(event);
    return;
  }

  // A long pan can reach the edge of the room reserved at press time.
  ensurePanRoom();

  const QPoint delta = event->pos() - lastPanPos;
  horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
  verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
  lastPanPos = event->pos();
  event->accept();
}

void SchViewBase::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() != Qt::MiddleButton || !panning) {
    QGraphicsView::mouseReleaseEvent(event);
    return;
  }

  panning = false;
  viewport()->unsetCursor();
  event->accept();
}

void SchViewBase::wheelEvent(QWheelEvent *event) { wheelZoom(event); }

// ---------------------------------------------------------------------------
// Grid
// ---------------------------------------------------------------------------

void SchViewBase::drawBackground(QPainter *painter, const QRectF &rect) {
  QGraphicsView::drawBackground(painter, rect);

  const ViewOptions opts = viewOptions();
  if (!opts.gridVisible) return;

  const qreal deviceScale = painter->transform().m11();

  // Each tier fades by its OWN on-screen spacing, so zooming out loses
  // the 10-unit dots first, then the 100-unit dots, then the 1000-unit
  // crosses, and nothing is left when even those would be a smear.
  const qreal minPixels    = opts.gridMinDevicePixels;
  const qreal fineOpacity  = gridTierOpacity(kFineGridSpacing * deviceScale, minPixels);
  const qreal minorOpacity = gridTierOpacity(kMinorGridSpacing * deviceScale, minPixels);
  const qreal majorOpacity = gridTierOpacity(kMajorGridSpacing * deviceScale, minPixels);

  if (majorOpacity <= 0.0) return; // nothing left to show

  // Derived from QSpice's own grid colour (CADbomberSight) rather than
  // fixed greys, so the grid stays legible against whichever background
  // is configured. QSpice stores ONE grid colour; the three tiers are
  // ours, each blended part-way toward the background -- the finest the
  // most -- so the grid stays behind the schematic.
  const QColor baseGridColor = RenderStyle::current().grid;
  const QColor background    = RenderStyle::current().background;

  // The fade is applied as ALPHA, so a fading tier dissolves into the
  // background behind it.
  QColor fineDotColor  = blendColor(baseGridColor, background, 0.50);
  QColor minorDotColor = blendColor(baseGridColor, background, 0.43);
  QColor majorColor    = blendColor(baseGridColor, background, 0.25);
  fineDotColor.setAlphaF(static_cast<float>(fineOpacity));
  minorDotColor.setAlphaF(static_cast<float>(minorOpacity));
  majorColor.setAlphaF(static_cast<float>(majorOpacity));

  constexpr qreal kFineDotRadius  = 1.5;
  constexpr qreal kMinorDotRadius = 2.5;
  constexpr qreal kMajorCrossArm  = 4.0;

  painter->save();

  // Finest first, so each coarser mark draws over the finer ones it
  // coincides with.
  if (fineOpacity > 0.0)
    drawGridDots(painter, rect, kFineGridSpacing, fineDotColor, kFineDotRadius, deviceScale);

  if (minorOpacity > 0.0)
    drawGridDots(painter, rect, kMinorGridSpacing, minorDotColor, kMinorDotRadius, deviceScale);

  drawGridCrosses(painter, rect, kMajorGridSpacing, majorColor, kMajorCrossArm, deviceScale);

  painter->restore();
}

} // namespace sch::render
