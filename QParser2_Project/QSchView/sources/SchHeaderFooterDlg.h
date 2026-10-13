#pragma once

#include <QDialog>

#include "SchPrint.h"
#include "SchPrintFields.h"

class QLabel;
class QLineEdit;

// Print > Configure... : the header and footer slots.
//
// A grid of six text fields -- Header and Footer, each Left, Center and
// Right -- holding text and {fields} (SchPrintFields), each with an
// Insert menu of the fields. Below them a SAMPLE, the slots expanded
// for the current document and its first page, updated as you type.
// Restore Defaults puts back what printing showed before templates.
//
// Works on a copy: apply() writes the result into settings, which the
// caller does only on OK.
//
// No Q_OBJECT -- every connection is a lambda -- so no moc.
class SchHeaderFooterDlg : public QDialog {
public:
  SchHeaderFooterDlg(const sch::output::SchPrintSettings &settings, const sch::output::SchPrintContext &sample,
                     QWidget *parent = nullptr);

  // The six slots into `settings`.
  void apply(sch::output::SchPrintSettings &settings) const;

protected:
  void buildUi();
  void updateSample();
  void restoreDefaults();

  sch::output::SchPrintContext sampleCtx;

  QLineEdit *edits[2][3]   = {}; // [header, footer][left, centre, right]
  QLabel    *samples[2][3] = {};
};
