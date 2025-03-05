@echo off
setlocal EnableDelayedExpansion
set PATH=..\windows;..\windows\lib;%PATH%
rem set PROJ_LIB=..\windows\lib

for %%o in (inputs\outlets*.shp) do (
	set o=%%~no
	set l=!o:outlets=lfp!
	melfp -l inputs\fdr.tif %%o cat outputs\!l!.tif
)
