#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchPrintDlg.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPrinterInfo>
#include <QProgressDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QPageSetupDialog>
#include <QPageLayout>
#include <QPainter>
#include <QPrintPreviewWidget>
#include <QPrinter>
#include <QPushButton>
#include <QSpinBox>
#include <QStringList>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <limits>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "SchHeaderFooterDlg.h"
#include "SchTextLayout.h" // fontPixelSize()
#include "SchRenderSource.h"
#include "Version.h"
#include "ViewerPrefs.h"

// SchOutput names used in this file.
using sch::output::SchPrintSettings;
using sch::output::SchPrintTile;
using sch::output::SchPrintLayout;
using sch::output::SchPrint;
using sch::output::SchPrintContext;

// SchRender names used in this file.
using sch::render::SchRenderSource;
using sch::render::fontPixelSize;

namespace {

QString tr(const char *text) {
  return QCoreApplication::translate("SchPrintDlg", text);
}

constexpr double kMmPerInch = 25.4;

QRectF fittedRect(const QSizeF &size, const QRectF &box) {
  if (size.isEmpty() || box.isEmpty()) return QRectF();

  const double scale  = std::min(box.width() / size.width(),
                                 box.height() / size.height());
  const QSizeF fitSize = size * scale;
  return QRectF(box.center().x() - fitSize.width() / 2.0,
                box.center().y() - fitSize.height() / 2.0, fitSize.width(),
                fitSize.height());
}

} // namespace

// ===========================================================================
// SchPageMap -- the printed area, small, with the page grid over it
// ===========================================================================

class SchPageMap : public QWidget {
public:
  // A page clicked: its tile index.
  std::function<void(int index)> onToggle;

  // Whether the thumbnail shows the canvas colour, as the pages will.
  bool background = true;

