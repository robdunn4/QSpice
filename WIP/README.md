# Work In Progress (WIP)

This folder contains some project code under development.  Feel free to browse but be warned that:

* This is totally undocumented stuff that I'm messing around with.
* It is unreliable and unsuitable for any production purpose so don't even think about it....
* The code is messy &mdash; it undoubtedly has shards of false starts, debugging stuff, and misleading comments.
* I might get bored or distracted and set any of this aside for a while.
* I might delete or remove anything at any time.

In short, none of this is guaranteed to see a proper release.  But, if you want to see what I'm working on and aren't bothered by seeing *"how the sausage is made,"* you're welcome to check it out.

## QParser2 Project

I've partially parsed QSpice \*.sch/\*.sym files for other projects in the past (e.g., the QCodeGen tool).  This is a fresh start in hopes of fully reverse-engineering the file format.  (It is significantly more complete than prior attempts.)

The folders contain MSVS 2026 project/solution files.  This is C++20 code and will not compile using the QSpice DMC compiler.

### QParser2 Project Folders

* [QParser2_Shared](./QParser2/QParser2_Shared/) &mdash; The parser code library.
* [QParser2_Tests](./QParser2/QParser2_Tests/) &mdash; Test code to ensure read/parse/write round-tripping is correct.
* [QBomGen](./QParser2/QBomGen/) &mdash; A simple QParser2 command-line tool to generate a Bill of Materials list.
