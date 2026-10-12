#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchHeaderFooterDlg.h"

#include <QAction>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// SchOutput names used in this file.
using sch::output::SchPrintSettings;
using sch::output::SchPrintContext;
namespace SchPrintFields = sch::output::SchPrintFields;

namespace {

QString tr(const char *text) {
  return QCoreApplication::translate("SchHeaderFooterDlg", text);
}

} // namespace

SchHeaderFooterDlg::SchHeaderFooterDlg(const SchPrintSettings &settings,
                                       const SchPrintContext &sample, QWidget *parent)
    : QDialog(parent), sampleCtx(sample) {
  setWindowTitle(tr("Header and Footer"));
  buildUi();

  for (int col = 0; col < 3; ++col) {
    edits[0][col]->setText(settings.headerText[col]);
    edits[1][col]->setText(settings.footerText[col]);
  }
  updateSample();
}

void SchHeaderFooterDlg::buildUi() {
  QGridLayout *grid = new QGridLayout;

  const QString colNames[3] = {tr("Left"), tr("Center"), tr("Right")};
  const QString rowNames[2] = {tr("Header:"), tr("Footer:")};

  for (int col = 0; col < 3; ++col)
    grid->addWidget(new QLabel(colNames[col], this), 0, col + 1, Qt::AlignHCenter);

  for (int row = 0; row < 2; ++row) {
    grid->addWidget(new QLabel(rowNames[row], this), row + 1, 0);

    for (int col = 0; col < 3; ++col) {
      QLineEdit *edit = new QLineEdit(this);
      edit->setMinimumWidth(180);
      edits[row][col] = edit;

      // Insert: the fields, put in at the cursor.
      QToolButton *insertBtn = new QToolButton(this);
      insertBtn->setText(tr("Insert"));
      insertBtn->setToolTip(tr("Insert a field at the cursor"));
      insertBtn->setPopupMode(QToolButton::InstantPopup);

      QMenu *menu = new QMenu(insertBtn);
      for (const std::pair<QString, QString> &entry : SchPrintFields::menuEntries()) {
        if (entry.second.isEmpty()) {
          menu->addSeparator();
          continue;
        }
        const QString token = entry.second;
        QAction      *act   = menu->addAction(QStringLiteral("%1   %2").arg(entry.first, token));
        connect(act, &QAction::triggered, edit, [edit, token]() {
          edit->insert(token);
          edit->setFocus();
        });
      }
      insertBtn->setMenu(menu);

      QHBoxLayout *cell = new QHBoxLayout;
      cell->setSpacing(2);
      cell->addWidget(edit, 1);
      cell->addWidget(insertBtn);
      grid->addLayout(cell, row + 1, col + 1);

      connect(edit, &QLineEdit::textChanged, this, [this](const QString &) { updateSample(); });
    }
  }

  // The sample: both bands as they would print on the first page.
  QFrame *sampleFrame = new QFrame(this);
  sampleFrame->setFrameShape(QFrame::StyledPanel);
  sampleFrame->setFrameShadow(QFrame::Sunken);
  sampleFrame->setAutoFillBackground(true);
  QPalette pal = sampleFrame->palette();
  pal.setColor(QPalette::Window, Qt::white);
  pal.setColor(QPalette::WindowText, Qt::black);
  sampleFrame->setPalette(pal);

  QGridLayout *sampleGrid = new QGridLayout(sampleFrame);
  const Qt::Alignment align[3] = {Qt::AlignLeft, Qt::AlignHCenter, Qt::AlignRight};
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 3; ++col) {
      QLabel *label = new QLabel(sampleFrame);
      label->setTextFormat(Qt::PlainText);
      samples[row][col] = label;
      sampleGrid->addWidget(label, row == 0 ? 0 : 2, col, align[col] | Qt::AlignVCenter);
    }
  }

  // Where the drawing goes, between the two.
  QLabel *drawing = new QLabel(tr("(drawing)"), sampleFrame);
  drawing->setStyleSheet(QStringLiteral("color: gray;"));
  sampleGrid->addWidget(drawing, 1, 0, 1, 3, Qt::AlignCenter);
  sampleGrid->setRowMinimumHeight(1, 40);
  for (int col = 0; col < 3; ++col) sampleGrid->setColumnStretch(col, 1);

  QLabel *help = new QLabel(
      tr("Text and fields such as {path}, {name}, {printed} or {pageinfo} -- "
         "see Insert. Dates and times take a format: {printed:yyyy-MM-dd "
         "HH:mm}, or {printed:short}, :long, :iso, :date, :time. A slot "
         "whose fields are all empty prints nothing. {{ and }} are literal "
         "braces."),
      this);
  help->setWordWrap(true);

  QDialogButtonBox *buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults,
      this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
          [this]() { restoreDefaults(); });

  QVBoxLayout *top = new QVBoxLayout(this);
  top->addLayout(grid);
  top->addWidget(help);
  top->addWidget(new QLabel(tr("Sample (this document, first page):"), this));
  top->addWidget(sampleFrame);
  top->addWidget(buttons);
}

void SchHeaderFooterDlg::updateSample() {
  for (int row = 0; row < 2; ++row)
    for (int col = 0; col < 3; ++col)
      samples[row][col]->setText(
          SchPrintFields::expand(edits[row][col]->text(), sampleCtx));
}

void SchHeaderFooterDlg::restoreDefaults() {
  edits[0][0]->setText(SchPrintFields::defaultHeaderLeft());
  edits[0][1]->setText(SchPrintFields::defaultHeaderCenter());
  edits[0][2]->setText(SchPrintFields::defaultHeaderRight());
  edits[1][0]->setText(SchPrintFields::defaultFooterLeft());
  edits[1][1]->setText(SchPrintFields::defaultFooterCenter());
  edits[1][2]->setText(SchPrintFields::defaultFooterRight());
}

void SchHeaderFooterDlg::apply(SchPrintSettings &settings) const {
  for (int col = 0; col < 3; ++col) {
    settings.headerText[col] = edits[0][col]->text();
    settings.footerText[col] = edits[1][col]->text();
  }
}
