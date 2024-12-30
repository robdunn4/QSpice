//------------------------------------------------------------------------------
// This file is part of the QTcpServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, documentation,
// and demontration code.
//------------------------------------------------------------------------------
#include "mainwindow.h"

#include "tcpserver.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QMessageBox>
#include <QtDebug>

/*
 * lazy global stuff
 */
TcpServer*  server   = 0;
MainWindow* pMainWin = 0;
QString     appName("QTcpServer");
const int   verMajor = 0;
const int   verMinor = 5;
const int   verDot   = 1;
QString appVersion(QString::number(verMajor) + "." + QString::number(verMinor) +
                   "." + QString::number(verDot));

/*
 * MainWindow implementation
 */

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  // setWindowTitle(appName + " v" + appVersion);
  QString sTitle = appName + " v" + appVersion;

#ifdef QT_DEBUG
  sTitle += " (Debug Build)";
#else
  sTitle += " (Release Build)";
#endif
  setWindowTitle(sTitle);

  connect(ui->closeBtn, &QPushButton::clicked, this, &MainWindow::close);
  connect(ui->clearBtn, &QPushButton::clicked, this, &MainWindow::btnClear);

  server = new TcpServer(this);
  connect(server, &TcpServer::userMsg, this, &MainWindow::addUserMsg);
  connect(server, &TcpServer::alertMsg, this, &MainWindow::addUserMsg);
  server->StartServer();

  ui->clearBtn->setFocus();

  // initialize alert dialog and connect beep timer to application beep function
  pAlertDlg =
      new QMessageBox(QMessageBox::Icon::Information, appName + " Alert",
          "One or more clients raised an alert.\nSee log window for details.",
          QMessageBox::StandardButton::Ok, this,
          Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
  connect(&beepTimer, &QTimer::timeout, this, &MainWindow::playAlertSound);
}

MainWindow::~MainWindow()
{
  delete pAlertDlg;
  delete server;
  delete ui;
}

void MainWindow::notImplemented()
{
  QString s("Not yet implemented.\nCheck back later.");
  QMessageBox::information(this, appName, s, QMessageBox::Ok);
}

bool MainWindow::isMuted() { return ui->muteCB->isChecked(); }

void MainWindow::closeEvent(QCloseEvent* event)
{
  // if any open connections, prompt before closing
  bool bClose = true;
  if (activeConnections)
  {
    QString s = QString("There may be open connections.\nClose server anyway?");
    bClose =
        (QMessageBox::question(this, appName, s,
             QMessageBox::Close | QMessageBox::Cancel) == QMessageBox::Close);
  }
  if (!bClose) event->ignore();
}

void MainWindow::btnClear()
{
  qInfo() << "Clear button was clicked...";
  ui->textEdit->clear();
}

void MainWindow::addUserMsg(qintptr socketID, QChar c, QString s)
{
  QDateTime dt        = QDateTime::currentDateTime();
  QString   sDt       = dt.toString("yyyy.MM.dd HH:mm:ss.zzz");
  QString   sSocketID = QString::number(socketID);

  // if socketID is zero, this is a server msg
  if (!socketID) sSocketID = "";

  QString msg = QString("%1 : %2 : %3 : %4")
                    .arg(sDt)
                    .arg(sSocketID, 5, ' ')
                    .arg(c)
                    .arg(s);
  ui->textEdit->appendPlainText(msg);
}

void MainWindow::clientConnected()
{
  activeConnections++;
  clientCntChanged();
}

void MainWindow::clientDisconnected()
{
  activeConnections--;
  clientCntChanged();
}

void MainWindow::clientCntChanged()
{
  ui->connectionsLbl->setText(QString::number(activeConnections));
}

void MainWindow::showAlert()
{
  // if alert dialog already active, nothing to do
  if (pAlertDlg->isVisible()) return;

  const int beepIntervalMsecs = 5000; // beep interval is hardcoded here...

  if (!ui->muteCB->isChecked()) beepTimer.start(beepIntervalMsecs);
  pAlertDlg->exec();
  beepTimer.stop();
}

void MainWindow::playAlertSound()
{
  // if standard QT beep isn't good enough, here's where to change it...
  QApplication::beep();
}
