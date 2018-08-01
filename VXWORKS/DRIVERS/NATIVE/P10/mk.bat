@echo off
REM ***********************************************************************
REM
REM          Author: rl
REM           $Date: 2006/02/23 12:23:47 $
REM       $Revision: 1.3 $
REM
REM     Description: script to make different CPU objects
REM
REM ---------------------------------[ History ]----------------------------
REM
REM    $Log: mk.bat,v $
REM    Revision 1.3  2006/02/23 12:23:47  ts
REM    support for EM3 family (85XX)
REM
REM    Revision 1.2  2002/04/24 15:49:42  Franke
REM    cosmetics
REM
REM    Revision 1.1  2000/03/14 14:30:35  loesel
REM    Initial Revision
REM
REM    
REM
REM    
REM ------------------------------------------------------------------------
REM    (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
REM
REM ************************************************************************
@echo "START"   > out

set WIND_HOST_TYPE=x86-win32
set GNUMAKE=%WIND_BASE%\host\%WIND_HOST_TYPE%\bin\make.exe
set GCC_EXEC_PREFIX=%WIND_BASE%\host\%WIND_HOST_TYPE%\lib\gcc-lib\

set TOOL=gnu
rem set TOOL=gnu
set MEN_WORK_DIR=s:/work


REM ====== NO DBG ==========
set DBG=
set DBGDIR=
REM

rem set CPU=I80386

rem set CPU=I80486

rem set CPU=PENTIUM3

rem set CPU=PENTIUM

rem set CPU=PPC403

set CPU=PPC85XX

rem set CPU=PPC604

rem set CPU=PPC860

rem set CPU=MC68000

rem set PATH=C:\Tornado\Tor221ppc\host\gnu\3.3\x86-win32\lib\gcc-lib\powerpc-wrs-vxworks\3.3-e500;%PATH%

%GNUMAKE% -r -f makef.mak   >> out
if errorlevel 1 GOTO FAIL

REM ====== NO DBG ==========
set DBG=
set DBGDIR=
REM


REM ====== DBG ==========
set DBG= -g -O0
set DBGDIR=test
REM

rem set CPU=I80386

rem set CPU=I80486

rem set CPU=PENTIUM

rem set CPU=MC68000

rem set DBG=-DDBG -gdwarf -O0

rem set CPU=PPC403

rem set CPU=PPC603

rem set CPU=PPC604

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
