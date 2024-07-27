# About the QSpice Compiler (DMC)

>*To be honest, the DMC compiler and QSpice code-editing environment are painfully basic.  You can use more modern compilers and IDEs.  See the VSCode information elsewhere in this repository to use the free VSCode IDE with free Microsoft Visual C++ and MinGW/GNU compilers.*

QSpice ships with a free C/C++ compiler from Digital Mars (DMC).  Unfortunately, complete documentation is not included.  I struggled to find things as basic and essential as what C/C++ standards the DMC compiler implements.

Hopefully, the below notes and links will save you some time.

## DMC Releases/Supported Standards

**C++98 Standard** &mdash; Implementation-specific information is [here](https://digitalmars.com/ctg/CPP-Language-Implementation.html).

**C99 Standard** &mdash; Implementation-specific information is [here](https://digitalmars.com/ctg/C-Language-Implementation.html).

Compiler and tools documentation is [here](https://www.digitalmars.com/ctg/ctg.html).

Compiler command line switches are documented [here](https://digitalmars.com/ctg/sc.html).

The DMC release changelog can be found [here](https://www.digitalmars.com/changelog.html).

* The last DMC release appears to be 8.57 (2013.08.01).

* The QSpice-shipped version appears to be 8.42 (2005.03.02).

I'm uncertain why the QSpice version is older but don't recommend updating if you want QSpice support.

## DMC Source Code

The DMC compiler is open source and available on this [GitHub repository](https://github.com/DigitalMars/Compiler).
