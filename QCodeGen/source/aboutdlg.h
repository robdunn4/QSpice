#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include "mainwindow.h"
#include <QDialog>

namespace Ui {
class AboutDlg;
}

class AboutDlg : public QDialog {
  Q_OBJECT

public:
  explicit AboutDlg(MainWindow *parent = nullptr);
  ~AboutDlg();

  static QString appName;
  static QString appLongName;
  static QString appVersion;
  static QString appCopyright;
  static QString appLicense;
  static QString companyName;
  static QString appNameVersion;

  void setIniFilePath(QString path);

protected:
  Ui::AboutDlg *ui;
};

#endif // ABOUTDLG_H
