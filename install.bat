@echo off
REM ##########################################################################
REM
REM       file: install.bat
REM
REM       installer batch for creating a MDIS for VxWorks system package.
REM       uses only basic DOS batch commands.
REM ##########################################################################

cls
set G_defaultDir=c:\13MD05-60

echo.
echo  _________________________________________________________________________________________
echo ^|                                                                                         ^|
echo ^| Welcome to the MDIS for VxWorks System Package installer from MEN Mikro Elektronik GmbH.^|
echo ^| This installer will guide you through the installation process and perform required     ^|
echo ^| steps.                                                                                  ^|
echo ^|_________________________________________________________________________________________^|
echo.
echo.
if "%1"=="" (
set G_vxPath=%G_defaultDir%
) else (
set G_vxPath=%1
)

@echo using install path %G_vxPath%

set answer=""
set /p answer=Continue installation ([y/n], ENTER=n)?

if "%answer%"=="y" (
  echo ok, installing to %G_vxPath%
) else (
  goto :END
)

REM --
REM -- Checks: if path doesn't exist, create it. If it exists and is nonempty, complain and exit ---
REM --

set answer=""
if not exist %G_vxPath% set /p answer=directory %G_vxPath% doesnt exist. Create ? [y/n]
if "%answer%"=="y" (
  echo Creating %G_vxPath% ...
  mkdir %G_vxPath%
)

REM TODO check folder is empty (when just created thats the case anyway)


REM -------------------------------------------------------
REM -- copy together our regular MDIS package directory ---
REM -------------------------------------------------------

echo Copying folder VXWORKS to %G_vxPath% ...
xcopy /S /E /I VXWORKS %G_vxPath%\VXWORKS > NUL
echo Copying folder LINUX to %G_vxPath% ...
xcopy /S /E /I LINUX %G_vxPath%\LINUX > NUL
echo Copying folder NT to %G_vxPath% ...
xcopy /S /E /I NT %G_vxPath%\NT > NUL

REM --
REM --  Copy LL Drivers content, therefor ('dir /B 13*-06') ---
REM --
for /F %%i in ('dir /B 13*-06') do (
   cd %%i
   echo Copying Low Level Driver %%i to %G_vxPath%\DRIVERS\MDIS_LL
   xcopy /S /E /I DRIVERS\MDIS_LL\* %G_vxPath%\VXWORKS\DRIVERS\MDIS_LL\
   xcopy /S /E  INCLUDE\COM\MEN\* %G_vxPath%\VXWORKS\INCLUDE\COM\MEN\
   cd ..
)

REM --
REM --  Copy native Drivers content, therefor ('dir /B 13*-60') ---
REM --
for /F %%i in ('dir /B 13*-60') do (
   cd %%i
   echo Copying native Driver %%i to %G_vxPath%\VXWORKS\DRIVERS\MDIS_LL
   xcopy /S /E /I DRIVERS\NATIVE\* %G_vxPath%\VXWORKS\DRIVERS\NATIVE\ > NUL
   cd ..
)

REM --
REM --  Copy XML files from LL and also native drivers, therefore ('dir /B 13*') ---
REM --

echo Copying XML Files %G_vxPath%\PACKAGE_DESC ...
REM mkdir %G_vxPath%\VXWORKS\PACKAGE_DESC

for /F %%i in ('dir /B 13*') do (
   cd %%i
   copy PACKAGE_DESC\*.xml %G_vxPath%\VXWORKS\PACKAGE_DESC\ > NUL
   cd ..
)




:END
exit /B

:USAGE
@echo usage: install.bat <folder name>
@echo e.g.  install.bat c:\work\MDIS