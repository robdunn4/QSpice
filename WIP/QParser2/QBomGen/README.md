# QParser2 Project &mdash; QBomGen

This folder contains a command-line tool to generate a Bill of Materials file from a QSpice schematic file.  The BOM file is a simple comma-separated-values file (*.csv) that you can open with Excel or LibreOffice Calc.

The current version of QBomGen doesn't have command-line switches or significant error handling code.  It is quite simple-minded, merely a proof-of-concept.

To give it a try, download the QBomGen.exe binary (or build it yourself), open a command-prompt window, and run:

    QBomGen.exe MySchematic.qsch

Where MySchematic.qsch is the name of the schematic to parse.  Assuming no errors, this will generate MySchematic_Out.csv.  If MySchematic_Out.csv exists, it will be overwritten without warning.
