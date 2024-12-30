//------------------------------------------------------------------------------
// This file is part of the QTcpServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, documentation,
// and demontration code.
//------------------------------------------------------------------------------
#include "mainwindow.h"

#include <QApplication>
#include <QTcpServer>
#include <QThread>

// TODO:  Add command line parsing for host name and port number

QHostAddress hostAddress = QHostAddress::LocalHost;
qint16       portNbr     = 666;

int main(int argc, char* argv[])
{
  // name thread for debugging
  QThread::currentThread()->setObjectName("Main Thread");

  QApplication app(argc, argv);
  QApplication::setApplicationName(appName);
  QApplication::setApplicationVersion(appVersion);

  MainWindow w;
  pMainWin = &w;
  w.show();

  return app.exec();
}
