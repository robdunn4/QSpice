#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "PrintDefaultsDlg.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPageSize>
#include <QPrinterInfo>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "SchHeaderFooterDlg.h"
#include "ViewerPrefs.h"

// SchOutput names used in this file.
using sch::output::SchPrintContext;
using sch::output::SchPrinterChoice;
using sch::output::SchPrintSettings;

namespace {

// The paper sizes offered, in this order. The printer's own list would
// need the printer opened.
const QPageSize::PageSizeId kPapers[] = {
    QPageSize::Letter, QPageSize::Legal, QPageSize::Tabloid, QPageSize::Executive,
    QPageSize::A5,     QPageSize::A4,    QPageSize::A3,      QPageSize::B5,
    QPageSize::B4,
};

} // namespace

PrintDefaultsDlg::PrintDefaultsDlg(const QString &docPath, QWidget *parent)
    : QDialog(parent), docPath(docPath) {
  setWindowTitle(tr("Printing Defaults"));

  settingsCopy = ViewerPrefs::storedPrintSettings();
  choiceCopy   = SchPrinterChoice::load(ViewerPrefs::iniFilePath());

  buildUi();
  showValues();
}

void PrintDefaultsDlg::buildUi() {
  QVBoxLayout *outer = new QVBoxLayout(this);

  // --- What is printed, and how big ---
  QGroupBox   *layoutBox  = new QGroupBox(tr("Layout"), this);
  QFormLayout *layoutForm = new QFormLayout(layoutBox);

  modeCombo = new QComboBox(this);
  modeCombo->addItems({tr("Visible area - fit to page"), tr("Whole schematic - fit to page"),
                       tr("Whole schematic - to scale"), tr("Visible area - to scale")});
  layoutForm->addRow(tr("Print:"), modeCombo);

  scaleSpin = new QDoubleSpinBox(this);
  scaleSpin->setKeyboardTracking(false);
  unitCombo = new QComboBox(this);
  unitCombo->addItems({tr("mm"), tr("in")});

  QHBoxLayout *scaleRow = new QHBoxLayout;
  scaleRow->addWidget(scaleSpin, 1);
  scaleRow->addWidget(unitCombo);
  layoutForm->addRow(tr("Scale (per 1000 units):"), scaleRow);
  connect(unitCombo, &QComboBox::currentIndexChanged, this, [this](int) { onUnitChanged(); });

  autoOrientChk = new QCheckBox(tr("Choose the orientation needing fewer pages"), this);
  layoutForm->addRow(QString(), autoOrientChk);
  outer->addWidget(layoutBox);

  // --- Pages ---
  QGroupBox   *pagesBox  = new QGroupBox(tr("Pages"), this);
  QFormLayout *pagesForm = new QFormLayout(pagesBox);

  overlapChk  = new QCheckBox(tr("Overlap tiled pages by"), this);
  overlapSpin = new QDoubleSpinBox(this);
  overlapSpin->setRange(0.0, 50.0);
  overlapSpin->setDecimals(1);
  overlapSpin->setSuffix(tr(" mm"));
  QHBoxLayout *overlapRow = new QHBoxLayout;
  overlapRow->addWidget(overlapChk);
  overlapRow->addWidget(overlapSpin, 1);
  pagesForm->addRow(QString(), overlapRow);

  cropChk = new QCheckBox(tr("Crop marks on tiled pages"), this);
  pagesForm->addRow(QString(), cropChk);

  minLineSpin = new QDoubleSpinBox(this);
  minLineSpin->setRange(0.0, 2.0);
  minLineSpin->setDecimals(2);
  minLineSpin->setSingleStep(0.05);
  minLineSpin->setSuffix(tr(" mm"));
  pagesForm->addRow(tr("Thinnest line:"), minLineSpin);

  pageLimitSpin = new QSpinBox(this);
  pageLimitSpin->setRange(1, SchPrintSettings::kMaxPagesHardLimit);
  pageLimitSpin->setSuffix(tr(" pages"));
  pagesForm->addRow(tr("Page limit:"), pageLimitSpin);

  headerChk = new QCheckBox(tr("Header"), this);
  footerChk = new QCheckBox(tr("Footer"), this);
  QPushButton *bandsBtn = new QPushButton(tr("Headers/Footers..."), this);
  QHBoxLayout *bandsRow = new QHBoxLayout;
  bandsRow->addWidget(headerChk);
  bandsRow->addWidget(footerChk);
  bandsRow->addStretch(1);
  bandsRow->addWidget(bandsBtn);
  pagesForm->addRow(tr("Borders:"), bandsRow);
  connect(bandsBtn, &QPushButton::clicked, this, [this]() { onHeaderFooter(); });

  backgroundChk = new QCheckBox(tr("Print the background color"), this);
  pagesForm->addRow(QString(), backgroundChk);
  outer->addWidget(pagesBox);

  // --- Printer and paper: by name only ---
  QGroupBox   *printerBox  = new QGroupBox(tr("Printer and paper"), this);
  QFormLayout *printerForm = new QFormLayout(printerBox);

  printerCombo = new QComboBox(this);
  printerCombo->addItem(tr("(Windows default printer)"), QString());
  for (const QString &name : QPrinterInfo::availablePrinterNames())
    printerCombo->addItem(name, name);
  printerForm->addRow(tr("Printer:"), printerCombo);

  paperCombo = new QComboBox(this);
  paperCombo->addItem(tr("(The printer's default)"), -1);
  for (QPageSize::PageSizeId id : kPapers)
    paperCombo->addItem(QPageSize::name(id), static_cast<int>(id));
  printerForm->addRow(tr("Paper:"), paperCombo);

  orientCombo = new QComboBox(this);
  orientCombo->addItems({tr("Portrait"), tr("Landscape")});
  printerForm->addRow(tr("Orientation:"), orientCombo);
  outer->addWidget(printerBox);

  // --- Buttons ---
  QDialogButtonBox *buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
                               QDialogButtonBox::RestoreDefaults,
                           this);
  connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
    readValues();
    accept();
  });
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
          [this]() { restoreDefaults(); });
  outer->addWidget(buttons);
}

