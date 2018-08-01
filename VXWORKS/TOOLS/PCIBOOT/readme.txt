README for EP04 PCI boot utility
--------------------------------

FILESET:
-use VXWORKS/TOOLS/pciboot_vx fileset

HW-INSTALLATION:
-make shure EP4 boot-sequencer eeprom is programmed (*.e00 file)
-connect EP4 to Host (via PCIe)
-make shure EP4 is in PCIe-mode (check DIP switch 10 at TB01)
-turn on Host

START PROGRAM:
-open command prompt
-change directory to PCIBOOT directory
-call ep04_pciboot (use -v=... fore more output; -h for help)

NOTE:	EP4 has to be connected to Host during Host's power-up. After that 
		EP4 is hot-pluggable.

NOTE: 	If EP4's PCI config registers are not set up correct (e.g. EP4 re-
 	   	connected) in the moment you start the programm, you have to use
 	   	options -c and -d (e.g. ep04_pciboot "-v=3 -c=fdb00000 -d=f8000000").


ts@men.de supplemental for update with Memphis, 02.07.2013:

Like mentioned above, the short version
     ep04_pciboot "-b=MENMON13.BIN -e"
works ONLY until a (manual) reset is applied to EP04. This can cause confusion
when repeating the test manually. The PCI BAR inits are lost then and a error
"invalid CCSR 0" appears.

=> Its safer to use always the long form
     ep04_pciboot "-b=MENMONnn.BIN -e -v=3 -c=fdb00000 -d=f8000000"
   because the BARs wont change due to the fixed setup.

!!! Attention !!!
The file MENMONnn.BIN is the RAM-version of the Menmon, menmon_STD.bin
(nn is the current Revision)

This can be seen on the first boot message
[SDRAM init skipped!]
that appears only for RAM based Version



The PCI Registers look like dumped below when initialized and empty (= after reset).

Unitialized PCI registers:
-------------------------


-> pciDeviceShow 2       
Scanning functions of each PCI device on bus 2
Using configuration mechanism 1
bus       device    function  vendorID  deviceID  class/rev
      2        0           0   0x1057    0x000d  0x0b200111
value = 0 = 0x0
-> pciHeaderShow 2,0
vendor ID =                   0x1057
device ID =                   0x000d
command register =            0x0000
status register =             0x00a0
revision ID =                 0x11
class code =                  0x0b
sub class code =              0x20
programming interface =       0x01
cache line =                  0x00
latency time =                0x00
header type =                 0x00
BIST =                        0x00
base address 0 =              0x00000000
base address 1 =              0x00000008
base address 2 =              0x00000004
base address 3 =              0x00000000
base address 4 =              0x00000004
base address 5 =              0x00000000
cardBus CIS pointer =         0x00000000
sub system vendor ID =        0x0000
sub system ID =               0x0000
expansion ROM base address =  0x00000000
interrupt line =              0x00
interrupt pin =               0x01
min Grant =                   0x00
max Latency =                 0x00
value = 0 = 0x0
-> Connection closed by foreign host.


Initialized PCI registers:
-------------------------

-> 
-> pciDeviceShow 2
Scanning functions of each PCI device on bus 2
Using configuration mechanism 1
bus       device    function  vendorID  deviceID  class/rev
      2        0           0   0x1057    0x000d  0x0b200111
value = 0 = 0x0
-> 
-> pciHeaderShow 2,0
vendor ID =                   0x1057
device ID =                   0x000d
command register =            0x0006
status register =             0x00a0
revision ID =                 0x11
class code =                  0x0b
sub class code =              0x20
programming interface =       0x01
cache line =                  0x00
latency time =                0x20
header type =                 0x00
BIST =                        0x00
base address 0 =              0xfdb00000
base address 1 =              0xf8000008
base address 2 =              0xfd400004
base address 3 =              0x00000000
base address 4 =              0xfd000004
base address 5 =              0x00000000
cardBus CIS pointer =         0x00000000
sub system vendor ID =        0x0000
sub system ID =               0x0000
expansion ROM base address =  0x00000000
interrupt line =              0x05
interrupt pin =               0x01
min Grant =                   0x00
max Latency =                 0x00
value = 0 = 0x0

