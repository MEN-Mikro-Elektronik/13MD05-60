#
# Automatically generated by mdiswiz 2.04.00-vxworks-4.4 Build Sep 16 2016 17:04:04
# 2016-10-01
#

cpu {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x0
    HW_TYPE = STRING PC
    _WIZ_MODEL = STRING CC10C
}
FPGA {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x2
    HW_TYPE = STRING CHAMELEON_PCITBL
    _WIZ_MODEL = STRING CHAMELEON_PCITBL
    _WIZ_BUSIF = STRING cpu,1

    # ------------------------------------------------------------------------
    #  		PCI configuration
    # ------------------------------------------------------------------------
    PCI_BUS_PATH = BINARY 0x0,0x0,0x2
    PCI_DEVICE_NUMBER = U_INT32 0x0

    # ------------------------------------------------------------------------
    #        Chameleon BBIS Device: 
    #            DEVICE_IDV2_X is:   ((Cham devId) << 8 | instance)
    #        inside groups:  
    #            DEVICE_IDV2_X is:   ((Cham devId) << 8 | index inside group )
    # ------------------------------------------------------------------------
    DEVICE_IDV2_0 = U_INT32 0x2200
    DEVICE_IDV2_1 = U_INT32 0x2201

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        these keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_BK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
}
gpio_1 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x1
    HW_TYPE = STRING Z17_SW
    _WIZ_MODEL = STRING 16Z034_GPIO

    # ------------------------------------------------------------------------
    #        reference to base board
    # ------------------------------------------------------------------------
    BOARD_NAME = STRING FPGA
    DEVICE_SLOT = U_INT32 0x0

    # ------------------------------------------------------------------------
    #        device parameters
    # ------------------------------------------------------------------------

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        these keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_MK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
}
gpio_2 {

    # ------------------------------------------------------------------------
    #        general parameters (don't modify)
    # ------------------------------------------------------------------------
    DESC_TYPE = U_INT32 0x1
    HW_TYPE = STRING Z17_SW
    _WIZ_MODEL = STRING 16Z034_GPIO

    # ------------------------------------------------------------------------
    #        reference to base board
    # ------------------------------------------------------------------------
    BOARD_NAME = STRING FPGA
    DEVICE_SLOT = U_INT32 0x1

    # ------------------------------------------------------------------------
    #        device parameters
    # ------------------------------------------------------------------------

    # ------------------------------------------------------------------------
    #        debug levels (optional)
    #        these keys have only effect on debug drivers
    # ------------------------------------------------------------------------
    DEBUG_LEVEL = U_INT32 0xc0008000
    DEBUG_LEVEL_MK = U_INT32 0xc0008000
    DEBUG_LEVEL_OSS = U_INT32 0xc0008000
    DEBUG_LEVEL_DESC = U_INT32 0xc0008000
}
# EOF
