#pragma once

#include <QDialog>

#include "SchPrint.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;

// Options > Printing Defaults: what the Print dialog opens with, and the
// ONLY place (besides Options > Headers/Footers, and the
// question the Print dialog's Configure... asks) that writes print
// settings to QSchView.ini. Changes made in the Print dialog itself last
// for that print only.
//
// Never opens the printer: printers are listed by name, and the paper
// from the standard sizes, so a printer that is asleep cannot hold the
// dialog up.
//
// Works on copies; OK writes them. No Q_OBJECT -- lambdas only -- so no
// moc.
class PrintDefaultsDlg : public QDialog {
public:
  // `docPath`: the file the window shows, for the header/footer sample;
  // empty while it shows none.
  explicit PrintDefaultsDlg(const QString &docPath, QWidget *parent = nullptr);

  // Writes everything to the .ini. Call on Accepted.
  void save() const;

  // What the header/footer fields become, for the sample line -- built
  // WITHOUT the printer: page 1 of 1, the saved printer's name. For
  // `docPath`, or a made-up "Example.qsch" when it is empty.
  static sch::output::SchPrintContext sampleContext(const QString &docPath,
                                                    const sch::output::SchPrintSettings &settings);

protected:
  void buildUi();

  // The controls from `settingsCopy` and `choiceCopy`.
  void showValues();

  // The controls back into `settingsCopy` and `choiceCopy`.
  void readValues();

  // QSchView's own defaults, as a fresh install has them.
  void restoreDefaults();

  void showScale();
  void onUnitChanged();
  void onHeaderFooter();

  QString                       docPath;
  sch::output::SchPrintSettings settingsCopy;
  sch::output::SchPrinterChoice choiceCopy;

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
