#pragma once

#include <QString>
#include <QStringList>

// The ONLY way to write into QSpice's registry settings.
//
// Kept apart from QSpicePrefs.h on purpose: a program that never
// includes this header cannot write QSpice's settings at all, and the
// compiler, not a convention, guarantees it. The print/export tool
// never includes it.
//
// Deliberate at every step:
//   - the vendor key is given to the constructor, with no default, so a
//     write cannot quietly go wherever the last load() happened to read;
//   - every write is LOGGED (qInfo), naming the value, the old text and
//     the new;
//   - there is one method per value written. Nothing writes an
//     arbitrary name.
//
// Writes also update the copy QSpicePrefs hands out, so a read after a
// write sees the new value.
//
// REVIEW: this writes into QSPICE'S settings, shared with QSpice itself.
// A QSpice running at the same time may overwrite them with its own
// copy when it saves its preferences, and vice versa.
namespace sch::link {

class PrefsWriter {
public:
  // An EMPTY vendor key makes a writer that writes nothing to the
  // registry: writes then update our copy only, and return false.
  explicit PrefsWriter(const QString &vendorKey);

  // UserSymbolDirectories, in QSpice's own |path||path| form. Our copy
  // is updated regardless; false if the registry write failed.
  bool setUserSymbolDirectories(const QStringList &dirs);

protected:
  bool write(const QString &name, const QString &value);

  QString keyPath; // "<vendor key>\QSPICE\[Preferences]"
};

} // namespace sch::link
