#************************** BBIS3 board descriptor **************************
#
#        Author: ds
#         $Date: 2000/03/17 15:08:23 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for D2
#
#****************************************************************************

D2 {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE           = U_INT32  2         # descriptor type (2=board)
    HW_TYPE             = STRING   D2        # hardware name of device

	#------------------------------------------------------------------------
	#	debug levels (optional)
	#   this keys have only effect on debug drivers
	#------------------------------------------------------------------------
    DEBUG_LEVEL         = U_INT32  0xc0008000  # BBIS driver
    DEBUG_LEVEL_BK      = U_INT32  0xc0008000  # BBIS kernel
    DEBUG_LEVEL_OSS     = U_INT32  0xc0008000  # OSS calls
    DEBUG_LEVEL_DESC    = U_INT32  0xc0008000  # DESC calls

	#------------------------------------------------------------------------
	#	ISAPNP device parameters
	#   
	#   The driver uses only the specified resources for a ISAPNP device
	#   if all resource keys for this device are specified. If not all
	#   resource keys for a device are specified the driver init fails.
	#
	#   If no resource keys are specified for a ISAPNP device, the driver
	#   gets the resources for this device from the ISAPNP controller.
	#   
	#   onboard device                ISAPNP device  resource keys
	#   -------------------------------------------------------------------
	#   0x1000 Z8536 (Watchdog)       ISAPNPDEV_0    IRQ_LEVEL_0, IO_ADDR_0
	#   0x1001 LM78 (HW-Monitor)      ISAPNPDEV_0    IRQ_LEVEL_0, IO_ADDR_1
	#------------------------------------------------------------------------
    ISAPNPDEV_0 {
		IO_ADDR_0		= U_INT32  0x100  # physical i/o address (Z8536)
		IO_ADDR_1		= U_INT32  0x108  # physical i/o address (LM78)
        IRQ_LEVEL_0     = U_INT32  5      # interrupt level (Z8536 + LM78)
    }
}
