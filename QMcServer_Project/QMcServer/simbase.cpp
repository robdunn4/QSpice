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
#include "simbase.h"

SimBase::SimBase(QString simDevice, QString simProgram, QObject* parent) :
    QProcess(parent), simDevice(simDevice), simProgram(simProgram)
{
  lastStepTime = 0.0;
}

SimBase::~SimBase() {}

double SimBase::getLastStepTime() { return lastStepTime; }

void SimBase::emitUserMsgs(QStringList msgs, const char* pfx)
{
  for (const QString& s : msgs) emit userMsg(pfx + s);
}
