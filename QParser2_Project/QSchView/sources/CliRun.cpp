#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Qt headers reference APIs deprecated for Qt 7
#endif

#include "CliRun.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPrinter>
#include <QPrinterInfo>
#include <QString>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <cstdio>
#include <optional>

#include "SchDocSource.h"
#include "SchPrint.h"
#include "SchRecordRules.h"
#include "SchSvgExport.h"
#include "Version.h"
#include "ViewerPrefs.h"

// SchCore names used in this file.
namespace recordrules = sch::core::recordrules;

// SchRender names used in this file.
using sch::render::SchDocSource;

// SchOutput names used in this file.
using sch::output::SchPrint;
using sch::output::SchPrintLayout;
using sch::output::SchPrintSettings;
using sch::output::SchSvgExport;

namespace CliRun {
namespace {

// --- Output -------------------------------------------------------------

// A GUI program has no console of its own. Started by QSchView.com, its
// standard handles are the pipes the launcher reads, and the C runtime
// already writes there. Started straight from a console, it has none:
// then it attaches to that console, so the messages are at least seen
// (the prompt does not wait -- which is what QSchView.com is for).
void connectConsole() {
#ifdef Q_OS_WIN
  const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
  if (out != nullptr && out != INVALID_HANDLE_VALUE && GetFileType(out) != FILE_TYPE_UNKNOWN)
    return;

  if (!AttachConsole(ATTACH_PARENT_PROCESS)) return;

  FILE *ignored = nullptr;
  freopen_s(&ignored, "CONOUT$", "w", stdout);
  freopen_s(&ignored, "CONOUT$", "w", stderr);
#endif
}

void say(const QString &text) {
  qInfo().noquote() << text;
  std::fputs(qPrintable(text + QLatin1Char('\n')), stdout);
  std::fflush(stdout);
}

int fail(int code, const QString &text) {
  qWarning().noquote() << text;
  std::fputs(qPrintable(QStringLiteral("%1: error: %2\n")
                            .arg(QString::fromLatin1(Version::kProgramName), text)),
             stderr);
  std::fflush(stderr);
  return code;
}

QString usage() {
  const QString name = QString::fromLatin1(Version::kProgramName);
  return QStringLiteral(
             "Usage:\n"
             "  %1 FILE.qsch [options]     print and/or export one schematic\n"
             "  %1 --list-printers         list the installed printers\n"
             "  %1 --help | --version\n"
             "\n"
             "Output (at least one):\n"
             "  --print[=PRINTER]   print; without a name, on the saved printer, or the\n"
             "                      Windows default\n"
             "  --pdf=FILE          write a PDF\n"
             "  --svg=FILE          write an SVG (one drawing; no print settings apply)\n"
             "\n"
             "Page (otherwise from the Print settings in %1.ini):\n"
             "  --fit               the whole schematic, fitted to one page\n"
             "  --scale=MM          at scale: MM on paper per 1000 schematic units, on as\n"
             "                      many pages as that needs\n"
             "  --orientation=O     portrait, landscape or auto\n"
             "  --paper=P           A4, Letter, Legal, A3, Tabloid, ...\n"
             "  --header | --no-header\n"
             "  --footer | --no-footer\n"
             "  --background | --no-background\n"
             "                      the canvas colour behind the drawing (print and PDF)\n"
             "  --copies=N          printed copies\n"
             "\n"
             "Values may also follow as the next argument: --pdf out.pdf\n"
             "\n"
             "Result codes: 0 done, 1 bad arguments, 2 file cannot be used,\n"
             "3 printer cannot be used, 4 output failed.")
      .arg(name);
}

// --- Arguments ----------------------------------------------------------

struct Options {
  QString file;

  bool                   print = false;
  QString                printer; // empty: saved, or the Windows default
  QString                pdfPath;
  QString                svgPath;
  bool                   fit = false;
  std::optional<double>  scaleMm;
  QString                orientation; // empty, "portrait", "landscape", "auto"
  std::optional<QPageSize> paper;
  std::optional<bool>    header;
  std::optional<bool>    footer;
  std::optional<bool>    background;
  int                    copies = 1;

