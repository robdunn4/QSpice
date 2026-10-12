#pragma once

#include <QIcon>
#include <QPixmap>

// The program's own icon, read straight from the .exe -- the resource
// named IDI_ICON1 in the program's .rc -- so a menu or the About box can
// never disagree with Explorer or the taskbar. Windows picks the nearest
// size stored in the .ico. Off Windows, or when the resource is missing,
// both return null.
namespace sch::app {

// At `size` LOGICAL pixels, loaded at the screen's scale so it is sharp
// on a high-DPI display.
QPixmap appIconPixmap(int size);

// In every size a menu or toolbar asks for at 100% to 200% scaling
// (16 to 64 device pixels), for actions. Qt picks the size that fits.
QIcon appIcon();

} // namespace sch::app
