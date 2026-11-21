#include "AppStyle.h"

#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QGuiApplication>
#include <QObject>
#include <QPainter>
#include <QSize>
#include <QStyleOptionMenuItem>
#include <QStyleOptionToolButton>
#include <QStyleOptionViewItem>
#include <QToolButton>
#include <QWidget>
#include <qdrawutil.h>

#include <algorithm>

namespace sch::app {

namespace {

// The held-down fill: the button's own background, darkened, so it reads
// on light and dark palettes alike.
constexpr int kDownDarker = 125; // QColor::darker() factor, percent

// Lets go of any toolbar button held down when the window, or the whole
// application, loses the focus. The button keeps the mouse while it is
// held, but a release that happens in another application -- or behind a
// window that popped up -- may never reach it, and it would stay drawn
// pressed. Popped up with setDown(false): a release arriving later finds
// it no longer down and does nothing, so nothing fires.
//
// An application-wide event filter. No Q_OBJECT needed: eventFilter() is
// an ordinary virtual.
class HeldButtonWatch : public QObject {
public:
  using QObject::QObject;

  bool eventFilter(QObject *watched, QEvent *event) override {
    const QEvent::Type type = event->type();

    const bool appInactive =
        (type == QEvent::ApplicationStateChange) &&
        (QGuiApplication::applicationState() != Qt::ApplicationActive);
    if (type == QEvent::WindowDeactivate || appInactive) releaseHeld();

    return QObject::eventFilter(watched, event);
  }

protected:
  static void releaseHeld() {
    for (QWidget *widget : QApplication::allWidgets()) {
      QToolButton *button = qobject_cast<QToolButton *>(widget);
      if (button == nullptr || !button->isDown()) continue;

      button->setDown(false);
      if (QWidget::mouseGrabber() == button) button->releaseMouse();
    }
  }
};

} // namespace

void AppStyle::install() {
  QStyle *current = QApplication::style();
  if (current == nullptr) return;

  // By NAME, not by pointer: QProxyStyle takes ownership of a style it is
  // given, and the current one belongs to the application.
  QApplication::setStyle(new AppStyle(current->name()));

  // Owned by the application, and filtering for its whole life.
  if (QApplication *app =
          qobject_cast<QApplication *>(QApplication::instance()))
    app->installEventFilter(new HeldButtonWatch(app));
}

void AppStyle::drawComplexControl(ComplexControl             control,
                                  const QStyleOptionComplex *option,
                                  QPainter                  *painter,
                                  const QWidget             *widget) const {
  const QStyleOptionToolButton *button =
      qstyleoption_cast<const QStyleOptionToolButton *>(option);

  if (control != CC_ToolButton || button == nullptr) {
    QProxyStyle::drawComplexControl(control, option, painter, widget);
    return;
  }

  const bool held    = button->state.testFlag(State_Sunken);
  const bool checked = button->state.testFlag(State_On);

  if (!held && !checked) {
    QProxyStyle::drawComplexControl(control, option, painter, widget);
    return;
  }

  // HELD DOWN: the darker fill first, under everything...
  if (held) {
    const QColor fill = button->palette.button().color().darker(kDownDarker);
    painter->fillRect(button->rect, fill);
  }

  // ...then the style's own drawing as if neither pressed nor checked --
  // its icon and text, without its pressed shade or its checked fill...
  QStyleOptionToolButton plain(*button);
  plain.state &= ~(State_On | State_Sunken);
  if (held) plain.state &= ~State_MouseOver; // no hover fill over ours
  QProxyStyle::drawComplexControl(control, &plain, painter, widget);

  // ...then pressed in: 2 pixels while held, 1 while merely checked.
  qDrawShadePanel(painter, button->rect, button->palette, /*sunken=*/true,
                  /*lineWidth=*/held ? 2 : 1, /*fill=*/nullptr);
}

QSize AppStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                 const QSize   &contentsSize,
                                 const QWidget *widget) const {
  QSize size =
      QProxyStyle::sizeFromContents(type, option, contentsSize, widget);
  if (option == nullptr) return size;

  // A list, tree or table row: its text, or its icon if taller.
  if (type == CT_ItemViewItem) {
    int content = option->fontMetrics.height();
    if (const QStyleOptionViewItem *item =
            qstyleoption_cast<const QStyleOptionViewItem *>(option))
      if (item->features.testFlag(QStyleOptionViewItem::HasDecoration))
        content = std::max(content, item->decorationSize.height());

    size.setHeight(std::min(size.height(), content + kRowPadding));
    return size;
  }

  // A menu item, the same way. A separator keeps the style's own.
  if (type == CT_MenuItem) {
    const QStyleOptionMenuItem *item =
        qstyleoption_cast<const QStyleOptionMenuItem *>(option);
    if (item == nullptr ||
        item->menuItemType == QStyleOptionMenuItem::Separator)
      return size;

    int content = option->fontMetrics.height();
    if (!item->icon.isNull())
      content =
          std::max(content, pixelMetric(PM_SmallIconSize, option, widget));

    size.setHeight(std::min(size.height(), content + kMenuPadding));
    return size;
  }

  return size;
}

} // namespace sch::app
