#pragma once

#include <QString>

#include <functional>

class QWinEventNotifier;

// Watches QSpice's [Preferences] registry key and calls back when
// anything in it changes -- so a program keeping its own copies of
// QSpice's values (QSchEdit's QSpiceShadow) can notice QSpice changing
// one while both are running.
//
// Windows watches a whole KEY, not single values, and says only THAT it
// changed, not what: the callback re-reads and compares. Only
// [Preferences] itself is watched, not its subkeys -- QSpice's window
// layout ("Workspace"), recent files and the rest live elsewhere and
// never wake it.
//
// QSpice may write several values in a burst (its Preferences dialog's
// OK); the callback then comes once per change, so the receiver waits
// for the burst to end before acting.
//
// Delivered in the main thread, through Qt's event loop. Does nothing on
// a non-Windows build (start() returns false).
//
// Deliberately has no Qt meta-object macro: the notifier's signal is
// connected to a lambda, so the header needs no moc step.
namespace sch::link {

class PrefsWatch {
public:
  PrefsWatch() = default;
  ~PrefsWatch();

  PrefsWatch(const PrefsWatch &)            = delete;
  PrefsWatch &operator=(const PrefsWatch &) = delete;

  // Starts watching the key at `keyPath` -- prefs::keyPath(), the
  // registry path QSettings uses ("HKEY_CURRENT_USER\..."). Any earlier
  // watch is stopped first. False if the key cannot be opened.
  bool start(const QString &keyPath, std::function<void()> changed);

  void stop();

  bool watching() const { return notifier != nullptr; }

protected:
  // Asks Windows for the next change. Each request reports one change,
  // so it is renewed after every notice.
  bool arm();

  std::function<void()> onChange;
  QWinEventNotifier    *notifier = nullptr;
  void                 *key      = nullptr; // HKEY
  void                 *event    = nullptr; // HANDLE
};

} // namespace sch::link
