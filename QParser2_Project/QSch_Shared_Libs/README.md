# The QParser2 Project &mdash; QSch_Shared_Libs

2026.10.23 &mdash; The libraries in this folder are under development and not yet stable.

2026.11.21 &mdash; Moved code from QSchView into the shared library in anticipation of QSchEdit project.  Also changed #includes to use <> instead of "" where appropriate (to correct an early project oversight).

These libraries require Qt 6 (built against Qt 6.11.2, MSVC, x64).

<table>
  <tr valign="top">
    <th align="left" valign="top">Library</th>
    <th align="left" valign="top">Qt modules</th>
    <th align="left" valign="top">Holds</th>
    <th align="left" valign="top">Depends on</th>
  </tr>
  <tr valign="top">
    <td valign="top">SchCore</td>
    <td valign="top">Core</td>
    <td valign="top">Connectivity model, record rules, edit planning, designators, hierarchical-block schematic names, connectivity dump</td>
    <td valign="top">QParser2</td>
  </tr>
  <tr valign="top">
    <td valign="top">SchProgAttr</td>
    <td valign="top">Core</td>
    <td valign="top">Programmable Attributes: taking a symbol attribute's text apart (<code>NAME=&lt;VALUE&gt;DESCRIPTION</code> and, later, QSpice's other forms), what it displays as, and its text with a new value</td>
    <td valign="top">&mdash;</td>
  </tr>
  <tr valign="top">
    <td valign="top">SchLink</td>
    <td valign="top">Core</td>
    <td valign="top">The QSpice installation: registry, QUX, simulators, viewer, tools, help</td>
    <td valign="top">SchCore</td>
  </tr>
  <tr valign="top">
    <td valign="top">SchRender</td>
    <td valign="top">Core, Gui, Widgets</td>
    <td valign="top">Scene items, drawing, the scene, the read-only view (SchViewBase), render sources</td>
    <td valign="top">QParser2, SchCore, SchProgAttr</td>
  </tr>
  <tr valign="top">
    <td valign="top">SchOutput</td>
    <td valign="top">Core, Gui, Widgets, PrintSupport, Svg</td>
    <td valign="top">Printing, page layout, the saved printer and paper, header/footer fields, SVG export, and the print dialogs QSchEdit and QSchView share: Print, Printing Defaults, Headers/Footers</td>
    <td valign="top">QParser2, SchCore, SchProgAttr, SchRender</td>
  </tr>
  <tr valign="top">
    <td valign="top">SchApp</td>
    <td valign="top">Core, Gui, Widgets</td>
    <td valign="top">What every program shares: the log file, the application style (toolbar buttons, compact list and menu rows), QSpice's colors as a drawing style, the program icon, the About box, self-documenting .ini values, opening a hierarchical block's schematic</td>
    <td valign="top">QParser2, SchCore, SchLink, SchOutput, SchProgAttr, SchRender</td>
  </tr>
  <tr valign="top">
    <td valign="top">SchCodeGen</td>
    <td valign="top">Core</td>
    <td valign="top">Code templates: template files (<code>.qschedtmpl</code>), their options, and the code generated from them (no dialogs)</td>
    <td valign="top">SchCore</td>
  </tr>
</table>

"Depends on" lists every library whose headers a library includes directly.

All are static libraries (`.lib`), x64 only, output to
`<Library>\lib\x64\<Configuration>`.

Namespaces: `sch::core`, `sch::link`, `sch::render`, `sch::output`, `sch::app`, `sch::codegen`, `sch::progattr`, and namespaces nested within them (`sch::core::refdes`, `sch::app::log`, ...). Each
library reports its name and version through `sch::<lib>::libraryInfo()`; the version's last number goes up with every change to the library's contents.

## Including the headers

Programs include these libraries' headers, and each library includes another library's headers, with angle brackets: `#include <SchPrint.h>`. A library includes its own headers with quotes.

A project's Additional Include Directories list the folders of the libraries it uses. They need not list the project's own folder: its own headers are included with quotes and found beside the file that includes them. Listed ahead of the libraries, the project's folder would let a stray file there with a library header's name be used in place of the library's.
