@echo off
rem This batch file is part of the C-Block Basics #8 project.
rem See https://github.com/robdunn4/QSpice/ for additional documentation/code.
rem
if not exist cblockbasics8_pp.exe ( 
exit -1
) else (
cblockbasics8_pp.exe %1 %2 %3
)
exit %errorlevel%
