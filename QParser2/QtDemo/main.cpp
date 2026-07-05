//-----------------------------------------------------------------------------
// This file is part of the the QParser2 project.  You can find the complete
// project here:  https://github.com/robdunn4/QSpice/
//-----------------------------------------------------------------------------
#include "QtDemo.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QtDemo       window;
  window.show();
  return app.exec();
}
