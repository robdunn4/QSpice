# QParser2 Project &mdash; QSymGen2

| Run QSymGen2 | Generated Symbol | Generated DLL Schematic | User-Created Schematic |
| --- | --- | --- | --- |
| <img src="./images/CmdPrompt.png" width=300>  | <img src="./images/Generated_Symbol.png" width=300> | <img src="./images/Generated_Schematic.png" width=300> | <img src="./images/User_Schematic.png" width=300>| 


QSymGen2 is a command-line utility to generate QSpice symbol and schematic files from a text file containing micro-controller pin definitions.  The generated symbol is suitable to drop onto a schematic.  The generated schematic includes GPIO pin support and contains the C-Block for the underlying micro-controller DLL.

This project is under development and full of half-baked code and, undoubtably, incorrect comments.

## Files

### Main Folder

* QSymGen2_User.pdf &mdash; User documentation.  Just enough to get you started.
* Various \*.cpp/\*.h &mdash; Source code.  Requires QParser2 shared libraries.
* QSymGen2.vxproj.* &mdash; MSVS 2026 project files.
* QSymGen2.exe &mdash; Compiled binary.  Run without parameters to get syntax help.

### Test Subfolder

* PIC16F1521x.qpins &mdash; Pin definitions for example PIC16F15213/PIC16F15214 micro-controller.
* GPIO_IMPL.qsch &mdash; Circuit used for GPIO pin implementation.  (User can edit for a different GPIO implementation.)
* PIC16F1521x.qsym &mdash; Example symbol generated from the PIC16F1521x.qpins file.  Not intended for user editing.
* PIC16F1521x.qsch &mdash; Example DLL implementation schematic generated from the PIC16F1521x.qpins file.  Not intended for user editing.  However, user would generate the DLL template code from the DLL block in this file.
* PIC16F1521x_DLL.cpp &mdash; Example DLL implementation code for demonstration only.
* PIC16F1521x_TopLevel.qsch &mdash; Example schematic using the generated PIC16F1521x.qsym file.  Users would start with a new schematic, drop the PIC16F1521x.qsym file, and add circuitry to create their version of the micro-controller project.  This is what that might look like.
