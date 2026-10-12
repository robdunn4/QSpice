#include "IpcMsg.h"

#include "SchStringDecode.h"

#include <QStringList>

namespace sch::link::ipcmsg {

bool TextBuf::add(quint64 wParam, qint64 lParam, QString &text) {
  const quint32 lo = static_cast<quint32>(wParam & 0xFFFFFFFFu);
  const quint32 hi = static_cast<quint32>(static_cast<quint64>(lParam) &
                                          0xFFFFFFFFu);

  if (lo == 0 && hi == 0) {
    text = sch::core::decodeCp1252(pending);
    pending.clear();
    return true;
  }

  // Low byte first; stop at the first zero, which ends the text early.
  for (const quint32 word : {lo, hi}) {
    for (int shift = 0; shift < 32; shift += 8) {
      const char byte = static_cast<char>((word >> shift) & 0xFFu);
      if (byte == '\0') return false;
      pending.append(byte);
    }
  }

  return false;
}

Rect Rect::fromParams(quint64 wParam, qint64 lParam) {
  const quint64 l = static_cast<quint64>(lParam);

  Rect rect;
  rect.left   = static_cast<qint32>(static_cast<quint32>(wParam & 0xFFFFFFFFu));
  rect.top    = static_cast<qint32>(static_cast<quint32>(wParam >> 32));
  rect.right  = static_cast<qint32>(static_cast<quint32>(l & 0xFFFFFFFFu));
  rect.bottom = static_cast<qint32>(static_cast<quint32>(l >> 32));
  return rect;
}

QString Rect::toString() const {
  return QStringLiteral("%1,%2,%3,%4").arg(left).arg(top).arg(right).arg(bottom);
}

Rect Rect::fromString(const QString &text) {
  const QStringList parts = text.split(QLatin1Char(','));
  if (parts.size() != 4) return Rect();

  int  values[4] = {};
  bool ok        = true;
  for (int i = 0; i < 4 && ok; ++i) values[i] = parts.at(i).trimmed().toInt(&ok);
  if (!ok) return Rect();

  Rect rect;
  rect.left   = values[0];
  rect.top    = values[1];
  rect.right  = values[2];
  rect.bottom = values[3];
  return rect;
}

} // namespace sch::link::ipcmsg
