@echo off
REM ***********************************************************************
REM
REM          Author: kp
REM           $Date: 2008/01/21 10:36:39 $
REM       $Revision: 1.1 $
REM
REM     Description: script to make different CPU objects
REM
REM ---------------------------------[ History ]----------------------------
REM
REM    $Log: mk.bat,v $
REM    Revision 1.1  2008/01/21 10:36:39  ts
REM    Initial Revision, 1st work checkin
REM
REM    Revision 1.2  2005/10/27 09:44:31  UFranke
REM    change
REM     - PPC603 activated
REM
REM    Revision 1.1  2001/08/27 10:20:26  franke
REM    Initial Revision
REM
REM    Revision 1.1  2001/03/21 11:16:23  Lange
REM    Initial Revision
REM
REM    
REM
REM    
REM
REM ------------------------------------------------------------------------
REM    (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
REM
REM ************************************************************************

@echo "START"   > out

rem set WIND_HOST_TYPE=x86-win32
set GNUMAKE=%WIND_BASE%\host\%WIND_HOST_TYPE%\bin\make.exe
rem set GCC_EXEC_PREFIX=%WIND_BASE%\host\%WIND_HOST_TYPE%\lib\gcc-lib\

set TOOL=GNU
set MEN_DIR=
set MEN_WORK_DIR=/work

rem REM ====== DBG ==========
rem set DBG=-DDBG -DDBG_DUMP
rem rem -g -O0
rem set DBGDIR=test
rem REM
rem
rem set CPU=I80486
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL
rem
rem goto ENDEND

set DBG=
set DBGDIR=

rem set CPU=I80386
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=I80486
rem set DBG=-D__LITTLE_ENDIAN
rem   %GNUMAKE% -r -f makef.mak   >> out
rem   if errorlevel 1 GOTO FAIL

rem set CPU=PPC403
rem  %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

set CPU=PPC603
set DBG=
  %GNUMAKE% -r -f makef.mak   >> out
  if errorlevel 1 GOTO FAIL

rem set CPU=PPC604
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC860
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=MC68000
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

REM ====== DBG ==========
REM set DBG= -g -O0
REM set DBGDIR=test
REM

rem set CPU=I80386
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=I80486
rem  %GNUMAKE% -r -f makef.mak   >> out
rem  if errorlevel 1 GOTO FAIL

rem set CPU=MC68000
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set DBG=-DDBG -gdwarf -O0

rem set CPU=PPC403
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC603
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC604
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

rem set CPU=PPC860
rem %GNUMAKE% -r -f makef.mak   >> out
rem if errorlevel 1 GOTO FAIL

goto ENDEND

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND


:ENDEND

