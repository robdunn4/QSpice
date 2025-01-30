//------------------------------------------------------------------------------
// This file is part of the QMcServer project, a TCP/Sockets-based client/
// server framework for micro-controller QSpice C-Block components.  See the
// GitHub repository at https://github.com/robdunn4/QSpice/ for the complete
// project, documentation, and demontration code.
//
// This is Proof-of-Concept code and subject to significant changes.
//------------------------------------------------------------------------------
#include "tcpsocket.h"

#include "mainwindow.h"
#include "mdbsim.h"
#include <QByteArray>
#include <QDir>
#include <QRegularExpression>
#include <QThread>

TcpSocket::TcpSocket(qintptr socketID, QDir initialCwd, QObject* parent) :
    QTcpSocket(parent), socketID(socketID), cwd(initialCwd)
{
  qInfo() << "Constructing TcpSocket" << socketID << QThread::currentThread();

  connect(this, &QTcpSocket::readyRead, this, &TcpSocket::handleReadyRead);
  connect(
      this, &QTcpSocket::disconnected, this, &TcpSocket::handleDisconnected);
}

TcpSocket::~TcpSocket()
{
  qInfo() << "Destroying TcpSocket" << socketID << QThread::currentThread();
  if (pSim) delete pSim;
}

void TcpSocket::run()
{
  QString s;
  qInfo() << "TcpSocket::run()" << socketID << QThread::currentThread();

  if (!setSocketDescriptor(socketID))
  {
    s = QString("Unexpected TCP socket error: %1").arg(socketID);
    qInfo() << socketID << s << QThread::currentThread();
    emit userMsg(socketID, 'r', s);
    return;
  }

  s = QString("Client connected");
  emit userMsg(socketID, '*', s);
  qInfo() << socketID << s << QThread::currentThread();
}

// main incoming message handler
void TcpSocket::handleReadyRead()
{
  QByteArray data  = readAll();
  QString    sData = QString(data).trimmed();

  // for now, just ignore empty requests
  if (sData.length() < 1) return;

  // dispatch requested operation
  switch (data[0])
  {
    case 'a': // audible alert
      doAlert(data);
      break;

    case 'c': // set CWD (must do before (l))
      doSetCwd(data);
      break;

    case 'e': // echo (for testing)
      doEcho(data);
      break;

    case 'l': // load simulator, device, program, etc.
      doStartSim(data);
      break;

    case 'm': // add message to output window (couldn't we just use echo?)
      doMsg(data);
      break;

    case 'q': // unload simulator gracefully
      doStopSim(data);
      break;

    case 'r': // get pin info
      doGetPin(data);
      break;

    case 's': // step one instr cycle
      doStepInst(data);
      break;

    case 'v': // return server version string
      doVersion(data);
      break;

    case 'w': // set pin voltage (write pin)
      doSetPin(data);
      break;

    case '?': // intended for TTY testing to query state information
      doNotImplemented(data);
      break;

    default: // invalid request
      doNotImplemented(data);
      break;
  }

  // nothing more to do...
}

void TcpSocket::handleDisconnected()
{
  qInfo() << "TcpSocket::handleDisconnected()" << socketID
          << QThread::currentThread();
  // doStopSim(data);
  endSim();
  emit userMsg(socketID, '*', "Client disconnected");
  pMainWin->clientDisconnected();
  this->deleteLater(); // ???
}

void TcpSocket::simMsg(QString msg) { emit userMsg(socketID, '*', msg); }

// send a TCP response to client
void TcpSocket::sendResponse(char cResponse, const QString msg)
{
  QByteArray data;
  data.append(cResponse);
  data.append(' ');
  data.append(msg.trimmed().toUtf8());
  data.append("\r\n"); // not needed by QSpice DLL clients but useful for TTY
  write(data);
  flush();
}

// send a TCP response to client
void TcpSocket::sendResponse(char cResponse, QByteArray& data)
{
  data[0] = cResponse;
  write(data);
  flush();
}

