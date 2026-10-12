#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "IniValues.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace sch::app::ini {
namespace {

// Written in place of a value that is simply the default.
constexpr const char *kDefaultMarker = "[]";

// The key each path is stored under in a path list.
constexpr const char *kPathKey = "path";

// How many OnlyMissing exist on this thread.
thread_local int onlyMissingDepth = 0;

QString boolText(bool value) { return value ? QStringLiteral("true") : QStringLiteral("false"); }

} // namespace

QString prefValue(QSettings &settings, const char *key, const QString &defValue) {
  const QString   raw   = settings.value(key).toString();
  const qsizetype at    = raw.indexOf(QStringLiteral("//"));
  const QString   value = (at < 0) ? raw.trimmed() : raw.left(at).trimmed();

  if (value.isEmpty() || value == QLatin1String(kDefaultMarker)) return defValue;
  return value;
}

double prefDouble(QSettings &settings, const char *key, double defValue) {
  bool         ok    = false;
  const double value = prefValue(settings, key, QString::number(defValue)).toDouble(&ok);
  return ok ? value : defValue;
}

bool prefBool(QSettings &settings, const char *key, bool defValue) {
  return prefValue(settings, key, boolText(defValue))
             .compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
}

void setPrefValue(QSettings &settings, const char *key, const QString &value,
                  const QString &defValue) {
  if (onlyMissing() && settings.contains(key)) return;

  const QString shown = (value == defValue) ? QString::fromLatin1(kDefaultMarker) : value;
  settings.setValue(key, shown + QStringLiteral(" // default: ") + defValue);
}

void setPrefDouble(QSettings &settings, const char *key, double value, double defValue) {
  setPrefValue(settings, key, QString::number(value), QString::number(defValue));
}

void setPrefBool(QSettings &settings, const char *key, bool value, bool defValue) {
  setPrefValue(settings, key, boolText(value), boolText(defValue));
}

OnlyMissing::OnlyMissing() { ++onlyMissingDepth; }
OnlyMissing::~OnlyMissing() { --onlyMissingDepth; }

bool onlyMissing() { return onlyMissingDepth > 0; }

QStringList readPaths(QSettings &settings, const char *group) {
  QStringList list;
  const int   count = settings.beginReadArray(group);
  for (int i = 0; i < count; ++i) {
    settings.setArrayIndex(i);
    list.append(settings.value(kPathKey).toString());
  }
  settings.endArray();
  return list;
}

void writePaths(QSettings &settings, const char *group, const QStringList &list) {
  settings.remove(group); // a shorter list leaves no tail
  settings.beginWriteArray(group);
  for (int i = 0; i < list.size(); ++i) {
    settings.setArrayIndex(i);
    settings.setValue(kPathKey, list.at(i));
  }
  settings.endArray();
  settings.sync();
}

void removePath(QStringList &list, const QString &path) {
  const QString key = QDir::cleanPath(QFileInfo(path).absoluteFilePath());

  for (qsizetype i = list.size() - 1; i >= 0; --i) {
    const QString other = QDir::cleanPath(QFileInfo(list.at(i)).absoluteFilePath());
    if (other.compare(key, Qt::CaseInsensitive) == 0) list.removeAt(i);
  }
}

} // namespace sch::app::ini
