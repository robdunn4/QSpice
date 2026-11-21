#include "QSpicePrefsWriter.h"

#include <QDebug>
#include <QSettings>

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
  return write(QStringLiteral("UserSymbolDirectories"),
               prefs::userSymbolDirectoriesText(dirs));
}

bool PrefsWriter::write(const QString &name, const QString &value) {
  prefs::detail::remember(name, value);

  if (keyPath.isEmpty()) return false;

  QSettings settings(keyPath, QSettings::NativeFormat);

  const QString old = settings.value(name).toString();
  qInfo().noquote()
      << QStringLiteral(
             "QSpice preferences: WRITING %1\\%2\n  was: %3\n  now: %4")
             .arg(keyPath, name, old, value);

  settings.setValue(name, value); // REG_SZ, as QSpice stores it
  settings.sync();

  if (settings.status() == QSettings::NoError) return true;

  qWarning("QSpice preferences: could not write %s", qPrintable(name));
  return false;
}

} // namespace sch::link
