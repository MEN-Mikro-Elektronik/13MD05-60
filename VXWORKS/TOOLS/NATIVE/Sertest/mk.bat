@echo off
setlocal
REM ***********************************************************************
REM
REM          Author: kp
REM           $Date: 2006/09/11 10:52:30 $
REM       $Revision: 1.3 $
REM
REM     Description: script to make different CPU objects
REM
REM ---------------------------------[ History ]----------------------------
REM
REM    $Log: mk.bat,v $
REM    Revision 1.3  2006/09/11 10:52:30  cs
REM    added support for VxW6.x
REM
REM    Revision 1.2  2004/03/02 08:50:16  UFranke
REM    fixed for Tornado 2.2
REM
REM    Revision 1.1  2000/01/14 15:00:39  loesel
REM    Initial Revision
REM
REM
REM
REM ------------------------------------------------------------------------
REM    (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
REM
REM ************************************************************************

@echo "START"   > out

if "%WIND_TOOLS%"=="" GOTO VXW_BEFORE_6X
GOTO VXW_6X

:VXW_BEFORE_6X
rem set WIND_HOST_TYPE=x86-win32
set GNUMAKE=%WIND_BASE%\host\%WIND_HOST_TYPE%\bin\make.exe
set TOOL=GNU
rem set GCC_EXEC_PREFIX=%WIND_BASE%\host\%WIND_HOST_TYPE%\lib\gcc-lib\
GOTO VXW_ALL

:VXW_6X
set GNUMAKE=make
GOTO VXW_ALL

:VXW_ALL
set TOOL=gnu

rem REM ====== DBG ==========
rem set DBG=-DDBG -DDBG_DUMP
rem rem -g -O0
rem set DBGDIR=test
rem REM
rem
rem set CPU=I80486
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL
rem
rem goto ENDEND




set DBG=
set DBGDIR=

rem set CPU=I80386
rem   %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=I80486
rem    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem  if errorlevel 1 GOTO FAIL

rem set CPU=PENTIUM
rem    %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem  if errorlevel 1 GOTO FAIL

rem set CPU=PPC403
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

set CPU=PPC603
 %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
 if errorlevel 1 GOTO FAIL

rem set CPU=PPC604
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC85XX
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC860
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=MC68000
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

REM ====== DBG ==========
set DBG= -g -O0
set DBGDIR=test


rem set CPU=I80386
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=I80486
rem   %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem  if errorlevel 1 GOTO FAIL

rem set CPU=MC68000
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set DBG=-DDBG -gdwarf -O0

rem set CPU=PPC403
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

 set CPU=PPC603
  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
 if errorlevel 1 GOTO FAIL

rem set CPU=PPC604
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC85XX
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC860
rem  %GNUMAKE% CPU=%CPU% TOOL=%TOOL% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

goto ENDEND

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND


:ENDEND

