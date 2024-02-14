#include "appsettings.h"

#include "aboutdlg.h"

AppSettings *appSettings;

AppSettings::AppSettings(QObject *parent)
    : QSettings(QSettings::IniFormat, QSettings::UserScope,
                AboutDlg::companyName, AboutDlg::appName, parent) {}

void AppSettings::saveSettings(MainWindow *mainWin) {
  beginGroup("MainWindow");
  setValue("geometry", mainWin->saveGeometry());
  endGroup();
}

void AppSettings::loadSettings(MainWindow *mainWin) {
  beginGroup("MainWindow");
  const auto geometry = value("geometry", QByteArray()).toByteArray();
  if (geometry.isEmpty())
    mainWin->setGeometry(200, 200, 400, 400);
  else
    mainWin->restoreGeometry(geometry);
  endGroup();
}
