#pragma once

#include <QProxyStyle>
#include <QString>

// The application's own style: the native one, with two changes.
//
// TOOLBAR BUTTONS are drawn plainly in two states the native style shows
// faintly or not at all:
//
//   held down   a darker fill and a 2-pixel sunken edge, whether or not
//               the button is checked -- so every click shows, even on a
//               toggle that is already on
//   checked     pressed in: a 1-pixel sunken edge in the palette's colors
//               rather than the style's highlight fill (a toggle on a
//               toolbar, such as Show Grid)
//
// Hover and unchecked, released buttons are drawn exactly as the style
// draws them.
//
// ROWS are kept tight (Robert, 2026-11-20). Windows 11's style pads every
// list, tree and table row and every menu item generously -- right for
// touch, mostly empty space in a list of pins or a context menu. A row
// is made no taller than its text (or icon) and kRowPadding; a menu item
// than its text (or icon) and kMenuPadding. Separators, and a style whose
// rows are already that tight, are left alone. Every list, drop-down,
// table, context menu and menu-bar menu in the program.
//
// install() also sees to a button held down when the window loses the
// focus (Alt-Tab, a window popping up): its release may never arrive, so
// it is let go -- popped up WITHOUT firing -- rather than left drawn
// pressed.
//
// Shared by QSchEdit and QSchView. Deliberately has NO Qt meta-object
// macro: no signals, no slots, so the header needs no moc. Once named
// SunkenToolStyle, when toolbar buttons were all it changed.
namespace sch::app {

class AppStyle : public QProxyStyle {
public:
  // Wraps the style named `baseKey` (QStyleFactory's names).
  explicit AppStyle(const QString &baseKey) : QProxyStyle(baseKey) {}

  // Wraps the application's current style and makes it the application's
  // style, and starts watching for the focus being lost. Call once, after
  // the QApplication exists and before any window is made.
  static void install();

  void drawComplexControl(ComplexControl             control,
                          const QStyleOptionComplex *option, QPainter *painter,
                          const QWidget *widget = nullptr) const override;

  QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                         const QSize   &contentsSize,
                         const QWidget *widget = nullptr) const override;

protected:
  // Above and below the text together, in pixels: a list, tree or table
  // row, and a menu item.
  static constexpr int kRowPadding  = 4;
  static constexpr int kMenuPadding = 8;
};

} // namespace sch::app
