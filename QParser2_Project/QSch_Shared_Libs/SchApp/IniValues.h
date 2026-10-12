#pragma once

#include <QString>
#include <QStringList>

class QSettings;

// Self-documenting settings files, shared by every program's .ini.
//
// Every preference is WRITTEN even when it holds its default, so a user
// opening the file can see what exists and how each key is spelled --
// settings nobody knows about might as well not be settings:
//
//     GridSpacing=50 // default: 100
//     SnapToGrid=[] // default: true
//
// "[]" means "the default": the file shows at a glance what has actually
// been changed. An EMPTY value reads as the default too, so a user who
// clears one by hand gets the default rather than a puzzle. (Not written
// empty, because QSettings quotes a value with leading whitespace.)
//
// Only a program's OWN preferences use this. Qt's geometry and state
// blobs are base64 -- an alphabet that includes '/', so cutting at a
// comment marker would corrupt them -- and are read and written as they
// are.
namespace sch::app::ini {

// The value with its "// default:" comment stripped, or `defValue` when
// it is empty or "[]". Also copes with a value written with no comment.
QString prefValue(QSettings &settings, const char *key, const QString &defValue);
double  prefDouble(QSettings &settings, const char *key, double defValue);
bool    prefBool(QSettings &settings, const char *key, bool defValue);

// Writes `value` with its default as a comment -- "[]" when it IS the
// default. While an OnlyMissing exists, a key already in the file is
// left alone.
void setPrefValue(QSettings &settings, const char *key, const QString &value,
                  const QString &defValue);
void setPrefDouble(QSettings &settings, const char *key, double value, double defValue);
void setPrefBool(QSettings &settings, const char *key, bool value, bool defValue);

// For as long as one of these exists, the setPref...() functions write
// only keys the file does not have yet. For writing every default the
// first time: one instance must never write its in-memory copy over a
// value another instance -- or the user, by hand -- has put there since
// it started.
class OnlyMissing {
public:
  OnlyMissing();
  ~OnlyMissing();

  OnlyMissing(const OnlyMissing &)            = delete;
  OnlyMissing &operator=(const OnlyMissing &) = delete;
};

// Whether an OnlyMissing exists -- for a program's own raw writes.
bool onlyMissing();

// --- A list of file paths (the recent list) ----------------------------
//
// Shared by every running instance, so each change is a read-modify-write
// of the file, never a write of one instance's copy: re-read what is
// there now, apply the one change, write it straight back.

// The array `group`, one "path" per entry.
QStringList readPaths(QSettings &settings, const char *group);

// Replaces the array `group` with `list`, and syncs.
void writePaths(QSettings &settings, const char *group, const QStringList &list);

// Removes every entry naming the same file as `path`, compared as
// Windows does -- without regard to case, and with either separator.
void removePath(QStringList &list, const QString &path);

} // namespace sch::app::ini