void PrintDefaultsDlg::showValues() {
  loading = true;

  modeCombo->setCurrentIndex(static_cast<int>(settingsCopy.mode));
  unitCombo->setCurrentIndex(settingsCopy.inchUnits ? 1 : 0);
  showScale();

  autoOrientChk->setChecked(settingsCopy.autoOrient);
  overlapChk->setChecked(settingsCopy.overlapOn);
  overlapSpin->setValue(settingsCopy.overlapMm);
  cropChk->setChecked(settingsCopy.cropMarks);
  minLineSpin->setValue(settingsCopy.minLineMm);
  pageLimitSpin->setValue(settingsCopy.maxPages);
  headerChk->setChecked(settingsCopy.headerOn);
  footerChk->setChecked(settingsCopy.footerOn);
  backgroundChk->setChecked(settingsCopy.printBackground);

  // A saved printer that is no longer installed is still shown, so OK
  // does not quietly replace it.
  int printerIndex = printerCombo->findData(choiceCopy.printer);
  if (printerIndex < 0) {
    printerCombo->addItem(tr("%1 (not installed)").arg(choiceCopy.printer), choiceCopy.printer);
    printerIndex = printerCombo->count() - 1;
  }
  printerCombo->setCurrentIndex(printerIndex);

  // Likewise a saved paper that is not in the list -- another standard
  // size, or a custom one.
  int paperIndex = paperCombo->findData(choiceCopy.paperId);
  if (paperIndex < 0) {
    const bool custom = choiceCopy.paperId == static_cast<int>(QPageSize::Custom);
    const QString name =
        custom ? tr("Custom %1 x %2 mm")
                     .arg(choiceCopy.paperWidthMm, 0, 'f', 1)
                     .arg(choiceCopy.paperHeightMm, 0, 'f', 1)
               : QPageSize::name(static_cast<QPageSize::PageSizeId>(choiceCopy.paperId));
    paperCombo->addItem(name, choiceCopy.paperId);
    paperIndex = paperCombo->count() - 1;
  }
  paperCombo->setCurrentIndex(paperIndex);

  orientCombo->setCurrentIndex(choiceCopy.landscape ? 1 : 0);

  loading = false;
}

void PrintDefaultsDlg::showScale() {
  const bool inch = settingsCopy.inchUnits;
  const bool wasLoading = loading;
  loading = true;

  // The decimals ARE the step: 0.1 mm or 0.01 inch.
  if (inch) {
    scaleSpin->setDecimals(2);
    scaleSpin->setRange(SchPrintSettings::kMinInch, SchPrintSettings::kMaxInch);
    scaleSpin->setSingleStep(0.1);
    scaleSpin->setValue(settingsCopy.savedMmPer1000 / SchPrintSettings::kMmPerInch);
  } else {
    scaleSpin->setDecimals(1);
    scaleSpin->setRange(SchPrintSettings::kMinMm, SchPrintSettings::kMaxMm);
    scaleSpin->setSingleStep(1.0);
    scaleSpin->setValue(settingsCopy.savedMmPer1000);
  }

  loading = wasLoading;
}

