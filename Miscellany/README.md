# Miscellany

Here we have some QSpice odds and ends -- symbols, snippets, utilities, tips, etc.  Just bits and bobs that might be useful but don't fit elsewhere.

## Voltage Controlled Potentiometer

Put 0-1.0V on the control pin and vary the wiper 0-100%.  Total pot resistance is set with an attribute (RTOT).

* Pot_VCtrl.qsym &mdash; Self-contained symbol (embedded subcircuit).
* PotVctrlTest.qsch &mdash; Test schematic.

## GPIO Pins

Micro-controllers have GPIO pins -- tri-state pins configurable for input/output in code at runtime.  QSpice C-Block ports are either input or output but not configurable at runtime.  We're going to need this bit to implement GPIO pins in QSpice.

* GPIO_Pin.qsch &mdash; The sub-circuit schematic.  Not required to use the symbol.  Provided in lieu of an explanation/documentation.
* GPIO_Pin.qsym &mdash; QSpice symbol (self-contained sub-circuit).

## Message Boxes From C-Block/DLL Code

If our DLL code encounters some unusual condition, we just write a message to the QSpice Output window and terminate the simulation.  But what if, instead of ending the simulation, we could display a dialog box informing the user of the condition and asking what to do?

Well, the 2025.06.26 QSpice update added a "GUI_HWND" so now we can.  Here's a demonstration using the standard Windows MessageBox() API.

* WinMsgBox.qsch &mdash; Example schematic.
* WinMsgBox.cpp &mdash; Example source code.
