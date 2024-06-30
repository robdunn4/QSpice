# QCodeGen &mdash; A QSpice C-Block Code Generator

QCodeGen is a stand-alone replacement for the QSpice C-Block template code generator with more features and flexibility.

Key features include:

* Generates code using custom user-defined templates.
* Updates existing *.cpp code for schematic component changes with minimal programmer effort.
* Generates code without overwriting existing *.cpp component code files.
* Improves error analysis for malformed port and string attributes.

Release v0.9.5.1 is now available and adds support for "bus ports" to connect to QSpice bus wires.  See the CBlockBasics6 documents for an explanation of "Using Bus Wires" with C-Block components and the related issues.

## Files/Folders

* /binaries &mdash; Project binaries/installation files (64-bit only).
* /source &mdash; Project source code.
* /templates &mdash; New and/or corrected template files (*.qcgt).
* QCodeGen_User_Doc.pdf &mdash; User documentation.
* QCodeGen_Dev_Doc.pdf &mdash; Developer notes (incomplete).
