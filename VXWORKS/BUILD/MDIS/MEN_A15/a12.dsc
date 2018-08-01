#************************** BBIS3 board descriptor **************************
#
#        Author: kp
#         $Date: 2004/05/10 12:30:15 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for A12
#
#****************************************************************************

A12 {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE           = U_INT32  2           # descriptor type (2=board)
    HW_TYPE             = STRING   A12         # hardware name of device
	DEBUG_LEVEL			= U_INT32  0xc0008003
	DEBUG_LEVEL_OSS     = U_INT32  0xc0008003  

}
