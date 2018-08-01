c204_1 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x2
    HW_TYPE = STRING C204_SW
    _WIZ_MODEL = STRING C204
    _WIZ_BUSIF = STRING ec01_1,0
    PCI_BUS_PATH = BINARY 
    PCI_BUS_SLOT = U_INT32 0x2
    PCI_DEVICE_ID = U_INT32 0x14

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        these keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_BK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
}
c204_2 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x2
    HW_TYPE = STRING C204_SW
    _WIZ_MODEL = STRING C204
    _WIZ_BUSIF = STRING ec01_1,0
    PCI_BUS_PATH = BINARY 
    PCI_BUS_SLOT = U_INT32 0x2
    PCI_DEVICE_ID = U_INT32 0x15

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        these keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_BK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
}
c204_3 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x2
    HW_TYPE = STRING C204_SW
    _WIZ_MODEL = STRING C204
    _WIZ_BUSIF = STRING ec01_1,0
    PCI_BUS_PATH = BINARY 
    PCI_BUS_SLOT = U_INT32 0x2
    PCI_DEVICE_ID = U_INT32 0x16

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        these keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_BK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
}
c204_4 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x2
    HW_TYPE = STRING C204_SW
    _WIZ_MODEL = STRING C204
    _WIZ_BUSIF = STRING ec01_1,0
    PCI_BUS_PATH = BINARY 
    PCI_BUS_SLOT = U_INT32 0x2
    PCI_DEVICE_ID = U_INT32 0x17

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        these keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_BK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
}
m99_1 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x1
    HW_TYPE = STRING M99_SW
    _WIZ_MODEL = STRING M99

    # ------------------------------------------------------------------------
    #        reference to base board
    # ------------------------------------------------------------------------
    BOARD_NAME = STRING c204_1
    DEVICE_SLOT = U_INT32 0x0

    # ------------------------------------------------------------------------
    #        device parameters
    # ------------------------------------------------------------------------

    # Define wether M-Module ID-PROM is checked, don't enable for M99, works only once after power up
    # 0 := disable -- ignore IDPROM
    # 1 := enable
    ID_CHECK = U_INT32 0

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        these keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_MK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
    DEBUG_LEVEL_MBUF = U_INT32 0xc0008000
}
m99_2 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x1
    HW_TYPE = STRING M99_SW
    _WIZ_MODEL = STRING M99

    # ------------------------------------------------------------------------
    #        reference to base board
    # ------------------------------------------------------------------------
    BOARD_NAME = STRING c204_2
    DEVICE_SLOT = U_INT32 0x0

    # ------------------------------------------------------------------------
    #        device parameters
    # ------------------------------------------------------------------------

    # Define wether M-Module ID-PROM is checked, don't enable for M99, works only once after power up
    # 0 := disable -- ignore IDPROM
    # 1 := enable
    ID_CHECK = U_INT32 0

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        these keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_MK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
    DEBUG_LEVEL_MBUF = U_INT32 0xc0008000
}
m99_3 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x1
    HW_TYPE = STRING M99_SW
    _WIZ_MODEL = STRING M99

    # ------------------------------------------------------------------------
    #        reference to base board
    # ------------------------------------------------------------------------
    BOARD_NAME = STRING c204_3
    DEVICE_SLOT = U_INT32 0x0

    # ------------------------------------------------------------------------
    #        device parameters
    # ------------------------------------------------------------------------

    # Define wether M-Module ID-PROM is checked, don't enable for M99, works only once after power up
    # 0 := disable -- ignore IDPROM
    # 1 := enable
    ID_CHECK = U_INT32 0

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        these keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_MK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
    DEBUG_LEVEL_MBUF = U_INT32 0xc0008000
}
m99_4 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x1
    HW_TYPE = STRING M99_SW
    _WIZ_MODEL = STRING M99

    # ------------------------------------------------------------------------
    #        reference to base board
    # ------------------------------------------------------------------------
    BOARD_NAME = STRING c204_4
    DEVICE_SLOT = U_INT32 0x0

    # ------------------------------------------------------------------------
    #        device parameters
    # ------------------------------------------------------------------------

    # Define wether M-Module ID-PROM is checked, don't enable for M99, works only once after power up
    # 0 := disable -- ignore IDPROM
    # 1 := enable
    ID_CHECK = U_INT32 0

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        these keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_MK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
    DEBUG_LEVEL_MBUF = U_INT32 0xc0008000
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

isa_1 {
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

isa_2 {
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

canodin_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1           # descriptor type (1=device)
    HW_TYPE          = STRING   CANODIN     # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   isa_1     # device name of baseboard
    DEVICE_SLOT      = U_INT32  0           # used slot on baseboard (0..n)

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
	CANCLOCK		 = U_INT32	64000000

	DEBUG_LEVEL = U_INT32 0xc0008007
}

canodin_2  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1           # descriptor type (1=device)
    HW_TYPE          = STRING   CANODIN     # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   isa_2     # device name of baseboard
    DEVICE_SLOT      = U_INT32  0           # used slot on baseboard (0..n)

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
	CANCLOCK		 = U_INT32	64000000

	DEBUG_LEVEL = U_INT32 0xc0008000
}
