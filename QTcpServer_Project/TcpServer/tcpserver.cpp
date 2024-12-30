//------------------------------------------------------------------------------
// This file is part of the QTcpServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, documentation,
// and demontration code.
//------------------------------------------------------------------------------
#include "tcpserver.h"

#include "mainwindow.h"
#include "tcpsocket.h"
#include <QThread>

TcpServer::TcpServer(QObject* parent) : QTcpServer(parent)
{
  qInfo() << "Creating TcpServer";
}

TcpServer::~TcpServer() { qInfo() << "Destroying TcpServer"; }

void TcpServer::StartServer()
{
  QString s = QString("Starting server on %1:%2")
                  .arg(::hostAddress.toString())
                  .arg(::portNbr);
  emit userMsg(0, '*', s);
  qInfo() << s << QThread::currentThread();

  if (!listen(::hostAddress, ::portNbr))
  {
    s = QString("Unable to start server on %1:%2")
            .arg(::hostAddress.toString())
            .arg(::portNbr);
    emit userMsg(0, '*', s);
    qInfo() << s << QThread::currentThread();
    return;
  }

  s = QString("Server started successfully")
          .arg(::hostAddress.toString())
          .arg(::portNbr);
  emit userMsg(0, '*', s);
  qInfo() << s << QThread::currentThread();
}

void TcpServer::incomingConnection(qintptr socketID)
{
  qInfo() << socketID << "TcpServer::incomingConnection(" << "socketID" << ") "
          << QThread::currentThread();

  QThread* thread = new QThread;

  TcpSocket* socket = new TcpSocket(socketID, QDir::current()); // no parent!
  socket->moveToThread(thread);

  connect(thread, &QThread::started, socket, &TcpSocket::run);
  connect(socket, &TcpSocket::userMsg, pMainWin, &MainWindow::addUserMsg);
  connect(socket, &TcpSocket::alertMsg, pMainWin, &MainWindow::showAlert);

  thread->start();
  pMainWin->clientConnected();
}
