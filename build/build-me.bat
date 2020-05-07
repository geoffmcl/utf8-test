@setlocal

@set TMPVER=1
@set TMPPRJ=utf8-test
@set TMPSRC=..
@set TMPBGN=%TIME%
@set TMPINS=D:\Projects\3rdParty.x64
@set DOTINST=0
@set TMPLOG=bldlog-1.txt

@set TMPOPTS=-DCMAKE_INSTALL_PREFIX=%TMPINS%
:RPT
@if "%~1x" == "x" goto GOTCMD
@set TMPOPTS=%TMPOPTS% %1
@shift
@goto RPT
:GOTCMD

@call chkmsvc %TMPPRJ%

@echo Build %DATE% %TIME% > %TMPLOG%

@if NOT EXIST %TMPSRC%\nul goto NOSRC

@echo Build source %TMPSRC% - all output to build log %TMPLOG%
@echo Build source %TMPSRC% - all output to build log %TMPLOG% >> %TMPLOG%

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
