#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "QSpicePrefs.h"

#include <QDir>
#include <QHash>
#include <QSettings>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace sch::link::prefs {

namespace {

// Appended to the vendor key. The square brackets are part of the key
// NAME -- QSpice really does call the subkey "[Preferences]".
const char *kPreferencesSubkey = "\\QSPICE\\[Preferences]";

QHash<QString, QString> values;
bool                    wasLoaded = false;
QString                 loadedKeyPath;

const QString kUserSymDirsKey = QStringLiteral("UserSymbolDirectories");

// A folder path reduced to what sameDirectory() compares.
QString dirKey(const QString &path) {
  QString key = QDir::cleanPath(QDir::fromNativeSeparators(path.trimmed())).toLower();

  // "c:/" keeps its separator: without it, it names the current folder
  // on drive C rather than the root.
  if (key.endsWith(QLatin1Char('/')) && key.size() > 3) key.chop(1);
  return key;
}

// The values from a stock QSpice installation, in the registry's text
// form: what every accessor falls back to, so a machine without QSpice
// (or a non-Windows build) still renders the way QSpice would rather
// than in an arbitrary palette. The ONE place they are written.
struct StockValue {
  const char *name;
  const char *text;
};

constexpr StockValue kStockDrawingValues[] = {
    {"CADHighlightColor", "0x326464"},
    {"CADBackgroundColor", "0xe9ffff"},
    {"CADbomberSight", "0x000000"},
    {"CADgrapicAnnotation", "0xff0000"},
    {"CADsolidAnnotation", "0x00ffff"},
    {"CADgrapicText", "0xff0000"},
    {"CADnormalText", "0x000000"},
    {"CADnormalSchText", "0x000000"},
    {"CADcommentText", "0xff0000"},
    {"CADnetLabelText", "0x0000ff"},
    {"CADwireColor", "0xff0000"},
    {"CADhighlightWireColor", "0x64e1e1"},
    {"CADpinDotColor", "0x000000"},
    {"CADspritedAnchor", "0x00ffff"},
    {"CADpinColor", "0x000000"},
    {"CADportBkColor", "0xffffff"},
    {"CADhierarchalBlock", "0xff0000"},
    {"CADhierarchalFill", "0xc8c8c8"},
    {"CADunconnectedPin", "0xffffff"},
    {"CADbusTapError", "0x0000ff"},
    {"JunctionSize", "1"},
    {"CADlineWidth", "1.5"},
    {"BUSwidthMultiplier", "2"},
    {"DefaultTextSize", "1"},
};

// The stock text for `key`; empty if it has none (a programming error).
QString stockText(const char *key) {
  for (const StockValue &stock : kStockDrawingValues)
    if (qstrcmp(stock.name, key) == 0) return QString::fromLatin1(stock.text);

  qWarning("QSpice preferences: no stock value for %s", key);
  return QString();
}

quint32 color(const char *key) {
  quint32 bgr = 0;
  bgrFromString(stockText(key), bgr);
  bgrFromString(values.value(QString::fromLatin1(key)), bgr);
  return bgr;
}

double number(const char *key) {
  bool         ok    = false;
  const double value = values.value(QString::fromLatin1(key)).toDouble(&ok);
  if (ok) return value;

  return stockText(key).toDouble();
}

} // namespace

// PrefsWriter's way to keep our copy in step with what it wrote.
// Deliberately NOT in the header: nothing else should change the copy.
namespace detail {
void remember(const QString &name, const QString &value) {
  values.insert(name, value);
}
} // namespace detail

bool bgrFromString(const QString &text, quint32 &bgr) {
  QString hex = text.trimmed();
  if (hex.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) hex = hex.mid(2);
  if (hex.isEmpty()) return false;

  bool       ok = false;
  const uint v  = hex.toUInt(&ok, 16);
  if (!ok) return false;

  bgr = static_cast<quint32>(v & 0xFFFFFFu);
  return true;
}

QString keyPathFor(const QString &vendorKey) {
  return vendorKey + QString::fromLatin1(kPreferencesSubkey);
}

