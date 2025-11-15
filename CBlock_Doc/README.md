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

## C-Block Basics #4

Revisits the Trunc() function, the QSpice simulation cycle, and more.  Implements a comparator component example.  (Builds on C-Block Basics #1.)

* CBlockBasics4.pdf ("Trunc() Revisited")
* CBlockBasics4.qsch &mdash; Demonstration schematic.
* CBlockBasics4.cpp &mdash; Demonstration C-Block DLL code.

## C-Block Basics #5

Revisits the MaxExtStepSize() function, a QSpice "peculiarity," and more.  Implements an internal clock component example.  (Builds on C-Block Basics #4.)

* CBlockBasics5.pdf ("MaxExtStepSize() Revisited")
* CBlockBasics5.qsch &mdash; Demonstration schematic.
* CBlockBasics5.cpp &mdash; Demonstration C-Block DLL code.

## C-Block Basics #6

Investigates connecting bus wires to C-Block components and why we can't use Bit Vectors in C++ components.  Discusses how QSpice passes data into/out of components.  Explores ways to implement "bit registers" as a substitute for Bit Vectors.

* CBlockBasics6.pdf ("Using Bus Wires")
* CBlockBasics6.qsch &mdash; Demonstration schematic.
* CBlockBasics6.cpp &mdash; Basic C-Block code for attached bus wire.
* CBlockBasics6_Alt1.cpp &mdash; C-Block code with types & functions to implement "bit registers" conveniently.

## C-Block Basics #7

Recent QSpice updates introduce a new Display() function and a simplified Trunc() function.  The changes are significant improvements.  For *most* existing code, the changes are fully backwards-compatible.

This short paper describes the changes and steps to update existing code.  It also details the subtle implications of some "edge-cases" that may break your code.

* CBlockBasics7.pdf ("Recent Changes (July/August 2024)")

## C-Block Basics #8

A complete "post-processing" framework using newest QSpice features (October 2024). Includes/demonstrates:

* Launching an external standalone program (\*.exe) or batch command file (\*.bat or \*.cmd) from within a C-Block component.
* Processing multiple component instances and multi-step simulations.
* Creating a binary log file and converting it to a *.csv in post-processing.
* Building file names and command line strings from user-supplied component attributes.
* Coding a standalone program (\*.exe) and compiling with the QSpice DMC compiler.
* And more....

*Note:  This is a bit more complex than the usual C-Block Basics fare.*

### Files
* CBlockBasics8.pdf ("Post-Processing Techniques")
* CBlockBasics8.qsch &mdash; Demonstration schematic.
* CBlockBasics8.cpp &mdash; C-Block code for component DLL.
* CBlockBasics8_pp.cpp &mdash; Sample code for standalone executable (converts binary to CSV).
* CBlockBasics8_pp_dmc.cmd &mdash; Batch command file to compile CBlockBasics8_pp.cpp to *.exe with DMC.
* CBlockBasics8_pp.cmd &mdash; Sample batch command to be launched from component.

## C-Block Basics #9

QSpice added a EngAtof() function that allows runtime evaluation of "expressions."  It's pretty clever but a bit opaque.  This paper details how it works.

### Files
* CBlockBasics9.pdf ("The EngAtof() Function")
* CBlockBasics9.qsch &mdash; Demonstration schematic.
* CBlockBasics9.cpp &mdash; Sample DLL code.

## C-Block Basics #10

QSpice now supports client/server components with the "Berkeley Sockets API."  This paper is a "technical reference" (not a tutorial) that provides details about the new API calls, the client/server messaging, and limitations of the basic QSpice template-generated code.

See C-Block Basics #11 for details about working around the template code limitiations.

### Files
* CBlockBasics10.pdf ("A Berkeley Sockets API Reference")

## C-Block Basics #11

How to work around the "Berkeley Sockets API framework" limitations identified in C-Block Basics #10.

This is a "code-heavy" conversion of a basic "monolithic DLL component" to a "full-featured" client/server component.  

C++, Python, and Java server code examples are included.

### Files
* CBlockBasics11.pdf ("Extending The 'Berkeley Sockets API'")
* CB11_Monolithic.qsch &mdash; Schematic for the basic DLL component.
* CB11_Monolithic.cpp &mdash; Sample code for the basic DLL component.
* CB11_SocketAPI_Cpp.qsch &mdash; Schematic for the converted client/server component (C++).
* CB11_SocketAPI_Py.qsch &mdash; Schematic for the converted client/server component (Python).
* CB11_SocketAPI_Java.qsch &mdash; Schematic for the converted client/server component (Java).
* CB11_Client.cpp &mdash; Client code for the converted component.
* CB11_Server.cpp &mdash; C++ server code for the converted component.
* CB11_Server.py &mdash; Python server code for the converted component.
* CB11_Server.java &mdash; Java server code for the converted component.

## C-Block Basics #12

Multi-client, multi-threading QSpice "Berkeley Sockets API" servers suitable for running on a separate host.  This builds on concepts from C-Block Basics #10 & #11.  Now includes code for C++, Python, and Java servers.

### Files
* CBlockBasics12.pdf (A “Berkeley Sockets API” Multi-Client Server)
* CB12_SocketAPI.qsch &mdash; Example schematic.
* CB12_Client.cpp &mdash; Example component client code (C++ DLL).
* CB12_HostServer.cpp &mdash; Example C++ server code.
* CB12_HostServer.py &mdash; Example Python server code.
* CB12_HostServer.java &mdash; Example Java server code.
* CB12_Binaries.zip &mdash; Schematic and pre-compiled client DLL and C++ server executable.
