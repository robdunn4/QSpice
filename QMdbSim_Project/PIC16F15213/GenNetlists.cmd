@rem generate *.cir for all *.qsch
@for %%f in (*.qsch) do (
	cmd /c "c:\\program files\\qspice\\qux.exe" -Netlist %%f
	@rem force linebreak to separate text sent by QUX to stdout
	@echo:
)

