#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "parser.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  bool selectFileOpen(QString &filePath, QString caption, QString filter,
      bool folder = false);

  const QString tmplFileSuffix = QString("qcgt"); // change this once decided

protected:
  void closeEvent(QCloseEvent *);

  bool loadFile(QString filePath, StrList &strList);

  void refreshGui();

  StrList loadStringResource(const QString &resPath);

  Parser parser;

  QString schFileName = QString("./test_schematics/*");
  QString tmplFileName =
      QString("./templates/qspice_default." + tmplFileSuffix);
  // QString tmplFileName =
  //     QString(":/templates/templates/qspice_default.") + tmplFileSuffix;

  QString defaultCodeTemplate =
      QString(":/templates/resources/qspice_default.") + tmplFileSuffix;

  QString helpUserDocName = QString("QCodeGen_User_Doc.pdf");

private slots:
  void closeSlot();
  void openSchematicSlot();
  void loadTemplateSlot();
  void reloadSlot(bool inInit = false);
  void saveSlot();
  void copyToClipbrdSlot();
  void helpAboutSlot();
  void helpUsingSlot();
  void prefsSlot();

  void tabChangedSlot();

private:
  Ui::MainWindow *ui;
  void connectGui();
};
#endif // MAINWINDOW_H
