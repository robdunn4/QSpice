#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchConnDump.h"

#include <QHash>
#include <QList>
#include <QMap>
#include <QSet>
#include <QStringList>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>
#include <utility>

#include "SchNetModel.h"
#include "SchPointKey.h"

namespace sch::core {
namespace {

QString pt(const PointKey &p) { return QStringLiteral("(%1,%2)").arg(p.x).arg(p.y); }

QString quoted(const QString &text) { return QLatin1Char('"') + text + QLatin1Char('"'); }

// A point's sort order: x, then y.
bool pointLess(const PointKey &a, const PointKey &b) {
  if (a.x != b.x) return a.x < b.x;
  return a.y < b.y;
}

// One connected group, as text lines, plus the names it carries.
struct NetFacts {
  QStringList   members;
  QSet<QString> wireNames;
  QSet<QString> labelNames;
  bool          hasWire = false;
};

struct BusSeg {
  QString  name;
  PointKey a;
  PointKey b;
};

// Everything the dump reports, gathered from NetModel and written by
// format(). (Kept separate from the gathering: while the scene code was
// being retired a second producer fed the same format, so the two could
// differ only in what they FOUND.)
struct Facts {
  int wires      = 0; // non-bus
  int buses      = 0;
  int comps      = 0;
  int compPins   = 0;
  int sheetPins  = 0;
  int labels     = 0;
  int junctions  = 0;
  int taps       = 0;