  explicit SchPageMap(QWidget *parent) : QWidget(parent) {
    setMinimumSize(220, 160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Click a page to include or exclude it."));
  }

  void setContent(const SchRenderSource *schView, const SchPrintLayout &printLayout,
                  const QSet<int> &excludedTiles, bool rerender) {
    view     = schView;
    layout   = printLayout;
    excluded = excludedTiles;
    if (rerender) renderThumb();
    update();
  }

protected:
  void resizeEvent(QResizeEvent *event) override {
    QWidget::resizeEvent(event);
    renderThumb();
  }

  void paintEvent(QPaintEvent *) override {
    QPainter painter(this);
    painter.fillRect(rect(), palette().window());

    const QRectF area = mapArea();
    if (area.isEmpty()) return;

    painter.drawImage(area.topLeft(), thumb);
    painter.setRenderHint(QPainter::Antialiasing);

    for (const SchPrintTile &tile : layout.tiles) {
      const QRectF box = toMap(tile.scene);

      if (tile.page == 0) {
        // Blank: skipped, shown only as an outline.
        painter.setPen(QPen(QColor(150, 150, 150), 1, Qt::DotLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(box);
        continue;
      }

      const bool off = excluded.contains(tile.index);
      painter.setPen(QPen(off ? QColor(120, 120, 120) : QColor(0, 90, 200),
                          off ? 1.0 : 2.0));
      painter.setBrush(off ? QColor(0, 0, 0, 90) : QColor(0, 120, 255, 28));
      painter.drawRect(box);

      if (layout.pages > 1) {
        painter.setPen(off ? QColor(90, 90, 90) : QColor(0, 60, 150));
        painter.drawText(box, Qt::AlignCenter, QString::number(tile.page));
      }
    }
  }

  void mousePressEvent(QMouseEvent *event) override {
    if (event->button() != Qt::LeftButton || layout.pages <= 1) return;

    // Overlapping pages: the one whose centre is nearest.
    const QPointF at   = event->position();
    int           best = -1;
    double        dist = std::numeric_limits<double>::max();

    for (const SchPrintTile &tile : layout.tiles) {
      if (tile.page == 0) continue;

      const QRectF box = toMap(tile.scene);
      if (!box.contains(at)) continue;

      const QPointF d = box.center() - at;
      const double  dd = d.x() * d.x() + d.y() * d.y();
      if (dd >= dist) continue;
      dist = dd;
      best = tile.index;
    }

    if (best >= 0 && onToggle) onToggle(best);
  }

  // Where the covered area is drawn in the widget.
  QRectF mapArea() const {
    return fittedRect(layout.covered.size(),
                      QRectF(rect()).adjusted(4.0, 4.0, -4.0, -4.0));
  }

  // Scene -> widget. Scene Y runs UP: the covered area's largest Y is
  // the map's top.
  QRectF toMap(const QRectF &scene) const {
    const QRectF area = mapArea();
    if (area.isEmpty() || layout.covered.isEmpty()) return QRectF();

    const double k = area.width() / layout.covered.width();
    return QRectF(area.left() + (scene.left() - layout.covered.left()) * k,
                  area.top() + (layout.covered.bottom() - scene.bottom()) * k,
                  scene.width() * k, scene.height() * k);
  }

  void renderThumb() {
    const QRectF area = mapArea();
    if (view == nullptr || area.isEmpty()) {
      thumb = QImage();
      return;
    }

    thumb = QImage(area.size().toSize(), QImage::Format_ARGB32_Premultiplied);
    thumb.fill(Qt::white);

    QPainter painter(&thumb);
    painter.setRenderHint(QPainter::Antialiasing);
    view->renderSceneForPrint(&painter, QRectF(QPointF(0.0, 0.0), area.size()),
                              layout.covered, background);
  }

  const SchRenderSource *view = nullptr;
  SchPrintLayout layout;
  QSet<int>      excluded;
  QImage         thumb;
};

// ===========================================================================
// SchPrintDlg
// ===========================================================================

SchPrintDlg::SchPrintDlg(SchPrint *schPrint, const SchRenderSource *schView,
                                       const QString &docTitle, QWidget *parent)
    : QDialog(parent), print(schPrint), view(schView), title(docTitle) {
  setWindowTitle(tr("Print - %1").arg(title));

  // A scale Fit left behind in an earlier dialog is not carried over.
  print->settings().mmPer1000 = print->settings().savedMmPer1000;

  buildUi();
  loadControls();
  relayout();
  resize(1150, 780);
}

void SchPrintDlg::buildUi() {
  // --- Printer -----------------------------------------------------------
  printerCombo = new QComboBox(this);
  printerCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  printerCombo->setMinimumContentsLength(16);

  copiesSpin = new QSpinBox(this);
  copiesSpin->setRange(1, 999);
  copiesSpin->setValue(1);

  // --- Settings --------------------------------------------------------
  modeCombo = new QComboBox(this);
  modeCombo->addItems({tr("Visible area - fit to page"),
                       tr("Whole schematic - fit to page"),
                       tr("Whole schematic - to scale"),
                       tr("Visible area - to scale")});

  scaleSpin = new QDoubleSpinBox(this);
  scaleSpin->setKeyboardTracking(false);
  // Two buttons, one always down: the unit the scale is shown in.
  mmBtn = new QToolButton(this);
  mmBtn->setText(tr("mm"));
  mmBtn->setCheckable(true);
  mmBtn->setToolTip(tr("Scale in millimetres, to 0.1 mm"));
  inchBtn = new QToolButton(this);
  inchBtn->setText(tr("in"));
  inchBtn->setCheckable(true);
  inchBtn->setToolTip(tr("Scale in inches, to 0.01 inch"));

  unitGroup = new QButtonGroup(this);
  unitGroup->setExclusive(true);
  unitGroup->addButton(mmBtn, 0);
  unitGroup->addButton(inchBtn, 1);

  resetScaleBtn = new QToolButton(this);
  resetScaleBtn->setText(QString(QChar(0x21BA))); // anticlockwise arrow
  resetScaleBtn->setToolTip(tr("Reset to the default scale: %1")
                                .arg(scaleText(SchPrintSettings::kDefaultMmPer1000)));

  QHBoxLayout *scaleRow = new QHBoxLayout;
  scaleRow->addWidget(scaleSpin, 1);
  scaleRow->addWidget(mmBtn);
  scaleRow->addWidget(inchBtn);
  scaleRow->addWidget(resetScaleBtn);
  scaleRow->addWidget(new QLabel(tr("per 1000 units"), this));

  fitWideSpin = new QSpinBox(this);
  fitWideSpin->setRange(1, 20);
  fitWideSpin->setValue(1);
  fitBtn = new QPushButton(tr("Fit"), this);
  fitBtn->setToolTip(tr("Set the scale so the drawing is this many pages "
                        "wide -- for this print only; the saved scale is "
                        "kept"));

  QHBoxLayout *fitRow = new QHBoxLayout;
  fitRow->addWidget(fitWideSpin);
  fitRow->addWidget(new QLabel(tr("pages wide"), this), 1);
  fitRow->addWidget(fitBtn);

  autoOrientChk = new QCheckBox(tr("Choose orientation automatically"), this);
  autoOrientChk->setToolTip(tr("Whichever needs fewer pages (to scale), or "
                               "gives the larger drawing (fit to page)"));

  overlapChk  = new QCheckBox(tr("Overlap"), this);
  overlapSpin = new QDoubleSpinBox(this);
  overlapSpin->setKeyboardTracking(false);
  overlapSpin->setRange(0.0, 50.0);
  overlapSpin->setDecimals(1);
  overlapSpin->setSuffix(tr(" mm"));

  QHBoxLayout *overlapRow = new QHBoxLayout;
  overlapRow->addWidget(overlapChk);
  overlapRow->addWidget(overlapSpin, 1);

  cropChk = new QCheckBox(tr("Crop marks"), this);

  minLineSpin = new QDoubleSpinBox(this);
  minLineSpin->setKeyboardTracking(false);
  minLineSpin->setRange(0.0, 2.0);
  minLineSpin->setDecimals(2);
  minLineSpin->setSingleStep(0.05);
  minLineSpin->setSuffix(tr(" mm"));

  pageLimitSpin = new QSpinBox(this);
  pageLimitSpin->setKeyboardTracking(false);
  pageLimitSpin->setRange(1, SchPrintSettings::kMaxPagesHardLimit);
  pageLimitSpin->setSuffix(tr(" pages"));
  pageLimitSpin->setToolTip(tr("The largest print allowed; a drawing needing "
                               "more pages is not printed until the scale or "
                               "this limit changes"));

  QFormLayout *form = new QFormLayout;
  form->addRow(tr("Printer:"), printerCombo);
  form->addRow(tr("Copies:"), copiesSpin);
  form->addRow(tr("Print:"), modeCombo);
  form->addRow(tr("Scale:"), scaleRow);
  form->addRow(tr("Fit:"), fitRow);
  form->addRow(QString(), autoOrientChk);
  form->addRow(tr("Pages:"), overlapRow);
  form->addRow(QString(), cropChk);
  form->addRow(tr("Thinnest line:"), minLineSpin);
  form->addRow(tr("Page limit:"), pageLimitSpin);

  // Header and footer: each on or off -- off gives its space to the
  // drawing -- and what they say. Beside them the background colour, for
  // this print: each starts from the printing defaults.
  headerChk = new QCheckBox(tr("Header"), this);
  footerChk = new QCheckBox(tr("Footer"), this);
  backgroundChk = new QCheckBox(tr("Background"), this);
  backgroundChk->setToolTip(tr("Print the canvas color behind the drawing"));
  QPushButton *bandsBtn = new QPushButton(tr("Configure..."), this);
  bandsBtn->setToolTip(tr("What the header and footer show"));

  QHBoxLayout *bandsRow = new QHBoxLayout;
  bandsRow->addWidget(headerChk);
  bandsRow->addWidget(footerChk);
  bandsRow->addWidget(backgroundChk);
  bandsRow->addStretch(1);
  bandsRow->addWidget(bandsBtn);
  form->addRow(tr("Include:"), bandsRow);
  connect(bandsBtn, &QPushButton::clicked, this, [this]() { onConfigureBands(); });

  infoLbl = new QLabel(this);
  infoLbl->setWordWrap(true);

  pageMap = new SchPageMap(this);
  pageMap->background = print->settings().printBackground;

  QPushButton *setupBtn = new QPushButton(tr("Page Setup..."), this);
  pdfBtn                = new QPushButton(tr("Print to PDF..."), this);
  printBtn              = new QPushButton(tr("Print"), this);
  QPushButton *closeBtn = new QPushButton(tr("Close"), this);
  printBtn->setDefault(true);
  printBtn->setToolTip(tr("Print the included pages, exactly as shown"));
  pdfBtn->setToolTip(tr("Write the included pages to a PDF file, exactly as shown"));
  // One row: Page Setup at the left, a settings button set apart by
  // the stretch; the two ways of printing and Close at the right. The
  // panel takes its width from this row (sizeHint, below), so no label
  // is cut short.
  QHBoxLayout *buttons = new QHBoxLayout;
  buttons->addWidget(setupBtn);
  buttons->addStretch(1);
  buttons->addWidget(pdfBtn);
  buttons->addWidget(printBtn);
  buttons->addWidget(closeBtn);

  QWidget     *side     = new QWidget(this);
  QVBoxLayout *sideLay  = new QVBoxLayout(side);
  sideLay->setContentsMargins(0, 0, 0, 0);
  sideLay->addLayout(form);
  sideLay->addWidget(infoLbl);
  sideLay->addWidget(new QLabel(tr("Pages (click to include or exclude):"), this));
  sideLay->addWidget(pageMap, 1);
  sideLay->addLayout(buttons);

  // As wide as its contents need -- buttons, labels and controls in
  // whatever font and language -- and never narrower than before.
  side->setFixedWidth(std::max(340, side->sizeHint().width()));

  // --- Preview -----------------------------------------------------------
  previewWidget = new QPrintPreviewWidget(print->ensurePrinter(), this);

  QToolButton *fitPageBtn = new QToolButton(this);
  fitPageBtn->setText(tr("Fit Page"));
  QToolButton *zoomInBtn = new QToolButton(this);
  zoomInBtn->setText(tr("Zoom In"));
  QToolButton *zoomOutBtn = new QToolButton(this);
  zoomOutBtn->setText(tr("Zoom Out"));

  QHBoxLayout *zoomRow = new QHBoxLayout;
  zoomRow->addWidget(fitPageBtn);
  zoomRow->addWidget(zoomInBtn);
  zoomRow->addWidget(zoomOutBtn);
  zoomRow->addStretch(1);

  QVBoxLayout *previewLay = new QVBoxLayout;
  previewLay->addLayout(zoomRow);
  previewLay->addWidget(previewWidget, 1);

  QHBoxLayout *top = new QHBoxLayout(this);
  top->addWidget(side);
  top->addLayout(previewLay, 1);

  // --- Wiring ------------------------------------------------------------
  connect(modeCombo, &QComboBox::currentIndexChanged, this,
          [this](int) { applyControls(); });
  connect(scaleSpin, &QDoubleSpinBox::valueChanged, this,
          [this](double) { onScaleEdited(); });
  connect(resetScaleBtn, &QToolButton::clicked, this, [this]() { resetScale(); });
  // Clicking the button already down does nothing -- the group keeps it
  // down and the unit is unchanged.
  connect(unitGroup, &QButtonGroup::idClicked, this,
          [this](int id) { onUnitChosen(id == 1); });
  connect(autoOrientChk, &QCheckBox::toggled, this, [this](bool) { applyControls(); });
  connect(overlapChk, &QCheckBox::toggled, this, [this](bool) { applyControls(); });
  connect(overlapSpin, &QDoubleSpinBox::valueChanged, this,
          [this](double) { applyControls(); });
  connect(cropChk, &QCheckBox::toggled, this, [this](bool) { applyControls(); });
  connect(headerChk, &QCheckBox::toggled, this, [this](bool) { applyControls(); });
  connect(footerChk, &QCheckBox::toggled, this, [this](bool) { applyControls(); });
  connect(backgroundChk, &QCheckBox::toggled, this, [this](bool) { applyControls(); });
  connect(pageLimitSpin, &QSpinBox::valueChanged, this,
          [this](int) { applyControls(); });
  connect(minLineSpin, &QDoubleSpinBox::valueChanged, this,
          [this](double) { applyControls(); });
  connect(fitBtn, &QPushButton::clicked, this, [this]() { fitWidth(); });

  connect(printerCombo, &QComboBox::currentIndexChanged, this, [this](int) {
    if (loading) return;

    // The paper is kept where the new printer can take it.
    QPrinter *printer = print->ensurePrinter();
    printer->setPrinterName(printerCombo->currentText());
    SchPrint::enforceMinimumMargins(printer);
    relayout();
  });
  connect(setupBtn, &QPushButton::clicked, this, [this]() { onPageSetup(); });
  connect(pdfBtn, &QPushButton::clicked, this, [this]() { onPrintPdf(); });
  connect(printBtn, &QPushButton::clicked, this, [this]() { onPrint(); });
  connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

  connect(fitPageBtn, &QToolButton::clicked, previewWidget,
          &QPrintPreviewWidget::fitInView);
  connect(zoomInBtn, &QToolButton::clicked, previewWidget,
          [this]() { previewWidget->zoomIn(); });
  connect(zoomOutBtn, &QToolButton::clicked, previewWidget,
          [this]() { previewWidget->zoomOut(); });

  connect(previewWidget, &QPrintPreviewWidget::paintRequested, this,
          [this](QPrinter *target) {
            SchPrint::render(target, view, title, print->settings(),
                             [this](const SchPrintTile &tile) {
                               return included(tile);
                             });
          });

  pageMap->onToggle = [this](int index) {
    if (excluded.contains(index)) excluded.remove(index);
    else excluded.insert(index);

    pageMap->setContent(view, pageLayout, excluded, false);
    updateInfo();
    previewWidget->updatePreview();
  };
}

void SchPrintDlg::fillPrinters() {
  const bool wasLoading = loading;
  loading = true;

  const QString current = print->ensurePrinter()->printerName();

  printerCombo->clear();
  printerCombo->addItems(QPrinterInfo::availablePrinterNames());
  printerCombo->setCurrentIndex(printerCombo->findText(current));

  loading = wasLoading;
}

void SchPrintDlg::loadControls() {
  const SchPrintSettings &s = print->settings();

  fillPrinters();

  loading = true;
  modeCombo->setCurrentIndex(static_cast<int>(s.mode));
  (s.inchUnits ? inchBtn : mmBtn)->setChecked(true);
  showScale(s.mmPer1000);
  autoOrientChk->setChecked(s.autoOrient);
  overlapChk->setChecked(s.overlapOn);
  overlapSpin->setValue(s.overlapMm);
  cropChk->setChecked(s.cropMarks);
  headerChk->setChecked(s.headerOn);
  footerChk->setChecked(s.footerOn);
  backgroundChk->setChecked(s.printBackground);
  minLineSpin->setValue(s.minLineMm);
  pageLimitSpin->setValue(s.maxPages);
  loading = false;

  updateScaleEnabled();
}

double SchPrintDlg::shownScale() const {
  return print->settings().inchUnits ? scaleSpin->value() * kMmPerInch
                                     : scaleSpin->value();
}

void SchPrintDlg::showScale(double mmPer1000) {
  const bool wasLoading = loading;
  loading = true;

  // Only whole steps -- 0.1 mm or 0.01 inch -- can be shown or typed:
  // the decimals ARE the step. Decimals before range, which they round.
  if (print->settings().inchUnits) {
    scaleSpin->setDecimals(2);
    scaleSpin->setRange(SchPrintSettings::kMinInch, SchPrintSettings::kMaxInch);
    scaleSpin->setSingleStep(0.1);
    scaleSpin->setValue(mmPer1000 / kMmPerInch);
  } else {
    scaleSpin->setDecimals(1);
    scaleSpin->setRange(SchPrintSettings::kMinMm, SchPrintSettings::kMaxMm);
    scaleSpin->setSingleStep(1.0);
    scaleSpin->setValue(mmPer1000);
  }

  loading = wasLoading;
  updateResetEnabled();
}

void SchPrintDlg::onScaleEdited() {
  if (loading) return;

  print->settings().setScale(shownScale());
  fitNote.clear();
  applyControls();
}

void SchPrintDlg::onUnitChosen(bool inch) {
  SchPrintSettings &s = print->settings();
  if (loading || s.inchUnits == inch) return;

  // The scale recalculated in the new unit, to its step. A fitted scale
  // is rounded DOWN, so the fit still holds; the saved one to the
  // nearest.
  const bool fitted = std::abs(s.mmPer1000 - s.savedMmPer1000) > 1e-9;
  s.inchUnits       = inch;
  s.savedMmPer1000  = SchPrintSettings::quantizeScale(s.savedMmPer1000, inch);
  s.mmPer1000       = fitted ? SchPrintSettings::quantizeScale(s.mmPer1000, inch, true)
                             : s.savedMmPer1000;

  showScale(s.mmPer1000);
  relayout();
}

void SchPrintDlg::resetScale() {
  print->settings().setScale(SchPrintSettings::kDefaultMmPer1000);
  fitNote.clear();
  showScale(print->settings().mmPer1000);
  applyControls();
}

void SchPrintDlg::updateResetEnabled() {
  const SchPrintSettings &s = print->settings();
  const bool offDefault =
      std::abs(s.mmPer1000 - SchPrintSettings::kDefaultMmPer1000) > 1e-6;
  resetScaleBtn->setEnabled(s.atScale() && offDefault);
}

void SchPrintDlg::applyControls() {
  if (loading) return;

  SchPrintSettings &s = print->settings();
  // The scale is not read back here: the field may be showing a fitted
  // scale, which must not become the saved one. onScaleEdited() and
  // resetScale() set it.
  s.mode       = static_cast<SchPrintSettings::Mode>(modeCombo->currentIndex());
  s.autoOrient = autoOrientChk->isChecked();
  s.overlapOn  = overlapChk->isChecked();
  s.overlapMm  = overlapSpin->value();
  s.cropMarks  = cropChk->isChecked();
  s.headerOn   = headerChk->isChecked();
  s.footerOn   = footerChk->isChecked();
  s.printBackground    = backgroundChk->isChecked();
  pageMap->background  = s.printBackground;
  s.minLineMm  = minLineSpin->value();
  s.maxPages   = pageLimitSpin->value();
  fitNote.clear(); // a different print now

  updateScaleEnabled();
  relayout();
}

void SchPrintDlg::updateScaleEnabled() {
  // Scale, fit and tiling options only mean something at scale.
  const bool atScale = print->settings().atScale();
  for (QWidget *w : std::initializer_list<QWidget *>{
           scaleSpin, mmBtn, inchBtn, fitWideSpin, fitBtn, overlapChk, overlapSpin,
           cropChk})
    w->setEnabled(atScale);

  overlapSpin->setEnabled(atScale && overlapChk->isChecked());
  updateResetEnabled();
}

void SchPrintDlg::relayout() {
  QPrinter *printer = print->ensurePrinter();

  // The preview must show the margins the printer will really use.
  SchPrint::enforceMinimumMargins(printer);
  SchPrint::chooseOrientation(printer, view, print->settings());
  pageLayout = SchPrint::layout(printer, view, print->settings());

  // Page choices belong to a grid; a different grid starts afresh.
  if (pageLayout.rows != gridRows || pageLayout.cols != gridCols) {
    excluded.clear();
    gridRows = pageLayout.rows;
    gridCols = pageLayout.cols;
  }

  pageMap->setContent(view, pageLayout, excluded, true);
  updateInfo();
  previewWidget->updatePreview();
}

void SchPrintDlg::updateInfo() {
  const SchPrintSettings &s = print->settings();

  // Nothing to print, nothing to print WITH.
  const bool printable = pageLayout.pages > 0;
  printBtn->setEnabled(printable);
  pdfBtn->setEnabled(printable);

  // A printer Windows could not open lays out as an empty page, which
  // would otherwise read as "Nothing to print".
  QPrinter *printer = print->ensurePrinter();
  if (!SchPrint::isUsable(printer)) {
    infoLbl->setText(tr("Windows could not open the printer \"%1\" -- it may be off, "
                        "asleep or disconnected. Choose another printer, or close "
                        "and reopen this dialog to try again.")
                         .arg(printer->printerName()));
    return;
  }

  if (pageLayout.overLimit) {
    infoLbl->setText(tr("This needs %1 pages (%2 x %3), more than the page "
                        "limit of %4. Use a smaller scale, or raise the "
                        "limit.")
                         .arg(pageLayout.rows * pageLayout.cols)
                         .arg(pageLayout.rows)
                         .arg(pageLayout.cols)
                         .arg(s.maxPages));
    return;
  }

  if (pageLayout.tiles.isEmpty()) {
    infoLbl->setText(tr("Nothing to print."));
    return;
  }

  const double mmPer1000 = effectiveMmPer1000();

  const double gridMm = 100.0 * mmPer1000 / 1000.0;
  const double textPt = fontPixelSize(1.0) * mmPer1000 / 1000.0 / kMmPerInch * 72.0;

  const QString grid = s.inchUnits
                           ? tr("%1 inch").arg(gridMm / kMmPerInch, 0, 'f', 3)
                           : tr("%1 mm").arg(gridMm, 0, 'f', 2);

  int chosen = 0;
  int blank  = 0;
  for (const SchPrintTile &tile : pageLayout.tiles) {
    if (tile.page == 0) {
      ++blank;
      continue;
    }
    if (included(tile)) ++chosen;
  }

  QString pages = tr("%1 of %2 page(s)").arg(chosen).arg(pageLayout.pages);
  if (pageLayout.tiled()) {
    pages += tr(", %1 x %2 grid").arg(pageLayout.rows).arg(pageLayout.cols);
    if (blank > 0) pages += tr(", %1 blank skipped").arg(blank);
  }

  QString info = tr("Grid %1 - text size 1.0 = %2 pt\n%3")
                     .arg(grid)
                     .arg(textPt, 0, 'f', 1)
                     .arg(pages);
  if (!fitNote.isEmpty()) info += QLatin1Char('\n') + fitNote;
  infoLbl->setText(info);
}

double SchPrintDlg::effectiveMmPer1000() const {
  if (pageLayout.tiles.isEmpty()) return 0.0;

  const SchPrintTile &first = pageLayout.tiles.first();
  if (first.scene.width() <= 0.0) return print->settings().mmPer1000;

  const double pxPerMm = print->ensurePrinter()->resolution() / kMmPerInch;
  return first.target.width() / first.scene.width() * 1000.0 / pxPerMm;
}

QString SchPrintDlg::scaleText(double mmPer1000) {
  return tr("%1 mm (%2 inch) per 1000 units")
      .arg(mmPer1000, 0, 'f', 1)
      .arg(mmPer1000 / kMmPerInch, 0, 'f', 2);
}

QString SchPrintDlg::pageRangesText(const QList<int> &pages) {
  QStringList parts;
  int i = 0;
  while (i < pages.size()) {
    int j = i;
    while (j + 1 < pages.size() && pages[j + 1] == pages[j] + 1) ++j;

    if (j == i) parts << QString::number(pages[i]);
    else if (j == i + 1) parts << QString::number(pages[i]) << QString::number(pages[j]);
    else parts << QStringLiteral("%1-%2").arg(pages[i]).arg(pages[j]);
    i = j + 1;
  }
  return parts.join(QStringLiteral(", "));
}

void SchPrintDlg::fitWidth() {
  SchPrintSettings &s = print->settings();

  const QRectF source =
      s.whole() ? view->printableSceneRect() : view->visibleSceneRect();
  if (source.width() <= 0.0) return;

  // N pages wide, each sharing its overlap with the next:
  //   N * pageWidth - (N - 1) * overlap = drawing width on paper.
  const int    n       = fitWideSpin->value();
  const double bodyMm  = SchPrint::bodySizeMm(print->ensurePrinter()).width();
  const double overlap = s.overlapOn ? s.overlapMm : 0.0;
  const double paperMm = n * bodyMm - (n - 1) * overlap;
  if (paperMm <= 0.0) return;

  // Rounded DOWN to a step of the unit shown -- 0.1 mm or 0.01 inch --
  // so never a page more. The hair under keeps an exact fit from tipping
  // over in floating point.
  const double exact  = paperMm * 1000.0 / source.width() * 0.9999;
  const double usable = SchPrintSettings::quantizeScale(exact, s.inchUnits, true);

  fitNote.clear();
  if (exact < SchPrintSettings::minScaleMm(s.inchUnits))
    fitNote = tr("%1 page(s) wide needs a scale below the minimum; using "
                 "the minimum.").arg(n);
  else if (exact > SchPrintSettings::maxScaleMm(s.inchUnits))
    fitNote = tr("%1 page(s) wide needs a scale above the maximum; using "
                 "the maximum.").arg(n);

  // In effect, NOT saved: savedMmPer1000 is left alone.
  s.mmPer1000 = usable;

  showScale(s.mmPer1000);
  relayout();
}

void SchPrintDlg::onConfigureBands() {
  // The sample is this document's first printed page.
  SchPrintTile tile;
  for (const SchPrintTile &candidate : pageLayout.tiles) {
    if (candidate.page == 0) continue;
    tile = candidate;
    break;
  }
  const SchPrintContext sample =
      SchPrint::context(view, title, pageLayout, tile, print->ensurePrinter());

  // For this print -- and, if the user says so, as the defaults too: the
  // one place in this dialog that can write to the .ini.
  SchHeaderFooterDlg dialog(print->settings(), sample, this);
  for (;;) {
    if (dialog.exec() != QDialog::Accepted) return;

    SchPrintSettings edited = print->settings();
    dialog.apply(edited);

    bool changed = false;
    for (int i = 0; i < 3; ++i)
      if (edited.headerText[i] != print->settings().headerText[i] ||
          edited.footerText[i] != print->settings().footerText[i])
        changed = true;

    if (!changed) return;

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Headers and Footers"),
        tr("Make these the default headers and footers?\n\n"
           "Yes: also for future prints. No: this print only."),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);
    if (answer == QMessageBox::Cancel) continue; // back to editing

    dialog.apply(print->settings());

    if (answer == QMessageBox::Yes) {
      // The six slots only, over what the file holds now.
      SchPrintSettings stored = ViewerPrefs::storedPrintSettings();
      dialog.apply(stored);
      stored.save();
    }
    break;
  }

  previewWidget->updatePreview();
}

void SchPrintDlg::onPageSetup() {
  // Settings only -- it never prints. Its Printer button can change the
  // printer, so the list is refreshed too.
  QPageSetupDialog dialog(print->ensurePrinter(), this);
  dialog.setWindowTitle(tr("Page Setup"));
  if (dialog.exec() != QDialog::Accepted) return;

  SchPrint::enforceMinimumMargins(print->ensurePrinter());
  fillPrinters();
  relayout();
}

void SchPrintDlg::onPrint() {
  // No printer dialog -- what prints is what the preview shows -- but
  // one last look at where it is going and how much of it there is.
  if (!confirmPrint()) return;

  QPrinter *printer = print->ensurePrinter();
  printer->setOutputFormat(QPrinter::NativeFormat);
  printer->setCopyCount(copiesSpin->value());

  QProgressDialog progress(tr("Printing..."), tr("Cancel"), 0, 1, this);
  const bool done =
      SchPrint::render(printer, view, title, print->settings(),
                       [this](const SchPrintTile &tile) { return included(tile); },
                       progressFn(progress, tr("Printing - %1").arg(title),
                                  tr("Printing page %1 of %2...")));

  // Cancelled: the job is aborted and the dialog stays open.
  if (!done) return;

  accept();
}

bool SchPrintDlg::confirmPrint() {
  QPrinter *printer = print->ensurePrinter();

  // Printer: its name, and where it is, when the driver says.
  const QString     name = printer->printerName();
  const QPrinterInfo info = QPrinterInfo::printerInfo(name);
  QStringList       about;
  if (!info.location().isEmpty()) about << info.location();
  if (!info.description().isEmpty() && info.description() != name)
    about << info.description();
  QString printerText = name.toHtmlEscaped();
  if (!about.isEmpty())
    printerText += QStringLiteral(" (%1)").arg(about.join(QStringLiteral(", ")).toHtmlEscaped());

  // Pages: how many of the layout, which, and in all with copies.
  QList<int> chosen;
  for (const SchPrintTile &tile : pageLayout.tiles)
    if (tile.page > 0 && included(tile)) chosen << tile.page;
  std::sort(chosen.begin(), chosen.end());

  QString pagesText = tr("%1 of %2").arg(chosen.size()).arg(pageLayout.pages);
  if (chosen.size() < pageLayout.pages)
    pagesText += tr(" (pages %1)").arg(pageRangesText(chosen));

  const int copies = copiesSpin->value();
  if (copies > 1)
    pagesText += tr(" &times; %1 copies = %2 pages in all")
                     .arg(copies)
                     .arg(chosen.size() * copies);

  // Paper and orientation -- auto-orientation may have turned it.
  const QPageLayout page = printer->pageLayout();
  const QString paperText =
      tr("%1, %2")
          .arg(page.pageSize().name().toHtmlEscaped(),
               page.orientation() == QPageLayout::Landscape ? tr("landscape")
                                                            : tr("portrait"));

  // Scale: the one in effect, and how it was arrived at.
  const double  mm        = effectiveMmPer1000();
  const QString scaleWord = print->settings().atScale() ? QString()
                                                        : tr("about ");
  const QString scaleRow  = tr("%1%2<br>%3")
                               .arg(scaleWord, scaleText(mm),
                                    modeCombo->currentText().toHtmlEscaped());

  QMessageBox box(this);
  box.setWindowTitle(tr("Print?"));
  box.setIcon(QMessageBox::Question);
  box.setTextFormat(Qt::RichText);
  box.setText(tr("Print <b>%1</b>?").arg(title.toHtmlEscaped()));
  box.setInformativeText(
      tr("<table cellspacing=\"4\">"
         "<tr><td><b>Printer:</b></td><td>%1</td></tr>"
         "<tr><td><b>Pages:</b></td><td>%2</td></tr>"
         "<tr><td><b>Paper:</b></td><td>%3</td></tr>"
         "<tr><td valign=\"top\"><b>Scale:</b></td><td>%4</td></tr>"
         "</table>")
          .arg(printerText, pagesText, paperText, scaleRow));

  QPushButton *printChoice = box.addButton(tr("Print"), QMessageBox::AcceptRole);
  box.addButton(QMessageBox::Cancel);
  box.setDefaultButton(printChoice);
  box.exec();

  return box.clickedButton() == printChoice;
}

std::function<bool(int, int)>
SchPrintDlg::progressFn(QProgressDialog &progress,
                               const QString &caption, const QString &label) {
  progress.setWindowTitle(caption);
  progress.setWindowModality(Qt::WindowModal);
  progress.setAutoClose(false);
  progress.setAutoReset(false);

  // NOT shown yet. QProgressDialog starts a timer when constructed and
  // shows itself when it fires (4 s by default) -- which, printing to a
  // file driver such as Microsoft Print to PDF, was in the middle of
  // the driver's own "Save Print Output As" dialog, and came up in front
  // of it. reset() stops that timer; the dialog appears at the FIRST
  // page instead, which render() reaches only once the printer has
  // started -- after any such driver dialog has closed.
  progress.reset();

  return [&progress, label](int pagesDone, int total) {
    if (!progress.isVisible()) {
      progress.setMinimumDuration(0); // shown at once: even one page can be slow
      progress.show();
    }

    progress.setMaximum(std::max(1, total));
    progress.setValue(pagesDone);
    progress.setLabelText(label.arg(std::min(pagesDone + 1, total)).arg(total));

    // The dialog paints, and its Cancel is seen, only while events run.
    QCoreApplication::processEvents();
    return !progress.wasCanceled();
  };
}

void SchPrintDlg::onPrintPdf() {
  // Beside the schematic, under its own name; an untitled one in the
  // home folder.
  const QString schPath = view->sourcePath();
  const QString suggested =
      schPath.isEmpty()
          ? QDir(QDir::homePath()).filePath(QFileInfo(title).completeBaseName() +
                                            QStringLiteral(".pdf"))
          : QDir(QFileInfo(schPath).absolutePath())
                .filePath(QFileInfo(schPath).completeBaseName() +
                          QStringLiteral(".pdf"));

  QString path = QFileDialog::getSaveFileName(this, tr("Print to PDF"), suggested,
                                              tr("PDF Files (*.pdf)"));
  if (path.isEmpty()) return;
  if (!path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
    path += QStringLiteral(".pdf");

  // Replaced, not appended to -- the Save dialog has already asked. And
  // so "does it exist afterwards" really means it was written.
  QFile::remove(path);

  // A printer of its own, so the real one is untouched -- with the SAME
  // page layout, so the pages are the ones previewed.
  QPrinter pdf(QPrinter::HighResolution);
  pdf.setOutputFormat(QPrinter::PdfFormat);
  pdf.setOutputFileName(path);
  pdf.setPageLayout(print->ensurePrinter()->pageLayout());
  pdf.setDocName(title);
  pdf.setCreator(QString::fromLatin1(Version::kProgramName));

  QProgressDialog progress(tr("Printing to PDF..."), tr("Cancel"), 0, 1, this);
  const bool done =
      SchPrint::render(&pdf, view, title, print->settings(),
                       [this](const SchPrintTile &tile) { return included(tile); },
                       progressFn(progress, tr("Printing to PDF - %1").arg(title),
                                  tr("Writing page %1 of %2...")));

  // Cancelled: no half-written file left behind.
  if (!done) {
    QFile::remove(path);
    return;
  }

  if (!QFileInfo::exists(path))
    QMessageBox::warning(this, tr("Print to PDF"),
                         tr("The PDF could not be written:\n\n%1")
                             .arg(QDir::toNativeSeparators(path)));
}

bool SchPrintDlg::included(const SchPrintTile &tile) const {
  return !excluded.contains(tile.index);
}
