#pragma once

#include <QDialog>

#include "SchPrint.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;

namespace sch::output {

// Printing Defaults -- Options in QSchView, Preferences in QSchEdit:
// what the Print dialog opens with, and the ONLY place (besides
// Headers/Footers, and the question the Print dialog's Configure... asks)
// that writes print settings to the program's .ini. Changes made in the
// Print dialog itself last for that print only. Shared by both programs.
//
// Never opens the printer: printers are listed by name, and the paper
// from the standard sizes, so a printer that is asleep cannot hold the
// dialog up.
//
// Works on copies; OK writes them. No Q_OBJECT -- lambdas only -- so no
// moc.
class SchPrintDefaultsDlg : public QDialog {
public:
  // `print`: the program's printing -- its .ini and its own defaults.
  // `shownPath`: the file the window shows, for the header/footer
  // sample; empty while it shows none.
  SchPrintDefaultsDlg(const SchPrint *print, const QString &shownPath,
                      QWidget *parent = nullptr);

  // Writes everything to the .ini. Call on Accepted.
  void save() const;

  // What the header/footer fields become, for the sample line -- built
  // WITHOUT the printer: page 1 of 1, the printer saved in `settings`'
  // file. For `filePath`, or a made-up "Example.qsch" when it is empty.
  static SchPrintContext sampleContext(const QString          &filePath,
                                       const SchPrintSettings &settings);

protected:
  void buildUi();

  // The controls from `settingsCopy` and `choiceCopy`.
  void showValues();

  // The controls back into `settingsCopy` and `choiceCopy`.
  void readValues();

  // The program's own defaults, as a fresh install has them.
  void restoreDefaults();

  void showScale();
  void onUnitChanged();
  void onHeaderFooter();

  QString                       docPath;
  SchPrintSettings              programDefaults; // as a fresh install
  QString                       iniPath;
  SchPrintSettings              settingsCopy;
  SchPrinterChoice              choiceCopy;

  bool loading = false;

  QComboBox      *modeCombo     = nullptr;
  QDoubleSpinBox *scaleSpin     = nullptr;
  QComboBox      *unitCombo     = nullptr;
  QCheckBox      *autoOrientChk = nullptr;
  QCheckBox      *overlapChk    = nullptr;
  QDoubleSpinBox *overlapSpin   = nullptr;
  QCheckBox      *cropChk       = nullptr;
  QDoubleSpinBox *minLineSpin   = nullptr;
  QSpinBox       *pageLimitSpin = nullptr;
  QCheckBox      *headerChk     = nullptr;
  QCheckBox      *footerChk     = nullptr;
  QCheckBox      *backgroundChk = nullptr;
  QComboBox      *printerCombo  = nullptr;
  QComboBox      *paperCombo    = nullptr;
  QComboBox      *orientCombo   = nullptr;
};

} // namespace sch::output
