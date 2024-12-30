//------------------------------------------------------------------------------
// This file is part of the QTcpServer project, a TCP/Sockets-based client/
// server framework for QSpice C-Block components.  See the GitHub repository at
// https://github.com/robdunn4/QSpice/ for the complete project, documentation,
// and demontration code.
//------------------------------------------------------------------------------
#include "tcpsocket.h"

#include "mainwindow.h"
#include <QDir>
#include <QRegularExpression>
#include <QThread>

TcpSocket::TcpSocket(qintptr socketID, QDir initialCwd, QObject* parent) :
    QTcpSocket(parent), socketID(socketID), process(this), cwd(initialCwd)
{
  qInfo() << "Constructing TcpSocket" << socketID << QThread::currentThread();

  connect(&process, &QProcess::readyReadStandardOutput, this,
      &TcpSocket::handleStdout);
  connect(&process, &QProcess::readyReadStandardError, this,
      &TcpSocket::handleStderr);

  connect(this, &QTcpSocket::readyRead, this, &TcpSocket::handleReadyRead);
  connect(
      this, &QTcpSocket::disconnected, this, &TcpSocket::handleDisconnected);
}

TcpSocket::~TcpSocket()
{
  qInfo() << "Destroying TcpSocket" << socketID << QThread::currentThread();
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

    case 'e': // echo
      doEcho(data);
      break;

    case 'l': // launch process
      doRunLaunch(data, true);
      break;

    case 'm': // add message to output window (couldn't we just use echo?)
      doMsg(data);
      break;

    case 'r': // run process
      doRunLaunch(data, false);
      break;

    case 'v': // return server version string
      doVersion(data);
      break;

    case 'w': // set CWD for run/launch
      doSetCwd(data);
      break;

    case '?': // intended for TTY testing to query state information (not
              // implemented yet)
      break;

    default: // invalid request
      qInfo() << "Invalid operation" << socketID << QThread::currentThread()
              << data;
      emit userMsg(socketID, '?', "Invalid request");
      sendResponse('!', "Invalid request");
      break;
  }

  // nothing more to do...
}

void TcpSocket::handleDisconnected()
{
  qInfo() << "TcpSocket::handleDisconnected()" << socketID
          << QThread::currentThread();
  emit userMsg(socketID, '*', "Client disconnected");
  pMainWin->clientDisconnected();
  this->deleteLater(); // ???
}

void TcpSocket::handleStdout()
{
  QByteArray data = process.readAllStandardOutput();
  handleStdDev(data, "stdout");
}

void TcpSocket::handleStderr()
{
  QByteArray data = process.readAllStandardError();
  handleStdDev(data, "stderr");
}

// handler to put stdout/stderr text in output window
void TcpSocket::handleStdDev(const QByteArray& data, const QString& stddev)
{
  // split into lines
  static QRegularExpression re("[\r\n]");
  QString                   s(data);
  QStringList               sList = s.split(re, Qt::SkipEmptyParts);
  QString                   sDev  = "[" + stddev + "] ";

  for (QString& s : sList) { emit userMsg(socketID, ' ', sDev + s); }
}

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

void TcpSocket::doAlert(const QByteArray& data)
{
  // don't allow empty alert messages...
  QString msg(data);
  msg = msg.mid(2).trimmed();
  if (msg.length() == 0) msg = "Client raised alert";

  emit userMsg(socketID, 'a', msg);
  sendResponse('A', "");
  emit alertMsg(msg);
}

void TcpSocket::doEcho(const QByteArray& data)
{
  QString s = QString("%1").arg(data).mid(1).trimmed();

  emit userMsg(socketID, 'e', QString("Echoing: \"") + s + "\"");
  qInfo() << "doEcho(arg)" << socketID << "Arg:" << data << "Thread"
          << QThread::currentThread();

  QByteArray d(data);
  d[0] = 'E';
  write(d);
  flush();
}

void TcpSocket::doMsg(const QByteArray& data)
{
  // don't allow empty messages...
  QString msg(data);
  msg = msg.mid(2).trimmed();
  if (data.length() < 4 || data[1] != ' ' || !msg.length())
  {
    emit userMsg(socketID, data[0], "Malformed message request");
    sendResponse('!', "Malformed message request");
    return;
  }

  emit userMsg(socketID, data[0], ">>> " + msg);
  sendResponse('M', "");
}

/*
 * doRunLaunch() -- common setup for launch and run commands; saves/restores CWD
 */
