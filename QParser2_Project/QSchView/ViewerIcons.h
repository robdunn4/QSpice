#pragma once

#include <QIcon>

// QSchView's menu and toolbar icons, drawn in the program icon's colours
// (navy line work on white) and kept here as SVG text -- no image files,
// no resource file.
//
// Each icon is rendered at every size a menu or toolbar asks for at 100%
// to 200% scaling, so none is a stretched bitmap. Disabled states are
// Qt's own greyed versions.
namespace ViewerIcons {

QIcon open();
QIcon recent();
QIcon print();
QIcon exportSvg();
QIcon exitApp();
QIcon zoomIn();
QIcon zoomOut();
QIcon zoomToFit();
QIcon help();

// Options
QIcon resistorPolarity();
QIcon printDefaults();
QIcon colorTheme();
QIcon headerFooter();

} // namespace ViewerIcons
