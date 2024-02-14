#ifndef ABOUTDLG_H
#define ABOUTDLG_H

#include <QDialog>

namespace Ui {
class AboutDlg;
}

class AboutDlg : public QDialog {
  Q_OBJECT

public:
  explicit AboutDlg(QWidget *parent = nullptr);
  ~AboutDlg();

  static QString appName;
  static QString appLongName;
  static QString appVersion;
  static QString appCopyright;
  static QString appLicense;
  static QString companyName;
  static QString appNameVersion;

protected:
  Ui::AboutDlg *ui;

  // static const int appVerMajor;
  // static const int appVerMinor;
  // static const int appVerRel;
};

#endif // ABOUTDLG_H
