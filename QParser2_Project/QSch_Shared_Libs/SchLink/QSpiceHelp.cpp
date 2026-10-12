#include "QSpiceHelp.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QObject>
#include <QSettings>

#include "QSpiceInstall.h"

#ifdef Q_OS_WIN
// Before windows.h, always -- it defines min and max as macros.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace sch::link::help {
namespace {

constexpr const char *kChmName = "QSPICE.chm";

// HTML Help commands, from htmlhelp.h -- declared here rather than
// including it, so the build needs no HTML Help SDK.
constexpr unsigned kHhDisplayTopic = 0x0000;
constexpr unsigned kHhCloseAll     = 0x0012;

// The device pages whose names do NOT follow the letter rule.
//
// The six non-letter symbol types, plus a note: QSpice's own help calls
// the A-ring page "Atilde-device.htm" -- Ã for Å -- so the FILE name is
// what is used here, not the character it ought to be.
const QHash<QString, QString> &specialPages() {
  static const QHash<QString, QString> pages = {
      {QStringLiteral("\u00C5"), QStringLiteral("Atilde-device.htm")},
      {QStringLiteral("\u00A5"), QStringLiteral("Yen-device.htm")},
      {QStringLiteral("\u20AC"), QStringLiteral("Euro-device.htm")},
      {QStringLiteral("\u00A3"), QStringLiteral("Sterling-device.htm")},
      {QStringLiteral("\u00D8(.dll)"), QStringLiteral("Oslash-device.htm")},
      {QStringLiteral("\u00D7"), QStringLiteral("Cross-device.htm")},

      // A blank type is a HIERARCHICAL schematic, which has no device
      // page of its own.
      {QString(), QStringLiteral("Hierarchy.htm")},
  };

  return pages;
}

// Overrides from the index file -- see loadOverrides().
QHash<QString, QString> &overrides() {
  static QHash<QString, QString> loaded;
  return loaded;
}

#ifdef Q_OS_WIN

// HtmlHelpW, found at run time.
using HtmlHelpFn = HWND(WINAPI *)(HWND, LPCWSTR, UINT, DWORD_PTR);

HtmlHelpFn htmlHelp() {
  static HtmlHelpFn fn     = nullptr;
  static bool       tried  = false;

  if (tried) return fn;

  tried = true;

  // hhctrl.ocx is a normal DLL despite the extension, and is present on
  // every Windows that has HTML Help.
  if (const HMODULE lib = LoadLibraryW(L"hhctrl.ocx"))
    fn = reinterpret_cast<HtmlHelpFn>(GetProcAddress(lib, "HtmlHelpW"));

  return fn;
}

#endif // Q_OS_WIN

} // namespace

QString chmPath() {
  return install::filePath(QLatin1String(kChmName));
}

void loadOverrides(const QString &iniPath) {
  overrides().clear();

  if (iniPath.isEmpty() || !QFileInfo::exists(iniPath)) return;

  QSettings settings(iniPath, QSettings::IniFormat);

  settings.beginGroup(QStringLiteral("SymbolTypes"));
  for (const QString &key : settings.childKeys())
    overrides().insert(key, settings.value(key).toString());
  settings.endGroup();

  qWarning().noquote() << QObject::tr("Help index overrides: %1 entr(ies) from %2")
                              .arg(overrides().size())
                              .arg(QDir::toNativeSeparators(iniPath));
}

QString pageForSymbolType(const QString &type) {
  const QString key = type.trimmed();

  // OVERRIDES first, so a renamed page can be corrected without a
  // rebuild -- including one that the rule below would otherwise get
  // right.
  const QHash<QString, QString>::const_iterator patched =
      overrides().constFind(key);

  if (patched != overrides().constEnd()) return patched.value();

  const QHash<QString, QString>::const_iterator special =
      specialPages().constFind(key);

  if (special != specialPages().constEnd()) return special.value();

  if (key.isEmpty()) return QString();

  // The RULE: the page is named after the SPICE letter, and a trailing
  // N or P is a polarity, not part of the device -- QN and QP are both
  // the bipolar transistor.
  QString letter = key;
  if (letter.size() == 2 &&
      (letter.endsWith(QLatin1Char('N'), Qt::CaseInsensitive) ||
       letter.endsWith(QLatin1Char('P'), Qt::CaseInsensitive)))
    letter.chop(1);

  if (letter.size() != 1 || !letter.at(0).isLetter()) {
    qWarning().noquote()
        << QObject::tr("No help page known for symbol type \"%1\"").arg(type);
    return QString();
  }

  return letter.toUpper() + QStringLiteral("-device.htm");
}

QString show(const QString &pageName) {
  const QString chm = chmPath();

  if (chm.isEmpty() || !QFileInfo::exists(chm))
    return QObject::tr("The QSpice help file was not found:\n\n%1")
        .arg(chm.isEmpty() ? QObject::tr("(installation folder unknown)")
                           : QDir::toNativeSeparators(chm));

#ifdef Q_OS_WIN
  const HtmlHelpFn fn = htmlHelp();
  if (fn == nullptr)
    return QObject::tr("HTML Help is not available on this machine, so the "
                       "help file cannot be shown.");

  // "<chm>::/html/<page>" addresses one topic; the file alone opens at
  // the default page.
  QString target = QDir::toNativeSeparators(chm);
  if (!pageName.isEmpty()) target += QStringLiteral("::/html/") + pageName;

  // A null parent HWND, deliberately: the help window is a TOP-LEVEL
  // window of this process, not a child of ours. It still closes with
  // us (closeAll(), at shutdown), but it can sit behind the main window
  // like any other program's window rather than floating over it.
  const HWND shown = fn(nullptr, reinterpret_cast<LPCWSTR>(target.utf16()),
                        kHhDisplayTopic, 0);
  if (shown != nullptr) return QString();

  return QObject::tr("The help topic could not be shown:\n\n%1").arg(target);
#else
  Q_UNUSED(pageName);
  return QObject::tr("The QSpice help file can only be shown on Windows.");
#endif
}

void closeAll() {
#ifdef Q_OS_WIN
  if (const HtmlHelpFn fn = htmlHelp()) fn(nullptr, nullptr, kHhCloseAll, 0);
#endif
}

} // namespace sch::link::help
