# GPIO Components

QSpice symbol and class to implement general purpose I/O pins (i.e., micro-controller tri-state pins).

## Documentation

At this point, there is no separate "how to" documentation.  The sources are listed below; hopefully, that's enough to get you started.

## GPIO Pin Symbol

* GPIO_Pin.qsch &mdash; The sub-circuit schematic.  Not required to use the symbol.
* GPIO_Pin.qsym &mdash; QSpice symbol (self-contained sub-circuit).

## GPIO Class

**Note:  The GPIO class code uses features not available in the QSpice DMC compiler.  MSVC is recommended but any compiler supporting C++17 or newer should work.**

* GPIO_Class.h &mdash; GPIO class implementation.
* GPIO_Test.qsch &mdash; QSpice schematic demonstrating the class.
* GPIO_uC.cpp &mdash; C-Block code demonstrating using the class.
* GPIO_uC.dll &mdash; Precompiled C-Block demonstration code.
* GPIO_Test.pfg &mdash; Waveform display configuration file for convenience.
* Cblock*.h &mdash;  Utility headers from the C-Block Template tools elsewhere in this repository.

## Finally...
These components are new, largely untested code.  Feel free to improve the code (and share), report bugs, or just let me know that you find this project useful.  You'll find me (@RDunn) on the [Qorvo QSpice forum](https://forum.qorvo.com/c/qspice/).