  QList<NetFacts>      nets; // every group; format() keeps the nets
  QHash<PointKey, int> wireEnds;
  QList<PointKey>      junctionsPresent;
  QStringList          unconnected; // pinText() + point
  QList<PointKey>      looseEnds;
  QList<BusSeg>        busSegs;
  QStringList          refdes; // empty for none
};

// "R1.2 \"IN-\"": designator, 1-based pin number in the symbol's record
// order, and the pin's label.
QString pinText(QString refdes, int number, const QString &label) {
  if (refdes.isEmpty()) refdes = QStringLiteral("?");
  return QStringLiteral("%1.%2 %3").arg(refdes).arg(number).arg(quoted(label));
}

QString wireMember(PointKey a, PointKey b, bool bus, const QString &name) {
  if (pointLess(b, a)) std::swap(a, b);
  return QStringLiteral("%1 %2-%3 net=%4")
      .arg(bus ? QStringLiteral("bus ") : QStringLiteral("wire"))
      .arg(pt(a), pt(b), quoted(name));
}

QString pinMember(const QString &text, const PointKey &p, bool internal) {
  return QStringLiteral("pin  %1 %2%3")
      .arg(text, pt(p), internal ? QStringLiteral(" internal") : QString());
}

QString sheetPinMember(const PointKey &p) { return QStringLiteral("pin  (sheet) %1").arg(pt(p)); }

QString labelMember(const QString &name, const PointKey &p) {
  return QStringLiteral("label %1 %2").arg(quoted(name), pt(p));
}

QString tapMember(const PointKey &p) { return QStringLiteral("tap  %1").arg(pt(p)); }

QStringList sorted(QStringList list) {
  std::sort(list.begin(), list.end());
  return list;
}

QString joinNames(const QSet<QString> &names) {
  QStringList list(names.begin(), names.end());
  list = sorted(list);
  for (QString &name : list) name = quoted(name);
  return list.isEmpty() ? QStringLiteral("-") : list.join(QLatin1Char(' '));
}

QString format(const Facts &facts, const QString &fileName) {
  QString out;
  out += QStringLiteral("# Connectivity dump -- %1\n").arg(fileName);
  out += QStringLiteral("# wires %1 (buses %2), components %3, component pins %4, "
                        "sheet pins %5, net labels %6, junctions %7, taps %8\n")
             .arg(facts.wires)
             .arg(facts.buses)
             .arg(facts.comps)
             .arg(facts.compPins)
             .arg(facts.sheetPins)
             .arg(facts.labels)
             .arg(facts.junctions)
             .arg(facts.taps);

  // --- Nets. A net is a group with a wire, or with two or more members
  // touching. A lone pin is not a net; it appears under unconnected pins.
  QList<NetFacts> listed;
  for (NetFacts net : facts.nets) {
    if (!net.hasWire && net.members.size() < 2) continue;
    net.members = sorted(net.members);
    listed.append(net);
  }
  std::sort(listed.begin(), listed.end(), [](const NetFacts &a, const NetFacts &b) {
    return a.members.first() < b.members.first();
  });

  out += QStringLiteral("\n[nets] %1\n").arg(listed.size());
  for (int i = 0; i < listed.size(); ++i) {
    out += QStringLiteral("net %1  wires: %2  labels: %3\n")
               .arg(i + 1)
               .arg(joinNames(listed[i].wireNames), joinNames(listed[i].labelNames));
    for (const QString &member : listed[i].members) out += QStringLiteral("  ") + member + '\n';
  }

  // --- Junctions: required (three or more non-bus wire endpoints) and
  // present (records), and where they disagree.
  QList<PointKey> required;
  for (QHash<PointKey, int>::const_iterator it = facts.wireEnds.constBegin();
       it != facts.wireEnds.constEnd(); ++it)
    if (it.value() >= kJunctionMinEndpoints) required.append(it.key());
  std::sort(required.begin(), required.end(), pointLess);

  QList<PointKey> present = facts.junctionsPresent;
  std::sort(present.begin(), present.end(), pointLess);

  out += QStringLiteral("\n[junctions required] %1\n").arg(required.size());
  for (const PointKey &p : required)
    out += QStringLiteral("  %1  wire ends %2\n").arg(pt(p)).arg(facts.wireEnds.value(p));

  out += QStringLiteral("\n[junctions present] %1\n").arg(present.size());
  for (const PointKey &p : present) out += QStringLiteral("  %1\n").arg(pt(p));

  QStringList mismatch;
  for (const PointKey &p : required)
    if (!present.contains(p)) mismatch << QStringLiteral("  missing %1").arg(pt(p));
  for (const PointKey &p : present)
    if (!required.contains(p)) mismatch << QStringLiteral("  extra   %1").arg(pt(p));
  out += QStringLiteral("\n[junction mismatches] %1\n").arg(mismatch.size());
  for (const QString &line : sorted(mismatch)) out += line + '\n';

  // --- Unconnected pins and loose wire ends: what the marks show.
  out += QStringLiteral("\n[unconnected pins] %1\n").arg(facts.unconnected.size());
  for (const QString &line : sorted(facts.unconnected)) out += QStringLiteral("  ") + line + '\n';

  QStringList loose;
  for (const PointKey &p : facts.looseEnds) loose << QStringLiteral("  %1").arg(pt(p));
  out += QStringLiteral("\n[loose wire ends] %1\n").arg(loose.size());
  for (const QString &line : sorted(loose)) out += line + '\n';

  // --- Buses, by name.
  QMap<QString, QStringList> buses;
  for (BusSeg seg : facts.busSegs) {
    if (pointLess(seg.b, seg.a)) std::swap(seg.a, seg.b);
    buses[seg.name] << QStringLiteral("  %1-%2").arg(pt(seg.a), pt(seg.b));
  }
  out += QStringLiteral("\n[buses] %1\n").arg(buses.size());
  for (QMap<QString, QStringList>::const_iterator it = buses.constBegin();
       it != buses.constEnd(); ++it) {
    out += QStringLiteral("bus %1\n").arg(quoted(it.key()));
    for (const QString &line : sorted(it.value())) out += line + '\n';
  }

  // --- Designators, case-insensitive duplicates marked.
  QStringList         refdes;
  QHash<QString, int> seen;
  for (const QString &name : facts.refdes) {
    refdes << (name.isEmpty() ? QStringLiteral("(none)") : name);
    if (!name.isEmpty()) seen[name.toLower()]++;
  }
  refdes = sorted(refdes);
  out += QStringLiteral("\n[designators] %1\n").arg(refdes.size());
  for (const QString &name : refdes)
    out += QStringLiteral("  %1%2\n")
               .arg(name, seen.value(name.toLower(), 0) > 1 ? QStringLiteral("  DUPLICATE")
                                                            : QString());

  return out;
}

} // namespace

namespace conndump {

QString dumpModel(const NetModel &model, const QString &fileName) {
  Facts facts;

  for (const NetModel::Wire &wire : model.wires())
    if (wire.bus) ++facts.buses;
  facts.wires = static_cast<int>(model.wires().size()) - facts.buses;
  facts.comps = static_cast<int>(model.comps().size());
  for (const NetModel::Pin &pin : model.pins()) {
    if (pin.comp >= 0) ++facts.compPins;
    else ++facts.sheetPins;
  }
  facts.labels    = static_cast<int>(model.labels().size());
  facts.junctions = static_cast<int>(model.junctions().size());
  facts.taps      = static_cast<int>(model.taps().size());

  // A pin's text, from the model.
  const QList<NetModel::Comp> &comps = model.comps();
  const QList<NetModel::Pin>  &pins  = model.pins();
  QStringList                     pinTexts;
  for (const NetModel::Pin &pin : pins)
    pinTexts << (pin.comp >= 0 ? pinText(comps[pin.comp].refdes, pin.number, pin.label)
                               : QString());

  // --- Nets: the model's groups, every one (format() keeps the nets).
  for (const NetModel::Net &net : model.nets()) {
    NetFacts out;
    out.wireNames  = net.wireNames;
    out.labelNames = net.labelNames;
    out.hasWire    = !net.wires.isEmpty();
    for (int i : net.wires) {
      const NetModel::Wire &wire = model.wires()[i];
      out.members << wireMember(wire.a, wire.b, wire.bus, wire.netName);
    }
    for (int i : net.pins) {
      const NetModel::Pin &pin = pins[i];
      out.members << (pin.comp >= 0 ? pinMember(pinTexts[i], pin.at, pin.internal)
                                    : sheetPinMember(pin.at));
    }
    for (int i : net.labels) {
      const NetModel::Label &label = model.labels()[i];
      out.members << labelMember(label.netName, label.at);
    }
    for (int code : net.tapEnds) {
      const NetModel::Tap &tap = model.taps()[code / 2];
      out.members << tapMember(code % 2 == 0 ? tap.a : tap.b);
    }
    facts.nets.append(out);
  }

  // --- Junctions.
  facts.wireEnds = model.wireEndCounts();
  for (const NetModel::Junction &junc : model.junctions())
    facts.junctionsPresent.append(junc.at);

  // --- Marks.
  for (int i : model.unconnectedPins())
    facts.unconnected << QStringLiteral("%1 %2").arg(pinTexts[i], pt(pins[i].at));
  facts.looseEnds = model.looseEnds();

  // --- Buses and designators.
  for (const NetModel::Wire &wire : model.wires())
    if (wire.bus) facts.busSegs.append(BusSeg{wire.netName, wire.a, wire.b});
  for (const NetModel::Comp &comp : comps) facts.refdes << comp.refdes;

  return format(facts, fileName);
}

QString firstDifference(const QString &a, const QString &b) {
  if (a == b) return QString();

  const QStringList linesA = a.split(QLatin1Char('\n'));
  const QStringList linesB = b.split(QLatin1Char('\n'));
  const qsizetype   count  = qMax(linesA.size(), linesB.size());
  for (qsizetype i = 0; i < count; ++i) {
    const QString lineA = linesA.value(i);
    const QString lineB = linesB.value(i);
    if (lineA == lineB) continue;
    return QStringLiteral("line %1: \"%2\" | \"%3\"").arg(i + 1).arg(lineA, lineB);
  }
  return QString();
}

} // namespace conndump
} // namespace sch::core
