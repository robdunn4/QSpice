#pragma once

#include <QRegularExpression>
#include <QString>

namespace sch::core {

// Buses, as QSpice defines them: names only, no drawing (the drawn
// width is SchItemWire's). Header-only, no moc.
//
// A BUS is not a separate record: it is an ordinary wire whose NET NAME
// has the form
//
//     Name[n1:n2]      n1, n2 unsigned decimal, in either order
//
// and its MEMBERS are the nets Name[k] for every k from min(n1,n2) to
// max(n1,n2). Names compare case-insensitively, as all QSpice net names
// do. Confirmed against BusStuff_QSpice.qsch: Bus[0:3] with members
// Bus[1], Bus[2], Bus[3] valid, and Bus2[0:3] with Bus2[10..13] all
// rejected as out of range.

struct BusName {
  QString base; // the part before '['
  int     lo = 0;
  int     hi = 0;
};

// Parses a bus name. False for anything else, including a single-index
// member name like Bus[1].
inline bool parseBusName(const QString &name, BusName &out) {
  static const QRegularExpression re(QStringLiteral(R"(^(.+)\[(\d+):(\d+)\]$)"));

  const QRegularExpressionMatch m = re.match(name.trimmed());
  if (!m.hasMatch()) return false;

  bool      ok1 = false;
  bool      ok2 = false;
  const int n1  = m.captured(2).toInt(&ok1);
  const int n2  = m.captured(3).toInt(&ok2);
  if (!ok1 || !ok2) return false; // too many digits for an int

  out.base = m.captured(1);
  out.lo   = qMin(n1, n2);
  out.hi   = qMax(n1, n2);
  return true;
}

inline bool isBusName(const QString &name) {
  BusName bus;
  return parseBusName(name, bus);
}

// Whether `net` is one of `bus`'s members: Name[k], same base ignoring
// case, k within the bus's range.
inline bool isBusMember(const QString &net, const BusName &bus) {
  static const QRegularExpression re(QStringLiteral(R"(^(.+)\[(\d+)\]$)"));

  const QRegularExpressionMatch m = re.match(net.trimmed());
  if (!m.hasMatch()) return false;

  if (m.captured(1).compare(bus.base, Qt::CaseInsensitive) != 0) return false;

  bool      ok    = false;
  const int index = m.captured(2).toInt(&ok);
  return ok && index >= bus.lo && index <= bus.hi;
}

} // namespace sch::core
