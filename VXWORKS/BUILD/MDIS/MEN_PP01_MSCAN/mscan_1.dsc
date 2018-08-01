#************************** MDIS4 device descriptor *************************
#
#        Author: uf
#         $Date: 2005/03/04 16:27:23 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for 2 MPC5200 MSCANs
#
#****************************************************************************

CANODIN_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1           # descriptor type (1=device)
    HW_TYPE          = STRING   CANODIN     # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   ISA_1     # device name of baseboard
    DEVICE_SLOT      = U_INT32  0           # used slot on baseboard (0..n)

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
	CANCLOCK		 = U_INT32	64000000

	DEBUG_LEVEL = U_INT32 0xc0008000
}

CANODIN_2  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1           # descriptor type (1=device)
    HW_TYPE          = STRING   CANODIN     # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   ISA_2     # device name of baseboard
    DEVICE_SLOT      = U_INT32  0           # used slot on baseboard (0..n)

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
	CANCLOCK		 = U_INT32	64000000

	DEBUG_LEVEL = U_INT32 0xc0008000
}
