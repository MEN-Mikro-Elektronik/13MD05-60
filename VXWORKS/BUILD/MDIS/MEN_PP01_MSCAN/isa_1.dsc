#************************** BBIS3 board descriptor **************************
#
#        Author: uf
#         $Date: 2005/03/04 16:27:22 $
#     $Revision: 1.1 $
#
#   Description: BBIS descriptor for MPC5200 on chip MSCANs
#
#****************************************************************************

ISA_1 {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE           = U_INT32  2           # descriptor type (2=board)
    HW_TYPE             = STRING   ISA         # hardware name of device

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
    DEVICE_ADDR 		= U_INT32 0xf0000900  # MSCAN 0 base address
	DEVICE_ADDRSIZE 	= U_INT32 0x00000080  # device address space size
	IRQ_NUMBER 			= U_INT32 0x41		  # irq number
}

ISA_2 {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE           = U_INT32  2           # descriptor type (2=board)
    HW_TYPE             = STRING   ISA         # hardware name of device

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
    DEVICE_ADDR 		= U_INT32 0xf0000980  # MSCAN base address 
	DEVICE_ADDRSIZE 	= U_INT32 0x00000080  # device address space size
	IRQ_NUMBER 			= U_INT32 0x42		  # irq number
}

