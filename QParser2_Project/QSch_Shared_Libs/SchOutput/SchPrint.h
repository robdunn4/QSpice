#pragma once

#include <QList>
#include <QRectF>
#include <QSizeF>
#include <QString>

#include <algorithm>
#include <functional>
#include <memory>

#include "SchPrintFields.h"

class QPainter;
class QPrinter;

namespace sch::render { class SchRenderSource; }

namespace sch::output {

// What to print and at what scale. Global, kept in the PROGRAM's ini
// file (the one SchPrint is given) under
// [Print], one value per key.
struct SchPrintSettings {
  enum class Mode {
    VisibleFit   = 0, // the area the window shows, fitted to one page
    WholeFit     = 1, // the whole schematic, fitted to one page
    WholeScale   = 2, // the whole schematic at scale, tiled
    VisibleScale = 3, // the visible area at scale, tiled
  };

  // Paper length of 1000 schematic units: the default, 1 inch, puts a
  // 100-unit grid step at 0.1 inch; the limits are about a tenth and ten
  // times that.
  //
  // A scale is always a whole number of STEPS in the unit shown -- 0.1 mm
  // or 0.01 inch -- with limits of its own in each unit.
  static constexpr double kDefaultMmPer1000 = 25.4;
  static constexpr double kMmPerInch        = 25.4;
  static constexpr double kMmStep           = 0.1;
  static constexpr double kMinMm            = 2.5;
  static constexpr double kMaxMm            = 254.0;
  static constexpr double kInchStep         = 0.01;
  static constexpr double kMinInch          = 0.10;
  static constexpr double kMaxInch          = 10.00;

  Mode   mode       = Mode::VisibleFit;

  // The scale IN EFFECT -- what layout() and render() use. Usually the
  // saved scale; Fit changes this alone, for the dialog it is used in.
  double mmPer1000  = kDefaultMmPer1000;

  // The scale the user chose (typed, stepped or reset) -- the one saved,
  // and the one every Print dialog starts from.
  double savedMmPer1000 = kDefaultMmPer1000;

  bool   inchUnits  = false; // the scale SHOWN in inches (stored in mm)
  bool   autoOrient = false; // orientation needing the fewest pages
  bool   overlapOn  = true;
  double overlapMm  = 6.0;  // shared strip between neighbouring pages
  bool   cropMarks  = true;

  // The canvas colour behind the drawing, as on screen. Off, the paper
  // shows through. Printing and PDF only: SVG export keeps its locked
  // background layer either way.
  bool   printBackground = true;
  double minLineMm  = 0.1;  // thinnest printed line
  int    maxPages   = 50;   // the largest page grid allowed, in pages

  // Header and footer: each band on or off, and its three slots (left,
  // centre, right) as templates of text and {fields} -- see
  // SchPrintFields. A band that is off takes no space on the page.
  bool    headerOn = true;
  bool    footerOn = true;
  QString headerText[3] = {SchPrintFields::defaultHeaderLeft(),
                           SchPrintFields::defaultHeaderCenter(),
                           SchPrintFields::defaultHeaderRight()};
  QString footerText[3] = {SchPrintFields::defaultFooterLeft(),
                           SchPrintFields::defaultFooterCenter(),
                           SchPrintFields::defaultFooterRight()};

  // The most maxPages can ever be set to.
  static constexpr int kMaxPagesHardLimit = 500;

  bool atScale() const {
    return mode == Mode::WholeScale || mode == Mode::VisibleScale;
  }
  bool whole() const {
    return mode == Mode::WholeFit || mode == Mode::WholeScale;
  }

  // Both scales set to `mm`, made a whole number of steps of the unit
  // shown (to the nearest) and held within its limits.
  void setScale(double mm);

  // `mm` as a whole number of steps of mm or inch -- the nearest, or the
  // one at or below -- within that unit's limits; returned in mm.
  static double quantizeScale(double mm, bool inch, bool roundDown = false);

  // The limits, in mm, for the unit.
  static double minScaleMm(bool inch) { return inch ? kMinInch * kMmPerInch : kMinMm; }
  static double maxScaleMm(bool inch) { return inch ? kMaxInch * kMmPerInch : kMaxMm; }

  // Read from and written to `settingsFile`, an ini file, [Print]
  // group -- set by SchPrint from what its owner passed in. Empty: not
  // persisted. The library never decides where settings live.
  QString settingsFile;

