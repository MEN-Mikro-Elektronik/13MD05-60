/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: oss_bustoaddr.c
 *      Project: OSS library
 *
 *       Author: Franke
 *        $Date: 2012/09/10 16:21:28 $
 *    $Revision: 1.11 $
 *
 *  Description: Address translation routines
 *-------------------------------[ History ]---------------------------------
 *
 * ts: removed changes in OSS_GetPciConfig() API from sy, was breaking the compile with other
  *    MDIS sources.
 * ------------- end of mcvs maintenance -----------
 * $Log: oss_bustoaddr.c,v $
 * Revision 1.11  2012/09/10 16:21:28  sy
 * Introduced OSS_VXBUS_SUPPORT marco to support multiple PCI domain
 *
 * Revision 1.10  2008/09/05 13:32:49  ufranke
 * R: diab compiler warning
 * M: cosmetics
 *
 * Revision 1.9  2008/08/18 16:41:00  cs
 * R: 1. DIAB compiler has more stringent/different error checking
 *       needless warnings were thrown
 * M: 1. changed internal variable types to avoid DIAB compiler warnings
 *
 * Revision 1.8  2006/06/08 14:11:59  ufranke
 * cosmetics
 *
 * Revision 1.7  2006/02/14 22:24:34  cs
 * removed RCSid
 *
 * Revision 1.6  2003/03/14 15:16:21  kp
 * include pciConfigLib rather pciIomapLib
 *
 * Revision 1.5  2000/03/17 14:38:57  kp
 * file description changed
 *
 * Revision 1.4  2000/03/16 16:05:26  kp
 * Enhanced functionality of OSS_PciGetConfig (allow to read arbitrary regs)
 * added OSS_PciSetConfig
 *
 * Revision 1.3  1999/12/08 17:16:36  Franke
 * added function OSS_PciAddrTranslationInit() e.g. for MPC106 Address Map A
 *
 * Revision 1.2  1999/08/30 11:03:27  Franke
 * cosmetics
 *
 * Revision 1.1  1999/05/05 11:11:28  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999..2006 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
const char *OSS_BusToIdentString="$Id: oss_bustoaddr.c,v 1.11 2012/09/10 16:21:28 sy Exp $";

#include <MEN/men_typs.h>
#include <MEN/mdis_err.h>
#include <MEN/oss.h>

#define DBG_MYLEVEL         OSS_DbgLev
#include <MEN/dbg.h>        /* debug module */
#include "oss_intern.h"

#if _WRS_VXWORKS_MAJOR != 7
#include <vme.h>
#endif
#ifdef PCI
#include <dllLib.h>       /* double link list lib */
#if _WRS_VXWORKS_MAJOR == 7
#include <hwif/buslib/pciDefines.h>  /* pci cfg register access */
#include <hwif/buslib/vxbPciLib.h>
#else
#include <drv/pci/pciConfigLib.h>  /* pci cfg register access */
#include <hwif/vxbus/vxbPciLib.h>
#endif /* vxworks7 */
#endif

#include <sysLib.h>

/*-----------------------------------------+
  |  TYPEDEFS                                |
  +------------------------------------------*/
/*-----------------------------------------+
  |  DEFINES & CONST                         |
  +------------------------------------------*/
#define PCI_SINGLE_FCT_DEV  0x00
#define PCI_BRIDGE          0x01

/*-----------------------------------------+
  |  GLOBALS                                 |
  +------------------------------------------*/
u_int32 OSS_PciIoBase  = 0;
u_int32 OSS_PciMemBase = 0;

/*-----------------------------------------+
  |  STATICS                                 |
  +------------------------------------------*/
#ifdef PCI
int G_pciAddrTranslatInit = 0;
#endif
/*-----------------------------------------+
  |  PROTOTYPES                              |
  +------------------------------------------*/
#ifdef PCI
static int32 PciGetReg(
		       OSS_HANDLE *oss,
		       u_int32 which,
		       int16 *idxP,
		       int16 *accessP );
