//------------------------------------------------------------------------------
// This file is part of the QMcServer project, a TCP/Sockets-based client/
// server framework for micro-controller QSpice C-Block components.  See the
// GitHub repository at https://github.com/robdunn4/QSpice/ for the complete
// project, documentation, and demontration code.
//
// This is Proof-of-Concept code and subject to significant changes.
//------------------------------------------------------------------------------
#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QDir>
#include <QTcpServer>
#include <QtDebug>

class TcpServer : public QTcpServer
{
  Q_OBJECT

public:
  explicit TcpServer(QObject* parent);
  ~TcpServer();

  void StartServer();

signals:
  void userMsg(qintptr socketID, QChar c, QString s);
  void alertMsg(qintptr socketID, QChar c, QString s);

public slots:

protected:
  void incomingConnection(qintptr socketID);
};

#endif // TCPSERVER_H
