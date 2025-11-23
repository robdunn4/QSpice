# QSpice

## Tools, components, symbols, code, etc., for Qorvo's free QSpice circuit simulator.

**QSpice/Qorvo Links**

Get the QSpice simulator from Qorvo [here](https://www.qorvo.com/).

The QSpice forum is [here](https://forum.qorvo.com/c/qspice/).

The Qorvo YouTube Channel is [here](https://www.youtube.com/c/qorvo).

Information about the Digitial Mars (DMC) compiler is [here](README_DMC.md).

**Other Recommended Sites**

The PyQSPICE Scripting Tool [GitHub repo](https://github.com/Qorvo/PyQSPICE).

QSpice Forum @KSKelvin's [Github repo](https://github.com/KSKelvin-Github/Qspice/).

QSpice Forum @Mgyger's [GitLab repo](https://gitlab.com/mgyger/qspice-symbols/).

QSpice Forum @physicboy's [Github repo](https://github.com/physicboy/QSPICE).

Professor Marcos Alonso's [QSpice YouTube Channel](https://www.youtube.com/@MarcosAlonsoElectronics) and [GitHub Repo](https://github.com/marcosalonsoelectronics/website).

**Useful Forum Threads**

"General Guide To Import A TI Spice Model" (KSKelvin) [[View Thread]](https://forum.qorvo.com/t/general-guide-to-import-a-ti-spice-model/23993)

"New User To Basic User (File Upload)" (KSKelvin) [[View Thread]](https://forum.qorvo.com/t/qspice-forum-new-user-to-basic-user-file-upload/24340)


## GPIO Components
QSpice symbol and class to implement general purpose I/O pins (i.e., micro-controller tri-state pins).

[Go to GPIO Components](./GPIO/)

## WavIO Components
QSpice C-Block/DLL components to read/write basic audio WAV files.  Source code and examples.

[Go to WavIO Components](./WavIO/)

## SerialIO Components
Proof of concept serial buffer.  See SpiIO components for improved serial buffer (in SpiIO.h).

[Go to SerialIO Components](./SerialIO/)

## SpiIO Components
QSpice C-Block code and schematics to implement custom SPI master & slave components.

[Go to SpiIO Components](./SpiIO/)

## PID Controller Component
QSpice C-Block implementation of a discrete PID controller courtesy of KSKelvin.

[Go to PID Controller Component](./PID_Controller/)

## C-Block Documentation
Various documentation and tools relevant to C-Block component development.

[Go to C-Block Documentation](./CBlock_Doc/)

## VSCode Configuration
Build configurations for C-Block component development using the VSCode IDE.

[Go to VSCode Configuration](./VSCode/)

## Microsoft Visual Studio (MSVC) Configuration
Step-by-step guide to configure a Visual Studio project to compile and debug C-Block components.

[Go to MSVC Configuration](./MSVS/)

## Miscellany
Odds and ends, bits and bobs that may be useful.

[Go to Misellany](./Miscellany/)

## QCodeGen
An alternative to the QSpice C-Block component code generator.

[Go to QCodeGen](./QCodeGen/)

## QTcpServer Project
A TCP-based client/server framework for QSpice C-Block components.

[Go to QTcpServer Project](./QTcpServer_Project/)

## QMcServer Project (On Hold)
A micro-controller simulation framework for QSpice.  Currently a (working) Proof of Concept for Microchip devices.

**Note:  This project is on hold.  See the QMdbSim Project below for the replacement.**

[Go to QMcServer Project](./QMcServer_Project/)

## QMdbSim Project
A micro-controller simulation framework for QSpice for Microchip devices.

[Go to QMdbSim Project](./QMdbSim_Project/)
