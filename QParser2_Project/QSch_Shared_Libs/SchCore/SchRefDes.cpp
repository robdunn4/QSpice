#include "SchRefDes.h"

#include "ItemText.h"
#include "SchStringDecode.h"

namespace sch::core::refdes {
namespace {

QString textOf(const NodePtr &textNode) {
  if (textNode == nullptr) return QString();

  const ItemTextPtr text =
      std::static_pointer_cast<ItemText>(textNode->item());
  if (text == nullptr) return QString();

  return decodeQSpiceString(text->text.getValue()).trimmed();
}

} // namespace

// The component's designator record: text record 0 inside its symbol.
//
// Positional, as everything else that touches attributes is -- record 0
// is the refdes and record 1 the value.
NodePtr refDesNode(const NodePtr &compNode) {
  if (compNode == nullptr || compNode->getEnumID() != QPI::COMP) return nullptr;

  const NodePtr symNode = compNode->findFirstOf(QPI::SYM);
  if (symNode == nullptr) return nullptr;

  for (const NodePtr &child : symNode->children())
    if (child->getEnumID() == QPI::TEXT) return child;

  return nullptr;
}

QString ofNode(const NodePtr &compNode) { return textOf(refDesNode(compNode)); }

QString prefixOf(const QString &refdes) {
  int end = refdes.size();
  while (end > 0 && refdes.at(end - 1).isDigit()) --end;

  return refdes.left(end);
}

QSet<QString> existing(const NodePtr &root) {
  QSet<QString> out;
  if (root == nullptr) return out;

  for (const NodePtr &child : root->children()) {
    if (child->getEnumID() != QPI::COMP) continue;

    const QString refdes = ofNode(child);
    if (!refdes.isEmpty()) out.insert(refdes.toLower());
  }

  return out;
}

QString nextFree(const QString &prefix, QSet<QString> &taken) {
  if (prefix.isEmpty()) return QString();

  // From 1 upward, taking the first gap. QSpice fills gaps rather than
  // continuing past the highest, so deleting R2 and placing a resistor
  // gives R2 back.
  for (int n = 1;; ++n) {
    const QString candidate = QStringLiteral("%1%2").arg(prefix).arg(n);

    if (taken.contains(candidate.toLower())) continue;

    taken.insert(candidate.toLower());
    return candidate;
  }
}

QString assign(const NodePtr &compNode, QSet<QString> &taken) {
  const NodePtr textNode = refDesNode(compNode);
  const QString current  = textOf(textNode);

  // A symbol with no designator -- or one the user has cleared -- gets
  // nothing: there is nothing to number.
  if (current.isEmpty()) return QString();

  const QString prefix = prefixOf(current);
  if (prefix.isEmpty()) return QString();

  const QString assigned = nextFree(prefix, taken);
  if (assigned.isEmpty() || assigned == current) return QString();

  const ItemTextPtr text = std::static_pointer_cast<ItemText>(textNode->item());
  if (text == nullptr) return QString();

  text->text.setValue(ArgString(encodeQSpiceString(assigned)).quote().getValue());
  return assigned;
}

} // namespace sch::core::refdes
