#************************** BBIS3 board descriptor **************************
#
#        Author: uf
#         $Date: 2000/03/17 15:08:05 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for A8910
#
#****************************************************************************

A8910 {
    #------------------------------------------------------------------------
    #   general parameters (don't modify)
    #------------------------------------------------------------------------
    DESC_TYPE       = U_INT32       2             # descriptor type (2=board)
    HW_TYPE         = STRING        A8910         # hardware name of board

    #------------------------------------------------------------------------
    #   debug levels (optional)
    #------------------------------------------------------------------------
    DEBUG_LEVEL         = U_INT32   0xc0008000    # a8910 bbis handler
                                                  # default debug level
                                                  # (print only errors)

    #------------------------------------------------------------------------
    #   M-Module slot interrupts
    #------------------------------------------------------------------------
    IRQ_LEVEL       = BINARY   0x03, 0x03, 0x03, 0x03  # irq level (slot 0..3)
}
