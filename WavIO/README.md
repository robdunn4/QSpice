# WavIO Components
Components to source/sink circuit signals from/to WAV files in the QSpice circuit simulator.

## Revision History
* 2023.09.03 -- Initial versions released (v0.1).
* 2024.04.26 -- Added support for 24-bit PCM to WavSrc (v0.2).
* 2024.05.08 -- Added support for 24-bit PCM to WavOut (v0.2). 

## WavSrc - WAV file as simulation signal source
* WavSrc.cpp & .h &mdash; DLL source code.
* WavSrc.qsch &mdash; Subcircuit schematic.
* WavSrc_Demo.qsrc &mdash; Top-level schematic demonstrating the WavSrc component subcircuit.
* WavSrc.dll &mdash; Compiled DLL.

Note:  WavSrc.cpp/h code can be compiled with the Digital Mars compiler shipped with QSpice.  This will likely change to require the Microsoft VC compiler in the future.

## WavOut - WAV file output from simulation
* WavOut.cpp & .h &mdash; DLL source code.
* WavOut.qsch &mdash; Subcircuit schematic.
* WavOut_Demo.qsrc &mdash; Top-level schematic demonstrating the WavOut component subcircuit.
* WavOut.dll &mdash; Compiled DLL.

Note:  WavOut.cpp/h code probably cannot be compiled with the Digital Mars compiler shipped with QSpice.  The Microsoft VC compiler (also free) or other modern C++ compiler is required.

## Both
* WavIO_Demo.qsch &mdash; Combines WavSrc & WavOut to read a WAV file and write a similar WAV file ("roundtrip").  In theory, the files should be identical.  As a practical matter, they likely aren't quite (see below).  Intended for testing.

## Known Issues/Limitations
* There is an asymmetry between the ranges of positive and negative two's-complement integers, i.e., +32,767 and -32,768.  I'm uncertain how to properly handle this.  For now, the components assume/force the minimum sample value to -32,767.
* The WavIO components currently support only basic 16- and 24-bit PCM, 1- and 2-channel WAV formats.

## Finally...
These components are new, largely untested code.  Feel free to improve the code (and share), report bugs, or just let me know that you find this project useful.  You'll find me (@RDunn) on the [Qorvo QSpice forum](https://forum.qorvo.com/c/qspice/).