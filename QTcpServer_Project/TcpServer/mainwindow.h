//------------------------------------------------------------------------------
// This file is part of the QTcpServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, documentation,
// and demontration code.
//------------------------------------------------------------------------------
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QDir>
#include <QMainWindow>
#include <QMessageBox>
#include <QTcpServer>
#include <QTimer>
#include <QtDebug>

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

  void notImplemented();
  bool isMuted();

public slots:
  void closeEvent(QCloseEvent* event);

  void btnClear();
  void addUserMsg(qintptr socketID, QChar c, QString s);

  void clientConnected();
  void clientDisconnected();
  void clientCntChanged();

  void showAlert();
  void playAlertSound();

protected:
  Ui::MainWindow* ui;
  int             activeConnections = 0;
  QTimer          beepTimer;
  QMessageBox*    pAlertDlg;
};

/*
 *  lazy global stuff
 */
extern MainWindow*  pMainWin;
extern QHostAddress hostAddress;
extern qint16       portNbr;
extern QString      appName;
extern QString      appVersion;
extern const int    verMajor;
extern const int    verMinor;
extern const int    verDot;

#endif // MAINWINDOW_H