#endif


/************************** OSS_BusToPhysAddr *******************************
 *
 *  Description:  Find out the local physical address of a bus device with
 *                the help of a OS- and CPU-Board specific routine.
 *
 *                if busType == OSS_BUSTYPE_NONE
 *                arg3      void  *localAddr
 *
 *                if busType == OSS_BUSTYPE_VME
 *                arg3   void    *vmeBusAddr
 *                arg4   u_int32 vmeSpace          VMEbus space (Axx/Dxx)
 *                          D00..D07 = VMEbus addr range (OSS_VME_A16/24/32)
 *                          D08..D15 = VMEbus data width (OSS_VME_D16/24/32)
 *                          D08..D15 always ignored -> see sysBusToLocal()
 *                arg5   u_int32 size
 *
 *                if busType == OSS_BUSTYPE_PCI
 *                arg3   int32  busNbr       0..255
 *                arg4   int32  pciDevNbr    0..31
 *                arg5   int32  pciFunction  0..7
 *                arg6   int32  addrNbr      0..5
 *
 *                if busType == OSS_BUSTYPE_ISA
 *                  not supported
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl   pointer to os specific struct for complicated os
 *                busType           OSS_BUSTYPE_NONE |  OSS_BUSTYPE_VME
 *                                  OSS_BUSTYPE_PCI |  OSS_BUSTYPE_ISA
 *                ...               arg3..argN
 *
 *  Output.....:  physicalAddrP     physical memory address
 *                return            0 | error code
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 OSS_BusToPhysAddr
(
 OSS_HANDLE *osHdl,
 int32       busType,
 void       **physicalAddrP,
 ...
 )
{
    DBGCMD( static const char functionName[] = "OSS_BusToPhysAddr"; )
	int32    retCode = 0;
    va_list  argptr;
    void     *inAddr;
    u_int32  addrMod;
    u_int32  size;
#ifdef OSS_VXBUS_SUPPORT
#if _WRS_VXWORKS_MAJOR == 7
    VXB_DEV_ID busCtrlID;
#else
    VXB_DEVICE_ID busCtrlID;
#endif
    u_int16  domain=0;
#endif
#ifdef PCI
	int32    busNbr      = 0;
	int32    pciDevNbr   = 0;
	int32    pciFunction = 0;
	int32    addrNbr     = 0;
	int32    cfgRegister;
	UINT32	pciBase;
#endif

    DBGWRT_1((DBH,"%s()\n", functionName));

    va_start( argptr, physicalAddrP );

    *physicalAddrP = NULL;

    switch( busType )
	{
	case OSS_BUSTYPE_NONE:
	    inAddr = va_arg( argptr, void* );
	    *physicalAddrP = inAddr;
	    break;
#if _WRS_VXWORKS_MAJOR != 7
	case OSS_BUSTYPE_VME:
	    inAddr  = va_arg( argptr, void* );
	    addrMod = va_arg( argptr, u_int32 );
	    size    = va_arg( argptr, u_int32 );
	    addrMod &= OSS_VME_AXX; /* select only address mode */
	    switch( addrMod )
		{
		case OSS_VME_A16:
		    addrMod = VME_AM_USR_SHORT_IO; /* A16 */
		    break;
		case OSS_VME_A24:
		    addrMod = VME_AM_STD_USR_DATA; /* A24 */
		    break;
		case OSS_VME_A32:
		    addrMod = VME_AM_EXT_USR_DATA; /* A32 */
		    break;

		default:
		    DBGWRT_ERR( ( DBH, "%s%s: addrMod %s%d%s\n",
				  OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
		    retCode = ERR_OSS_ILL_PARAM;
		}/*switch*/
	    if( !retCode )
		{
		    /* ts@men: sysBusToLocalAdrs() doesn't exist on x86 architecture BSPs, adresses are mapped 1:1 */
#if ( _VX_CPU_FAMILY==_VX_I80X86 )
		    *physicalAddrP = inAddr;
#else
		    retCode = sysBusToLocalAdrs( addrMod, (char*)inAddr, (char**)physicalAddrP );
		    if( retCode )
			{
			    DBGWRT_ERR( ( DBH, "%s%s: sysBusToLocalAdrs() %s%d%s\n",
					  OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			    retCode = ERR_OSS_ILL_PARAM;
			}/*if*/
#endif
		}/*if*/
	    break;
#endif
	case OSS_BUSTYPE_PCI:
#ifdef PCI

		busNbr      = va_arg( argptr, u_int32 );
		pciDevNbr   = va_arg( argptr, u_int32 );
		pciFunction = va_arg( argptr, u_int32 );
		addrNbr     = va_arg( argptr, u_int32 );

		if( !( 0 <= busNbr || busNbr <= 255 ) )
		    {
			retCode = ERR_OSS_ILL_PARAM;
			DBGWRT_ERR( ( DBH, "%s%s: busNbr %s%d%s\n",
				      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			goto BUSTOPHYS_END;
		    }/*if*/

		if( !( 0 <= pciDevNbr || pciDevNbr <= 31 ) )
		    {
			retCode = ERR_OSS_ILL_PARAM;
			DBGWRT_ERR( ( DBH, "%s%s: pciDevNbr %s%d%s\n",
				      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			goto BUSTOPHYS_END;
		    }/*if*/

		if( !( 0 <= pciFunction || pciFunction <= 7 ) )
		    {
			retCode = ERR_OSS_ILL_PARAM;
			DBGWRT_ERR( ( DBH, "%s%s: pciFunction %s%d%s\n",
				      OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			goto BUSTOPHYS_END;
		    }/*if*/
		switch( addrNbr )
		    {
		    case 0:
			cfgRegister = PCI_CFG_BASE_ADDRESS_0;
			break;

		    case 1:
			cfgRegister = PCI_CFG_BASE_ADDRESS_1;
			break;

		    case 2:
			cfgRegister = PCI_CFG_BASE_ADDRESS_2;
			break;

		    case 3:
			cfgRegister = PCI_CFG_BASE_ADDRESS_3;
			break;

		    case 4:
			cfgRegister = PCI_CFG_BASE_ADDRESS_4;
			break;

		    case 5:
			cfgRegister = PCI_CFG_BASE_ADDRESS_5;
			break;

		    default:
			retCode = ERR_OSS_PCI_UNK_REG;
			DBGWRT_ERR( ( DBH, "%s%s: addrNbr=%d %s%d%s\n",
				      OSS_ErrorStartStr, functionName, addrNbr, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
			goto BUSTOPHYS_END;
		    }/*switch*/

		/*---------------+
		  | get Address    |
		  +---------------*/
#ifdef OSS_VXBUS_SUPPORT
		domain = OSS_DOMAIN_NBR( busNbr );
		busCtrlID = sysGetPciCtrlID(domain);
		if ( busCtrlID == NULL ) {
		  DBGWRT_ERR(( DBH, "*** %s no busCtrlID for domain %d found! \n", functionName, domain ));
		  retCode = ERR_OSS_PCI;

		}
#if _WRS_VXWORKS_MAJOR == 7
		retCode = men_vxbPciConfigInLong(busCtrlID,
#else
		retCode = vxbPciConfigInLong(busCtrlID,
#endif
					     OSS_BUS_NBR(busNbr),
					     pciDevNbr,
					     pciFunction,
					     cfgRegister,
					     &pciBase );
#else
		retCode = pciConfigInLong(
					  busNbr,
					  pciDevNbr, pciFunction,
					  cfgRegister, &pciBase );
#endif

#if (CPU_FAMILY==I80X86)
#else
		/*------------------------------------------------------------------------+
		  | check for all non IBM PC's that the address translation was initialized |
		  +------------------------------------------------------------------------*/

#endif /*CPU_FAMILY*/
		if( !retCode )
		    {
			if( pciBase & 0x1 )
			    /*--- mapped to PCI I/O space ---*/
			    pciBase = (pciBase & ~0x3) + OSS_PciIoBase;
			else
			    pciBase = (pciBase & ~0xf) + OSS_PciMemBase;

		    }/*if*/

		*physicalAddrP = (void*)pciBase;

#else
	    DBGWRT_ERR( ( DBH, "%s%s: bustype %s%d%s\n",
			  OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	    retCode = ERR_OSS_UNK_BUSTYPE;
#endif /*PCI*/
	    break;

	case OSS_BUSTYPE_ISA:
	default:
	    DBGWRT_ERR( ( DBH, "%s%s: bustype %s%d%s\n",
			  OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	    retCode = ERR_OSS_UNK_BUSTYPE;
	}/*switch*/

#ifdef PCI
 BUSTOPHYS_END:
#endif /*PCI*/

    va_end( argptr );
    return( retCode );
}/*OSS_BusToPhysAddr*/

#ifdef PCI

/************************ OSS_PciAddrTranslationInit ************************
 *
 *  Description:  Initializes the offsets to the PCI IO and MEMORY space.
 *---------------------------------------------------------------------------
 *  Input......:  pciIoBase	 e.g. 0x80000000 for MPC106 Addr Map A | 0x00 for IBM PC
 *				  pciMemBase e.g. 0xC0000000 for MPC106 Addr Map A | 0x00 for IBM PC
 *  Output.....:  -
 *  Globals....:  G_pciAddrTranslatInit
 ****************************************************************************/
void OSS_PciAddrTranslationInit( u_int32 pciIoBase, u_int32 pciMemBase )
{
    OSS_PciIoBase  = pciIoBase;
    OSS_PciMemBase = pciMemBase;
    G_pciAddrTranslatInit = 1;
}/*OSS_PciAddrTranslationInit*/


/**************************** OSS_PciGetConfig *******************************
 *
 *  Description:  Gets PCI configuration registers.
 *
 *                NOTE: Specification 2.1 compliant.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl        pointer to os specific struct for complicated os
 *                busNbr       pci bus nbr  0..255
 *                pciDevNbr    device nbr   0..31
 *                pciFunction  function nbr 0..7
 *                which        0x000000xx  xx=OSS_PCI_VENDOR..OSS_PCI_INT_LINE
 *							   0x010000xx  xx=reg idx, byte access
 *							   0x020000xx  xx=reg idx, word access
 *							   0x040000xx  xx=reg idx, long access
 *
 *  Output.....:  valueP       output value
 *                return       0 | error code
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 OSS_PciGetConfig
(
 OSS_HANDLE *osHdl,
 int32       busNbr,
 int32       pciDevNbr,
 int32       pciFunction,
 int32       which,
 int32       *valueP
 )
{
    DBGCMD( static const char functionName[] = "OSS_PciGetConfig"; )
    int32   retCode = 0;
    UINT32	value32;
    UINT16	value16;
    UINT8	value8;
    int16 idx, access;
#ifdef OSS_VXBUS_SUPPORT
#if _WRS_VXWORKS_MAJOR == 7
    VXB_DEV_ID busCtrlID;
#else
    VXB_DEVICE_ID busCtrlID;
#endif
    int16 domain=0;
#endif

    DBGWRT_1((DBH,"%s bus %x dev %x func %x which %x\n",
	      functionName, OSS_BUS_NBR(busNbr), pciDevNbr, pciFunction, which));

    *valueP = 0;

    /*--- determine config reg offset and access ---*/
    if( (retCode = PciGetReg( osHdl, which, &idx, &access )))
	goto GETCFG_END;

      /* determine which PCI domain ( =PCI controller vxBus driver instance ID ) to use */
#ifdef OSS_VXBUS_SUPPORT
      domain = OSS_DOMAIN_NBR( busNbr );

      busCtrlID = sysGetPciCtrlID(domain);
      if ( busCtrlID == NULL ) {
	  DBGWRT_ERR(( DBH, "*** %s no busCtrlID for domain %d found! \n", functionName, domain ));
	  retCode = ERR_OSS_PCI;
	  goto GETCFG_END;
      }
      DBGWRT_1((DBH,"%s domain 0x%x bus 0x%x dev 0x%x func 0x%x which 0x%x\n",
    		  	  functionName, OSS_DOMAIN_NBR(busNbr), OSS_BUS_NBR(busNbr), pciDevNbr, pciFunction, which));
#else
      DBGWRT_1((DBH,"%s bus %x dev %x func %x which %x\n", functionName, busNbr, pciDevNbr, pciFunction, which));
#endif

    switch( access )
	{
	case 4:
#ifdef OSS_VXBUS_SUPPORT
#if _WRS_VXWORKS_MAJOR == 7
	    retCode = men_vxbPciConfigInLong( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, &value32);
#else
	    retCode = vxbPciConfigInLong( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, &value32);
#endif
#else
	    retCode = pciConfigInLong( busNbr, pciDevNbr, pciFunction, idx, &value32);
#endif
	    *valueP = (u_int32)value32;
	    break;

	case 2:
#ifdef OSS_VXBUS_SUPPORT
#if _WRS_VXWORKS_MAJOR == 7
	    retCode = men_vxbPciConfigInWord( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, &value16 );
#else
	    retCode = vxbPciConfigInWord( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, &value16 );
#endif
#else
	    retCode = pciConfigInWord( busNbr, pciDevNbr, pciFunction, idx, &value16 );
#endif
	    *valueP = (u_int32)value16 & 0xffff;
	    break;

	case 1:
#ifdef OSS_VXBUS_SUPPORT
#if _WRS_VXWORKS_MAJOR == 7
	    retCode = men_vxbPciConfigInByte( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, &value8);
#else
	    retCode = vxbPciConfigInByte( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, &value8);
#endif
#else
	    retCode = pciConfigInByte( busNbr, pciDevNbr, pciFunction, idx, &value8 );
#endif
	    *valueP = (u_int32)value8 & 0xff;
	    break;
	}/*switch*/


    if( retCode )
	{
	    retCode = ERR_OSS_PCI;
	    DBGWRT_ERR( ( DBH, "%s%s: pciConfigInB/W/L() reg 0x%04x %s%d%s\n",
			  OSS_ErrorStartStr, functionName, idx,
			  OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	    *valueP = 0;
	}/*if*/

 GETCFG_END:
    DBGWRT_2((DBH, "  value=0x%08x\n", *valueP));

    return( retCode );
}/*OSS_PciGetConfig*/

/**************************** OSS_PciSetConfig *******************************
 *
 *  Description:  Sets PCI configuration registers.
 *
 *                NOTE: Specification 2.1 compliant.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl        pointer to os specific struct for complicated os
 *                busNbr       pci bus nbr  0..255
 *                pciDevNbr    device nbr   0..31
 *                pciFunction  function nbr 0..7
 *                which        0x000000xx  xx=OSS_PCI_VENDOR..OSS_PCI_INT_LINE
 *							   0x010000xx  xx=reg idx, byte access
 *							   0x020000xx  xx=reg idx, word access
 *							   0x040000xx  xx=reg idx, long access
 *
 *				  value		   value to write
 *  Output.....:  return       0 | error code
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 OSS_PciSetConfig
(
OSS_HANDLE *osHdl,
    int32       busNbr,
    int32       pciDevNbr,
    int32       pciFunction,
    int32       which,
    int32       value
    )
{
      DBGCMD( static const char functionName[] = "OSS_PciSetConfig"; )
      int32   retCode = 0;
      int16 idx, access;
#ifdef OSS_VXBUS_SUPPORT
#if _WRS_VXWORKS_MAJOR == 7
      VXB_DEV_ID busCtrlID;
#else
      VXB_DEVICE_ID busCtrlID;
#endif
      int16 domain=0;
#endif

      /*--- determine config reg offset and access ---*/
      if( (retCode = PciGetReg( osHdl, which, &idx, &access )))
	  goto SETCFG_END;

      /* determine which PCI domain ( = PCI controller vxBus driver instance ID ) to use */
#ifdef OSS_VXBUS_SUPPORT
      domain = OSS_DOMAIN_NBR( busNbr );
      busCtrlID = sysGetPciCtrlID(domain);

      if ( busCtrlID == NULL ) {
	  DBGWRT_ERR(( DBH, "*** %s no busCtrlID for domain %d found! \n", functionName, domain ));
	  retCode = ERR_OSS_PCI;
	  goto SETCFG_END;
      }
      DBGWRT_1((DBH,"%s domain 0x%x bus 0x%x dev 0x%x func 0x%x which 0x%x\n", functionName, domain,busNbr, pciDevNbr, pciFunction, which));
#else
      DBGWRT_1((DBH,"%s bus %x dev %x func %x which %x\n", functionName, busNbr, pciDevNbr, pciFunction, which));
#endif

switch( access )
    {
    case 4:
#ifdef OSS_VXBUS_SUPPORT
#if _WRS_VXWORKS_MAJOR == 7
	retCode = men_vxbPciConfigOutLong( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, (int)value);
#else
	retCode = vxbPciConfigOutLong( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, (int)value);
#endif
#else
        retCode = pciConfigOutLong( busNbr, pciDevNbr, pciFunction, idx, (int)value);
#endif
	break;

    case 2:
#ifdef OSS_VXBUS_SUPPORT
#if _WRS_VXWORKS_MAJOR == 7
	retCode = men_vxbPciConfigOutWord( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, (short)value );
#else
	retCode = vxbPciConfigOutWord( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, (short)value );
#endif
#else
	retCode = pciConfigOutWord( busNbr, pciDevNbr, pciFunction, idx, (short)value );
#endif
	break;

    case 1:
#ifdef OSS_VXBUS_SUPPORT
#if _WRS_VXWORKS_MAJOR == 7
	retCode = men_vxbPciConfigOutByte( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, (char)value);
#else
	retCode = vxbPciConfigOutByte( busCtrlID, busNbr, pciDevNbr, pciFunction, idx, (char)value);
#endif
#else
	retCode = pciConfigOutByte( busNbr, pciDevNbr, pciFunction, idx, (char)value);
#endif
	break;
    }/*switch*/

if( retCode )
    {
	retCode = ERR_OSS_PCI;
	DBGWRT_ERR( ( DBH, "%s%s: pciConfigOutB/W/L() reg 0x%04x %s%d%s\n",
		      OSS_ErrorStartStr, functionName, idx,
		      OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
    }/*if*/

SETCFG_END:

return( retCode );
}/*OSS_PciSetConfig*/




/************************ OSS_PciSlotToPciDevice ****************************
 *
 *  Description:  Converts the mechanical slot number to pci device nbr.
 *
 *                NOTE:
 *                ( PCI device nbr. => IDSEL ) with the help of a table.
 *                The table values are depend on the PCI to PCI brigde.
 *
 *---------------------------------------------------------------------------
 *  Input......:  osHdl        os specific data for complicated os
 *                busNbr       pci bus nbr 0..255
 *                mechSlot     mech slot 1..8
 *
 *  Output.....:  pciDevNbrP   1..31
 *                return       0 | code
 *
 *  Globals....:  ---
 *
 ****************************************************************************/
int32 OSS_PciSlotToPciDevice
(
 OSS_HANDLE *osHdl,
 int32      busNbr,
 int32      mechSlot,
 int32      *pciDevNbrP
 )
{
    DBGCMD( static const char functionName[] = "OSS_PciSlotToPciDevice"; )
	int32 retCode;
    int32 devNbr;
    int32 i;

    DBGWRT_1((DBH,"%s()\n", functionName));

    retCode = 0;
    *pciDevNbrP = 0xff;


    if( !(1 <= mechSlot && mechSlot <= 8) )
	{
	    retCode = ERR_OSS_ILL_PARAM;
	    DBGWRT_ERR( ( DBH, "%s%s: mechSlot %s%d%s\n",
			  OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	    goto END;
	}/*if*/

    if( !( 0 <= busNbr || busNbr <= 255 ) )
	{
	    retCode = ERR_OSS_ILL_PARAM;
	    DBGWRT_ERR( ( DBH, "%s%s: busNbr %s%d%s\n",
			  OSS_ErrorStartStr, functionName, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	    goto END;
	}/*if*/


    devNbr = OSS_PciSlot1DevNbr;

    /* count up or down */
    for( i = 1; i < mechSlot; i++ )
	{
	    if( OSS_PciSlot1DevNbr > OSS_PciSlot2DevNbr )
		{
		    /* count down */
		    devNbr--;
		}
	    else
		{
		    /* count up */
		    devNbr++;
		}/*if*/
	}/*for*/

    /* validation */
    if( 0 <= devNbr && devNbr <= 31 )
	{
	    *pciDevNbrP = devNbr;
	    retCode = 0;
	}
    else
	{
	    retCode = ERR_OSS_PCI_ILL_DEVNBR;
	    DBGWRT_ERR( ( DBH, "%s%s: computed devNbr=%d %s%d%s\n",
			  OSS_ErrorStartStr, functionName, devNbr, OSS_ErrorLineStr, __LINE__, OSS_ErrorEndStr ));
	}/*if*/

 END:

    return( retCode );
}/*OSS_PciSlotToPciDevice*/

/********************************* PciGetReg ********************************
 *
 *  Description: Convert <which> parameter of OSS_PciGet/SetConfig
 *
 *			   	 Convert to register index and access size
 *---------------------------------------------------------------------------
 *  Input......: oss	OSS handle
 *			 which			parameter as passed to OSS_PciGet/SetConfig
 *  Output.....: return success (0) or error code
 *			 *idxP			register index
 *			 *accessP		access size (1,2,4)
 *  Globals....: -
 ****************************************************************************/
static int32 PciGetReg(
		       OSS_HANDLE *oss,
		       u_int32 which,
		       int16 *idxP,
		       int16 *accessP )
{
    const struct {
		int16 idx;	/* PCI configuration space byte index */
		int16 access;	/* access mode byte/word/long */
    } regTbl[] = {
		{ 0, 0 },	/* - */
		{ 0x00, 2 },	/* OSS_PCI_VENDOR_ID */
		{ 0x02, 2 },	/* OSS_PCI_DEVICE_ID */
		{ 0x04, 2 },	/* OSS_PCI_COMMAND */
		{ 0x06, 2 },	/* OSS_PCI_STATUS */
		{ 0x08, 1 },	/* OSS_PCI_REVISION_ID */
		{ 0x09, 1 },	/* OSS_PCI_CLASS */
		{ 0x0a, 1 },	/* OSS_PCI_SUB_CLASS */
		{ 0x0b, 1 },	/* OSS_PCI_PROG_IF */
		{ 0x0c, 1 },	/* OSS_PCI_CACHE_LINE_SIZE */
		{ 0x0d, 1 },	/* OSS_PCI_PCI_LATENCY_TIMER */
		{ 0x0e, 1 },	/* OSS_PCI_HEADER_TYPE */
		{ 0x0f, 1 },	/* OSS_PCI_BIST */
		{ 0x10, 4 },	/* OSS_PCI_ADDR_0 */
		{ 0x14, 4 },	/* OSS_PCI_ADDR_1 */
		{ 0x18, 4 },	/* OSS_PCI_ADDR_2 */
		{ 0x1c, 4 },	/* OSS_PCI_ADDR_3 */
		{ 0x20, 4 },	/* OSS_PCI_ADDR_4 */
		{ 0x24, 4 },	/* OSS_PCI_ADDR_5 */
		{ 0x28, 4 },	/* OSS_PCI_CIS */
		{ 0x2c, 2 },	/* OSS_PCI_SUBSYS_VENDOR_ID */
		{ 0x2e, 2 },	/* OSS_PCI_SUBSYS_ID */
		{ 0x30, 4 },	/* OSS_PCI_EXPROM_ADDR */
		{ 0x3d, 1 },	/* OSS_PCI_INTERRUPT_PIN */
		{ 0x3c, 1 },	/* OSS_PCI_INTERRUPT_LINE */
		/* -- new defines for PCI type 1 (bridges) config space -- */
		{ 0x18, 1 },	/* OSS_PCI_PRIMARY_BUS  */
		{ 0x19, 1 },	/* OSS_PCI_SECONDARY_BUS */
		{ 0x1a, 1 },	/* OSS_PCI_SUBORDINATE_BUS */
		{ 0x1b, 1 },	/* OSS_PCI_SEC_LATENCY */
		{ 0x1c, 1 },	/* OSS_PCI_IO_BASE */
		{ 0x1d, 1 },	/* OSS_PCI_IO_LIMIT */
		{ 0x1e, 2 },	/* OSS_PCI_SEC_STATUS */
		{ 0x20, 2 },	/* OSS_PCI_MEM_BASE */
		{ 0x22, 2 },	/* OSS_PCI_MEM_LIMIT */
		{ 0x24, 2 },	/* OSS_PCI_PRE_MEM_BASE */
		{ 0x26, 2 },	/* OSS_PCI_PRE_MEM_LIMIT */
		{ 0x28, 4 },	/* OSS_PCI_PRE_MEM_BASE_U */
		{ 0x2c, 4 },	/* OSS_PCI_PRE_MEM_LIMIT_U */
		{ 0x30, 2 },	/* OSS_PCI_IO_BASE_U */
		{ 0x32, 2 },	/* OSS_PCI_IO_LIMIT_U */
		{ 0x38, 4 },	/* OSS_PCI_ROM_BASE */
		{ 0x3c, 1 },	/* OSS_PCI_BRG_INT_LINE */
		{ 0x3d, 1 },	/* OSS_PCI_BRG_INT_PIN */
		{ 0x3e, 2 },	/* OSS_PCI_BRIDGE_CONTROL */
    };

    switch( which & OSS_PCI_ACCESS ){
    case 0x00000000:

	/*--- standard PCI regs ---*/
	if( which < OSS_PCI_VENDOR_ID || which > OSS_PCI_INTERRUPT_LINE ){
	    DBGWRT_ERR((DBH," *** OSS:PciGetReg: bad which parameter 0x%x\n",
			which));
	    return ERR_OSS_PCI_UNK_REG;
	}
	*idxP 		= regTbl[which].idx;
	*accessP 	= regTbl[which].access;
	break;

    case OSS_PCI_ACCESS_8:
	/*--- byte access to any PCI config reg ---*/
	*idxP 		= (int16)which & 0xff;
	*accessP	= 1;
	break;

    case OSS_PCI_ACCESS_16:
	/*--- word access to any PCI config reg ---*/
	*idxP 		= (int16)which & 0xff;
	*accessP	= 2;
	break;

    case OSS_PCI_ACCESS_32:
	/*--- long access to any PCI config reg ---*/
	*idxP 		= (int16)which & 0xff;
	*accessP	= 4;
	break;

    default:
	DBGWRT_ERR((DBH," *** OSS:PciGetReg: bad which parameter 0x%x\n",
		    which));
	return ERR_OSS_PCI_UNK_REG;
    }
    return 0;
}

#endif /*PCI*/




