#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>

class TabBar : public QTabBar {
public:
  TabBar(QWidget *parent = nullptr);

  void clear();
};

#endif // TABBAR_H
