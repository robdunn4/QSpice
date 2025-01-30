//------------------------------------------------------------------------------
// This file is part of the QMcServer project, a TCP/Sockets-based client/
// server framework for micro-controller QSpice C-Block components.  See the
// GitHub repository at https://github.com/robdunn4/QSpice/ for the complete
// project, documentation, and demontration code.
//
// This is Proof-of-Concept code and subject to significant changes.
//------------------------------------------------------------------------------
#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include "simbase.h"
#include <QDir>
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

  // forward simulator process messages
  void simMsg(QString msg);

signals:
  void userMsg(qintptr socketID, QChar c, QString msg);
  void alertMsg(QString msg);

protected:
  qintptr  socketID;       // this instance connection ID
  SimBase* pSim = nullptr; // simulator
  QDir     cwd;            // CWD for launch/run commands

  void sendResponse(char cResponse, QString msg);
  void sendResponse(char cResponse, QByteArray& data);
  bool simRunningCheck(QByteArray& data);

  void endSim(); // delete pSim, etc.

  // general commands
  void doNotImplemented(QByteArray& data);
  bool doSetCwd(QByteArray& data);  // (c) set CWD (must do before (l))
  void doVersion(QByteArray& data); // (v) get server version info
  void doAlert(QByteArray& data);   // (a) raise alert
  void doEcho(QByteArray& data);    // (e) echo (for testing)
  void doMsg(QByteArray& data);     // (m) write msg to log window

  // simulator commands
  void doStartSim(QByteArray& data); // (l) load simulator
  void doStopSim(QByteArray& data);  // (q) unload simulator
  void doStepInst(QByteArray& data); // (s) step one instr cycle
  void doGetPin(QByteArray& data);   // (r) get pin info (print pin)
  void doSetPin(QByteArray& data);   // (w) set pin voltage (write pin)
  // void doPassThru(QByteArray& data); // (p) pass raw string to simulator
};

#endif // TCPSOCKET_H
