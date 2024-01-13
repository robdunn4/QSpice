# CBlock_Doc

Various documentation and tools relevant to C-Block component development.


## C-Block Basics

This is an attempt to de-mystify C-Block component development.  The first couple of sections should (I hope) get less experienced C/C++ coders past the first hurdles.  Remaining sections share my (very likely wrong and definitely incomplete) understanding of the MaxExtStepTime() and Trunc() functions.

* CBlock_Basics.pdf ("The Absolute Basics") &mdash; The basic concepts.
* CBlock_Basics.qsch &mdash; An incomplete tutorial schematic with C-Block component (EvilAmp).
* evilamp.cpp &mdash; Sample DLL source for CBlock_Basics stuff.

## C-Block Debugging

DbgLog &mdash; A small utility class to write debugging messages to a log file.  Supports DMC, MSVC, and MinGW compilers.

* DbgLog.pdf &mdash; Documentation for the DbgLog class.
* DbgLog.h &mdash; The DbgLog header file.  Redirects to DMC, MSVC, or MinGW versions of header.
* DbgLog_DM.h &mdash; Header file for DMC.
* DbgLog_VC.h &mdash; Header file for MSVC.
* DbgLog_MGW.h &mdash; Header file for MinGW.
* DbgLogTest.qsch &mdash; A test schematic with a test component to demonstrate the logging features.
* DbgLogTest.cpp &mdash; A test component to demonstrate the logging features.

## C-Block Templates

My "more modern C++" C-Block template.  Supports DMC, MSVC, and MinGW compilers.

* CBlockTemplates.pdf &mdash; Documentation for the base template.
* CBlockTemplate.qsch &mdash; A test schematic for the base template.
* Cblock.cpp &mdash; The base template.
* Cblock.h &mdash; Base header, redirects to DMC, MSVC, and MinGW header versions.
* Cblock_DM.h &mdash; Header for DMC compiler.
* Cblock_VC.h &mdash; Header for MSVC compiler.
* Cblock_MGW.h &mdash; Header for MinGW compiler.
* DbgLog*.h &mdash; DbgLog headers (see above).

## Trunc() Function Tests

Want to understand how the QSpice C-Block Trunc() function works?  Maybe start with this code to test your assumptions.

*Caution: This is ugly code, poorly commented, and not intended for beginners.*

* TruncTest.zip &mdash; QSpice schematic and C-Block source files.

Please do share what you learn.

## C-Block Basics #2

A "properly coded" C-Block component should handle multiple schematic instances, multiple simulation steps, and shared resources.  Here is sample component code for both C and C++.

#### Documentation (C & C++)
* CBlockBasics2.pdf ("Shared Resources & Refernce Counting") &mdash; The code samples demonstrate *how*.  This document attempts to explain *why*.

#### C Code Versions (DMC)

* CBlockBasics2_c.qsch &mdash; Demonstration schematic.
* CBlockBasics2_c.cpp &mdash; Demonstration C-Block DLL code.

#### C++ Code Versions (DMC, MSVC, & MinGW)

* CBlockBasics2.qsch &mdash; Demonstration schematic.
* CBlockBasics2.cpp &mdash; Demonstration C-Block DLL code.
* CBlockBasics2.h &mdash; Generic header (includes compiler-specific headers).
* CBlockBasics_*.h &mdash; Compiler-specific headers.

## C-Block Basics #3

Execute C-Block code when a QSpice simulation starts/ends using DllMain().  (Builds on C-Block Basics #2.)

* CBlockBasics3.pdf ("DllMain() & Simulation Start/End Tasks") &mdash; How DllMain() works and how we can use it.
* CBlockBasics3_c.qsch &mdash; Demonstration schematic.
* CBlockBasics3_c.cpp &mdash; Demonstration C-Block DLL code (C-Style).
