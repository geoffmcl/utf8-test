@setlocal
@set TMPCNT0=0
@set TMPCNT1=0
@set TMPCNT2=0

@for %%i in (Release\*.exe) do @(call :CNTIT %%i)

@echo Got %TMPCNT1% of %TMPCNT0% EXE to update...
@if %TMPCNT1% EQU 0 goto END

@choice /D N /T 10 /M "Pausing for 10 seconds. Def=N"
@if ERRORLEVEL 2 goto END

@for %%i in (Release\*.exe) do @(call :DOIT %%i)

@echo Updated %TMPCNT2% of %TMPCNT1% EXE...

@goto END

:CNTIT
@if "%~1x" == "x" goto :EOF
@set TMPSRC=%1
@set TMPDST=C:\MDOS\%~nx1
@set /A TMPCNT0+=1
@if NOT EXIST %TMPDST% goto CNTIT2
@fc4 -q -v0 -b %TMPSRC% %TMPDST% >nul
@REM if ERRORLEVEL 2 goto NOFC42
@if ERRORLEVEL 1 goto CNTIT2
@goto :EOF
:CNTIT2
@echo Copy %TMPSRC% %TMPDST%
@set /A TMPCNT1+=1
@goto :EOF

:DOIT
@if "%~1x" == "x" goto :EOF
@set TMPSRC=%1
@set TMPDST=C:\MDOS\%~nx1
@if NOT EXIST %TMPDST% goto DOCOPY
@fc4 -q -v0 -b %TMPSRC% %TMPDST% >nul
@if ERRORLEVEL 2 goto NOFC42
@if ERRORLEVEL 1 goto DOCOPY2
@echo Files %TMPSRC% %TMPDST% are the SAME... Nothing done...
@goto :EOF
:NOFC42
@echo Can not locate 'fc4' utility
@goto :EOF
:DOCOPY2
@echo.
@echo Need update of %TMPDST%...
:DOCOPY
@echo Copy %TMPSRC% %TMPDST%
@choice /D N /T 10 /M "Pausing for 10 seconds. Def=N"
@if ERRORLEVEL 2 goto :EOF
Copy %TMPSRC% %TMPDST%
@set /A TMPCNT2+=1
@goto :EOF

:END
