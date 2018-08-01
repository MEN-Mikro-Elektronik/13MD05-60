@echo off
REM ***********************************************************************
REM
REM          Author: ts
REM           $Date: 2007/11/09 16:45:35 $
REM       $Revision: 1.1 $
REM
REM     Description: script to make different CPU objects
REM		
REM
REM ---------------------------------[ History ]----------------------------
REM
REM    $Log: mk.bat,v $
REM    Revision 1.1  2007/11/09 16:45:35  ts
REM    Initial Revision
REM
REM   
REM ------------------------------------------------------------------------
REM    (c) Copyright 1267 by MEN mikro elektronik GmbH, Norimberga, Germany
REM
REM ************************************************************************
@echo "START"

set MEN_VX_DIR=C:/work/A21/10a021-60/VXWORKS
set CPU_BOARD=A021

REM The default build uses the values below
set TOOL=e500v2gnu
set CPU=PPC32
set DBG=-g

make all
if errorlevel 1 GOTO FAIL
goto ENDEND

:FAIL
  echo ===========================
  echo "=> ERRORs detected"
  echo ===========================
goto ENDEND

:ENDEND