void load(const QString &vendorKey) {
  values.clear();
  wasLoaded = false;

  loadedKeyPath = keyPathFor(vendorKey);

  QSettings settings(loadedKeyPath, QSettings::NativeFormat);

  const QStringList keys = settings.childKeys();
  for (const QString &key : keys)
    values.insert(key, settings.value(key).toString());

  // "Loaded" means the key existed and had something in it. An empty
  // read is indistinguishable from a missing key through QSettings, and
  // both mean the same thing here: use the stock defaults.
  wasLoaded = !keys.isEmpty();

  if (wasLoaded)
    qInfo("QSpice preferences: %lld value(s) read from the registry",
          static_cast<long long>(keys.size()));
  else
    qInfo("QSpice preferences: registry key not found -- using stock "
          "QSpice defaults");
}

bool loaded() { return wasLoaded; }

void fillMissing(const QHash<QString, QString> &fallbackValues) {
  int added = 0;
  for (QHash<QString, QString>::const_iterator it = fallbackValues.constBegin();
       it != fallbackValues.constEnd(); ++it) {
    if (values.contains(it.key())) continue;
    if (it.value().trimmed().isEmpty()) continue;

    values.insert(it.key(), it.value().trimmed());
    ++added;
  }

  if (added > 0)
    qInfo("QSpice preferences: %d value(s) taken from the program's settings", added);
}

QList<QPair<QString, QString>> stockDrawingValues() {
  QList<QPair<QString, QString>> list;
  for (const StockValue &stock : kStockDrawingValues)
    list.append(qMakePair(QString::fromLatin1(stock.name), QString::fromLatin1(stock.text)));
  return list;
}

const QString &keyPath() { return loadedKeyPath; }

quint32 highlightColor() { return color("CADHighlightColor"); }
quint32 backgroundColor() { return color("CADBackgroundColor"); }
quint32 gridColor() { return color("CADbomberSight"); }
quint32 graphicAnnotation() { return color("CADgrapicAnnotation"); }
quint32 solidAnnotationFill() { return color("CADsolidAnnotation"); }
quint32 graphicText() { return color("CADgrapicText"); }
quint32 componentText() { return color("CADnormalText"); }
quint32 schematicText() { return color("CADnormalSchText"); }
quint32 commentText() { return color("CADcommentText"); }
quint32 netLabelText() { return color("CADnetLabelText"); }
quint32 wireColor() { return color("CADwireColor"); }
quint32 highlightWireColor() { return color("CADhighlightWireColor"); }
quint32 pinDotColor() { return color("CADpinDotColor"); }
quint32 spritedAnchor() { return color("CADspritedAnchor"); }
quint32 pinColor() { return color("CADpinColor"); }
quint32 portBkColor() { return color("CADportBkColor"); }
quint32 hierarchicalBlock() { return color("CADhierarchalBlock"); }
quint32 hierarchicalFill() { return color("CADhierarchalFill"); }
quint32 unconnectedPin() { return color("CADunconnectedPin"); }
quint32 busTapError() { return color("CADbusTapError"); }

double junctionSize() { return number("JunctionSize"); }
double cadLineWidth() { return number("CADlineWidth"); }
double busWidthMultiplier() { return number("BUSwidthMultiplier"); }
double defaultTextSize() { return number("DefaultTextSize"); }

bool fastMath() {
  const QString value = values.value(QStringLiteral("FastMath")).trimmed();
  if (value.isEmpty()) return true;

  return value.compare(QStringLiteral("false"), Qt::CaseInsensitive) != 0;
}

QStringList userSymbolDirectories() {
  const QStringList pieces =
      values.value(kUserSymDirsKey).split(QLatin1Char('|'), Qt::SkipEmptyParts);

  QStringList dirs;
  QStringList seen; // normalised, for the duplicate check

  for (const QString &piece : pieces) {
    const QString path = piece.trimmed();
    if (path.isEmpty()) continue;

    const QString key = dirKey(path);
    if (seen.contains(key)) continue;

    seen.append(key);
    dirs.append(path);
  }

  return dirs;
}

bool sameDirectory(const QString &a, const QString &b) {
  return dirKey(a) == dirKey(b);
}

} // namespace sch::link::prefs
