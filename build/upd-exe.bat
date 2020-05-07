@setlocal

@set TMPDST=C:\MDOS
@fc4 -? >nul
@if ERRORLEVEL 4 goto NOFC4
@if ERRORLEVEL 3 goto GOTFC4
@goto NOFC4
:GOTFC4

@set TMPEXES=chk-BOM.exe chk-con.exe chk-utf8.exe uni2utf8.exe unicode_utf8.exe utf8-test.exe

@for %%i in (%TMPEXES%) do @(call :CHKIT %%i)

@goto END

:CHKIT
@if "%~1x" == "x" goto :EOF
@set TMPEXE=%1
@set TMPSRC=Release\%TMPEXE%
@set TMPFIL=%TMPDST%\%TMPEXE%

@REM sanity checks
@if NOT EXIST %TMPSRC% goto NOSRC
@if NOT EXIST %TMPDST%\nul goto NODST

@if NOT EXIST %TMPFIL% goto DOCOPY

@fc4 -q -v0 %TMPSRC% %TMPFIL% >nul

@if ERRORLEVEL 1 goto DOCOPY
@echo.
@echo Files %TMPSRC% and %TMPFIL% appear exactly the SAME
@echo *** Nothing to do here... ***
@echo.
@goto END

:DOCOPY
copy %TMPSRC% %TMPFIL%
@goto END

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
