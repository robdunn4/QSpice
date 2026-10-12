#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "SchHierBlock.h"

#include <QDir>
#include <QFileInfo>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "ItemText.h"
#include "ItemType.h"
#include "SchStringDecode.h"

namespace sch::core::hier {

namespace {

NodePtr symbolOf(const NodePtr &compNode) {
  if (compNode == nullptr || compNode->getEnumID() != QPI::COMP) return nullptr;
  return compNode->findFirstOf(QPI::SYM);
}

} // namespace

bool isBlock(const NodePtr &compNode) {
  const NodePtr symNode = symbolOf(compNode);
  if (symNode == nullptr) return false;

  // Absent is the same as blank: QSpice drops the record when the type is
  // cleared in its Symbol Properties.
  const NodePtr typeNode = symNode->findFirstOf(QPI::TYPE);
  if (typeNode == nullptr) return true;

  const ItemTypePtr type = std::static_pointer_cast<ItemType>(typeNode->item());
  if (type == nullptr) return true;

  return decodeQSpiceString(type->text.getValue()).trimmed().isEmpty();
}

QString schematicName(const NodePtr &compNode) {
  const NodePtr symNode = symbolOf(compNode);
  if (symNode == nullptr) return QString();

  // Positional, as every attribute is: text record 0 is the designator,
  // record 1 the first string attribute.
  int textIndex = 0;
  for (const NodePtr &child : symNode->children()) {
    if (child->getEnumID() != QPI::TEXT) continue;
    if (textIndex++ == 0) continue;

    const ItemTextPtr text = std::static_pointer_cast<ItemText>(child->item());
    if (text == nullptr) return QString();

    const QString name = decodeQSpiceString(text->text.getValue()).trimmed();
    if (name.isEmpty()) return QString();

    return name + QStringLiteral(".qsch");
  }

  return QString();
}

Result schematicPath(const NodePtr &compNode, const QString &docPath, QString &path) {
  path.clear();

  if (!isBlock(compNode)) return Result::NotABlock;

  const QString name = schematicName(compNode);
  if (name.isEmpty()) return Result::NoName;

  // Either separator is accepted on any platform: QSpice writes
  // backslashes, which only Windows would otherwise read as separators.
  QString written = name;
  written.replace(QLatin1Char('\\'), QLatin1Char('/'));
  path = written;

  if (QDir::isRelativePath(written)) {
    if (docPath.isEmpty()) return Result::NoFolder;
    path = QFileInfo(docPath).absoluteDir().filePath(written);
  }

  path = QDir::cleanPath(path);

  const QFileInfo info(path);
  if (!info.exists()) return Result::NotFound;
  if (!info.isFile()) return Result::NotAFile;

  path = info.absoluteFilePath();
  return Result::Ok;
}

} // namespace sch::core::hier
