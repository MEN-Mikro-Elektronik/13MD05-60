#************************** BBIS3 board descriptor **************************
#
#        Author: kp
#         $Date: 2001/09/03 09:26:29 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for B11 onboard PC-MIP slots
#
#****************************************************************************

B11_OBPCMIP {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE           = U_INT32  2           # descriptor type (2=board)
    HW_TYPE             = STRING   PCI         # hardware name of device

	#------------------------------------------------------------------------
	#	debug levels (optional)
	#   this keys have only effect on debug drivers
	#------------------------------------------------------------------------
    DEBUG_LEVEL         = U_INT32  0xc0008000  # BBIS driver
    DEBUG_LEVEL_BK      = U_INT32  0xc0008000  # BBIS kernel
    DEBUG_LEVEL_OSS     = U_INT32  0xc0008000  # OSS calls
    DEBUG_LEVEL_DESC    = U_INT32  0xc0008000  # DESC calls

	#------------------------------------------------------------------------
	#	PCI specific parameters (don't modify it !)
	#------------------------------------------------------------------------
	PCI_BUS_PATH		= BINARY   			   # leave blank!

	DEVICE_SLOT_0		= U_INT32  0x1a
	DEVICE_SLOT_1		= U_INT32  0x1d
}
