//------------------------------------------------------------------------------
// This file is part of the QMcServer project, a TCP/Sockets-based client/
// server framework for micro-controller QSpice C-Block components.  See the
// GitHub repository at https://github.com/robdunn4/QSpice/ for the complete
// project, documentation, and demontration code.
//
// This is Proof-of-Concept code and subject to significant changes.
//------------------------------------------------------------------------------
/*
 * Microchip MDB simulator class.
 *
 * This is Proof of Concept code and subject to significant revisions as
 * the project proceeds.
 */
#ifndef MDBSIM_H
#define MDBSIM_H

#include "simbase.h"

class MdbSim : public SimBase
{
  Q_OBJECT

public:
  explicit MdbSim(const QString simDevice, const QString simProgram,
      QObject* parent = nullptr);
  ~MdbSim() override;

  bool startSim() override;
  void stopSim() override;
  bool stepInst(QString& str, double timeStamp = 0) override;
  bool getPin(const QString& pinName, bool& isInput, bool& isHigh,
      bool& isDigital, QString& info) override;
  bool setPin(const QString& pinName, const QString& value) override;

protected:
  QStringList respData; // response data

  bool readUntilPrompt(int msecs = 10000);

  bool sendCmd(QString cmd);

signals:
};

#endif // MDBSIM_H