  void load();
  void save() const; // the SAVED scale, not a fitted one
};

// The printer and paper prints start with, as kept in [Print] -- read and
// written WITHOUT opening the printer, for a settings dialog that must not
// wait on one that is asleep. The same keys SchPrint::savePrinter() writes
// and a new QPrinter is given (margins are left as they are).
struct SchPrinterChoice {
  QString printer;          // empty: the system's default printer
  int     paperId    = -1;  // a QPageSize::PageSizeId; -1: none saved
  double  paperWidthMm  = 0.0; // for a custom size (paperId Custom)
  double  paperHeightMm = 0.0;
  bool    landscape  = false;

  static SchPrinterChoice load(const QString &settingsFile);
  void                    save(const QString &settingsFile) const;
};

// One page of a layout: the scene area it shows and where that lands on
// the page, in device pixels relative to the printable area.
struct SchPrintTile {
  int    index = 0; // in SchPrintLayout::tiles, row-major
  int    row   = 0;
  int    col   = 0;
  int    page  = 0; // 1-based page number; 0 = blank, not printed
  QRectF scene;
  QRectF target;
};

struct SchPrintLayout {
  QList<SchPrintTile> tiles; // every grid cell, blank ones included
  int    rows  = 1;
  int    cols  = 1;
  int    pages = 0;       // tiles with a page number
  QRectF source;          // the scene area being printed
  QRectF covered;         // the area the whole grid covers
  QRectF body;            // the drawing area of a page, device pixels
  double overlapPx = 0.0; // overlap on the page, device pixels

  // The grid would have exceeded SchPrintSettings::maxPages: rows and
  // cols say how big it would have been, and there are NO tiles -- an
  // over-limit job is refused whole rather than cut down to a part.
  bool overLimit = false;

  bool tiled() const { return rows * cols > 1; }
};

// File > Print for one window.
//
// Four modes (SchPrintSettings::Mode): the visible area or the whole
// schematic, fitted to one page or printed at a fixed scale -- 1000
// schematic units = mmPer1000 on paper -- and tiled across as many pages
// as that needs, with overlap and crop marks to join them. Pages with
// nothing on them are skipped. Screen colours, background included; no
// grid, no highlights. Vector output. Lines are never thinner than
// minLineMm on paper (SchPen::printMinDevicePx).
//
// A header and a footer frame each page, either or both switched off in
// the settings: three slots each (left, centre, right), templates of
// text and {fields} (SchPrintFields; PrintHeaderFooterFields.md). By
// default: file path left and file date right; page number left (when
// there are several) and print date right.
//
// PRINTING ALWAYS GOES THROUGH THE PRINT DIALOG (SchPrintDlg): the
// printer, copies, paper, orientation and margins are all chosen there,
// and Print prints exactly what is shown, with no further dialog. Save
// as PDF writes the same pages with Qt's own PDF writer.
//
// Printer name, paper, orientation and margins are saved in [Print] and
// reapplied when the printer still exists; the print settings are
// global too.
//
// No signals -- no moc.
class SchPrint {
public:
  // `settingsFile`: the ini file the print settings and the saved
  // printer live in ([Print]); the application's own. Empty for none.
  explicit SchPrint(const QString &settingsFile);

  // As above, with `defaults` in place of the library's for every setting
  // the file does not have -- a program whose defaults differ (QSchView
  // prints without the background unless asked).
  SchPrint(const QString &settingsFile, const SchPrintSettings &defaults);
  ~SchPrint();

  // The printer, its paper, orientation and margins -> [Print].
  void savePrinter() const;

  // Back to what the file says: the settings re-read over the defaults
  // given at construction, and the printer dropped so the next
  // ensurePrinter() opens the saved one afresh. For a program whose Print
  // dialog changes nothing permanently (QSchView). Call ONLY while
  // nothing holds the printer.
  void reloadDefaults();

  SchPrintSettings &settings() { return printSettings; }
  QPrinter         *ensurePrinter();

  // Whether Windows actually opened `printer`. One that is off, asleep
  // or unreachable can still be SELECTED, but then reports 0 dpi and an
  // empty page, so nothing lays out and the preview stays blank (seen:
  // "QWin32PrintEngine::initialize: CreateDC failed").
  static bool isUsable(const QPrinter *printer);

