@rem This batch command file is part of the C-Block Basics #8 project.
@rem See https://github.com/robdunn4/QSpice/ for additional documentation/code.
@rem
@rem Compiles cblockbasics8_pp.cpp to *.exe.  Edit DMCPATH as needed for
@rem non-default QSpice installation.
@rem
@set DMCPATH=C:\Program Files\QSPICE\dm\bin
@set path=%DMCPATH%;%path%
@rem 
cmd /k dmc.exe -mn -WA cblockbasics8_pp.cpp kernel32.lib
@rem command line window remains open...