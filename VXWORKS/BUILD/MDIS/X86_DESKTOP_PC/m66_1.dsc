#****************************************************************************
#
#        Author: kp
#         $Date: 2002/05/03 14:22:49 $
#     $Revision: 1.2 $
#
#   Description: Metadescriptor for M66
#
#****************************************************************************

M66_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE       = U_INT32       1         # descriptor type (1=device)
    HW_TYPE         = STRING        M066      # hardware name of device

	#------------------------------------------------------------------------
	#	base board configuration
	#------------------------------------------------------------------------
    BOARD_NAME      = STRING        C203_1    # device name of baseboard
    DEVICE_SLOT     = U_INT32       0         # used slot on baseboard (0..n)
}

