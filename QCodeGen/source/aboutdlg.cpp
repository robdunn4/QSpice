#include "aboutdlg.h"
#include "ui_aboutdlg.h"

QString AboutDlg::appName("QCodeGen");
QString AboutDlg::appLongName("QSpice Code Generator");
QString AboutDlg::appVersion("v0.9.3");
QString AboutDlg::appNameVersion(
    AboutDlg::appName + " " + AboutDlg::appVersion);
QString AboutDlg::appCopyright("Copyright \u00a9 2024 Thin Air Enterprises &\n"
                               "Robert Dunn.  All rights reserved.");
QString AboutDlg::appLicense("GNU GPL-3.0");
QString AboutDlg::companyName("Thin Air Enterprises");

AboutDlg::AboutDlg(MainWindow *parent) : QDialog(parent), ui(new Ui::AboutDlg) {
  ui->setupUi(this);

  setWindowTitle("About " + AboutDlg::appName);
  setModal(true);

  ui->appNameLbl->setText(appName + " " + appVersion);
  ui->copyrightLbl->setText(appCopyright);
  ui->licenseLbl->setText(
      "\n" + appName +
      " is copyrighted software.  It is licensed for your "
      "use under the <a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">" +
      appLicense + " license</a>.\n");

  // todo - add link to repo
  ui->repoLbl->setText( //
      "\nThe most current version of the " + appName +
      " souce code and binaries is available on my <a "
      "href=\"https://github.com/robdunn4/QSpice\"> GitHub QSpice "
      "repository</a>.");

  adjustSize();
}

AboutDlg::~AboutDlg() { delete ui; }

void AboutDlg::setIniFilePath(QString path) {
  QString s = "\nPreferences settings are stored in <a href=\"file:" + path +
              "\"> " + path + "<\a>.";
  ui->iniFileLbl->setText(s);
}
