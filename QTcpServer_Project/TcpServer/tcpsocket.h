//------------------------------------------------------------------------------
// This file is part of the QTcpServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, documentation,
// and demontration code.
//------------------------------------------------------------------------------
#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <QDir>
#include <QProcess>
#include <QTcpSocket>
#include <QtDebug>

class TcpSocket : public QTcpSocket
{
  Q_OBJECT

public:
  TcpSocket(qintptr socketID, QDir initialCwd, QObject* parent = nullptr);
  ~TcpSocket();

public slots:
  void run();
  void handleReadyRead();
  void handleDisconnected();

  // capture stdout & stderr
  void handleStdout();
  void handleStderr();
  void handleStdDev(const QByteArray& data, const QString& stddev);

signals:
  void userMsg(qintptr socketID, QChar c, QString msg);
  void alertMsg(QString msg);

protected:
  qintptr  socketID; // this instance connection ID
  QProcess process;  // process for launch/run commands
  QDir     cwd;      // CWD for launch/run commands

  void sendResponse(char cResponse, QString msg);

  void doAlert(const QByteArray& data);
  void doEcho(const QByteArray& data);
  void doMsg(const QByteArray& data);
  void doRunLaunch(const QByteArray& data, bool bLaunch);
  void doLaunch();
  void doRun();
  bool doSetCwd(const QByteArray& data);
  void doVersion(QByteArray& data);
};

#endif // TCPSOCKET_H
