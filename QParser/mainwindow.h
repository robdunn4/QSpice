#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
private slots:
  void on_actionExit_triggered();

  void on_actionOpen_triggered();

  void on_actionGenerate_triggered();

private:
  Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
