#************************** BBIS3 board descriptor **************************
#
#        Author: ds
#         $Date: 2001/12/07 10:17:26 $
#     $Revision: 1.2 $
#
#   Description: Metadescriptor for D202 carrier board in a D2 system
#
#****************************************************************************

D202_1 {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE           = U_INT32  2           # descriptor type (2=board)
    HW_TYPE             = STRING   PCI         # hardware name of device

	#------------------------------------------------------------------------
	# Card Slot - modify it if not in Slot#3
	# define device IDs of bridges to CompactPCI backplane
	# see D201/DOC/pcibuspath.txt for list
	# or use the VxWorks tool pciScanner
	#------------------------------------------------------------------------
	#PCI_BUS_PATH		= BINARY   0x08,0x0c	# for slot 5
	#PCI_BUS_PATH		= BINARY   0x08,0x0d	# for slot 4
	PCI_BUS_PATH		= BINARY   0x08,0x0e	# for slot 3
	#PCI_BUS_PATH		= BINARY   0x08,0x0f	# for slot 2

	#------------------------------------------------------------------------
	#	debug levels (optional)
	#   this keys have only effect on debug drivers
	#------------------------------------------------------------------------
    DEBUG_LEVEL         = U_INT32  0xc0008000  # BBIS driver
    DEBUG_LEVEL_BK      = U_INT32  0xc0008000  # BBIS kernel
    DEBUG_LEVEL_OSS     = U_INT32  0xc0008000  # OSS calls
    DEBUG_LEVEL_DESC    = U_INT32  0xc0008000  # DESC calls

	#------------------------------------------------------------------------
	#	Board specific parameters (don't modify it !)
	#------------------------------------------------------------------------
	DEVICE_SLOT_0		= U_INT32  0x0C
	DEVICE_SLOT_1		= U_INT32  0x0D
	DEVICE_SLOT_2		= U_INT32  0x0E
	DEVICE_SLOT_3		= U_INT32  0x0F
	DEVICE_SLOT_4		= U_INT32  0x0A
	DEVICE_SLOT_5		= U_INT32  0x0B
}
