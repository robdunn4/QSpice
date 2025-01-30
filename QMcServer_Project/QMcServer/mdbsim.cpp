//------------------------------------------------------------------------------
// This file is part of the QMcServer project, a TCP/Sockets-based client/
// server framework for micro-controller QSpice C-Block components.  See the
// GitHub repository at https://github.com/robdunn4/QSpice/ for the complete
// project, documentation, and demontration code.
//
// This is Proof-of-Concept code and subject to significant changes.
//------------------------------------------------------------------------------
/*
 * Simulator class for Microchip devices.
 *
 * This is Proof of Concept code and subject to significant revisions as
 * the project proceeds.
 *
 * This code uses the MDB Java command-line interface.  The MDB Java source
 * needs to be revised to return text intended for parsing in code OR otherwise
 * interfaces more directly into the MDB Java API.  Until then, be aware that
 * the parsing code is fragile.
 */

#include "mdbsim.h"
#include "mainwindow.h"

#include <QDir>
#include <QRegularExpression>

MdbSim::MdbSim(const QString simDevice, const QString simProgram,
    QObject* parent) : SimBase(simDevice, simProgram, parent)
{
  setProgram(::mdbSimPath);
}

MdbSim::~MdbSim()
{
  // do stuff
}

/*
 * load/start simulator, configure for device, set simulator type to software
 * simulation (SIM), and load devce program binary (e.g., *.hex, *.elf).
 */
bool MdbSim::startSim()
{
  [[maybe_unused]] QProcess::ProcessError err;

  emit userMsg("*** Starting simulation processor (MDB)...");

  start();
  err = error();

  if (!waitForStarted(10000))
  {
    err = error();
    emit userMsg(
        "*** Unable to start MDB.  Check path and Java configuration.");
    return false;
  }

  if (!readUntilPrompt(10000))
  {
    emit userMsg("*** Unexpected error waiting for MDB response.");
    return false;
  }
  // we expect "initializing for dumb terminal" which is not an error
  emitUserMsgs(respData, "[MDB] ");
  emit userMsg("*** MDB simulator loaded.");

  // send DEVICE command
  emit userMsg("*** Configuring MDB simulation device (" + simDevice + ")...");
  sendCmd("device " + simDevice);
  if (!readUntilPrompt(10000))
  {
    emit userMsg("*** Unxpected error waiting for MDB response.");
    return false;
  }
  // we expect nothing except a prompt
  emitUserMsgs(respData, "[MDB] ");
  if (respData.size())
  {
    emit userMsg("*** Device configuration failed.");
    return false;
  }
  emit userMsg("*** Simulation device configured.");

  // send HWTOOL command
  emit userMsg("*** Loading MDB software simulator (SIM)...");
  sendCmd("hwtool SIM");
  if (!readUntilPrompt(10000))
  {
    emit userMsg("*** Unxpected error waiting for MDB response.");
    return false;
  }
  emitUserMsgs(respData, "[MDB] ");
  // we expect the last non-blank line to be "Resetting peripherals"
  if (respData.last().trimmed().compare("Resetting peripherals"))
  {
    emit userMsg(
        "*** Unexpected response setting tool to softare simulator (SIM).");
    return false;
  }
  emit userMsg("*** MDB software simulator loaded.");

  // send PROGRAM command
  emit userMsg("*** Loading device program (" + simProgram + ")...");
  sendCmd("program " + simProgram);
  if (!readUntilPrompt(10000))
  {
    emit userMsg("*** Unxpected error waiting for MDB response.");
    return false;
  }
  emitUserMsgs(respData, "[MDB] ");
  // we expect the last non-blank line to be "Program succeeded."
  if (respData.last().trimmed().compare("Program succeeded."))
  {
    emit userMsg(
        "*** Unexpected response loading program (\"" + simProgram + "\").");
    return false;
  }

  // succeeded...
  emit userMsg("*** Device program successfully loaded.");
  return true;
}

void MdbSim::stopSim()
{
  // TODO:  Check first if running.  For now, just send quit command.
  emit userMsg("*** Stopping MDB simulation.");
  sendCmd("quit");
}

/*
 * readUntilPrompt() -- reads stdout/stderr until a prompt ('>') is recieved.
 * text is returned in a stringlist (respData) with leading/trailing whitespace
 * removed.  prompt line is not included.  returns false on timeout.
 */
bool MdbSim::readUntilPrompt(int msecs)
{
  static QRegularExpression reLineFeeds("[\r\n]");

  bool done = false;

  // clear prior response data
  respData.clear();

  QByteArray data;

  while (!done)
  {
    if (!waitForReadyRead(msecs))
    {
      emit userMsg("*** Unexpected timeout in readUntilPrompt().");
      break;
    }

    QString str;

    data = readAllStandardError();
    if (data.length())
    {
      str                 = data;
      QStringList strList = str.split(reLineFeeds, Qt::SkipEmptyParts);
      respData.append(strList);
    }

    data = readAllStandardOutput();
    if (data.length())
    {
      str                 = data;
      QStringList strList = str.split(reLineFeeds, Qt::SkipEmptyParts);
      respData.append(strList);
    }

    if (respData.size() && respData.last()[0] == '>')
    {
      respData.removeLast();
      done = true;
    }
  }

  return done;
}

/*
 * append "/r/n" to command and send to simulator.  returns false on write
 * error.
 */
bool MdbSim::sendCmd(QString cmd)
{
  cmd += "\r\n";
  return (write(cmd.toLatin1()) != -1);
}

/*
 * Step instruction (stepi)
 *
 * Expecting:
 *   Stepping
 *   Stop at
 *       address:0x1
 * OR:
 *   Stepping
 *   Stop at
 *       address:0x7e9
 *       file:C:/Users/rediv/AppData/Local/Temp/xcAs2cus/driver_tmp_1.s
 *       source line:1736
 *
 *
 */
bool MdbSim::stepInst(QString& str, double timeStamp)
{
  // save last timestep
  lastStepTime = timeStamp;

  if (!sendCmd("stepi")) return false;

  if (!readUntilPrompt()) return false;

  if (respData.size() != 3 && respData.size() != 5) return false;

  str = respData[2].trimmed();
  if (respData.size() > 3)
  {
    str += " " + respData[3].trimmed();
    str += " " + respData[4].trimmed();
  }

  return true;
}

bool MdbSim::getPin(const QString& pinName, bool& isInput, bool& isHigh,
    bool& isDigital, QString& info)
{
  static QRegularExpression reWhiteSpace("[\\s]");

  if (!sendCmd("print pin " + pinName)) return false;

  if (!readUntilPrompt()) return false;

  if (respData.length() != 2) return false;

  QStringList sList = respData[1].split(reWhiteSpace, Qt::SkipEmptyParts);
  if (sList.length() != 4) return false;

  isDigital = sList[1][0] == 'D';
  isInput   = sList[1][1] == 'i';
  isHigh    = sList[2][0] != 'L' && sList[2][0] != '0';
  info      = sList[3];
  return true;
}

bool MdbSim::setPin(const QString& pinName, const QString& value)
{
  // note:  I think that simulator ignores setting pin states for output pins
  // using "write pin"... is this a problem?  could add checks but already
  // probably slow.  need to address in MDB Java interface revisions...

  if (!sendCmd("write pin " + pinName + " " + value)) return false;

  if (!readUntilPrompt()) return false;

  // expecting an empty response
  return !respData.length();
}
