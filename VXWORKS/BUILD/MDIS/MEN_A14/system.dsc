M66_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE       = U_INT32       1         # descriptor type (1=device)
    HW_TYPE         = STRING        M66       # hardware name of device

	#------------------------------------------------------------------------
	#	base board configuration
	#------------------------------------------------------------------------
    BOARD_NAME      = STRING        A201_1    # device name of baseboard
    DEVICE_SLOT     = U_INT32       0         # used slot on baseboard (0..n)
}
fpga {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x2
    HW_TYPE = STRING CHAMELEON
    _WIZ_MODEL = STRING CHAMELEON
    _WIZ_BUSIF = STRING cpu,1
    PCI_BUS_NUMBER = U_INT32 0x0
    PCI_DEVICE_NUMBER = U_INT32 0x1d
}

a201_1 {
    #------------------------------------------------------------------------
    #   general parameters (don't modify)
    #------------------------------------------------------------------------
    DESC_TYPE       = U_INT32       2             # descriptor type (2=board)
    HW_TYPE         = STRING        A201          # hardware name of board

    #------------------------------------------------------------------------
    #   debug levels (optional)
    #------------------------------------------------------------------------
    DEBUG_LEVEL         = U_INT32   0xc0008000    # BBIS driver
    DEBUG_LEVEL_BK      = U_INT32   0xc0008000    # BBIS kernel
    DEBUG_LEVEL_OSS     = U_INT32   0xc0008000    # OSS calls
    DEBUG_LEVEL_DESC    = U_INT32   0xc0008000    # DESC calls

    VME_A24_ADDR    = U_INT32  0x00e00000          # VMEbus relative address
    VME_DATA_WIDTH  = U_INT32  1                   # VMEbus data access mode
                                                   # (1=D16, 2=D24, 3=D32)
    #------------------------------------------------------------------------
    #   slot interrupts
    #------------------------------------------------------------------------
    IRQ_VECTOR      = BINARY   0x81, 0x82, 0x83, 0x84  # irq vectors  (slot 0..3) 
    IRQ_LEVEL       = BINARY   0x03, 0x03, 0x03, 0x03  # irq level    (slot 0..3)
}
