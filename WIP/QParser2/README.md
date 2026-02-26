# QParser2 Project

I've partially parsed QSpice \*.sch/\*.sym files for other projects in the past (e.g., the QCodeGen tool).  This is a fresh start in hopes of fully reverse-engineering the file format.  (It is significantly more complete than prior attempts.)

Note:  This is a MSVS 2026 project using C++20 code.  It will not compile using the QSpice DMC compiler.

## Folders

* [QParser2_Shared](./QParser2_Shared/) &mdash; The parser code library.
* [QParser2_Tests](./QParser2_Tests/) &mdash; Test code to ensure read/parse/write round-tripping is correct.
* [QBomGen](./QBomGen/) &mdash; A simple QParser2 command-line tool to generate a Bill of Materials list.

