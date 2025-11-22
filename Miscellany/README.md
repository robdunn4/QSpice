# Miscellany

Here we have some QSpice odds and ends -- symbols, snippets, utilities, tips, etc.  Just bits and bobs that might be useful but don't fit elsewhere.

## Basic Potentiometer Symbol

A simple potentiometer symbol.  Set total resistance (RTOT) and wiper to 0-100% (RPCT).  You can set RTOT and RPCT with .param, step values, etc.

* Pot.qsym &mdash; Drag and drop symbol.
* PotTest.qsch &mdash; Test schematic.

*Note: Internal limits stop a bit shy of 0% and 100% to prevent QSpice issues.*

## Voltage Controlled Potentiometer Symbol

Put 0V-1.0V on the control pin to vary the wiper 0-100%.  Total pot resistance is set with an attribute (RTOT).

* Pot_VCtrl.qsym &mdash; Self-contained symbol (embedded subcircuit).
* PotVctrlTest.qsch &mdash; Test schematic.

*Note: Internal limits stop a bit shy of 0% and 100% to prevent QSpice issues.*

## Message Boxes From C-Block/DLL Code

If our DLL code encounters some unusual condition, we just write a message to the QSpice Output window and terminate the simulation.  But what if, instead of ending the simulation, we could display a dialog box informing the user of the condition and asking what to do?

Well, the 2025.06.26 QSpice update added a "GUI_HWND" so now we can.  Here's a demonstration using the standard Windows MessageBox() API.

* WinMsgBox.qsch &mdash; Example schematic.
* WinMsgBox.cpp &mdash; Example source code.
* WinMsgBox2.qsch &mdash; Example schematic.
* WinMsgBox2.cpp &mdash; Example Source code.

Note:  The difference between the versions is that the first does not have an active "X button" (it is present but greyed out) and does not respond to the ESC key.  The second version has both.  See comments in the second version *.cpp or the WinAPI documentation for more information.

## TL081 Op Amp Symbol

TL081 Op Amp from TI Spice model.  *(Not extensively tested.)*

## WhereIsQS.exe

Code demonstrating how to retrieve the QSpice installation folder location from the Windows Registry.  See comments in the source code.

* WhereIsQS.cpp &mdash; Demonstration source code.
* WhereIsQS.exe &mdash; Compiled executable.

## LM358 Op Amp Symbol

LM358 Op Amp from OnSemi Spice model.  *(Not extensively tested.)*
