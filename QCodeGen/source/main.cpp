#include "mainwindow.h"

#include "aboutdlg.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  a.setApplicationName("Application Name");
  a.setOrganizationName(AboutDlg::companyName);

  MainWindow w;
  w.show();
  return a.exec();
}
