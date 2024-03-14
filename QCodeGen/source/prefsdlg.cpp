#include "prefsdlg.h"
#include "ui_prefsdlg.h"

#include "aboutdlg.h"
#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>

PrefsSettings prefsSettings;

PrefsDlg::PrefsDlg(PrefsSettings &ps, MainWindow *parent)
    : QDialog(parent), ps(ps), ui(new Ui::PrefsDlg) {
  ui->setupUi(this);

  setWindowTitle(AboutDlg::appName + " - Preferences Settings");
  setModal(true);

  ui->authorSystemRB->setText("Use System ID (" + qgetenv("username") + ")");

  // set button IDs to ensure that designer changes don't alter values
  ui->authBG->setId(ui->authorSystemRB, PrefsSettings::AT_User);
  ui->authBG->setId(ui->authorCustomRB, PrefsSettings::AT_Custom);

  ui->tmplBG->setId(ui->tmplDefaultRB, PrefsSettings::TT_Internal);
  ui->tmplBG->setId(ui->tmplLastRB, PrefsSettings::TT_LastUsed);
  ui->tmplBG->setId(ui->tmplFileRB, PrefsSettings::TT_File);

  ui->schBG->setId(ui->schLastRB, PrefsSettings::ST_LastUsed);
  ui->schBG->setId(ui->schFolderRB, PrefsSettings::ST_Folder);

  // connect UI
  // schematic radio buttons
  connect(ui->schFolderRB, &QRadioButton::clicked, this,
      &PrefsDlg::startSchRbChgdSlot);
  connect(ui->schLastRB, &QRadioButton::clicked, this,
      &PrefsDlg::startSchRbChgdSlot);

  // schematic selection
  connect(ui->schPathBtn, &QToolButton::clicked, this,
      &PrefsDlg::selectSchSlot);

  // template radio buttons
  connect(ui->tmplFileRB, &QRadioButton::clicked, this,
      &PrefsDlg::startTmplRbChgdSlot);
  connect(ui->tmplDefaultRB, &QRadioButton::clicked, this,
      &PrefsDlg::startTmplRbChgdSlot);
  connect(ui->tmplLastRB, &QRadioButton::clicked, this,
      &PrefsDlg::startTmplRbChgdSlot);

  // template selection
  connect(ui->tmplPathBtn, &QToolButton::clicked, this,
      &PrefsDlg::selectTmplSlot);

  // author radio buttons
  connect(ui->authorSystemRB, &QRadioButton::clicked, this,
      &PrefsDlg::authorRbChgdSlot);
  connect(ui->authorCustomRB, &QRadioButton::clicked, this,
      &PrefsDlg::authorRbChgdSlot);
}

PrefsDlg::~PrefsDlg() { delete ui; }

int PrefsDlg::exec() {
  PrefsSettings ps = this->ps;

  ui->tmplPathLE->setText(ps.tmplPathText);
  ui->authorLE->setText(ps.authText);
  ui->user1TE->setText(ps.userFields[0]);
  ui->user2TE->setText(ps.userFields[1]);
  ui->user3TE->setText(ps.userFields[2]);
  ui->schPathLE->setText(ps.schPathText);

  // btn->click() to invoke slots/update gui
  qobject_cast<QRadioButton *>(ui->authBG->button(ps.authActiveBtn))->click();
  qobject_cast<QRadioButton *>(ui->tmplBG->button(ps.tmplActiveBtn))->click();
  qobject_cast<QRadioButton *>(ui->schBG->button(ps.schActiveBtn))->click();

  int result = QDialog::exec();
  if (result == QDialog::Accepted) applyPrefsSettings();
  return result;
}

void PrefsDlg::applyPrefsSettings() {
  ps.tmplPathText  = ui->tmplPathLE->text();
  ps.authText      = ui->authorLE->text();
  ps.userFields[0] = ui->user1TE->text();
  ps.userFields[1] = ui->user2TE->text();
  ps.userFields[2] = ui->user3TE->text();
  ps.authActiveBtn = ui->authBG->checkedId();
  ps.tmplActiveBtn = ui->tmplBG->checkedId();
  ps.schActiveBtn  = ui->schBG->checkedId();
  ps.schPathText   = ui->schPathLE->text();
}

void PrefsDlg::startTmplRbChgdSlot() {
  bool b = ui->tmplFileRB->isChecked();
  ui->tmplPathBtn->setEnabled(b);
  ui->tmplPathLE->setEnabled(b);
}

void PrefsDlg::authorRbChgdSlot() {
  ui->authorLE->setEnabled(ui->authorCustomRB->isChecked());
}

void PrefsDlg::selectTmplSlot() {
  QString fpath = ui->tmplPathLE->text();

  MainWindow *mainWin = qobject_cast<MainWindow *>(parent());
  Q_ASSERT(mainWin); // should not fail

  bool res = mainWin->selectFileOpen(fpath, "Template",
      AboutDlg::appName + " Template Files (*." + mainWin->tmplFileSuffix +
          ");;All Files (*.*)");
  if (!res) return;

  ui->tmplPathLE->setText(fpath);
}

void PrefsDlg::startSchRbChgdSlot() {
  bool b = ui->schFolderRB->isChecked();
  ui->schPathBtn->setEnabled(b);
  ui->schPathLE->setEnabled(b);
}

void PrefsDlg::selectSchSlot() {
  MainWindow *mainWin = qobject_cast<MainWindow *>(parent());
  Q_ASSERT(mainWin); // should not fail

  QString fpath = ui->schPathLE->text();

  bool res = mainWin->selectFileOpen(fpath, "Schematic Folder",
      AboutDlg::appName + "QSpice Schematic Files (*.qsch);;All Files (*.*)",
      true);
  if (!res) return;

  ui->schPathLE->setText(fpath);
}

// note:  for use only when first starting application
QString PrefsSettings::getSchPath() const {
  if (schActiveBtn == ST_Folder) return schPathText;
  return schLastPath;
}

// note:  for use only when first starting application
QString PrefsSettings::getTmplPath() const {
  switch (tmplActiveBtn) {
  case TT_LastUsed: return tmplLastPath;
  case TT_File: return tmplPathText;
  case TT_Internal:
  default:
    // todo -- assert?  return empty string?
    return "";
  }
}

bool PrefsSettings::useInternalTmpl() const {
  return tmplActiveBtn == TT_Internal;
}

QString PrefsSettings::getAuthor() const {
  if (authActiveBtn == AT_User) return qgetenv("username");
  return authText;
}

QString PrefsSettings::getUserField(int index) const {
  return userFields[index];
}
