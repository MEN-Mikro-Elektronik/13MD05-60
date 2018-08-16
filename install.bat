@echo off
REM ##########################################################################
REM
REM       file: install.bat
REM       author: thomas.schnuerer@men.de
REM
REM       installer batch for creating a MDIS for VxWorks system package
REM       containing all lowlevel drivers.
REM       uses only basic DOS batch commands.
REM
REM ##########################################################################

cls
set G_defaultDir=c:\13MD05-60
set G_logfile=setup.log

echo.
echo  _________________________________________________________________________________________
echo ^|                                                                                         ^|
echo ^| Welcome to the MDIS for VxWorks System Package installer from MEN Mikro Elektronik GmbH.^|
echo ^| This installer will guide you through the installation process and perform required     ^|
echo ^| steps.                                                                                  ^|
echo ^| Usage: install.bat [^<installation path^>]                                                ^|
echo ^|_________________________________________________________________________________________^|
echo.
echo.
if "%1"=="" (
set G_MdisInstallPath=%G_defaultDir%
) else (
set G_MdisInstallPath=%1
)

echo.
echo using install path %G_MdisInstallPath%

set answer=""
set /p answer=Continue installation ([y/n], ENTER=n)?

if "%answer%"=="y" (
  echo ok, installing to %G_MdisInstallPath%
) else (
  goto :END
)

REM ------------------------------------------------------------
REM -- if path doesn't exist, create it. If it exists and is
REM -- nonempty, complain and exit
REM ------------------------------------------------------------
set answer=""
if not exist %G_MdisInstallPath% set /p answer=directory %G_MdisInstallPath% doesnt exist. Create ? [y/n]
if "%answer%"=="y" (
  echo Creating %G_MdisInstallPath% ...
  mkdir %G_MdisInstallPath%
)


REM TODO check if folder is empty (seems to be an impossible nightmare under plain DOS)...
REM however in 99% of cases the folder was created fresh.


REM ------------------------------------------------------------
REM -- write logfile with all relevant informations for support
REM ------------------------------------------------------------
echo. >> %G_MdisInstallPath%\%G_logfile%
echo --------------------------------------------------------------- >> %G_MdisInstallPath%\%G_logfile%
echo MEN Mikro Elektronik GmbH %G_logfile% for MDIS for VxWorks System Package >> %G_MdisInstallPath%\%G_logfile%
echo Logfile created: %DATE% %TIME% >> %G_MdisInstallPath%\%G_logfile%
echo. >> %G_MdisInstallPath%\%G_logfile%
echo installed versions: >> %G_MdisInstallPath%\%G_logfile%
git describe --dirty --long --tags --always >> %G_MdisInstallPath%\%G_logfile%


REM ------------------------------------------------------------
REM -- copy together our regular MDIS package directory
REM ------------------------------------------------------------

echo Copying folder VXWORKS to %G_MdisInstallPath% ...
xcopy /S /E /I VXWORKS %G_MdisInstallPath%\VXWORKS > NUL
echo Copying folder LINUX to %G_MdisInstallPath% ...
xcopy /S /E /I LINUX %G_MdisInstallPath%\LINUX > NUL
echo Copying folder NT to %G_MdisInstallPath% ...
xcopy /S /E /I NT %G_MdisInstallPath%\NT > NUL


REM ------------------------------------------------------------
REM -- copy LL Drivers content, therefore ('dir /B 13*-06')
REM ------------------------------------------------------------
for /F %%i in ('dir /B 13*-06') do (
   cd %%i
   echo Copying Low Level Driver %%i to %G_MdisInstallPath%\DRIVERS\MDIS_LL
   git describe --dirty --long --tags --always >> %G_MdisInstallPath%\%G_logfile%
   xcopy /S /E /I DRIVERS\MDIS_LL\* %G_MdisInstallPath%\VXWORKS\DRIVERS\MDIS_LL\ > NUL
   xcopy /S /E  INCLUDE\COM\MEN\* %G_MdisInstallPath%\VXWORKS\INCLUDE\COM\MEN\ > NUL
   cd ..
)


REM ------------------------------------------------------------
REM -- copy native Drivers content, therefore ('dir /B 13*-60')
REM ------------------------------------------------------------
for /F %%i in ('dir /B 13*-60') do (
   cd %%i
   echo Copying native Driver %%i to %G_MdisInstallPath%\VXWORKS\DRIVERS\NATIVE
   git describe --dirty --long --tags --always >> %G_MdisInstallPath%\%G_logfile%
   xcopy /S /E /I DRIVERS\NATIVE\* %G_MdisInstallPath%\VXWORKS\DRIVERS\NATIVE\ > NUL
   REM -- INCLUDE/NATIVE/MEN folder exists not always e.g. 13M077-60. we just copy and ignore the error
   xcopy /S /E INCLUDE\NATIVE\MEN\* %G_MdisInstallPath%\VXWORKS\INCLUDE\NATIVE\MEN\ > NUL 2>&1
   cd ..
)


REM ------------------------------------------------------------
REM -- copy XML files of all drivers, therefore ('dir /B 13*')
REM ------------------------------------------------------------
echo Copying XML Files %G_MdisInstallPath%\PACKAGE_DESC ...
mkdir %G_MdisInstallPath%\VXWORKS\PACKAGE_DESC
for /F %%i in ('dir /B 13*') do (
   cd %%i
   copy PACKAGE_DESC\*.xml %G_MdisInstallPath%\VXWORKS\PACKAGE_DESC\ > NUL
   cd ..
)


echo.
echo Success! Installation is finished. Your MDIS for VxWorks System package plus drivers has been installed to %G_MdisInstallPath%.
echo Your next possibilities: create a MDIS project using MDIS wizard
echo (%G_MdisInstallPath%/NT/OBJ/EXE/MEN/I386/FREE/mdiswizvx.exe) or open an existing MDIS project with it.
echo See also MDIS for VxWorks User Guide!
echo.
echo If you want you can now delete this folder.
echo.


:END
exit /B
