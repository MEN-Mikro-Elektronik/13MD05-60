#****************************************************************************
#
#        Author: kp
#         $Date: 2002/09/16 13:36:32 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for M66
#                swapped variant for BigEndian CPU on a little endian bus
#                here PowerPC on PCIbus
#****************************************************************************

M66_SW_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE       = U_INT32       1         # descriptor type (1=device)
    HW_TYPE         = STRING        M66_SW    # hardware name of device

	#------------------------------------------------------------------------
	#	base board configuration
	#------------------------------------------------------------------------
    BOARD_NAME      = STRING        F201_SW_1 # device name of baseboard
    DEVICE_SLOT     = U_INT32       0         # used slot on baseboard (0..n)
}

