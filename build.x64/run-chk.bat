@setlocal
@set TMPEXE=Release\chk-utf8.exe
@if NOT EXIST %TMPEXE% goto NOEXE
@set TMPFIL=F:\Projects\tidy-test\test\input5\in_467-5.html
@if NOT EXIST %TMPFIL% goto NOFIL

%TMPEXE% %TMPFIL% %*

@goto END

:NOEXE
@echo NOT EXIST %TMPEXE%! *** FIX ME ***
@goto END

:NOFIL
@echo NOT EXIST %TMPFIL%! *** FIX ME ***
@goto END

:END