  bool help         = false;
  bool version      = false;
  bool listPrinters = false;
};

// The paper named `text` -- by Qt's key ("A4", "Letter") or its display
// name -- without regard to case. Invalid if there is none.
QPageSize paperNamed(const QString &text) {
  for (int id = 0; id <= static_cast<int>(QPageSize::LastPageSize); ++id) {
    const QPageSize::PageSizeId sizeId = static_cast<QPageSize::PageSizeId>(id);
    if (sizeId == QPageSize::Custom) continue;

    if (QPageSize::key(sizeId).compare(text, Qt::CaseInsensitive) == 0 ||
        QPageSize::name(sizeId).compare(text, Qt::CaseInsensitive) == 0)
      return QPageSize(sizeId);
  }
  return QPageSize();
}

// Fills `opts` from `args`; empty on success, otherwise what was wrong.
QString parse(const QStringList &args, Options &opts) {
  for (int i = 0; i < args.size(); ++i) {
    const QString arg = args.at(i);

    if (!arg.startsWith(QLatin1Char('-')) && arg != QLatin1String("/?")) {
      if (!opts.file.isEmpty())
        return QStringLiteral("only one file may be given (\"%1\" and \"%2\")")
            .arg(opts.file, arg);
      opts.file = arg;
      continue;
    }

    // "--name=value", or "--name" (a value, if needed, is the next
    // argument).
    const qsizetype eq       = arg.indexOf(QLatin1Char('='));
    const QString   name     = (eq < 0) ? arg : arg.left(eq);
    const bool      hasValue = (eq >= 0);
    QString         value    = hasValue ? arg.mid(eq + 1) : QString();

    const auto needValue = [&](QString &out) -> bool {
      if (hasValue) {
        out = value;
        return !out.isEmpty();
      }
      if (i + 1 >= args.size()) return false;
      out = args.at(++i);
      return true;
    };

    if (name == QLatin1String("--help") || name == QLatin1String("-h") ||
        name == QLatin1String("/?") || name == QLatin1String("-?")) {
      opts.help = true;
    } else if (name == QLatin1String("--version")) {
      opts.version = true;
    } else if (name == QLatin1String("--list-printers")) {
      opts.listPrinters = true;
    } else if (name == QLatin1String("--print")) {
      // The printer only after "=": "--print FILE" must not take the
      // file for a printer name.
      opts.print   = true;
      opts.printer = value;
    } else if (name == QLatin1String("--pdf")) {
      if (!needValue(opts.pdfPath)) return QStringLiteral("--pdf needs a file name");
    } else if (name == QLatin1String("--svg")) {
      if (!needValue(opts.svgPath)) return QStringLiteral("--svg needs a file name");
    } else if (name == QLatin1String("--fit")) {
      opts.fit = true;
    } else if (name == QLatin1String("--scale")) {
      QString text;
      bool    ok = false;
      if (needValue(text)) opts.scaleMm = text.toDouble(&ok);
      if (!ok || *opts.scaleMm <= 0.0)
        return QStringLiteral("--scale needs a positive number of mm per 1000 units");
    } else if (name == QLatin1String("--orientation")) {
      if (!needValue(opts.orientation))
        return QStringLiteral("--orientation needs portrait, landscape or auto");
      opts.orientation = opts.orientation.toLower();
      if (opts.orientation != QLatin1String("portrait") &&
          opts.orientation != QLatin1String("landscape") &&
          opts.orientation != QLatin1String("auto"))
        return QStringLiteral("--orientation must be portrait, landscape or auto, not \"%1\"")
            .arg(opts.orientation);
    } else if (name == QLatin1String("--paper")) {
      QString text;
      if (!needValue(text)) return QStringLiteral("--paper needs a paper size, e.g. A4 or Letter");
      const QPageSize size = paperNamed(text);
      if (!size.isValid()) return QStringLiteral("unknown paper size \"%1\"").arg(text);
      opts.paper = size;
    } else if (name == QLatin1String("--header")) {
      opts.header = true;
    } else if (name == QLatin1String("--no-header")) {
      opts.header = false;
    } else if (name == QLatin1String("--footer")) {
      opts.footer = true;
    } else if (name == QLatin1String("--no-footer")) {
      opts.footer = false;
    } else if (name == QLatin1String("--background")) {
      opts.background = true;
    } else if (name == QLatin1String("--no-background")) {
      opts.background = false;
    } else if (name == QLatin1String("--copies")) {
      QString text;
      bool    ok = false;
      if (needValue(text)) opts.copies = text.toInt(&ok);
      if (!ok || opts.copies < 1) return QStringLiteral("--copies needs a whole number, 1 or more");
    } else {
      return QStringLiteral("unknown option \"%1\" (see --help)").arg(arg);
    }
  }

  if (opts.help || opts.version || opts.listPrinters) return QString();

  if (opts.file.isEmpty()) return QStringLiteral("no schematic file given (see --help)");
  if (!opts.print && opts.pdfPath.isEmpty() && opts.svgPath.isEmpty())
    return QStringLiteral("nothing to do: give --print, --pdf or --svg (see --help)");
  if (opts.fit && opts.scaleMm) return QStringLiteral("--fit and --scale cannot both be given");
  return QString();
}

// The saved Print settings, with the command line's changes. Never saved
// back: a batch run must not change what the window uses.
SchPrintSettings settingsFor(SchPrint &schPrint, const Options &opts) {
  SchPrintSettings settings = schPrint.settings();

  // No window, so no "visible area": the saved mode's fit-or-scale
  // choice, applied to the whole schematic.
  if (settings.mode == SchPrintSettings::Mode::VisibleFit)
    settings.mode = SchPrintSettings::Mode::WholeFit;
  if (settings.mode == SchPrintSettings::Mode::VisibleScale)
    settings.mode = SchPrintSettings::Mode::WholeScale;

  if (opts.fit) settings.mode = SchPrintSettings::Mode::WholeFit;
  if (opts.scaleMm) {
    settings.mode = SchPrintSettings::Mode::WholeScale;
    settings.setScale(*opts.scaleMm);
  }

  if (!opts.orientation.isEmpty())
    settings.autoOrient = (opts.orientation == QLatin1String("auto"));
  if (opts.header) settings.headerOn = *opts.header;
  if (opts.footer) settings.footerOn = *opts.footer;
  if (opts.background) settings.printBackground = *opts.background;
  return settings;
}

// --paper and a fixed --orientation, applied to `printer`.
void applyPage(QPrinter *printer, const Options &opts) {
  QPageLayout layout = printer->pageLayout();
  if (opts.paper) layout.setPageSize(*opts.paper);
  if (opts.orientation == QLatin1String("portrait")) layout.setOrientation(QPageLayout::Portrait);
  if (opts.orientation == QLatin1String("landscape"))
    layout.setOrientation(QPageLayout::Landscape);
  printer->setPageLayout(layout);
}

// What `layout` means for a run: false, reported, if nothing would come
// out.
bool checkLayout(const SchPrintLayout &layout, const SchPrintSettings &settings) {
  if (layout.overLimit) {
    fail(kOutputFailed, QStringLiteral("this needs %1 pages (%2 x %3), more than the page "
                                       "limit of %4; use a smaller --scale, or --fit")
                            .arg(layout.rows * layout.cols)
                            .arg(layout.rows)
                            .arg(layout.cols)
                            .arg(settings.maxPages));
    return false;
  }
  if (layout.pages <= 0) {
    fail(kOutputFailed, QStringLiteral("there is nothing to print"));
    return false;
  }
  return true;
}

QString pageCount(int pages) {
  return (pages == 1) ? QStringLiteral("1 page") : QStringLiteral("%1 pages").arg(pages);
}

int listPrinters() {
  const QStringList names = QPrinterInfo::availablePrinterNames();
  const QString     deflt = QPrinterInfo::defaultPrinterName();
  if (names.isEmpty()) {
    say(QStringLiteral("No printers are installed."));
    return kOk;
  }
  for (const QString &name : names)
    say(name == deflt ? name + QStringLiteral("  (default)") : name);
  return kOk;
}

int doPrint(SchPrint &schPrint, const SchDocSource &source, const QString &title,
            const Options &opts, const SchPrintSettings &settings) {
  QPrinter *printer = schPrint.ensurePrinter();

  if (!opts.printer.isEmpty()) {
    // Matched as Windows does, without regard to case.
    QString found;
    for (const QString &name : QPrinterInfo::availablePrinterNames())
      if (name.compare(opts.printer, Qt::CaseInsensitive) == 0) found = name;
    if (found.isEmpty())
      return fail(kPrinterFailed, QStringLiteral("no printer named \"%1\" is installed "
                                                 "(see --list-printers)")
                                      .arg(opts.printer));
    printer->setPrinterName(found);
  }

  if (printer->printerName().isEmpty())
    return fail(kPrinterFailed, QStringLiteral("no printer is installed"));

  if (!SchPrint::isUsable(printer))
    return fail(kPrinterFailed, QStringLiteral("Windows could not open the printer \"%1\" -- "
                                               "it may be off, asleep or disconnected")
                                    .arg(printer->printerName()));

  printer->setOutputFormat(QPrinter::NativeFormat);
  printer->setDocName(title);
  printer->setCopyCount(opts.copies);
  applyPage(printer, opts);
  SchPrint::enforceMinimumMargins(printer);
  SchPrint::chooseOrientation(printer, &source, settings);

  const SchPrintLayout layout = SchPrint::layout(printer, &source, settings);
  if (!checkLayout(layout, settings)) return kOutputFailed;

  if (!SchPrint::render(printer, &source, title, settings, nullptr, nullptr))
    return fail(kOutputFailed, QStringLiteral("printing on \"%1\" failed")
                                   .arg(printer->printerName()));

  say(QStringLiteral("Printed %1 on \"%2\"%3.")
          .arg(pageCount(layout.pages), printer->printerName(),
               (opts.copies > 1) ? QStringLiteral(", %1 copies").arg(opts.copies) : QString()));
  return kOk;
}

int doPdf(const SchDocSource &source, const QString &title, const Options &opts,
          const SchPrintSettings &settings) {
  QString path = QFileInfo(opts.pdfPath).absoluteFilePath();
  if (!path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) path += QStringLiteral(".pdf");

  // Replaced, not appended to; and so "does it exist afterwards" means
  // it was written.
  QFile::remove(path);

  // A PDF needs no printer -- so none is woken. The paper is --paper, or
  // the system's default.
  QPrinter pdf(QPrinter::HighResolution);
  pdf.setOutputFormat(QPrinter::PdfFormat);
  pdf.setOutputFileName(path);
  pdf.setDocName(title);
  pdf.setCreator(QString::fromLatin1(Version::kProgramName));
  applyPage(&pdf, opts);
  SchPrint::chooseOrientation(&pdf, &source, settings);

  const SchPrintLayout layout = SchPrint::layout(&pdf, &source, settings);
  if (!checkLayout(layout, settings)) return kOutputFailed;

  const bool rendered = SchPrint::render(&pdf, &source, title, settings, nullptr, nullptr);
  if (!rendered || !QFileInfo::exists(path)) {
    QFile::remove(path);
    return fail(kOutputFailed,
                QStringLiteral("could not write %1").arg(QDir::toNativeSeparators(path)));
  }

  say(QStringLiteral("Wrote %1 (%2).").arg(QDir::toNativeSeparators(path), pageCount(layout.pages)));
  return kOk;
}

int doSvg(const SchDocSource &source, const QString &title, const Options &opts) {
  QString path = QFileInfo(opts.svgPath).absoluteFilePath();
  if (!path.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)) path += QStringLiteral(".svg");

