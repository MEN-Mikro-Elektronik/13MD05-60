# 13MD05-60

Containing toplevel repository with MDIS for VxWorks System Package plus lowlevel (MDIS LL) drivers

This repository represents the complete MEN MDIS for VxWorks System Package plus lowlevel drivers.

Cloning the repository:
-----------------------

run the commands to clone 13MD05-60 and populate the submodule folder:

git clone --recursive https://github.com/MEN-Mikro-Elektronik/13MD05-60.git <br />

or (ssh key is needed):

git clone git@github.com:MEN-Mikro-Elektronik/13MD05-60.git <br />
git submodule update --init --recursive


After cloning the repository run either install.bat (for a Windows host) or install.sh for linux hosts to copy the Files to the desired installation directory.

Modifying the sources:
----------------------

The rules for the MDIS-Linux variant should be followed also for VxWorks, in order to establish and/or maintain good quality and structure:

https://github.com/MEN-Mikro-Elektronik/13MD05-90/wiki/Configuration-Management-Plan-for-MDIS


Notes:
----------------------
MDIS5 for VxWorks doesn't support vxWorks 64bits
