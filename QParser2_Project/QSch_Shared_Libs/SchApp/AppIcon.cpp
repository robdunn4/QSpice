#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "AppIcon.h"

#include <QGuiApplication>
#include <QImage>
#include <QtGlobal>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace sch::app {
namespace {

// The sizes, in device pixels, gathered into appIcon(). Every one is
// stored in the .ico, so none is resampled.
constexpr int kMenuIconPx[] = {16, 20, 24, 32, 40, 48, 64};

// The icon at `px` DEVICE pixels. Null if it cannot be loaded.
QPixmap loadAppIcon(int px) {
#ifdef Q_OS_WIN
  const HICON handle = static_cast<HICON>(
      LoadImageW(GetModuleHandleW(nullptr), L"IDI_ICON1", IMAGE_ICON, px, px, LR_DEFAULTCOLOR));
  if (handle == nullptr) return QPixmap();

  const QPixmap pixmap = QPixmap::fromImage(QImage::fromHICON(handle));
  DestroyIcon(handle);
  return pixmap;
#else
  Q_UNUSED(px);
  return QPixmap();
#endif
}

} // namespace

QPixmap appIconPixmap(int size) {
  const qreal dpr    = qApp->devicePixelRatio();
  QPixmap     pixmap = loadAppIcon(qRound(size * dpr));
  if (pixmap.isNull()) return pixmap;

  pixmap.setDevicePixelRatio(dpr);
  return pixmap;
}

QIcon appIcon() {
  QIcon icon;
  for (int px : kMenuIconPx) {
    const QPixmap pixmap = loadAppIcon(px);
    if (!pixmap.isNull()) icon.addPixmap(pixmap);
  }
  return icon;
}

} // namespace sch::app