  // As File > Export as SVG: nothing from the print settings -- the fixed
  // export scale, and the thinnest line from [Export].
  ViewerPrefs prefs;
  prefs.load();

  if (!SchSvgExport::write(path, &source, title, SchPrintSettings::kDefaultMmPer1000,
                           prefs.svgMinLineMm(), QString::fromLatin1(Version::kProgramName)))
    return fail(kOutputFailed,
                QStringLiteral("could not write %1").arg(QDir::toNativeSeparators(path)));

  say(QStringLiteral("Wrote %1.").arg(QDir::toNativeSeparators(path)));
  return kOk;
}

} // namespace

bool wanted(const QStringList &args) {
  for (const QString &arg : args)
    if (arg.startsWith(QLatin1Char('-')) || arg == QLatin1String("/?")) return true;
  return false;
}

int run(const QStringList &args) {
  connectConsole();

  Options       opts;
  const QString problem = parse(args, opts);
  if (!problem.isEmpty()) return fail(kBadArguments, problem);

  if (opts.help) {
    say(usage());
    return kOk;
  }
  if (opts.version) {
    say(QString::fromStdString(Version::winTitleStr()));
    return kOk;
  }
  if (opts.listPrinters) return listPrinters();

  const QString path       = QFileInfo(opts.file).absoluteFilePath();
  const QString nativePath = QDir::toNativeSeparators(path);
  const QString title      = QFileInfo(path).fileName();

  if (QFileInfo(path).suffix().compare(QStringLiteral("qsym"), Qt::CaseInsensitive) == 0)
    return fail(kBadInput, QStringLiteral("symbol files (.qsym) are not supported yet: %1")
                               .arg(nativePath));
  if (!QFileInfo(path).isFile())
    return fail(kBadInput, QStringLiteral("no such file: %1").arg(nativePath));

  SchDocSource                source;
  QString                     error;
  QList<recordrules::Omitted> omitted;
  if (!source.load(path, error, omitted)) return fail(kBadInput, error);

  recordrules::log(title, omitted);
  say(QStringLiteral("%1: %2").arg(QString::fromStdString(Version::winTitleStr()), nativePath));
  if (!omitted.isEmpty())
    say(QStringLiteral("%1 record(s) not allowed where they are were left out -- see the log.")
            .arg(omitted.size()));

  SchPrint               schPrint(ViewerPrefs::iniFilePath(), ViewerPrefs::printDefaults());
  const SchPrintSettings settings = settingsFor(schPrint, opts);

  // Each output is tried even if an earlier one failed; the result is
  // the first failure.
  int result = kOk;
  const auto keep = [&result](int code) {
    if (result == kOk) result = code;
  };

  if (opts.print) keep(doPrint(schPrint, source, title, opts, settings));
  if (!opts.pdfPath.isEmpty()) keep(doPdf(source, title, opts, settings));
  if (!opts.svgPath.isEmpty()) keep(doSvg(source, title, opts));
  return result;
}

} // namespace CliRun
