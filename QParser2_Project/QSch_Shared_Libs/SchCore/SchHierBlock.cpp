#include "SchHierBlock.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <cmath>
#include <vector>

#include <ItemNet.h>
#include <ItemText.h>
#include <ItemTreeIO.h>
#include <ItemType.h>

#include "SchStringDecode.h"

namespace sch::core::hier {

namespace {

NodePtr symbolOf(const NodePtr &compNode) {
  if (compNode == nullptr || compNode->getEnumID() != QPI::COMP) return nullptr;
  return compNode->findFirstOf(QPI::SYM);
}

// Whether two absolute paths name the same file: without regard to case
// on Windows, as its file system has it.
bool samePath(const QString &a, const QString &b) {
#ifdef Q_OS_WIN
  const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
  const Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
  return QDir::cleanPath(QFileInfo(a).absoluteFilePath())
             .compare(QDir::cleanPath(QFileInfo(b).absoluteFilePath()), cs) ==
         0;
}

// The ground names, which are drawn as ground symbols and are never
// ports -- as sch::render::SchItemNet has them.
bool isGroundName(const QString &name) {
  return name.compare(QStringLiteral("GND"), Qt::CaseInsensitive) == 0 ||
         name.compare(QStringLiteral("EARTH"), Qt::CaseInsensitive) == 0 ||
         name.compare(QStringLiteral("CHASSIS"), Qt::CaseInsensitive) == 0;
}

// Whether the schematic `text`, saved as `parentPath`, has a block that
// opens `childPath`.
bool opensChild(const QByteArray &text, const QString &parentPath,
                const QString &childPath) {
  const ItemTreeIO::ParseResult res = ItemTreeIO::readString(
      std::string_view(text.constData(), static_cast<size_t>(text.size())));
  if (!res.ok || res.tree.root() == nullptr) return false;

  for (const NodePtr &child : res.tree.root()->children()) {
    if (child->getEnumID() != QPI::COMP) continue;

    QString path;
    if (schematicPath(child, parentPath, path) != Result::Ok) continue;
    if (samePath(path, childPath)) return true;
  }

  return false;
}

// One port of the child, for the parent's block.
struct Port {
  std::string raw;      // its name as written in the child
  QString     name;     // decoded, for measuring
  int         x    = 0; // where its label sits in the child
  int         y    = 0;
  int         side = 0; // -1 left, +1 right, 0 not known yet
};

// The child's ports, each once.
std::vector<Port> childPorts(const NodePtr &root) {
  std::vector<Port> ports;
  if (root == nullptr) return ports;

  for (const NodePtr &child : root->children()) {
    if (child->getEnumID() != QPI::NET) continue;

    const ItemNetPtr net = std::static_pointer_cast<ItemNet>(child->item());
    if (net == nullptr || net->netType.getValue() == 0) continue;

    // The name as written, without the quotes the record keeps on it.
    Port port;
    port.raw = net->netName.getValue();
    if (port.raw.size() >= 2 && port.raw.front() == '"' &&
        port.raw.back() == '"')
      port.raw = port.raw.substr(1, port.raw.size() - 2);
    port.name = decodeQSpiceString(port.raw).trimmed();
    if (port.name.isEmpty() || isGroundName(port.name)) continue;

    const bool seen =
        std::any_of(ports.begin(), ports.end(), [&port](const Port &other) {
          return other.name.compare(port.name, Qt::CaseInsensitive) == 0;
        });
    if (seen) continue;

    const Point at = net->pt1.getValue();
    port.x         = at.x;
    port.y         = at.y;

    // The alignment names the edge the label's point is on; the text
    // runs away from it. On the EAST edge, the text is to the left: a
    // connection on the left of the drawing.
    const unsigned char alignH = net->rotateAlign.getAlignH();
    if (alignH == ArgRotAlign::EAST) port.side = -1;
    else if (alignH == ArgRotAlign::WEST) port.side = 1;

    ports.push_back(port);
  }

  return ports;
}

// `value` rounded UP to a multiple of `step`.
int roundUp(double value, int step) {
  return static_cast<int>(std::ceil(value / step)) * step;
}

std::string point(int x, int y) {
  return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
}

}   // namespace

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

Result schematicPath(
    const NodePtr &compNode, const QString &docPath, QString &path) {
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

QStringList findParents(const QString &childPath) {
  QStringList found;

  const QFileInfo child(childPath);
  if (!child.exists()) return found;

  // The child's name, as the bytes a block naming it would hold.
  const QByteArray needle = encodeCp1252(child.completeBaseName()).toLower();
  if (needle.isEmpty()) return found;

  QStringList folders = {child.absolutePath()};
  QDir        above(child.absolutePath());
  if (above.cdUp()) folders << above.absolutePath();

  for (const QString &folder : folders) {
    const QFileInfoList files = QDir(folder).entryInfoList(
        {QStringLiteral("*.qsch")}, QDir::Files | QDir::Readable, QDir::Name);

    for (const QFileInfo &file : files) {
      const QString path = file.absoluteFilePath();
      if (samePath(path, childPath)) continue;

      QFile in(path);
      if (!in.open(QIODevice::ReadOnly)) continue;
      const QByteArray text = in.readAll();

      if (!text.toLower().contains(needle)) continue;
      if (opensChild(text, path, childPath)) found << QDir::cleanPath(path);
    }
  }

  found.sort(Qt::CaseInsensitive);
  return found;
}

QStringList portNames(const NodePtr &root) {
  QStringList names;
  for (const Port &port : childPorts(root)) names << port.name;
  return names;
}

std::string
parentSchematicText(const NodePtr &childRoot, const QString &childPath,
                    const QString                                &parentPath,
                    const std::function<double(const QString &)> &textWidth) {
  std::vector<Port> ports = childPorts(childRoot);
  if (ports.empty()) return std::string();

  // A label pointing neither way goes by where it sits: left or right of
  // the middle of the ports.
  int minX = ports.front().x;
  int maxX = minX;
  for (const Port &port : ports) {
    minX = std::min(minX, port.x);
    maxX = std::max(maxX, port.x);
  }
  const double middle = (minX + maxX) / 2.0;
  for (Port &port : ports)
    if (port.side == 0) port.side = (port.x < middle) ? -1 : 1;

  // Top to bottom as in the child -- its Y runs upward -- then left to
  // right.
  std::stable_sort(ports.begin(), ports.end(),
                   [](const Port &a, const Port &b) {
                     if (a.y != b.y) return a.y > b.y;
                     return a.x < b.x;
                   });

  // Pin names at the size QSpice gives a block's name: at full size, a
  // name is taller than the 100 between pins.
  constexpr double kPinFont = 0.681;

  std::vector<Port> left;
  std::vector<Port> right;
  double            leftWidth  = 0.0;
  double            rightWidth = 0.0;
  for (const Port &port : ports) {
    const double width = textWidth ? textWidth(port.name) * kPinFont : 0.0;
    if (port.side < 0) {
      left.push_back(port);
      leftWidth = std::max(leftWidth, width);
    } else {
      right.push_back(port);
      rightWidth = std::max(rightWidth, width);
    }
  }

  // Pins 100 apart on the electrical grid, centered on the origin as
  // nearly as the grid allows; the box 100 beyond the first and last.
  constexpr int kGrid  = 100;
  const int     rows   = static_cast<int>(std::max(left.size(), right.size()));
  const int     topPin = kGrid * ((rows - 1) / 2);
  const int     top    = topPin + kGrid;
  const int     bottom = topPin - kGrid * rows;

  // Each name is INDENTED from its edge, so it does not touch the box's
  // line: its anchor is that far inside, on the pin's row.
  constexpr int kIndent = 10;

  // Both columns of names, their indents, and a gap between them; the
  // edges on the grid, so the width is a multiple of twice it.
  const int width =
      std::max(2 * kGrid, roundUp(leftWidth + rightWidth + 2 * kIndent + kGrid,
                                  2 * kGrid));
  const int half = width / 2;

  // The child's name as the block holds it: relative to the parent's
  // folder, without ".qsch", with Windows separators.
  QString name = QDir(QFileInfo(parentPath).absolutePath())
                     .relativeFilePath(QFileInfo(childPath).absoluteFilePath());
  if (name.endsWith(QStringLiteral(".qsch"), Qt::CaseInsensitive)) name.chop(5);
  name.replace(QLatin1Char('/'), QLatin1Char('\\'));

  const char ab = static_cast<char>(ItemTreeIO::BEGIN_NODE);
  const char bb = static_cast<char>(ItemTreeIO::END_NODE);

  std::string text(reinterpret_cast<const char *>(ItemTreeIO::FILE_ID),
                   sizeof(ItemTreeIO::FILE_ID));
  text += ab + std::string("schematic\n");
  text += "  " + std::string(1, ab) + "component (0,0) 0 0\n";
  text += "    " + std::string(1, ab) + "symbol\n";
  text += "      " + std::string(1, ab) + "shorted pins: false" + bb + "\n";
  text += "      " + std::string(1, ab) + "rect " + point(-half, bottom) + " " +
          point(half, top) + " 0 0 0 0xff0000 0xc8c8c8 -1 1 -1" + bb + "\n";

  // The designator above the box, the child's name below it.
  text += "      " + std::string(1, ab) + "text " + point(0, top) +
          " 1 14 0 0x1000000 -1 -1 \"X1\"" + bb + "\n";
  text += "      " + std::string(1, ab) + "text " + point(0, bottom) +
          " 0.681 13 0 0x1000000 -1 -1 \"" + encodeQSpiceString(name) + "\"" +
          bb + "\n";

  // Pin names inside the box, anchored on their vertical edge, indented
  // from it (the label offset): from the left edge they run right (7,
  // west), from the right edge left (11, east).
  for (size_t i = 0; i < left.size(); ++i)
    text += "      " + std::string(1, ab) + "pin " +
            point(-half, topPin - kGrid * static_cast<int>(i)) + " " +
            point(kIndent, 0) + " 0.681 7 0 0x0 -1 \"" + left[i].raw + "\"" +
            bb + "\n";
  for (size_t i = 0; i < right.size(); ++i)
    text += "      " + std::string(1, ab) + "pin " +
            point(half, topPin - kGrid * static_cast<int>(i)) + " " +
            point(-kIndent, 0) + " 0.681 11 0 0x0 -1 \"" + right[i].raw + "\"" +
            bb + "\n";

  text += "    " + std::string(1, bb) + "\n";
  text += "  " + std::string(1, bb) + "\n";
  text += std::string(1, bb) + "\n";
  return text;
}

}   // namespace sch::core::hier
