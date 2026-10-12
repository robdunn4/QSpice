#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchPrint.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QLocale>
#include <QPageLayout>
#include <QStringList>
#include <QPainter>
#include <QPrinter>
#include <QPrinterInfo>
#include <QSettings>

#include <algorithm>
#include <cmath>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "SchPen.h"
#include "SchSvgExport.h"
#include "SchRenderSource.h"

namespace sch::output {

// SchRender names used in this file.
using sch::render::SchRenderSource;
namespace SchPen = sch::render::SchPen;

namespace {

QString tr(const char *text) {
  return QCoreApplication::translate("SchPrint", text);
}

constexpr double kMmPerInch = 25.4;

// Header and footer type: small, plain, black. A POINT size,
// deliberately: this text is sized for paper, and a point size is
// converted at the printer's own resolution.
constexpr double kHeaderPoints = 9.0;

// Space between header/footer and the drawing, in text lines.
constexpr double kBandGapLines = 0.5;

// Crop mark length on paper.
constexpr double kCropMarkMm = 5.0;

// Overlap never takes more than this share of a page.
constexpr double kMaxOverlapShare = 0.4;

// --- Settings keys --------------------------------------------------
constexpr const char *kPrintGroup     = "Print";
constexpr const char *kModeKey        = "Mode";
constexpr const char *kScaleKey       = "MmPer1000Units";
constexpr const char *kInchUnitsKey   = "ScaleInInches";
constexpr const char *kAutoOrientKey  = "AutoOrientation";
constexpr const char *kOverlapOnKey   = "Overlap";
constexpr const char *kOverlapMmKey   = "OverlapMm";
constexpr const char *kCropMarksKey   = "CropMarks";
constexpr const char *kBackgroundKey  = "Background";
constexpr const char *kMinLineMmKey   = "MinLineWidthMm";
constexpr const char *kMaxPagesKey    = "MaxPages";
constexpr const char *kHeaderOnKey    = "HeaderOn";
constexpr const char *kFooterOnKey    = "FooterOn";
constexpr const char *kHeaderKeys[3]  = {"HeaderLeft", "HeaderCenter", "HeaderRight"};
constexpr const char *kFooterKeys[3]  = {"FooterLeft", "FooterCenter", "FooterRight"};
constexpr const char *kPrinterKey     = "Printer";
constexpr const char *kPaperIdKey     = "PaperSizeId";
constexpr const char *kPaperWKey      = "PaperWidthMm";
constexpr const char *kPaperHKey      = "PaperHeightMm";
constexpr const char *kOrientKey      = "Landscape";
constexpr const char *kMarginsKey     = "MarginsMm"; // "left,top,right,bottom"

QFont headerFont() {
  QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
  font.setPointSizeF(kHeaderPoints);
  return font;
}

double pxPerMm(double resolution) { return resolution / kMmPerInch; }

// A rectangle of `size` fitted into `box`, proportions kept, centred.
QRectF fitted(const QSizeF &size, const QRectF &box) {
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
// Settings
// ===========================================================================

double SchPrintSettings::quantizeScale(double mm, bool inch, bool roundDown) {
  const double unit  = inch ? kMmPerInch : 1.0;
  const double step  = inch ? kInchStep : kMmStep;
  const double steps = mm / unit / step;

  // The small allowance stops a value already on a step (2.54 mm is 0.1
  // inch, give or take floating point) from dropping to the one below.
  double value = (roundDown ? std::floor(steps + 1e-6) : std::round(steps)) * step;
  value = std::clamp(value, inch ? kMinInch : kMinMm, inch ? kMaxInch : kMaxMm);
  return value * unit;
}

void SchPrintSettings::setScale(double mm) {
  mmPer1000      = quantizeScale(mm, inchUnits);
  savedMmPer1000 = mmPer1000;
}

void SchPrintSettings::load() {
  if (settingsFile.isEmpty()) return;

  QSettings settings(settingsFile, QSettings::IniFormat);
  settings.beginGroup(QString::fromLatin1(kPrintGroup));

  const int modeValue = settings.value(kModeKey, static_cast<int>(mode)).toInt();
  if (modeValue >= 0 && modeValue <= 3) mode = static_cast<Mode>(modeValue);

  // The unit first: the scale is made a whole number of its steps.
  inchUnits  = settings.value(kInchUnitsKey, inchUnits).toBool();

  const double scale = settings.value(kScaleKey, savedMmPer1000).toDouble();
  if (scale > 0.0) setScale(scale);

  autoOrient = settings.value(kAutoOrientKey, autoOrient).toBool();
  overlapOn  = settings.value(kOverlapOnKey, overlapOn).toBool();
  cropMarks  = settings.value(kCropMarksKey, cropMarks).toBool();
  printBackground = settings.value(kBackgroundKey, printBackground).toBool();

  const double overlap = settings.value(kOverlapMmKey, overlapMm).toDouble();
  if (overlap >= 0.0) overlapMm = overlap;

  const double minLine = settings.value(kMinLineMmKey, minLineMm).toDouble();
  if (minLine >= 0.0) minLineMm = minLine;

  maxPages = std::clamp(settings.value(kMaxPagesKey, maxPages).toInt(), 1,
                        kMaxPagesHardLimit);

  // A slot saved EMPTY stays empty; only a missing key takes the default.
  headerOn = settings.value(kHeaderOnKey, headerOn).toBool();
  footerOn = settings.value(kFooterOnKey, footerOn).toBool();
  for (int i = 0; i < 3; ++i) {
    headerText[i] = settings.value(kHeaderKeys[i], headerText[i]).toString();
    footerText[i] = settings.value(kFooterKeys[i], footerText[i]).toString();
  }

  settings.endGroup();
}

void SchPrintSettings::save() const {
  if (settingsFile.isEmpty()) return;

  // Each key on its own -- other instances share the file.
  QSettings settings(settingsFile, QSettings::IniFormat);
  settings.beginGroup(QString::fromLatin1(kPrintGroup));
  settings.setValue(kModeKey, static_cast<int>(mode));
  settings.setValue(kScaleKey, savedMmPer1000);
  settings.setValue(kInchUnitsKey, inchUnits);
  settings.setValue(kAutoOrientKey, autoOrient);
  settings.setValue(kOverlapOnKey, overlapOn);
  settings.setValue(kOverlapMmKey, overlapMm);
  settings.setValue(kCropMarksKey, cropMarks);
  settings.setValue(kBackgroundKey, printBackground);
  settings.setValue(kMinLineMmKey, minLineMm);
  settings.setValue(kMaxPagesKey, maxPages);
  settings.setValue(kHeaderOnKey, headerOn);
  settings.setValue(kFooterOnKey, footerOn);
  for (int i = 0; i < 3; ++i) {
    settings.setValue(kHeaderKeys[i], headerText[i]);
    settings.setValue(kFooterKeys[i], footerText[i]);
  }
  settings.endGroup();
  settings.sync();
}

// ===========================================================================
// SchPrint
// ===========================================================================

SchPrint::SchPrint(const QString &settingsFile) {
  printSettings.settingsFile   = settingsFile;
  defaultSettings.settingsFile = settingsFile;
  printSettings.load();
}

SchPrint::SchPrint(const QString &settingsFile, const SchPrintSettings &defaults) {
  // load() keeps each value it finds no key for, so the defaults are
  // simply what it starts from.
  defaultSettings              = defaults;
  defaultSettings.settingsFile = settingsFile;
  printSettings                = defaultSettings;
  printSettings.load();
}

void SchPrint::reloadDefaults() {
  printSettings = defaultSettings;
  printSettings.load();
  printer.reset();
}

SchPrinterChoice SchPrinterChoice::load(const QString &settingsFile) {
  SchPrinterChoice choice;
  if (settingsFile.isEmpty()) return choice;

  QSettings settings(settingsFile, QSettings::IniFormat);
  settings.beginGroup(QString::fromLatin1(kPrintGroup));
  choice.printer       = settings.value(kPrinterKey).toString();
  choice.paperId       = settings.value(kPaperIdKey, -1).toInt();
  choice.paperWidthMm  = settings.value(kPaperWKey, 0.0).toDouble();
  choice.paperHeightMm = settings.value(kPaperHKey, 0.0).toDouble();
  choice.landscape     = settings.value(kOrientKey, false).toBool();
  settings.endGroup();
  return choice;
}

void SchPrinterChoice::save(const QString &settingsFile) const {
  if (settingsFile.isEmpty()) return;

  QSettings settings(settingsFile, QSettings::IniFormat);
  settings.beginGroup(QString::fromLatin1(kPrintGroup));
  settings.setValue(kPrinterKey, printer);
  settings.setValue(kPaperIdKey, paperId);
  settings.setValue(kPaperWKey, paperWidthMm);
  settings.setValue(kPaperHKey, paperHeightMm);
  settings.setValue(kOrientKey, landscape);
  settings.endGroup();
  settings.sync();
}

SchPrint::~SchPrint() = default;

QPrinter *SchPrint::ensurePrinter() {
  if (printer) return printer.get();

  printer = std::make_unique<QPrinter>(QPrinter::HighResolution);
  applySavedPrinter(printer.get());

  if (!isUsable(printer.get()))
    qWarning().noquote() << QStringLiteral("Print: the printer \"%1\" could not be opened")
                                .arg(printer->printerName());
  return printer.get();
}

bool SchPrint::isUsable(const QPrinter *printer) {
  return printer != nullptr && printer->isValid() && printer->resolution() > 0;
}

void SchPrint::reopenPrinterIfUnusable() {
  if (!printer || isUsable(printer.get())) return;

  printer.reset();
  ensurePrinter();
}

void SchPrint::applySavedPrinter(QPrinter *target) const {
  if (printSettings.settingsFile.isEmpty()) return;

  QSettings settings(printSettings.settingsFile, QSettings::IniFormat);
  settings.beginGroup(QString::fromLatin1(kPrintGroup));

  // Only a printer that still exists; otherwise QPrinter's own choice,
  // the Windows default, stands -- and so does its paper. An EMPTY name
  // means the Windows default by choice (SchPrinterChoice): the saved
  // paper and orientation still apply to it.
  const QString name = settings.value(kPrinterKey).toString();
  if (!name.isEmpty()) {
    if (!QPrinterInfo::availablePrinterNames().contains(name)) {
      settings.endGroup();
      return;
    }
    target->setPrinterName(name);
  }

  // The page: a standard size by its id, a custom one by its size.
  QPageSize size;
  const int id = settings.value(kPaperIdKey, -1).toInt();
  if (id >= 0 && id != static_cast<int>(QPageSize::Custom)) {
    size = QPageSize(static_cast<QPageSize::PageSizeId>(id));
  } else {
    const double w = settings.value(kPaperWKey, 0.0).toDouble();
    const double h = settings.value(kPaperHKey, 0.0).toDouble();
    if (w > 0.0 && h > 0.0) size = QPageSize(QSizeF(w, h), QPageSize::Millimeter);
  }

  QMarginsF margins = target->pageLayout().margins(QPageLayout::Millimeter);
  const QStringList parts = settings.value(kMarginsKey).toString().split(QLatin1Char(','));
  if (parts.size() == 4)
    margins = QMarginsF(parts.at(0).toDouble(), parts.at(1).toDouble(),
                        parts.at(2).toDouble(), parts.at(3).toDouble());

  const QPageLayout::Orientation orient =
      settings.value(kOrientKey, false).toBool() ? QPageLayout::Landscape
                                                 : QPageLayout::Portrait;
  settings.endGroup();

  QPageLayout pageLayout = target->pageLayout();
  if (size.isValid()) pageLayout.setPageSize(size);
  pageLayout.setOrientation(orient);
  pageLayout.setUnits(QPageLayout::Millimeter);
  pageLayout.setMargins(margins);

  // A printer that cannot take the layout keeps its own.
  if (!target->setPageLayout(pageLayout))
    qWarning() << "Print: saved page layout not accepted by" << target->printerName();

  // Saved margins may be another printer's -- or a PDF's.
  enforceMinimumMargins(target);
}

bool SchPrint::enforceMinimumMargins(QPrinter *printer) {
  if (printer == nullptr || printer->outputFormat() != QPrinter::NativeFormat)
    return false;

  QPageLayout pageLayout = printer->pageLayout();
  pageLayout.setUnits(QPageLayout::Millimeter);

  const QMarginsF current = pageLayout.margins();
  const QMarginsF minimum = pageLayout.minimumMargins();
  const QMarginsF wanted(std::max(current.left(), minimum.left()),
                         std::max(current.top(), minimum.top()),
                         std::max(current.right(), minimum.right()),
                         std::max(current.bottom(), minimum.bottom()));
  if (wanted == current) return false;

  pageLayout.setMode(QPageLayout::StandardMode);
  pageLayout.setMargins(wanted);
  if (!printer->setPageLayout(pageLayout)) return false;

  qInfo("Print: margins raised to the printer's minimum: %.1f/%.1f/%.1f/%.1f mm",
        wanted.left(), wanted.top(), wanted.right(), wanted.bottom());
  return true;
}

void SchPrint::savePrinter() const {
  if (!printer) return;
  if (printSettings.settingsFile.isEmpty()) return;

  const QPageLayout pageLayout = printer->pageLayout();
  const QPageSize   size       = pageLayout.pageSize();
  const QSizeF      sizeMm     = size.size(QPageSize::Millimeter);
  const QMarginsF   margins    = pageLayout.margins(QPageLayout::Millimeter);

  // Each key on its own -- other instances share the file.
  QSettings settings(printSettings.settingsFile, QSettings::IniFormat);
  settings.beginGroup(QString::fromLatin1(kPrintGroup));
  settings.setValue(kPrinterKey, printer->printerName());
  settings.setValue(kPaperIdKey, static_cast<int>(size.id()));
  settings.setValue(kPaperWKey, sizeMm.width());
  settings.setValue(kPaperHKey, sizeMm.height());
  settings.setValue(kOrientKey, pageLayout.orientation() == QPageLayout::Landscape);
  settings.setValue(kMarginsKey, QStringLiteral("%1,%2,%3,%4")
                                     .arg(margins.left())
                                     .arg(margins.top())
                                     .arg(margins.right())
                                     .arg(margins.bottom()));
  settings.endGroup();
  settings.sync();
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

double SchPrint::bandHeight(QPrinter *printer) {
  const QFontMetricsF metrics(headerFont(), printer);
  return metrics.height() * (1.0 + kBandGapLines);
}

QSizeF SchPrint::bodySizeMm(QPrinter *printer) {
  const double res  = printer->resolution();
  const QSizeF page = printer->pageLayout().paintRectPixels(static_cast<int>(res)).size();
  const double band = bandHeight(printer);
  return QSizeF(page.width() / pxPerMm(res),
                (page.height() - 2.0 * band) / pxPerMm(res));
}

SchPrintLayout SchPrint::layout(QPrinter *printer, const SchRenderSource *view,
                                const SchPrintSettings &settings) {
  const int    res  = printer->resolution();
  const double band = bandHeight(printer);
  return layoutFor(printer->pageLayout().paintRectPixels(res).size(), res,
                   settings.headerOn ? band : 0.0, settings.footerOn ? band : 0.0,
                   view, settings);
}

SchPrintLayout SchPrint::layoutFor(const QSizeF &pageSize, double resolution,
                                   double headerBand, double footerBand,
                                   const SchRenderSource *view, const SchPrintSettings &settings) {
  SchPrintLayout out;

  const QRectF page(QPointF(0.0, 0.0), pageSize);
  out.body = page.adjusted(0.0, headerBand, 0.0, -footerBand);

  // What is printed. An empty document prints what the window shows.
  QRectF source = settings.whole() ? view->printableSceneRect()
                                   : view->visibleSceneRect();
  if (source.isEmpty()) source = view->visibleSceneRect();
  out.source = source;

  if (source.isEmpty() || out.body.isEmpty()) return out;

  // --- Fitted: one page, the source scaled into it ---------------------
  if (!settings.atScale()) {
    SchPrintTile tile;
    tile.page   = 1;
    tile.scene  = source;
    tile.target = fitted(source.size(), out.body);
    out.tiles.append(tile);
    out.pages   = 1;
    out.covered = source;
    return out;
  }

  // --- At scale: as many pages as it takes -----------------------------
  const double devPerUnit = settings.mmPer1000 / 1000.0 * pxPerMm(resolution);
  if (devPerUnit <= 0.0) return out;

  const double pageW = out.body.width() / devPerUnit;  // scene units
  const double pageH = out.body.height() / devPerUnit;

  double overlap = settings.overlapOn
                       ? settings.overlapMm * 1000.0 / settings.mmPer1000
                       : 0.0;
  overlap = std::min(overlap, kMaxOverlapShare * std::min(pageW, pageH));
  out.overlapPx = overlap * devPerUnit;

  const double stepX = pageW - overlap;
  const double stepY = pageH - overlap;

  const auto countFor = [](double extent, double pageExtent, double step) {
    if (extent <= pageExtent) return 1;
    return std::max(1, static_cast<int>(std::ceil((extent - pageExtent) / step)) + 1);
  };

  out.cols = countFor(source.width(), pageW, stepX);
  out.rows = countFor(source.height(), pageH, stepY);

  // A runaway guard: a small scale on a large schematic can ask for
  // hundreds of pages. Refused WHOLE -- a grid cut down to the limit
  // would print part of the drawing as if it were all of it.
  const int limit = std::clamp(settings.maxPages, 1,
                               SchPrintSettings::kMaxPagesHardLimit);
  if (out.cols * out.rows > limit) {
    out.overLimit = true;
    return out;
  }

  // The drawing centred within the whole grid, so the spare space is
  // shared out rather than all left on the last row and column.
  const double gridW = out.cols * stepX + overlap;
  const double gridH = out.rows * stepY + overlap;
  const double left  = source.left() - (gridW - source.width()) / 2.0;

  // Scene Y runs UP the page: row 0 is the TOP, the largest Y.
  const double top = source.bottom() + (gridH - source.height()) / 2.0;

  out.covered = QRectF(left, top - gridH, gridW, gridH);

  const bool single = out.cols * out.rows == 1;

  for (int row = 0; row < out.rows; ++row) {
    for (int col = 0; col < out.cols; ++col) {
      SchPrintTile tile;
      tile.index  = static_cast<int>(out.tiles.size());
      tile.row    = row;
      tile.col    = col;
      tile.scene  = QRectF(left + col * stepX, top - row * stepY - pageH, pageW,
                           pageH);
      tile.target = out.body;

      // Blank pages are skipped -- never the only one.
      if (single || view->hasPrintableIn(tile.scene)) tile.page = ++out.pages;

      out.tiles.append(tile);
    }
  }

  return out;
}

void SchPrint::chooseOrientation(QPrinter *printer, const SchRenderSource *view,
                                 const SchPrintSettings &settings) {
  if (!settings.autoOrient || printer == nullptr || view == nullptr) return;

  const int    res        = printer->resolution();
  const double band       = bandHeight(printer);
  const double headerBand = settings.headerOn ? band : 0.0;
  const double footerBand = settings.footerOn ? band : 0.0;

  QPageLayout portrait  = printer->pageLayout();
  QPageLayout landscape = printer->pageLayout();
  portrait.setOrientation(QPageLayout::Portrait);
  landscape.setOrientation(QPageLayout::Landscape);

  const SchPrintLayout p =
      layoutFor(portrait.paintRectPixels(res).size(), res, headerBand, footerBand,
                view, settings);
  const SchPrintLayout l =
      layoutFor(landscape.paintRectPixels(res).size(), res, headerBand, footerBand,
                view, settings);

  // At scale: fewer pages. Fitted: the bigger drawing. A tie keeps the
  // printer's own choice.
  QPageLayout::Orientation best = printer->pageLayout().orientation();

  if (settings.atScale()) {
    if (p.pages < l.pages) best = QPageLayout::Portrait;
    if (l.pages < p.pages) best = QPageLayout::Landscape;
  } else if (!p.tiles.isEmpty() && !l.tiles.isEmpty()) {
    const double pArea = p.tiles.first().target.width() * p.tiles.first().target.height();
    const double lArea = l.tiles.first().target.width() * l.tiles.first().target.height();
    if (pArea > lArea) best = QPageLayout::Portrait;
    if (lArea > pArea) best = QPageLayout::Landscape;
  }

  if (best != printer->pageLayout().orientation())
    printer->setPageOrientation(best);
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

SchPrintContext SchPrint::context(const SchRenderSource *view, const QString &title,
                                  const SchPrintLayout &layout,
                                  const SchPrintTile &tile, QPrinter *printer) {
  SchPrintContext ctx;
  ctx.title   = title;
  ctx.printed = QDateTime::currentDateTime();
  ctx.page    = std::max(1, tile.page);
  ctx.pages   = std::max(1, layout.pages);
  ctx.row     = tile.row + 1;
  ctx.col     = tile.col + 1;
  ctx.tiled   = layout.tiled();

  if (view != nullptr && !view->sourcePath().isEmpty()) {
    const QFileInfo info(view->sourcePath());
    ctx.path = QDir::toNativeSeparators(info.absoluteFilePath());
    if (info.exists()) ctx.modified = info.lastModified();
  }

  if (printer != nullptr) {
    ctx.printer = (printer->outputFormat() == QPrinter::PdfFormat)
                      ? tr("PDF file")
                      : printer->printerName();

    // The scale IN EFFECT -- for the fit modes, whatever the fit gave.
    if (tile.scene.width() > 0.0 && printer->resolution() > 0)
      ctx.mmPer1000 = tile.target.width() / tile.scene.width() * 1000.0 /
                      pxPerMm(printer->resolution());
  }
  return ctx;
}

SchPrint::PageText SchPrint::pageText(const SchRenderSource *view, const QString &title,
                                      const SchPrintLayout &layout,
                                      const SchPrintTile &tile, QPrinter *printer,
                                      const SchPrintSettings &settings) {
  const SchPrintContext ctx = context(view, title, layout, tile, printer);

  PageText text;
  for (int i = 0; i < 3; ++i) {
    if (settings.headerOn) text.header[i] = SchPrintFields::expand(settings.headerText[i], ctx);
    if (settings.footerOn) text.footer[i] = SchPrintFields::expand(settings.footerText[i], ctx);
  }
  return text;
}

void SchPrint::drawBand(QPainter &painter, const QRectF &band,
                        const QString (&texts)[3]) {
  const QFontMetricsF metrics(painter.font(), painter.device());
  const double        gap   = metrics.height();
  const double        width = band.width();

  // The centre first, at most what leaves a gap either side; the left
  // and right then share what remains of their halves. With no centre,
  // the right keeps its whole text if it can (a date) and the left gives
  // way (a path, shortened in the middle -- drive and file name kept).
  double centreW = 0.0;
  QString centre;
  if (!texts[1].isEmpty()) {
    centre  = metrics.elidedText(texts[1], Qt::ElideMiddle, std::max(0.0, width - 2.0 * gap));
    centreW = metrics.horizontalAdvance(centre);
  }

  double leftRoom  = 0.0;
  double rightRoom = 0.0;
  if (centreW > 0.0) {
    leftRoom  = std::max(0.0, (width - centreW) / 2.0 - gap);
    rightRoom = leftRoom;
  } else {
    rightRoom = std::min(metrics.horizontalAdvance(texts[2]), width / 2.0);
    leftRoom  = std::max(0.0, width - rightRoom - (texts[2].isEmpty() ? 0.0 : gap));
  }

  painter.drawText(band, Qt::AlignLeft | Qt::AlignVCenter,
                   metrics.elidedText(texts[0], Qt::ElideMiddle, leftRoom));
  if (!centre.isEmpty()) painter.drawText(band, Qt::AlignHCenter | Qt::AlignVCenter, centre);
  painter.drawText(band, Qt::AlignRight | Qt::AlignVCenter,
                   metrics.elidedText(texts[2], Qt::ElideMiddle, rightRoom));
}

void SchPrint::drawCropMarks(QPainter &painter, const SchPrintLayout &layout,
                             const SchPrintTile &tile, double resolution,
                             double penPx) {
  // Marks on the MIDDLE of each overlap strip shared with a neighbour --
  // the same scene line on both sheets, so one is trimmed there and laid
  // on the other's marks. Without overlap they fall on the page edge.
  const QRectF body = layout.body;
  const double half = layout.overlapPx / 2.0;
  const double len  = kCropMarkMm * pxPerMm(resolution);

  QPen pen(Qt::black);
  pen.setWidthF(std::max(1.0, penPx));
  painter.setPen(pen);

  const auto vertical = [&](double x) {
    painter.drawLine(QPointF(x, body.top()), QPointF(x, body.top() + len));
    painter.drawLine(QPointF(x, body.bottom() - len), QPointF(x, body.bottom()));
  };
  const auto horizontal = [&](double y) {
    painter.drawLine(QPointF(body.left(), y), QPointF(body.left() + len, y));
    painter.drawLine(QPointF(body.right() - len, y), QPointF(body.right(), y));
  };

  if (tile.col > 0) vertical(body.left() + half);
  if (tile.col < layout.cols - 1) vertical(body.right() - half);
  if (tile.row > 0) horizontal(body.top() + half);
  if (tile.row < layout.rows - 1) horizontal(body.bottom() - half);
}

bool SchPrint::render(QPrinter *printer, const SchRenderSource *view, const QString &title,
                      const SchPrintSettings &settings,
                      const std::function<bool(const SchPrintTile &)> &include,
                      const std::function<bool(int done, int total)> &progress) {
  if (printer == nullptr || view == nullptr) return false;

  enforceMinimumMargins(printer);
  chooseOrientation(printer, view, settings);
  const SchPrintLayout pages = layout(printer, view, settings);

  {
    // DIAGNOSTIC: what the page really is, for comparing against paper.
    const QPageLayout pl = printer->pageLayout();
    const QMarginsF   m  = pl.margins(QPageLayout::Millimeter);
    QPageLayout       mm = pl;
    mm.setUnits(QPageLayout::Millimeter);
    const QMarginsF min = mm.minimumMargins();
    qInfo("Print: \"%s\" %s %s, %d dpi, margins %.1f/%.1f/%.1f/%.1f mm "
          "(printer minimum %.1f/%.1f/%.1f/%.1f), %d page(s) of %d x %d",
          qPrintable(printer->printerName()), qPrintable(pl.pageSize().name()),
          pl.orientation() == QPageLayout::Landscape ? "landscape" : "portrait",
          printer->resolution(), m.left(), m.top(), m.right(), m.bottom(),
          min.left(), min.top(), min.right(), min.bottom(), pages.pages,
          pages.rows, pages.cols);
  }

  // The pages actually going out, for the progress count.
  int total = 0;
  for (const SchPrintTile &tile : pages.tiles)
    if (tile.page != 0 && (!include || include(tile))) ++total;

  QPainter painter;
  if (!painter.begin(printer)) {
    qWarning() << "Print: could not start painting on the printer";
    return false;
  }

  int done = 0;

  const double res     = printer->resolution();
  const double minLine = settings.minLineMm * pxPerMm(res);

  const QFont         font = headerFont();
  const QFontMetricsF metrics(font, printer);
  const double        line = metrics.height();

  const QRectF page(QPointF(0.0, 0.0),
                    QSizeF(printer->pageLayout().paintRectPixels(static_cast<int>(res)).size()));
  const QRectF header(page.left(), page.top(), page.width(), line);
  const QRectF footer(page.left(), page.bottom() - line, page.width(), line);

  bool first = true;
  for (const SchPrintTile &tile : pages.tiles) {
    if (tile.page == 0) continue;
    if (include && !include(tile)) continue;

    // Asked BETWEEN pages, never mid-page: the print-only state below
    // (minimum line width, hidden highlights) is never in force while
    // the progress dialog lets the window repaint.
    if (progress && !progress(done, total)) {
      printer->abort();
      painter.end();
      return false;
    }
    ++done;

    if (!first) printer->newPage();
    first = false;

    // --- Header and footer --------------------------------------------
    const PageText text = pageText(view, title, pages, tile, printer, settings);

    painter.setFont(font);
    painter.setPen(Qt::black);
    if (settings.headerOn) drawBand(painter, header, text.header);
    if (settings.footerOn) drawBand(painter, footer, text.footer);

    // --- The drawing -----------------------------------------------------
    SchPen::printMinDevicePx = minLine;
    view->renderSceneForPrint(&painter, tile.target, tile.scene, settings.printBackground);
    SchPen::printMinDevicePx = 0.0;

    if (pages.tiled() && settings.cropMarks)
      drawCropMarks(painter, pages, tile, res, minLine);
  }

  if (progress) progress(total, total);
  return true;
}

// ===========================================================================
// SVG export
// ===========================================================================

bool SchPrint::writeSvg(const QString &path, const SchRenderSource *view, const QString &title,
                        const SchPrintSettings &settings, const QString &creator) {
  // Layered for Inkscape -- see SchSvgExport.
  return SchSvgExport::write(path, view, title, SchPrintSettings::kDefaultMmPer1000,
                             settings.minLineMm, creator);
}

} // namespace sch::output
