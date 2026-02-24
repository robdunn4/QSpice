@echo off
ECHO :
ECHO : This batch file creates MyBackup.qcolorpref theme containing
ECHO : the current QSpice color settings.  If MyBackup.qcolorpref 
ECHO : already exists and would be changed, you will be prompted to
ECHO : confirm the operation.
ECHO :
ECHO : To exit this command prompt window, type EXIT and press return
ECHO :
cmd.exe /k QColorPrefs.exe -sall MyBackup
REM
REM End of file