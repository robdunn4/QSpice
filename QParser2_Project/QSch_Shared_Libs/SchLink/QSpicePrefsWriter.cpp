#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "QSpicePrefsWriter.h"

#include <QDebug>
#include <QDir>
#include <QSettings>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "QSpicePrefs.h"

// Defined in QSpicePrefs.cpp; deliberately not in any header.
namespace sch::link::prefs::detail {
void remember(const QString &name, const QString &value);
} // namespace sch::link::prefs::detail

namespace sch::link {

PrefsWriter::PrefsWriter(const QString &vendorKey) {
  // An empty vendor key writes nothing -- write() then only updates our
  // copy -- rather than "\QSPICE\[Preferences]" under some default root.
  if (!vendorKey.trimmed().isEmpty()) keyPath = prefs::keyPathFor(vendorKey);
}

bool PrefsWriter::setUserSymbolDirectories(const QStringList &dirs) {
  // QSpice's form: every path enclosed in bars, so two paths meet as
  // "||". An empty list is an empty string.
  QString value;
  for (const QString &dir : dirs)
    value += QLatin1Char('|') + QDir::toNativeSeparators(dir.trimmed()) +
             QLatin1Char('|');

  return write(QStringLiteral("UserSymbolDirectories"), value);
}

bool PrefsWriter::write(const QString &name, const QString &value) {
  prefs::detail::remember(name, value);

  if (keyPath.isEmpty()) return false;

  QSettings settings(keyPath, QSettings::NativeFormat);

  const QString old = settings.value(name).toString();
  qInfo().noquote() << QStringLiteral("QSpice preferences: WRITING %1\\%2\n  was: %3\n  now: %4")
                           .arg(keyPath, name, old, value);

  settings.setValue(name, value); // REG_SZ, as QSpice stores it
  settings.sync();

  if (settings.status() == QSettings::NoError) return true;

  qWarning("QSpice preferences: could not write %s", qPrintable(name));
  return false;
}

} // namespace sch::link
