#************************** BBIS3 board descriptor **************************
#
#        Author: kp
#         $Date: 2000/03/17 15:08:16 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for A11 onboard PC-MIP slots
#
#****************************************************************************

A11_OBPCMIP {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE           = U_INT32  2           # descriptor type (2=board)
    HW_TYPE             = STRING   PCI         # hardware name of device

    DEBUG_LEVEL         = U_INT32   0xc0008007    # LL driver
    DEBUG_LEVEL_BK      = U_INT32   0xc0008000    # BBIS kernel
    DEBUG_LEVEL_OSS     = U_INT32   0xc0008000    # OSS calls
    DEBUG_LEVEL_DESC    = U_INT32   0xc0008000    # DESC calls
	
	#------------------------------------------------------------------------
	#	PCI specific parameters (don't modify it !)
	#------------------------------------------------------------------------
	PCI_BUS_PATH		= BINARY   0x10

	DEVICE_SLOT_0		= U_INT32  0x00
	DEVICE_SLOT_1		= U_INT32  0x01

}
