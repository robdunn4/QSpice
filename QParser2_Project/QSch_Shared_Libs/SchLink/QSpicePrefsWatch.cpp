#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "QSpicePrefsWatch.h"

#include <QDebug>

#ifdef Q_OS_WIN
#include <QWinEventNotifier>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef Q_OS_WIN
// Before windows.h, always: it defines min and max as macros otherwise.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef _MSC_VER
#pragma comment(lib, "advapi32.lib") // RegOpenKeyExW, RegNotifyChangeKeyValue
#endif
#endif

namespace sch::link {

PrefsWatch::~PrefsWatch() { stop(); }

#ifdef Q_OS_WIN

namespace {

// QSettings' path names the hive in full; the API takes the hive as a
// handle and the rest as the subkey.
bool splitHive(const QString &keyPath, HKEY &hive, QString &subkey) {
  const QString hkcu = QStringLiteral("HKEY_CURRENT_USER\\");
  const QString hklm = QStringLiteral("HKEY_LOCAL_MACHINE\\");

  if (keyPath.startsWith(hkcu, Qt::CaseInsensitive)) {
    hive   = HKEY_CURRENT_USER;
    subkey = keyPath.mid(hkcu.size());
    return true;
  }

  if (keyPath.startsWith(hklm, Qt::CaseInsensitive)) {
    hive   = HKEY_LOCAL_MACHINE;
    subkey = keyPath.mid(hklm.size());
    return true;
  }

  return false;
}

} // namespace

bool PrefsWatch::start(const QString &keyPath, std::function<void()> changed) {
  stop();

  HKEY    hive = nullptr;
  QString subkey;
  if (!splitHive(keyPath, hive, subkey)) {
    qWarning().noquote() << "QSpice settings watch: not a registry path:"
                         << keyPath;
    return false;
  }

  HKEY opened = nullptr;
  if (RegOpenKeyExW(hive, reinterpret_cast<LPCWSTR>(subkey.utf16()), 0,
                    KEY_NOTIFY | KEY_QUERY_VALUE, &opened) != ERROR_SUCCESS) {
    qWarning().noquote() << "QSpice settings watch: cannot open" << keyPath;
    return false;
  }

  // Auto-reset: one signal per notice.
  HANDLE created = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (created == nullptr) {
    RegCloseKey(opened);
    return false;
  }

  key      = opened;
  event    = created;
  onChange = std::move(changed);

  if (!arm()) {
    stop();
    return false;
  }

  notifier = new QWinEventNotifier(static_cast<HANDLE>(event));
  QObject::connect(notifier, &QWinEventNotifier::activated, notifier,
                   [this](HANDLE) {
                     // Renewed BEFORE the callback reads anything, so a
                     // change made while it reads is not missed.
                     arm();
                     if (onChange) onChange();
                   });
  notifier->setEnabled(true);

  qInfo().noquote() << "QSpice settings watch: watching" << keyPath;
  return true;
}

void PrefsWatch::stop() {
  delete notifier;
  notifier = nullptr;

  if (key != nullptr) RegCloseKey(static_cast<HKEY>(key));
  key = nullptr;

  if (event != nullptr) CloseHandle(static_cast<HANDLE>(event));
  event = nullptr;

  onChange = nullptr;
}

bool PrefsWatch::arm() {
  if (key == nullptr || event == nullptr) return false;

  // The key itself, not its subkeys; values written or added/removed.
  const LONG result = RegNotifyChangeKeyValue(static_cast<HKEY>(key), FALSE,
                                              REG_NOTIFY_CHANGE_LAST_SET |
                                                  REG_NOTIFY_CHANGE_NAME,
                                              static_cast<HANDLE>(event), TRUE);

  if (result == ERROR_SUCCESS) return true;

  qWarning("QSpice settings watch: could not renew the watch (%ld)", result);
  return false;
}

#else // not Windows: nothing to watch

bool PrefsWatch::start(const QString &keyPath, std::function<void()> changed) {
  Q_UNUSED(keyPath);
  Q_UNUSED(changed);
  return false;
}

void PrefsWatch::stop() {}

bool PrefsWatch::arm() { return false; }

#endif

} // namespace sch::link
