#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchRecordRules.h"

#include <QByteArray>
#include <QDebug>
#include <QMap>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <vector>

#include "ItemCmp.h"
#include "ItemSym.h"
#include "ItemTypes.h"
#include "ItemWire.h"
#include "SchBus.h"
#include "SchRefDes.h"
#include "SchStringDecode.h"

namespace sch::core::recordrules {
namespace {

// What a record's children are, for the rules.
enum class Ctx { Sheet, Comp, Sym };

QStringList libraryNotes;

QString recordText(const NodePtr &node) {
  const std::string raw = node->item()->toString();
  QString text = decodeCp1252(QByteArray::fromStdString(raw));
  if (node->hasChildren()) text += QStringLiteral(" (and its contents)");
  return text;
}

// "wire" or "bus", "net label", ... -- the record type as a person says
// it.
QString kindOf(const NodePtr &node) {
  switch (node->getEnumID()) {
  case QPI::WIRE: {
    const ItemWirePtr rec = std::static_pointer_cast<ItemWire>(node->item());
    if (rec != nullptr && isBusName(decodeQSpiceString(rec->netName.getValue()).trimmed()))
      return QStringLiteral("bus");
    return QStringLiteral("wire");
  }
  case QPI::TAP: return QStringLiteral("bus tap");
  case QPI::NET: return QStringLiteral("net label");
  case QPI::JUNCTION: return QStringLiteral("junction");
  case QPI::PIN: return QStringLiteral("pin");
  case QPI::SYM: return QStringLiteral("symbol");
  case QPI::COMP: return QStringLiteral("component");
  case QPI::SCH: return QStringLiteral("schematic");
  default: return QString::fromStdString(std::string(ItemTypes::getStr(node->getEnumID())));
  }
}

// Names a component for the message: its designator, else its symbol's
// name, else where it is.
QString compName(const NodePtr &compNode) {
  const QString refdes = refdes::ofNode(compNode);
  if (!refdes.isEmpty()) return refdes;

  if (const NodePtr symNode = compNode->findFirstOf(QPI::SYM)) {
    const ItemSymPtr sym = std::static_pointer_cast<ItemSym>(symNode->item());
    if (sym != nullptr) {
      const QString name = decodeQSpiceString(sym->text.getValue()).trimmed();
      if (!name.isEmpty()) return name;
    }
  }

  const ItemCmpPtr comp = std::static_pointer_cast<ItemCmp>(compNode->item());
  if (comp == nullptr) return QStringLiteral("a component");

  const Point at = comp->pt1.getValue();
  return QStringLiteral("the component at (%1,%2)").arg(at.x).arg(at.y);
}

bool forbidden(Ctx ctx, QPI type) {
  switch (ctx) {
  case Ctx::Sheet:
    return type == QPI::PIN || type == QPI::SYM || type == QPI::SCH;
  case Ctx::Comp:
    return type == QPI::SCH || type == QPI::COMP;
  case Ctx::Sym:
    return type == QPI::WIRE || type == QPI::TAP || type == QPI::NET ||
           type == QPI::JUNCTION || type == QPI::SYM || type == QPI::COMP ||
           type == QPI::SCH;
  }
  return false;
}

QString whereText(Ctx ctx, QPI type, const QString &owner) {
  if (ctx == Ctx::Sym)
    return owner.isEmpty() ? QStringLiteral("inside this symbol")
                           : QStringLiteral("inside the symbol of %1").arg(owner);
  if (ctx == Ctx::Comp)
    return (type == QPI::SYM) ? QStringLiteral("beyond the first in %1").arg(owner)
                              : QStringLiteral("inside %1").arg(owner);
  if (type == QPI::SYM) return QStringLiteral("outside a component");
  if (type == QPI::SCH) return QStringLiteral("inside the schematic");
  return QStringLiteral("at the top level of the schematic");
}

// `owner` names whose content this is, for the message; empty for the
// document itself.
void sweep(const NodePtr &parent, Ctx ctx, const QString &owner, QList<Omitted> &out) {
  // Collected first, detached after: detaching changes the list being
  // walked.
  std::vector<NodePtr> doomed;
  bool                 haveSymbol = false;

  for (const NodePtr &child : parent->children()) {
    const QPI type = child->getEnumID();

    // A component holds ONE symbol: any after the first goes.
    const bool extraSymbol = (ctx == Ctx::Comp && type == QPI::SYM && haveSymbol);
    if (ctx == Ctx::Comp && type == QPI::SYM) haveSymbol = true;

    if (extraSymbol || forbidden(ctx, type)) {
      out.append(Omitted{kindOf(child), whereText(ctx, type, owner), recordText(child)});
      doomed.push_back(child);
      continue;
    }

    if (ctx == Ctx::Sheet && type == QPI::COMP) {
      sweep(child, Ctx::Comp, compName(child), out);
      continue;
    }

    if (ctx == Ctx::Comp && type == QPI::SYM) sweep(child, Ctx::Sym, owner, out);
  }

  for (const NodePtr &node : doomed) node->detach();
}

} // namespace

QList<Omitted> dropForbidden(const NodePtr &node, Dest dest) {
  QList<Omitted> out;
  if (node == nullptr) return out;

  switch (node->getEnumID()) {
  case QPI::SYM: sweep(node, Ctx::Sym, QString(), out); break;
  case QPI::COMP: sweep(node, Ctx::Comp, compName(node), out); break;
  case QPI::SCH:
    if (dest == Dest::Symbol) sweep(node, Ctx::Sym, QString(), out);
    else sweep(node, Ctx::Sheet, QString(), out);
    break;
  default: break;
  }

  return out;
}

QStringList summary(const QList<Omitted> &omitted) {
  // Ordered, so the same file always reads the same way.
  QMap<QString, int> counts;
  for (const Omitted &one : omitted) ++counts[one.kind + QLatin1Char(' ') + one.where];

  QStringList out;
  for (QMap<QString, int>::const_iterator it = counts.constBegin(); it != counts.constEnd(); ++it)
    out << QStringLiteral("%1 x %2").arg(it.value()).arg(it.key());
  return out;
}

void log(const QString &source, const QList<Omitted> &omitted) {
  if (omitted.isEmpty()) return;

  qWarning().noquote() << QStringLiteral("Records omitted from %1 (%2):")
                              .arg(source)
                              .arg(omitted.size());
  for (const Omitted &one : omitted)
    qWarning().noquote() << QStringLiteral("  %1 %2: %3").arg(one.kind, one.where, one.record);
}

void noteLibrary(const QString &fileName, const QList<Omitted> &omitted) {
  if (omitted.isEmpty()) return;

  log(fileName, omitted);
  for (const QString &line : summary(omitted))
    libraryNotes << QStringLiteral("%1: %2").arg(fileName, line);
}

QStringList takeLibraryNotes() {
  QStringList out;
  out.swap(libraryNotes);
  return out;
}

} // namespace sch::core::recordrules
