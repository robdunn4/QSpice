#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "parser.h"
#include "tabbar.h"
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

protected:
  void closeEvent(QCloseEvent *);

  bool loadFile(QString filePath, StrList &strList);
  bool selectFileOpen(QString &filePath, QString caption, QString filter);

  void refreshGui();

  void reload();
  void saveFile();
  void copyToClipbrd();

  Parser parser;

private slots:
  void closeSlot();
  void openSchematicSlot();
  void loadTemplateSlot();
  void reloadSlot();
  void saveSlot();
  void copyToClipbrdSlot();
  void helpAboutSlot();
  void helpUsingSlot();

  void tabChangedSlot(int index);

private:
  Ui::MainWindow *ui;
  void connectGui();
};
#endif // MAINWINDOW_H
