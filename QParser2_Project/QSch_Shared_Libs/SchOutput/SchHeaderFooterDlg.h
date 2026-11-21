#pragma once

#include <QDialog>

#include "SchPrint.h"
#include "SchPrintFields.h"

class QLabel;
class QLineEdit;

namespace sch::output {

// Print > Configure..., and Preferences > Headers/Footers: the header and
// footer slots. Shared by QSchEdit and QSchView.
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
  SchHeaderFooterDlg(const SchPrintSettings &settings,
                     const SchPrintContext &sample, QWidget *parent = nullptr);

  // The six slots into `settings`.
  void apply(SchPrintSettings &settings) const;

protected:
  void buildUi();
  void updateSample();
  void restoreDefaults();

  SchPrintContext sampleCtx;

  QLineEdit *edits[2][3]   = {}; // [header, footer][left, center, right]
  QLabel    *samples[2][3] = {};
};

} // namespace sch::output
