@setlocal

@set VCVERS=14
@set TMPPRJ=utf8-test
@set TMPSRC=..
@set TMPBGN=%TIME%
@set TMPINS=..\..\software.x64
@set DOTINST=0
@set TMPLOG=bldlog-1.txt

@REM ############################################
@REM NOTE: MSVC XX INSTALL LOCATION
@REM Adjust to suit your environment
@REM ##########################################
@set GENERATOR=Visual Studio %VCVERS% Win64
@set VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio %VCVERS%.0
@set VC_BAT=%VS_PATH%\VC\vcvarsall.bat
@if NOT EXIST "%VS_PATH%" goto NOVS
@if NOT EXIST "%VC_BAT%" goto NOBAT
@set BUILD_BITS=%PROCESSOR_ARCHITECTURE%

@set TMPOPTS=-DCMAKE_INSTALL_PREFIX=%TMPINS%
@if EXIST X:\3rdParty.x64\nul (
@set TMPOPTS=%TMPOPTS% -DCMAKE_PREFIX_PATH:PATH=X:\3rdParty.x64
) else (
@echo Can NOT locate X:\3rdParty.x64! *** FIX ME ***
@exit /b 1
)
@set TMPOPTS=%TMPOPTS% -G "%GENERATOR%"

:RPT
@if "%~1x" == "x" goto GOTCMD
@set TMPOPTS=%TMPOPTS% %1
@shift
@goto RPT
:GOTCMD

@if NOT EXIST %TMPSRC%\nul goto NOSRC

@call chkmsvc %TMPPRJ%

@echo Build %TMPPRJ% msvc140.x64 %DATE% %TIME% > %TMPLOG%

@echo Build source %TMPSRC% - all output to build log %TMPLOG%
@echo Build source %TMPSRC% - all output to build log %TMPLOG% >> %TMPLOG%

@echo Setting environment - CALL "%VC_BAT%" %BUILD_BITS%
@echo Setting environment - CALL "%VC_BAT%" %BUILD_BITS% >> %TMPLOG%
@call "%VC_BAT%" %BUILD_BITS% >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto NOSETUP

@if EXIST build-cmake.bat (
@call build-cmake >> %TMPLOG%
)

@if NOT EXIST %TMPSRC%\CMakeLists.txt goto NOCM

@echo Doing 'cmake %TMPSRC% %TMPOPTS%'
@echo Doing 'cmake %TMPSRC% %TMPOPTS%' >> %TMPLOG%
@cmake %TMPSRC% %TMPOPTS% >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR1

@echo Doing 'cmake --build . --config Debug'
@echo Doing 'cmake --build . --config Debug'  >> %TMPLOG%
@cmake --build . --config Debug  >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR2

@echo Doing 'cmake --build . --config Release'
@echo Doing 'cmake --build . --config Release'  >> %TMPLOG%
@cmake --build . --config Release  >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR3

@fa4 "***" %TMPLOG%
@call elapsed %TMPBGN%
@echo Appears a successful build... see %TMPLOG%

@echo.
@echo No install at this time...
@echo There may be an updexe.bat...
@echo.
@goto END

@echo Continue with install? Only Ctrl+c aborts...

@pause

cmake --build . --config Debug  --target INSTALL >> %TMPLOG% 2>&1
@if EXIST install_manifest.txt (
@copy install_manifest.txt install_manifest_dbg.txt >nul
@echo. >> %TMPINS%/installed.txt
@echo = %TMPRJ% Debug install %DATE% %TIME% >> %TMPINS%/installed.txt
@type install_manifest.txt >> %TMPINS%/installed.txt
)

cmake --build . --config Release  --target INSTALL >> %TMPLOG% 2>&1
@if EXIST install_manifest.txt (
@copy install_manifest.txt install_manifest_rel.txt >nul
@echo. >> %TMPINS%/installed.txt
@echo = %TMPRJ% Release install %DATE% %TIME% >> %TMPINS%/installed.txt
@type install_manifest.txt >> %TMPINS%/installed.txt
)

@call elapsed %TMPBGN%
@echo All done... see %TMPLOG%

@goto END

:NOSETUP
@echo MSVC setup FAILED!
@goto ISERR

:NOBAT
@echo Can not locate "%VC_BAT%"! *** FIX ME *** for your environment
@goto ISERR

:NOVS
@echo Can not locate "%VS_PATH%"! *** FIX ME *** for your environment
@goto ISERR

:NOSRC
@echo Can NOT locate source %TMPSRC%! *** FIX ME ***
@echo Can NOT locate source %TMPSRC%! *** FIX ME *** >> %TMPLOG%
@goto ISERR

:NOCM
@echo Can NOT locate %TMPSRC%\CMakeLists.txt!
@echo Can NOT locate %TMPSRC%\CMakeLists.txt! >> %TMPLOG%
@goto ISERR

:ERR1
@echo cmake configuration or generations ERROR
@echo cmake configuration or generations ERROR >> %TMPLOG%
@goto ISERR

:ERR2
@echo ERROR: Cmake build Debug FAILED!
@echo ERROR: Cmake build Debug FAILED! >> %TMPLOG%
@goto ISERR

:ERR1
@echo ERROR: Cmake build Release FAILED!
@echo ERROR: Cmake build Release FAILED! >> %TMPLOG%
@goto ISERR

:ISERR
@echo See %TMPLOG% for details...
@endlocal
@exit /b 1

:END
@endlocal
@exit /b 0

@REM eof
