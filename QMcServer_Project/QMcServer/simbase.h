//------------------------------------------------------------------------------
// This file is part of the QMcServer project, a TCP/Sockets-based client/
// server framework for micro-controller QSpice C-Block components.  See the
// GitHub repository at https://github.com/robdunn4/QSpice/ for the complete
// project, documentation, and demontration code.
//
// This is Proof-of-Concept code and subject to significant changes.
//------------------------------------------------------------------------------
/*
 * Abstract base class for micro-controller simulators.
 *
 * This is Proof of Concept code and subject to significant revisions as
 * the project proceeds.
 */

#ifndef SIMBASE_H
#define SIMBASE_H

#include <QProcess>

// Abstract base class for simulators
class SimBase : public QProcess
{
  Q_OBJECT

  // we're going to need some state information eventually...
  enum SimStates
  {
    Inactive,
    Running,
    Etc
  };

public:
  SimBase(const QString simDevice, const QString simProgram,
      QObject* parent = nullptr);
  ~SimBase();

  // note: each simulator will have it's own commands and responses.  each
  // supported simulator is derived from this class and performs its own unique
  // formatting and parsing through derived classes...

  virtual bool startSim()                                           = 0;
  virtual void stopSim()                                            = 0;
  virtual bool stepInst(QString& str, double timeStamp = 0.0)       = 0;
  virtual bool getPin(const QString& pinName, bool& isInput, bool& isHigh,
      bool& isDigital, QString& info)                               = 0;
  virtual bool setPin(const QString& pinName, const QString& value) = 0;

#if 0
  virtual bool setVDD(double value) = 0;
  virtual bool passThru(const QByteArray& data)               = 0;
#endif

  double getLastStepTime(); // not currently used

protected:
  SimStates simState;    // not used yet but we'll need state info eventually...
  QString   simProgPath; // path to simulator
  QString   simDevice;   // device to simulate
  QString   simProgram;  // device program (hex/elf/etc.) to load into simulator
  double    lastStepTime; // last timestep (from QSpice); not currently used

  void emitUserMsgs(QStringList msgs, const char* pfx = "");

signals:
  void stdoutMsg(QString msg);
  void stderrMsg(QString msg);
  void userMsg(QString msg);
};

#endif // SIMBASE_H