bool TcpSocket::simRunningCheck(QByteArray& data)
{
  if (!pSim)
  {
    emit userMsg(
        socketID, data[0], "*** Command error: Simulator not running.");
    sendResponse('!', data);
    return false;
  }
  return true;
}

void TcpSocket::endSim()
{
  if (!pSim) return;
  pSim->stopSim();
  delete pSim;
  pSim = nullptr;
}

void TcpSocket::doNotImplemented(QByteArray& data)
{
  char    c = data[0];
  QString s = data;
  s         = s.trimmed();

  emit userMsg(socketID, c, "Command not implemented (\"" + s + "\")");
  sendResponse('!', data);
}

void TcpSocket::doAlert(QByteArray& data)
{
  // don't allow empty alert messages...
  QString msg(data);
  msg = msg.mid(2).trimmed();
  if (msg.length() == 0) msg = "Client raised alert";

  emit userMsg(socketID, 'a', msg);
  sendResponse('A', data);
  emit alertMsg(msg);
}

void TcpSocket::doEcho(QByteArray& data)
{
  QString s = QString("%1").arg(data).mid(1).trimmed();

  emit userMsg(socketID, 'e', QString("Echoing: \"") + s + "\"");
  qInfo() << "doEcho(arg)" << socketID << "Arg:" << data << "Thread"
          << QThread::currentThread();

  sendResponse('E', data);
}

void TcpSocket::doMsg(QByteArray& data)
{
  // don't allow empty messages...
  QString msg(data);
  msg = msg.mid(2).trimmed();
  if (data.length() < 4 || data[1] != ' ' || !msg.length())
  {
    emit userMsg(socketID, data[0], "Malformed message request");
    sendResponse('!', data);
    return;
  }

  emit userMsg(socketID, data[0], ">>> " + msg);
  sendResponse('M', data);
}

void TcpSocket::doStartSim(QByteArray& data)
{
  // first check for already running....
  if (pSim)
  {
    emit userMsg(socketID, 'l',
        "*** Error: Attempt to start simulator when already running.");
    sendResponse('!', data);
    return;
  }

  // clean up command string & split into tokens
  QString     sData     = QString(data).trimmed();
  QStringList sArgsList = QProcess::splitCommand(QStringView(sData));

  // if not four tokens, it's malformed
  if (sArgsList.size() != 4)
  {
    emit userMsg(socketID, 'l', "Malformed simulator start request.");
    sendResponse('!', data);
    return;
  }

  // parse device and hex/elf path
  QString device  = sArgsList[2];
  QString program = sArgsList[3];

  // for now, if program is "*", use hard-coded program for TTY debugging
  // convenience
  if (program == "*")
    program = "C:\\Dev\\QSpice\\Microprocessors\\PIC16F15213."
              "X\\dist\\default\\debug\\PIC16F15213.X.debug.elf";

  // create instance for requested simulator
  QString simArg = sArgsList[1];
  if (!pSim && !simArg.compare("MDB", Qt::CaseInsensitive))
  {
    pSim = new MdbSim(device, program);
  }
  /*
   * add other simulators here like this after adding XXXSim class
   *
   * if (!pSim && !simArg.compare("XXX", Qt::CaseInsensitive))
   * {
   *   pSim = new XXXSim(device, program);
   * }
   */

  // requested simulator not found
  if (!pSim)
  {
    emit userMsg(socketID, 'l', "Unsupported simulator (" + simArg + ").");
    sendResponse('!', data);
    return;
  }

  // connect signals/slots
  connect(pSim, &SimBase::userMsg, this, &TcpSocket::simMsg);

  // save current server CWD and temporarily switch to client CWD
  QDir saveDir = QDir::current();
  QDir::setCurrent(cwd.path());

  // finally, run simulator
  if (!pSim->startSim())
  {
    // handle error
    emit userMsg(socketID, 'l', "*** Error: Unable to start simulation.");
    sendResponse('!', data);
    delete pSim;
    pSim = nullptr;
    return;
  }

  // restore server CWD
  QDir::setCurrent(saveDir.path());

  emit userMsg(socketID, 'L',
      "Simulator successfully loaded and configured for device/program.");
  sendResponse('L', data);
  return;
}

