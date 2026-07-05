# QParser2 Project

I've partially parsed QSpice \*.sch/\*.sym files for other projects in the past (e.g., the QCodeGen tool).  This is a fresh start in hopes of fully reverse-engineering the file format.  It is significantly more complete than prior attempts.

*Note:  This is a MSVS 2026 project using C++20 code.  It will not compile using the QSpice DMC compiler.*

#### Major Revisions &mdash; 2026.04.18:

The prior code was sufficient for parsing an existing QSpice schematic or symbol file.  However, it was poorly planned (and inadequate) for generating new schematic/symbol files entirely from code.  This release attempts to rectify that shortcoming.  However, that required a major rework and prior sample code may no longer work.

* The shared code folder (QParser2_Shared) has been replaced with a shared static library (QParser2_Shared_Lib) to eliminate unnecessary rebuilds.
* The shared code library was significantly reworked.
* The QParser2_Tests and QBomGen code has been updated to work with the new shared static library.

## Folders

* [QParser2_Shared_Lib](./QParser2_Shared_Lib/) &mdash; The parser code library.
* [QParser2_Tests](./QParser2_Tests/) &mdash; Test code to ensure read/parse/write round-tripping is correct.
* [QBomGen](./QBomGen/) &mdash; A simple QParser2 command-line tool to generate a Bill of Materials list.
* [QSymGen](./QSymGen/) &mdash; A QSpice symbol generator for the *QMdbSim* micro-controller simulator project.
* [QtDemo](./QtDemo/) &mdash; Using MSVS with Qt Extension for GUI interface to the QParser2 Project shared libraries.
* [QSymGen2](./QSymGen2/) &mdash; QSpice symbol generator for the *QMdbSim* micro-controller simulator project.  Similar to QSymGen but recoded from scratch.  Adds support for GPIO pins.
* [QSymGen3](./QSymGen3/) &mdash; QSpice symbol, schematic, and code generator.  Similar to QSymGen2 but designed specifically as a part of a toolchain to generate devices for the QMdbSym2 micro-controller project.