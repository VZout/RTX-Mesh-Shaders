@echo off
setlocal

IF EXIST build_vs2019 (
	cd build_vs2019
	IF EXIST CMakeCache.txt (
		del CMakeCache.txt
	)
) ELSE (
	mkdir build_vs2019
	cd build_vs2019
)

cmake -G "Visual Studio 16 2019" -Xa64 ..

set /P c=Open Solution[[Y]/N]?
if /I "%c%" EQU "Y" goto :OPEN
if /I "%c%" EQU "N" goto :END

:OPEN
TuringMeshShaders.sln

:END
endlocal
