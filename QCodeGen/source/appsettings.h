#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "mainwindow.h"
#include "prefsdlg.h"
#include <QSettings>

class AppSettings : public QSettings {
public:
  AppSettings(QObject *parent = nullptr);

  void saveSettings(MainWindow *mainWin);
  void loadSettings(MainWindow *mainWin);
  void saveSettings(PrefsSettings &ps);
  void loadSettings(PrefsSettings &ps);

  QString getIniFilePath() const;

protected:
  bool saveWindowPos = true;
};

extern AppSettings *appSettings;

#endif // APPSETTINGS_H
