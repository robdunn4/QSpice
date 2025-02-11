# QMdbSim Project

The QMdbSim Project is a framework for creating QSpice micro-controller components for
Microchip devices (PIC, AVR).  Under the covers, it uses Microchip's software simulator (MDB).  In theory, it should support the same devices to the same extent as the simulator in MPLabX.

This first release demonstrates a PIC16F15213 component.  I'm working on documentation but, at this point, it's "read the code."

* Compiled component DLL (PIC16F15213.dll) is included but assumes that you have MPLabX v6.20 installed in the default location.

* Compiled "Charlie-Plexing" device test code (PIC16F15213_CPlex.X.debug.elf) is included.  The MPLabX project source is in PIC16F15213_CPlex.zip.

* MSVS 2022 project files are included if you need to recompile the DLL.  If QSpice is installed in the default location, you can debug component code in MSVS using QSPICE64.exe (i.e., non-GUI).  Hopefully all of the bits are there.

The framework is OK for the 8-pin PIC16F15213 device but will become unwieldy for large pin-count devices.  I plan to make changes to simplify creating new devices.  But, for now, it works.

## Contributors Needed

This project will be useful only to the extent that popular Microchip devices are supported.  If you implement a new device, please share.  I'll add your contribution to this repository and, of course, I'll be sure that you get credit.

## A Special Plea To Java-Coders

If you're a Java-coder with a little spare time, please contact me.  The framework is slow (and potentially unreliable) due to parsing the MDB command-line stuff.  A minimal rewrite of the MDB terminal interface to simplify machine-parsing could speed things up significantly.  I can do Java but I'm not really a fan and not currently properly set up for Java development.  I could use your help.

## Contact Me

You'll find me as @RDunn on the QSpice forums.  Send me a message if you have questions, suggestions, corrections, or code contributions.

--robert