#pragma once

#include <QStringList>

// QSchView's command line: print and export ONE file with no window,
// report what was done, and return a result code.
//
//   QSchView <file.qsch> [--print[=PRINTER]] [--pdf=FILE] [--svg=FILE]
//                        [--fit | --scale=MM] [--orientation=O] [--paper=P]
//                        [--header | --no-header] [--footer | --no-footer]
//                        [--background | --no-background]
//                        [--copies=N]
//   QSchView --list-printers
//   QSchView --help | --version
//
// Run from a console through QSchView.com, which waits and passes the
// messages and the result code through. Messages go to stdout, errors
// to stderr, and both to QSchView.log.
namespace CliRun {

// Result codes.
enum Result : int {
  kOk            = 0,
  kBadArguments  = 1, // unknown option, missing value, no file, ...
  kBadInput      = 2, // the file cannot be read, is not a schematic, or is a .qsym
  kPrinterFailed = 3, // the printer is not installed or cannot be opened
  kOutputFailed  = 4, // a page or file could not be written, or too many pages
};

// Whether `args` (the program name left off) ask for the command line
// rather than a window: any argument that is an option.
bool wanted(const QStringList &args);

// Does what `args` ask. QApplication must exist, and the drawing style
// must already be set (RenderStyle); no window is shown.
int run(const QStringList &args);

} // namespace CliRun
