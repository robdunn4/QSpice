# QParser2 Project &mdash; QSch_Shared_Libs

2026.10.09 &mdash; The libraries in this folder are under development and not yet stable.

These libraries require Qt 6 (built against Qt 6.11.2, MSVC, x64).

| Library   | Qt modules                  | Holds                                                        | Depends on            |
|-----------|-----------------------------|--------------------------------------------------------------|-----------------------|
| SchCore   | Core                        | connectivity model, record rules, edit planning, designators, connectivity dump | QParser2 |
| SchLink   | Core                        | the QSpice installation: registry, QUX, simulators, viewer, tools, help | SchCore |
| SchRender | Core, Gui, Widgets          | scene items, drawing, the scene, the read-only view (SchViewBase), render sources | SchCore |
| SchOutput | Core, Gui, Widgets, PrintSupport, Svg | printing, page layout, header/footer fields, SVG export (no dialogs) | SchRender |
| SchApp    | Core, Gui, Widgets          | what every program shares: the log file, QSpice's colours as a drawing style, the program icon, the About box, self-documenting .ini values | SchLink, SchOutput |

All are static libraries (`.lib`), x64 only, output to
`<Library>\lib\x64\<Configuration>`.

Namespaces: `sch::core`, `sch::link`, `sch::render`, `sch::output`, `sch::app`. Each
library reports its name and version through `sch::<lib>::libraryInfo()`.