void TcpSocket::doStopSim(QByteArray& data)
{
  if (!pSim)
  {
    emit userMsg(socketID, 'q',
        "*** Error: Attempt to stop simulator when not running.");
    sendResponse('!', data);
    return;
  }

  endSim();

  emit userMsg(socketID, 'q', "Simulator stopped.");
  sendResponse('Q', data);
}

void TcpSocket::doStepInst(QByteArray& data)
{
  if (!simRunningCheck(data)) return;

  QString str;

  // fix timeStamp stuff later or remove entirely
  if (!pSim->stepInst(str, 0.0))
  {
    emit userMsg(socketID, 's', "*** Step instruction failed.");
    sendResponse('!', data);
    return;
  }

  sendResponse('S', str);
}

void TcpSocket::doGetPin(QByteArray& data)
{
  if (!simRunningCheck(data)) return;

  if (data.length() < 3)
  {
    emit userMsg(socketID, 'r', "*** Invalid pin name.");
    sendResponse('!', "Invalid pin name or malformed request.");
    return;
  }

  QString pinName;
  bool    isInput;
  bool    isHigh;
  bool    isDigital;
  QString info;

  pinName = data.mid(1).trimmed();

  if (!pSim->getPin(pinName, isInput, isHigh, isDigital, info))
  {
    emit userMsg(socketID, 'r', "Unexpected error getting pin information.");
    sendResponse('!', data);
    return;
  }

  QString str;
  str += isDigital ? "D" : "A";
  str += isInput ? "I" : "O";
  str += isHigh ? "H" : "L";
  str += " " + info;

  sendResponse('R', str);
}

void TcpSocket::doSetPin([[maybe_unused]] QByteArray& data)
{
  if (!simRunningCheck(data)) return;

  QString     str   = data;
  QStringList sList = str.split(' ');

  if (sList.length() != 3)
  {
    emit userMsg(socketID, 'w', "*** Invalid pin set instruction.");
    sendResponse('!', "Invalid pin name or malformed request.");
    return;
  }

  // QString cmd = data.mid(1).trimmed();

  if (!pSim->setPin(sList[1].trimmed(), sList[2].trimmed()))
  {
    emit userMsg(socketID, 'w', "Unexpected error setting pin state.");
    sendResponse('!', data);
    return;
  }

  sendResponse('W', data);
}

// doSetCwd() attempts to return failure if resulting directory does not
// exist...
bool TcpSocket::doSetCwd(QByteArray& data)
{
  QString sCwd = QString(data).trimmed();
  if (sCwd.length() == 1)
  {
    sCwd = cwd.path();
    emit userMsg(socketID, 'c', "CWD=" + sCwd);
    sendResponse('C', sCwd);
    return true;
  }

  if (data[1] != ' ')
  {
    emit userMsg(socketID, 'c', "Malformed set CWD request");
    sendResponse('!', data);
    return false;
  }

  // // try to prevent changing CWD to invalid location
  sCwd        = sCwd.mid(2);
  QDir newCwd = cwd;
  newCwd.setPath(sCwd);
  newCwd.makeAbsolute();

  if (newCwd.exists())
  {
    cwd = newCwd;
    emit userMsg(socketID, 'c', QString("CWD Set=") + cwd.path());
    sendResponse('C', data);
    return true;
  }

  QString msg = "Invalid directory/folder \"" + newCwd.path() + "\"";
  emit    userMsg(socketID, 'c', msg);
  sendResponse('!', data);
  return false;
}

void TcpSocket::doVersion([[maybe_unused]] QByteArray& data)
{
  emit userMsg(socketID, 'v', QString("Version: \"") + appVersion + "\"");
  sendResponse('V', appVersion);
}
