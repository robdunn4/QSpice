# CBlock_Doc

Various documentation and tools relevant to C-Block component development.


## C-Block Basics

This is an attempt to de-mystify C-Block component development.  The first couple of sections should (I hope) get less experienced C/C++ coders past the first hurdles.  Remaining sections share my (very likely wrong and definitely incomplete) understanding of the MaxExtStepTime() and Trunc() functions.

* CBlock_Basics.pdf &mdash; The basics document.
* CBlock_Basics.qsch &mdash; An incomplete tutorial schematic with C-Block component (EvilAmp).
* evilamp.cpp &mdash; Sample DLL source for CBlock_Basics stuff.

## C-Block Debugging

DbgLog &mdash; A small utility class to write debugging messages to a log file.  Supports the DMC and MSVC compilers.

* DbgLog.pdf &mdash; Documentation for the DbgLog class.
* DbgLog.h &mdash; The DbgLog header file.  Redirects to DMC or MSVC versions of header.
* DbgLog_DM.h &mdash; Header file for DMC (declares and defines the DbgLog class).
* DbgLog_VC.h &mdash; Header file for MSVC (declares and defines the DbgLog class).
* DbgLogTest.qsch &mdash; A test schematic with a test component to demonstrate the logging features.
* DbgLogTest.cpp &mdash; A test component to demonstrate the logging features.  (Compiles cleanly under DMC only.)

## C-Block Templates

My "more modern C++" C-Block template.  Supports DMC and MSVC compilers.

* CBlockTemplates.pdf &mdash; Documentation for the base template.
* CBlockTemplate.qsch &mdash; A test schematic for the base template.
* Cblock.cpp &mdash; The base template.
* Cblock.h &mdash; Base header, redirects to DMC and VC header versions.
* Cblock_DM.h &mdash; Header for DMC compiler.
* Cblock_VC.h &mdash; Header for MSVC compiler.
* DbgLog*.h &mdash; DbgLog headers (see above).

## Trun() Function Tests

Want to understand how the QSpice C-Block Trunc() function works?  Maybe start with this code to test your assumptions.

*Caution: This is ugly code, poorly commented, and not intended for beginners.*

* TruncTest.zip &mdash; QSpice schematic and C-Block source files.

Please do share what you learn.