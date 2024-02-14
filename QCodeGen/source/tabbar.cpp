#include "tabbar.h"

TabBar::TabBar(QWidget *parent) : QTabBar(parent) {}

void TabBar::clear() {
  while (count())
    removeTab(0);
}
