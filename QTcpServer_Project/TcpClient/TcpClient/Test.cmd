@REM Example batch file for QTcpServer project
@REM
@echo Test.cmd called with parameter: %1
@echo send text to test.txt >> test.txt
@echo current working directory is...
@cd 
@echo send text to stderr >&2
@exit 666
