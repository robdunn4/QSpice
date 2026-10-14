# The QParser2 Project &mdash; QSch_Shared_Libs

2026.10.14 &mdash; The libraries in this folder are under development and not yet stable.

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
    <td valign="top">SchLink</td>
    <td valign="top">Core</td>
    <td valign="top">The QSpice installation: registry, QUX, simulators, viewer, tools, help</td>
    <td valign="top">SchCore</td>
  </tr>
  <tr valign="top">
    <td valign="top">SchRender</td>
    <td valign="top">Core, Gui, Widgets</td>
    <td valign="top">Scene items, drawing, the scene, the read-only view (SchViewBase), render sources</td>
    <td valign="top">SchCore</td>
  </tr>
  <tr valign="top">
    <td valign="top">SchOutput</td>
    <td valign="top">Core, Gui, Widgets, PrintSupport, Svg</td>
    <td valign="top">Printing, page layout, the saved printer and paper, header/footer fields, SVG export (no dialogs)</td>
    <td valign="top">SchRender</td>
  </tr>
  <tr valign="top">
    <td valign="top">SchApp</td>
    <td valign="top">Core, Gui, Widgets</td>
    <td valign="top">What every program shares: the log file, QSpice's colors as a drawing style, the program icon, the About box, self-documenting .ini values, opening a hierarchical block's schematic</td>
    <td valign="top">SchLink, SchOutput</td>
  </tr>
</table>

All are static libraries (`.lib`), x64 only, output to
`<Library>\lib\x64\<Configuration>`.

Namespaces: `sch::core`, `sch::link`, `sch::render`, `sch::output`, `sch::app`. Each
library reports its name and version through `sch::<lib>::libraryInfo()`.
