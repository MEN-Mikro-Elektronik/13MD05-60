#************************** BBIS3 board descriptor **************************
#
#        Author: ds
#         $Date: 2001/12/07 10:20:11 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for D302
#
#****************************************************************************

D302_1 {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE           = U_INT32  2		# descriptor type (2=board)
    HW_TYPE             = STRING   D302		# hardware name of device

	#------------------------------------------------------------------------
	# Card Slot - modify it if not in Slot#3
	#------------------------------------------------------------------------
    PCI_BUS_SLOT        = U_INT32  3		# system slot (CPU card) = 1

	# define device IDs of bridges to CompactPCI backplane
	# see D201/DOC/pcibuspath.txt for list
	# or use the VxWorks tool pciScanner
	PCI_BUS_PATH		= BINARY   0x08		# device IDs of bridges to 
											# compact PCI bus (D002)
    #PCI_BUS_NUMBER     = U_INT32  1		# optional overwrites PCI_BUS_PATH
    #PCI_DEVICE_ID      = U_INT32 10		# optional overwrites PCI_BUS_SLOT
    PCI_CHECK_LOCATION  = U_INT32  0		# don't check geographic location

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
    ID_CHECK            = U_INT32  1		# check board ID prom
}