  // Opens the printer afresh if it is not usable -- a retry, for a
  // printer that was asleep. Call ONLY while nothing holds the printer
  // (before the Print dialog opens): the old QPrinter is deleted.
  void reopenPrinterIfUnusable();

  // With autoOrient on: turns the printer to whichever orientation the
  // layout needs fewer pages in (fit modes: the larger drawing). Called
  // before laying out or painting.
  static void chooseOrientation(QPrinter *printer, const sch::render::SchRenderSource *view,
                                const SchPrintSettings &settings);

  static SchPrintLayout layout(QPrinter *printer, const sch::render::SchRenderSource *view,
                               const SchPrintSettings &settings);

  // Paints the layout's pages for which `include` is true, one printer
  // page each. A null `include` prints them all.
  //
  // `progress`, if given, is called before each page with the pages done
  // and the total, and once more at the end; returning false CANCELS --
  // the job is aborted and render() returns false.
  static bool render(QPrinter *printer, const sch::render::SchRenderSource *view, const QString &title,
                     const SchPrintSettings &settings,
                     const std::function<bool(const SchPrintTile &)> &include,
                     const std::function<bool(int done, int total)> &progress = nullptr);

  // A real printer's margins raised, side by side, to at least the
  // printer's own minimum -- its unprintable border. A saved or typed
  // margin smaller than that puts the header and footer, and the page
  // edges of the drawing, where the printer cannot put ink. True if
  // anything changed. PDF output has no minimum and is left alone.
  static bool enforceMinimumMargins(QPrinter *printer);

  // The drawing area of a page, in millimetres -- for "fit N pages wide".
  static QSizeF bodySizeMm(QPrinter *printer);

  // What the header/footer fields refer to, for one page of `layout`.
  // Public for the header/footer dialog's sample line.
  static SchPrintContext context(const sch::render::SchRenderSource *view, const QString &title,
                                 const SchPrintLayout &layout,
                                 const SchPrintTile &tile, QPrinter *printer);

  // --- SVG export ---
  //
  // File > Export as SVG: ONE file,
  // the WHOLE schematic, drawing only -- no header, footer, page grid or
  // crop marks -- at the DEFAULT scale (25.4 mm per 1000 units), so it
  // imports at a known physical size. Whatever the print settings say
  // about mode and scale is ignored; the thinnest-line setting applies.
  // Screen colours, canvas background, no grid or highlights, as in
  // print.
  //
  // Writes the file, no questions asked: a LAYERED file for Inkscape
  // (SchSvgExport). False on failure. Asking where, and reporting a
  // failure, are the program's (QSchEdit: MainWin::onExportSvg()) -- this class has
  // no dialogs of its own.
  //
  // `creator` is the PROGRAM's name, written into the file's description
  // ("Exported by <creator> ..."); a library has no name of its own to
  // give.
  static bool writeSvg(const QString &path, const sch::render::SchRenderSource *view, const QString &title,
                       const SchPrintSettings &settings, const QString &creator);

protected:
  struct PageText {
    QString header[3]; // left, centre, right
    QString footer[3];
  };

  static PageText pageText(const sch::render::SchRenderSource *view, const QString &title,
                           const SchPrintLayout &layout, const SchPrintTile &tile,
                           QPrinter *printer, const SchPrintSettings &settings);

  // One band's three slots in `band`: the centre slot centred, the left
  // and right in what is left; anything too long is shortened in the
  // middle.
  static void drawBand(QPainter &painter, const QRectF &band,
                       const QString (&texts)[3]);

  // Layout for a page of `pageSize` device pixels at `resolution` dpi,
  // with `headerBand` and `footerBand` pixels taken by the bands (0 for a
  // band that is off).
  static SchPrintLayout layoutFor(const QSizeF &pageSize, double resolution,
                                  double headerBand, double footerBand,
                                  const sch::render::SchRenderSource *view, const SchPrintSettings &settings);

  // Header/footer band height, device pixels, for this printer.
  static double bandHeight(QPrinter *printer);

  // The saved printer and page layout, applied to a new QPrinter.
  void applySavedPrinter(QPrinter *target) const;

  static void drawCropMarks(QPainter &painter, const SchPrintLayout &layout,
                            const SchPrintTile &tile, double resolution,
                            double penPx);

  std::unique_ptr<QPrinter> printer;
  SchPrintSettings          printSettings;
  SchPrintSettings          defaultSettings; // what load() starts from
};

} // namespace sch::output