void TcpSocket::doRunLaunch(const QByteArray& data, bool bLaunch)
{
  // strip trailing whitespace
  QString sData = QString(data).trimmed();

  // check if properly formed
  if (sData.length() < 3 || sData[1] != ' ')
  {
    emit userMsg(socketID, data[0], "Malformed launch or run request");
    sendResponse('!', "Malformed launch or run request");
    return;
  }

  // split command line into tokens (program name & argument list)
  sData                = sData.mid(2);
  QStringList sArgList = QProcess::splitCommand(QStringView(sData));

  // separate program from args and delete it from argument list
  QString sProgram(sArgList[0]);
  sArgList.removeFirst();

  QString sCmd = bLaunch ? "Launching: " : "Running: ";
  qInfo() << sCmd << sProgram << sArgList;
  emit userMsg(socketID, data[0], sCmd + "\"" + sData + "\"");

  // common QProcess set-up for launch or run
  process.setWorkingDirectory(cwd.path());
  process.setProgram(sProgram);
  process.setArguments(sArgList);

  /* there's a problem with the above: the working directory used to "start"
   * process.setProgram() is this server process's CWD, NOT the working
   * directory specified in the process.setWorkingDirecctory().  need to set
   * both the process's working directory and this server's process to that
   * folder temporarily to get the desired behavior...
   */

  // save current server CWD and temporarily switch to client CWD
  QDir saveDir = QDir::current();
  QDir::setCurrent(cwd.path());

  // finally, launch or run
  if (bLaunch)
    doLaunch();
  else
    doRun();

  // restore server CWD
  QDir::setCurrent(saveDir.path());
}

void TcpSocket::doLaunch()
{
  QString     path = process.workingDirectory();
  QString     pgm  = process.program();
  QStringList args = process.arguments();

  qint64 pid;
  if (process.startDetached(&pid))
  // if (process.startDetached(pgm, args, path + "/", &pid))
  {
    emit userMsg(socketID, 'l', "Launch succeeded PID=" + QString::number(pid));
    sendResponse('L', QString::number(pid));
    return;
  }

  // failed
  emit userMsg(socketID, 'l', "Launch failed");
  sendResponse('!', "Launch failed");
}

void TcpSocket::doRun()
{
  process.start();
  bool result = process.waitForStarted(10000); // -1 waits forever
  if (!result)
  {
    qInfo() << socketID << "process.waitForStarted()" << result
            << process.exitStatus() << process.errorString() << process.error();

    emit userMsg(socketID, 'r', "Run failed");
    sendResponse('!', process.errorString());

    process.close(); // ???
    return;
  }

  const int timeout = -1; // timeout in msecs; -1 == forever
  result            = process.waitForFinished(timeout);

  qInfo() << socketID << "process.waitForFinished()" << result
          << process.exitStatus() << process.errorString() << process.error();

  if (!result)
  {
    qInfo() << socketID << "process.waitForFinished()" << result
            << process.exitStatus() << process.errorString() << process.error();

    emit userMsg(socketID, 'r', "Run failed");
    sendResponse('!', process.errorString());

    process.close(); // ???
    return;
  }

  if (process.exitStatus() == QProcess::NormalExit)
  {
    // should be good?
    qInfo() << socketID << process.exitStatus() << process.errorString();

    int     exitCode  = process.exitCode();
    QString sExitCode = QString::number(exitCode);

    emit userMsg(socketID, 'r', "Run returned: " + sExitCode);
    sendResponse('R', sExitCode);
    return;
  }

  // there was an error???
  qInfo() << socketID << process.exitStatus() << process.errorString();
  emit userMsg(socketID, 'r', "Run failed");
  sendResponse('!', process.errorString());
}

// doSetCwd() attempts to return failure if resulting directory does not
// exist...
bool TcpSocket::doSetCwd(const QByteArray& data)
{
  QString sCwd = QString(data).trimmed();
  if (sCwd.length() == 1)
  {
    sCwd = cwd.path();
    emit userMsg(socketID, 'w', "CWD=" + sCwd);
    sendResponse('W', sCwd);
    return true;
  }

  if (data[1] != ' ')
  {
    emit userMsg(socketID, 'w', "Malformed set CWD request");
    sendResponse('!', "Malformed set CWD request");
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
    emit userMsg(socketID, 'w', QString("CWD Set=") + cwd.path());
    sendResponse('W', cwd.path());
    return true;
  }

  QString msg = "Invalid directory/folder \"" + newCwd.path() + "\"";
  emit    userMsg(socketID, 'w', msg);
  sendResponse('!', "Invalid directory/folder");
  return false;
}

void TcpSocket::doVersion([[maybe_unused]] QByteArray& data)
{
  emit userMsg(socketID, 'v', QString("Version: \"") + appVersion + "\"");
  sendResponse('V', appVersion);
}
