#************************** BBIS3 board descriptor **************************
#
#        Author: see
#         $Date: 2001/12/06 17:25:58 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for A201
#
#****************************************************************************

A201_1 {
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

    #------------------------------------------------------------------------
    #   base address
    #------------------------------------------------------------------------
    # board address and access mode (VMEbus relative)
    # The resulting physical address is calculated internally for known
    # CPU-boards depending on VME_XXX_ADDR and VME_DATA_WIDTH,
    # XXX defines the used VMEbus address space (A16, A24 or A32)

    VME_A24_ADDR    = U_INT32  0x00e00000          # VMEbus relative address
    VME_DATA_WIDTH  = U_INT32  1                   # VMEbus data access mode
                                                   # (1=D16, 2=D24, 3=D32)
    # board address (physical)
    # The physical address can optionally defined with PHYS_ADDR for unknown
    # CPU-boards. In this case all VME_xxx definitions are ignored.

    #PHYS_ADDR      = U_INT32  0x88e00000          # physical address

    #------------------------------------------------------------------------
    #   slot interrupts
    #------------------------------------------------------------------------
    IRQ_VECTOR      = BINARY   0x81, 0x82, 0x83, 0x84  # irq vectors  (slot 0..3) 
    IRQ_LEVEL       = BINARY   0x03, 0x03, 0x03, 0x03  # irq level    (slot 0..3)
}
