#************************** BBIS3 board descriptor **************************
#
#        Author: kp
#         $Date: 2004/05/10 12:30:16 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for A12 onboard PC-MIP/PMC slots
#
#****************************************************************************

A12_OBPCMIP {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE           = U_INT32  2           # descriptor type (2=board)
    HW_TYPE             = STRING   PCI         # hardware name of device

	#------------------------------------------------------------------------
	#	PCI specific parameters (don't modify it !)
	#------------------------------------------------------------------------
	PCI_BUS_PATH		= BINARY   0x1d		   # device number of PCI-PCI
											   # bridge on bus 0 to PC-MIPs

	DEVICE_SLOT_0		= U_INT32  0x00		   # PC-MIP 0		
	DEVICE_SLOT_1		= U_INT32  0x01		   # PC-MIP 1	
	DEVICE_SLOT_2		= U_INT32  0x02		   # PC-MIP 2	
	DEVICE_SLOT_3		= U_INT32  0x03		   # PMC 0
	DEVICE_SLOT_4		= U_INT32  0x04		   # PMC 1

}
