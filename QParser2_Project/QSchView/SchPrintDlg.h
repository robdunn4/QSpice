#pragma once

#include <QDialog>
#include <QList>
#include <QSet>
#include <QString>

#include <functional>

#include "SchPrint.h"

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPrintPreviewWidget;
class QProgressDialog;
class QPushButton;
class QSpinBox;
class QToolButton;
class SchPageMap;
namespace sch::render { class SchRenderSource; }

// File > Print (titled "Print") -- the ONLY way to print: the printer and print
// settings, a live preview, and a PAGE MAP -- the printed area with the
// page grid over it, where a click includes or excludes a page.
//
// Everything that changes the output is chosen HERE, and the preview
// follows it; Print then prints exactly what is shown, after one "Print?"
// confirmation (printer, pages, copies, paper, scale) and no printer
// dialog. Page Setup (paper, orientation, margins, and the driver's own
// properties behind its Printer button) only changes settings. Print to
// PDF writes the same pages with Qt's PDF writer. (SVG is File > Export
// as SVG, not here: it ignores every page setting.)
//
// QSchView's copy: NOTHING here is saved. Changes -- settings, printer,
// Page Setup -- apply to the preview and to this print only; the next
// Print opens with the .ini values again (Options > Printing Defaults).
// The one exception is Configure..., which asks whether changed header
// and footer text should become the default. Page choices start afresh
// each time the dialog opens, and whenever the grid itself changes.
//
// No Q_OBJECT -- every connection is a lambda -- so no moc.
class SchPrintDlg : public QDialog {
public:
  SchPrintDlg(sch::output::SchPrint *schPrint, const sch::render::SchRenderSource *schView,
                     const QString &docTitle, QWidget *parent = nullptr);

protected:
  void buildUi();
  void loadControls();

  // Controls -> settings (this print only); then everything recomputed.
  void applyControls();

  // Layout recomputed; page choices kept only if the grid is unchanged.
  void relayout();

  void updateInfo();
  void updateScaleEnabled();

  // The scale field shows mm or inches per 1000 units.
  double shownScale() const;
  void   showScale(double mmPer1000);

  // The scale typed or stepped: the scale in effect AND the saved one.
  void onScaleEdited();

  // The mm or in button clicked: the scale recalculated in that unit.
  void onUnitChosen(bool inch);

  // The default scale, 25.4 mm (1 inch) per 1000 units.
  void resetScale();

  // The reset button, enabled only at scale and off the default.
  void updateResetEnabled();

  // Fit: the scale for N pages wide, rounded DOWN to 0.1 mm and held
  // within the limits. In effect for this dialog only -- never saved.
  void fitWidth();

  // The scale the layout really prints at -- for the fit modes, whatever
  // the fit gave. 0 when there is nothing to print.
  double effectiveMmPer1000() const;

  // "25.4 mm (1.00 inch) per 1000 units".
  static QString scaleText(double mmPer1000);

  // "1-4, 6, 8, 9" -- page numbers in ascending order.
  static QString pageRangesText(const QList<int> &pages);

  // The "Print?" box: printer, pages, copies, paper and scale. True to
  // print.
  bool confirmPrint();
  void onPageSetup();
  void onPrint();
  void onPrintPdf();

  // Borders > Configure...: the header/footer slots (SchHeaderFooterDlg).
  void onConfigureBands();

  // A progress callback for SchPrint::render() driving `progress`,
  // titled `caption` -- "page n of N" through `label` (%1, %2), Cancel
  // honoured.
  std::function<bool(int, int)> progressFn(QProgressDialog &progress,
                                           const QString &caption,
                                           const QString &label);

  // The printer list, with the current printer selected.
  void fillPrinters();

  bool included(const sch::output::SchPrintTile &tile) const;

  sch::output::SchPrint      *print  = nullptr;
  const sch::render::SchRenderSource *view   = nullptr;
  QString        title;
  sch::output::SchPrintLayout pageLayout;
  QSet<int>      excluded; // tile indices the user switched off
  int            gridRows = 0;
  int            gridCols = 0;
  bool           loading  = false; // controls being filled, not edited
  QString        fitNote;          // why Fit fell short; cleared by edits

  QComboBox           *printerCombo  = nullptr;
  QSpinBox            *copiesSpin    = nullptr;
  QComboBox           *modeCombo     = nullptr;
  QDoubleSpinBox      *scaleSpin     = nullptr;
  QToolButton         *mmBtn         = nullptr;
  QToolButton         *inchBtn       = nullptr;
  QButtonGroup        *unitGroup     = nullptr;
  QSpinBox            *fitWideSpin   = nullptr;
  QPushButton         *fitBtn        = nullptr;
  QToolButton         *resetScaleBtn = nullptr;
  QCheckBox           *autoOrientChk = nullptr;
  QCheckBox           *overlapChk    = nullptr;
  QDoubleSpinBox      *overlapSpin   = nullptr;
  QSpinBox            *pageLimitSpin = nullptr;
  QPushButton         *printBtn      = nullptr;
  QPushButton         *pdfBtn        = nullptr;
  QCheckBox           *cropChk       = nullptr;
  QCheckBox           *headerChk     = nullptr;
  QCheckBox           *footerChk     = nullptr;
  QCheckBox           *backgroundChk = nullptr; // the canvas colour, this print
  QDoubleSpinBox      *minLineSpin   = nullptr;
  QLabel              *infoLbl       = nullptr;
  SchPageMap          *pageMap       = nullptr;
  QPrintPreviewWidget *previewWidget = nullptr;
};
