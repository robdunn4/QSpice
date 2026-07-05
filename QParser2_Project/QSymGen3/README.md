# QParser2 Project &mdash; QSymGen3

QSymGen3 is a command-line utility to generate several files specifically for the QMdbSim2 project.  It uses a text file that describes a Microchip device and a DLL template file to produce QMdbSim2-ready symbol, schematic, and DLL source files.

The code demonstrates a number of techniques to parse text files and generate QSpice symbols and schematics using the QParser2 library.  MSVS project files are included so that you can modify/compile QSymGen3 from sources.

Because this tool is tightly coupled to the QMdbSim2 project, it is not documented in detail here.  See the QMdbSim2 Project for more information about using QSymGen3.

### Folder Contents
* Various \*.cpp/\*.h &mdash; Source code.  Requires QParser2 shared libraries.
* QSymGen3.vxproj.* &mdash; MSVS 2026 project files.
* QSymGen3.exe &mdash; Compiled binary.