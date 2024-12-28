# Microsoft Visual Studio (MSVC) Configuration

QSpice includes a compiler (DMC) but no proper IDE and no debugger.  And the compiler doesn't support current C/C++ standards.  For serious QSpice C-Block component development, we need modern tools.

The Microsoft Visual Studio 2022 Community Edition (free) not only compiles C-Block component DLLs, it supports full debugging features (set breakpoints, step through code, view variables, all of the normal debugging stuff).

This is a step-by-step guide to create a MSVC project for compiling and debugging C-Block components.

## Files

QSpice_MSVS.pdf &mdash; Detailed instructions for a first Visual Studio/QSpice component project.

MyDemo.qsch &mdash; The sample schematic used in the PDF guide.

GenNetlists.cmd &mdash; A batch file to generate netlist files from schematic files.
