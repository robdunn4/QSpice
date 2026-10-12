#pragma once

#include <QString>

class QWidget;

// The About box every program shows: its name and version, the shared
// libraries it was linked with -- each as it reports itself -- and the
// program's own icon beside the text.
namespace sch::app {

// The libraries, one row each (name, version), lowest first: each depends
// only on those above it. Rich text, for QMessageBox.
QString libraryVersionsHtml();

// Shows the box, modal. `version` as the program formats it ("0.1.0", or
// "0.1.0 (DEBUG)").
void showAbout(QWidget *parent, const QString &programName, const QString &version);

} // namespace sch::app
