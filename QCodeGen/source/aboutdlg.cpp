#include "aboutdlg.h"
#include "ui_aboutdlg.h"

QString AboutDlg::appName("QCodeGen");
QString AboutDlg::appLongName("QSpice Code Generator");
QString AboutDlg::appVersion("v0.1.1");
QString AboutDlg::appNameVersion(AboutDlg::appName + " " +
                                 AboutDlg::appVersion);
QString AboutDlg::appCopyright("Copyright \u00a9 2024 Thin Air Enterprises &\n"
                               "Robert Dunn.  All rights reserved.");
QString AboutDlg::appLicense("GNU GPL-3.0");
QString AboutDlg::companyName("Thin Air Enterprises");

AboutDlg::AboutDlg(QWidget *parent) : QDialog(parent), ui(new Ui::AboutDlg) {
  ui->setupUi(this);

  setWindowTitle("About " + AboutDlg::appName);
  setModal(true);

  ui->appNameLbl->setText(appName + " " + appVersion);
  ui->copyrightLbl->setText(appCopyright);
  ui->licenseLbl->setText(
      appName +
      " is copyrighted software.  It is licensed for your "
      "use under the <a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">" +
      appLicense + " license</a>.");

  // todo - add link to repo
}

AboutDlg::~AboutDlg() { delete ui; }
