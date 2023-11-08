# Miscellany

Here we have some QSpice odds and ends -- symbols, snippets, utilities etc.


## Voltage Controlled Potentiometer

Put 0-1.0V on the control pin and vary the wiper 0-100%.  Total pot resistance is set with an attribute (RTOT).

* Pot_VCtrl.qsym &mdash; Self-contained symbol (embedded subcircuit).
* PotVctrlTest.qsch &mdash; Test schematic.

## GPIO Pins

Micro-controllers have GPIO pins -- tri-state pins configurable for input/output in code at runtime.  QSpice C-Block ports are either input or output but not configurable at runtime.  We're going to need this bit to implement GPIO pins in QSpice.

* GPIO_Pin.qsch &mdash; The sub-circuit schematic.  Not required to use the symbol.  Provided in lieu of an explanation/documentation.
* GPIO_Pin.qsym &mdash; QSpice symbol (self-contained sub-circuit).