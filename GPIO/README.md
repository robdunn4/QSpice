# GPIO Components

QSpice symbol and class to implement general purpose I/O pins (i.e., micro-controller tri-state pins).

## Documentation

At this point, there is no separate "how to" documentation.  The sources are listed below; hopefully, that's enough to get you started.

## GPIO Pin Symbol #1

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

## GPIO Pin Symbol #2

This is an alternate implementation of a GPIO tri-state pin based on a circuit suggested by Mike Engelhardt.  It is *not* a drop-in replacement for the first version.  It will likely be used in future versions of the Microchip simulator projects available elsewhere in this repository.

* GPIO_Pin2.qsch &mdash; The sub-circuit schematic.  Not required to use the symbol but does contain comments about how to use the symbol.
* GPIO_Pin2.qsym &mdash; QSpice symbol (self-contained sub-circuit).
* GPIO_Pin2_Test_Sch.qsch &mdash; Test for GPIO_Pin2.qsch.
* GPIO_Pin2_Test_Sym.qsch &mdash; Test for GPIO_Pin2.qsym.

## Finally...
These components are new, largely untested code.  Feel free to improve the code (and share), report bugs, or just let me know that you find this project useful.  You'll find me (@RDunn) on the [Qorvo QSpice forum](https://forum.qorvo.com/c/qspice/).