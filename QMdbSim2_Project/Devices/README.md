# QMdbSim2 Project &mdash; Available Devices & Miscellaneous Tools

## Available Devices
These devices are currently available in the repository.  If you don't find what you need, it is really, really easy to create new device defintions &mdash; see the Device Developer documentation.

| Device Folder | Supported Devices | Full<br>Demo |
|---|---|:---:|
| [ATtiny85](./ATtiny85/) | ATtiny25, ATtiny45, ATtiny85 | ✓ |
| [PIC16F15213](./PIC16F15213/) | PIC16F15213, PIC16F15214 | ✓ |
| [PIC18F4550](./PIC18F4550/) | PIC18F4455, PIC18F4550  | ✗ |
| [PIC18F45K50](./PIC18F45K50/) | PIC18F45K50, PIC18LF45K50  | ✗ |

All devices include sources and compiled DLL binaries.  Items with Full Demo include top-level demonstration schematic and device-specific source and binary (e.g., \*.elf/\*.hex).  Otherwise, assume that this is an untested device definition.  (For these, be a hero and provide the missing demonstration bits.)

## Miscellaneous Device Developer Tools

* [Claude Tools](./Claude_Prompts/) &mdash; "Playbook" and "How To" for using Claude.AI to generate Pin Definition files from Microchip datasheets.  (Works better than I expected.)
* [MSVS Device Templates](./MSVS_Device_Templates/) &mdash; A MSVS 2026 template for adding a new device-specific project to the Devices folder.

## Contributors Needed

This project will be useful only to the extent that popular Microchip devices are supported.  If you implement a new device, please share.  I'll add your contribution to this repository and, of course, I'll be sure that you get credit.

Contact me as @RDunn on [Qorvo's QSpice forum](https://forum.qorvo.com/c/qspice/).
