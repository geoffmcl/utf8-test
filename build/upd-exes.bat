@setlocal

@set TMPDST=C:\MDOS
@fc4 -? >nul
@if ERRORLEVEL 4 goto NOFC4
@if ERRORLEVEL 3 goto GOTFC4
@goto NOFC4
:GOTFC4

@if NOT EXIST %TMPDST%\nul goto NODST

@set TMPCNT1=0
@set TMPCNT2=0
@set TMPEXES=chk-BOM.exe chk-con.exe chk-utf8.exe uni2utf8.exe unicode_utf8.exe utf8-test.exe
@set TMPUPDS=

@for %%i in (%TMPEXES%) do @(call :CNTIT %%i)

@echo.
@if "%TMPCNT2%x" == "0x" (
@echo Checked: %TMPEXES%
@echo.
@echo Of files %TMPCNT1% files checked, ***NONE*** need as update...
@echo.
@goto END
)

@echo Have %TMPCNT2% to update...
@echo Updates: %TMPUPDS%
@pause


@for %%i in (%TMPUPDS%) do @(call :CHKIT %%i)

@goto END

:CNTIT
@if "%~1x" == "x" goto :EOF
@set TMPEXE=%1
@set TMPSRC=Release\%TMPEXE%
@set TMPFIL=%TMPDST%\%TMPEXE%

@REM sanity checks
@if NOT EXIST %TMPSRC% goto :EOF

@set /A TMPCNT1+=1

@if NOT EXIST %TMPFIL% goto ISCOPY

@fc4 -q -v0 %TMPSRC% %TMPFIL% >nul

@if ERRORLEVEL 1 goto ISCOPY
@goto :EOF

:ISCOPY
@set TMPUPDS=%TMPUPDS% %1
@set /A TMPCNT2+=1
@goto :EOF

:CHKIT
@if "%~1x" == "x" goto :EOF
@set TMPEXE=%1
@set TMPSRC=Release\%TMPEXE%
@set TMPFIL=%TMPDST%\%TMPEXE%

@REM sanity checks
@if NOT EXIST %TMPSRC% goto NOSRC

@if NOT EXIST %TMPFIL% goto DOCOPY

@fc4 -q -v0 %TMPSRC% %TMPFIL% >nul

@if ERRORLEVEL 1 goto DOCOPY
@echo.
@echo Files %TMPSRC% and %TMPFIL% appear exactly the SAME
@echo *** Nothing to do here... ***
@echo.
@goto :EOF

:DOCOPY
copy %TMPSRC% %TMPFIL%
@goto :EOF

:NOSRC
@echo Can NOT locate src %TMPSRC%! *** FIX ME ***
@pause
@goto NOSRC

:NODST
@echo Can NOT locate dst %TMPDST%! *** FIX ME ***
@echo Set a VALID destination directory...
@pause
@goto NODST

:NOFC4
@echo Can NOT run comare utility! *** FIX ME ***
@echo Either get a copy of FC4, or
@echo change this batch to use another binary comare utility...
@pause
@goto NOFC4

:END