void PrintDefaultsDlg::onUnitChanged() {
  if (loading) return;

  // The scale as typed so far, carried into the other unit at its step.
  const bool   wasInch = settingsCopy.inchUnits;
  const double shownMm =
      wasInch ? scaleSpin->value() * SchPrintSettings::kMmPerInch : scaleSpin->value();

  settingsCopy.inchUnits = (unitCombo->currentIndex() == 1);
  settingsCopy.setScale(shownMm);
  showScale();
}

void PrintDefaultsDlg::readValues() {
  settingsCopy.mode       = static_cast<SchPrintSettings::Mode>(modeCombo->currentIndex());
  settingsCopy.inchUnits  = (unitCombo->currentIndex() == 1);
  settingsCopy.setScale(settingsCopy.inchUnits ? scaleSpin->value() * SchPrintSettings::kMmPerInch
                                               : scaleSpin->value());
  settingsCopy.autoOrient      = autoOrientChk->isChecked();
  settingsCopy.overlapOn       = overlapChk->isChecked();
  settingsCopy.overlapMm       = overlapSpin->value();
  settingsCopy.cropMarks       = cropChk->isChecked();
  settingsCopy.minLineMm       = minLineSpin->value();
  settingsCopy.maxPages        = pageLimitSpin->value();
  settingsCopy.headerOn        = headerChk->isChecked();
  settingsCopy.footerOn        = footerChk->isChecked();
  settingsCopy.printBackground = backgroundChk->isChecked();

  choiceCopy.printer = printerCombo->currentData().toString();

  // A standard size is kept by its id alone; a custom one keeps its size.
  const int paperId = paperCombo->currentData().toInt();
  if (paperId != choiceCopy.paperId) {
    choiceCopy.paperId = paperId;

    // None: no size either, or a new printer would be given the old one.
    choiceCopy.paperWidthMm  = 0.0;
    choiceCopy.paperHeightMm = 0.0;
    if (paperId >= 0) {
      const QSizeF sizeMm =
          QPageSize(static_cast<QPageSize::PageSizeId>(paperId)).size(QPageSize::Millimeter);
      choiceCopy.paperWidthMm  = sizeMm.width();
      choiceCopy.paperHeightMm = sizeMm.height();
    }
  }

  choiceCopy.landscape = (orientCombo->currentIndex() == 1);
}

void PrintDefaultsDlg::restoreDefaults() {
  // Everything as a fresh install has it; the file is untouched until OK.
  const QString file = settingsCopy.settingsFile;
  settingsCopy              = ViewerPrefs::printDefaults();
  settingsCopy.settingsFile = file;
  choiceCopy                = SchPrinterChoice();
  showValues();
}

void PrintDefaultsDlg::onHeaderFooter() {
  // The working copy: written with everything else on OK.
  readValues();

  SchHeaderFooterDlg dialog(settingsCopy, sampleContext(docPath, settingsCopy), this);
  if (dialog.exec() != QDialog::Accepted) return;

  dialog.apply(settingsCopy);
}

void PrintDefaultsDlg::save() const {
  settingsCopy.save();
  choiceCopy.save(ViewerPrefs::iniFilePath());
}

SchPrintContext PrintDefaultsDlg::sampleContext(const QString &docPath,
                                                const SchPrintSettings &settings) {
  SchPrintContext sample;
  sample.printed   = QDateTime::currentDateTime();
  sample.page      = 1;
  sample.pages     = 1;
  sample.mmPer1000 = settings.savedMmPer1000;

  const QString printer = SchPrinterChoice::load(ViewerPrefs::iniFilePath()).printer;
  sample.printer = printer.isEmpty() ? QObject::tr("Default printer") : printer;

  if (docPath.isEmpty()) {
    // No file: an example, so the fields still show what they become.
    const QDir docs(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    sample.path     = QDir::toNativeSeparators(docs.filePath(QStringLiteral("Example.qsch")));
    sample.title    = QStringLiteral("Example.qsch");
    sample.modified = sample.printed;
    return sample;
  }

  const QFileInfo info(docPath);
  sample.path     = QDir::toNativeSeparators(info.absoluteFilePath());
  sample.title    = info.fileName();
  sample.modified = info.lastModified();
  return sample;
}
