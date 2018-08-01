@echo off
REM ***********************************************************************
REM
REM          Author: ag
REM           $Date: 2002/07/03 15:26:22 $
REM       $Revision: 1.1 $
REM
REM     Description: script to make different CPU objects
REM
REM ---------------------------------[ History ]----------------------------
REM
REM    $Log: mk.bat,v $
REM    Revision 1.1  2002/07/03 15:26:22  agromann
REM    Initial Revision
REM
REM
REM ------------------------------------------------------------------------
REM    (c) Copyright 2002 by MEN mikro elektronik GmbH, Nuernberg, Germany
REM
REM ************************************************************************
@echo "START"   > out

set WIND_HOST_TYPE=x86-win32
set GNUMAKE=%WIND_BASE%\host\%WIND_HOST_TYPE%\bin\make.exe
set GCC_EXEC_PREFIX=%WIND_BASE%\host\%WIND_HOST_TYPE%\lib\gcc-lib\

set TOOL=gnu
set MEN_DIR=
set SMEN_DIR=s:/work

rem REM ====== DBG ==========
rem set DBG=-DDBG -DDBG_DUMP
rem rem -g -O0
rem set DBGDIR=test
rem REM
rem
rem set CPU=I80486
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL
rem
rem goto ENDEND


REM #### Build non Debug version ####
set DBG=
set DBGDIR=

rem set CPU=I80386
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=I80486
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

set CPU=PPC85XX
 %GNUMAKE% -r -f makef   >> out
 if errorlevel 1 GOTO FAIL

rem set CPU=PPC403
rem  %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC603
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC604
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC860
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=MC68040
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=MC68060
rem  %GNUMAKE% -r -f makef   >> out
rem  if errorlevel 1 GOTO FAIL

REM #### Build Debug version ####
set DBG= -g -O0
set DBGDIR=test
REM

rem set CPU=I80386
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=I80486
rem  %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

REM set CPU=PENTIUM
REM %GNUMAKE% -r -f makef   >> out
REM if errorlevel 1 GOTO FAIL

rem set CPU=MC68040
rem  %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=MC68060
rem  %GNUMAKE% -r -f makef   >> out
rem  if errorlevel 1 GOTO FAIL

rem set DBG=-DDBG -gdwarf -O0

rem set CPU=PPC403
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC603
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC604
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC860
rem %GNUMAKE% -r -f makef   >> out
rem if errorlevel 1 GOTO FAIL

goto ENDEND

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND


:ENDEND

