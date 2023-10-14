# Miscellany

Here we have some QSpice odds and ends -- symbols, snippets, utilities etc.


## Voltage Controlled Potentiometer

Put 0-1.0V on the control pin and vary the wiper 0-100%.  Total pot resistance is set with an attribute (RTOT).

* Pot_VCtrl.qsym &mdash; Self-contained symbol (embedded subcircuit).
* PotVctrlTest.qsch &mdash; Test schematic.

## DbgLog

A small utility class to write debugging messages to a log file.

* DbgLog.pdf &mdash; Documentation for the DbgLog class.
* DbgLog.h &mdash; The DbgLog header file (declares and defines the DbgLog class).
* DbgLogTest.qsch &mdash; A test schematic with a test component to demonstrate the logging features.
* DbgLogTest.cpp &mdash; A test component to demonstrate the logging features.