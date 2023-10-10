# SpiIO Components

QSpice C-Block code and schematics to implement custom SPI master & slave components.

See the SpiIO_Dev_Doc.pdf document for information about using and customizing  these components for specific use cases.

*At this point, this is <b>Proof of Concept</b> stuff.  The code is not well-thought-through, well-tested, well-organized, nor well-commented.*

***You have been warned.***

## Original SPI Framework Files

* SpiIO_Dev_Doc.pdf &mdash; Developer documentation for using/modifying the components.
* DemoSpiIO.qsch &mdash; The top-level QSpice schematic demonstrating the master and slave components.
* SpiMaster.qsch &mdash; Master component ADC/DAC driver schematic.
* SpiMaster.cpp &mdash; SPI master component code.  Drives the ADC and DAC slave components.
* SpiMaster.dll &mdash; Compiled SpiMaster DLL.
* SpiADC.qsch &mdash; 8-bit ADC component schematic.
* SpiADC.cpp &mdash; 8-bit ADC component C-Block code.
* SpiADC.dll &mdash; Compiled SpiADC DLL.
* SpiDAC.qsch &mdash; 8-bit DAC component schematic.
* SpiDAC.cpp &mdash; 8-bit DAC component C-Block code.
* SpiDAC.dll &mdash; Compiled SpiADC DLL.
* PinIO.h &mdash; Header file containing "pin" state management classes.
* SpiIO.h &mdash; SPI serial buffer class and SPI mode management code.

The code compiles with MS VC.

## SpiPot Component (A Framework Demo)

These two files demonstrate how easy it is to use the SpiIO framework to implement a new SPI slave device.  This SPI potentiometer slave started with a copy of DemoSpiIO.qsch, a copy of SpiDAC.qsch, and my voltage-controlled potentiometer subcircuit/symbol (see Pot_Vctrl.qsym in the Miscellany folder).  No C-Block code changes required.

* DemoSpiIO_Pot.qsch &mdash; The top-level demonstration schematic.
* SpiPot.qsch &mdash; The SPI potentiometer slave device.

