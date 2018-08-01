
#ifdef INCLUDE_PCI				/* board level PCI routines */


void pciRegWrite(
	UINT32 * adrs,
	UINT32 value
)
{
	*adrs = value;
	WRS_ASM("sync;eieio");
}

UINT32 pciRegRead(
	UINT32 * adrs
)
{
	return (*adrs);
}

/*******************************************************************************
* setBusSpecConfRegs - select PCI configuration access registers
*
* This routine selects the PCI configuration access registers of
* host controller for the selected bus
*
* NOMANUAL
* RETURNS: n/a
*
*/
static void setBusSpecConfRegs(
	int busNo,
	UINT32 * pciConfAddr,
	UINT32 * pciConfData
)
{
#ifdef MEN_BOARD_HAS_PCIE
	if( busNo >= sysPciEMinBus &&
		busNo <= sysPciEMaxBus ){
		*pciConfAddr = (UINT32) sysPciEConfAddr;
		*pciConfData = (UINT32) sysPciEConfData;
	} else
#endif /* MEN_BOARD_HAS_PCIE */

#ifdef MEN_BOARD_HAS_PCI2
	if( busNo >= sysPci2MinBus &&
		busNo <= sysPci2MaxBus ) {
		*pciConfAddr = (UINT32) sysPci2ConfAddr;
		*pciConfData = (UINT32) sysPci2ConfData;
	} else
#endif /* MEN_BOARD_HAS_PCI2 */

	{
		*pciConfAddr = (UINT32) sysPciConfAddr;
		*pciConfData = (UINT32) sysPciConfData;
	}
}

#if defined(MEN_BOARD_HAS_PCIE) || defined(MEN_BOARD_HAS_PCI2)
/*******************************************************************************
* setBusSpecDev0Bus - select bus number to be used for actual access
*
* This routine selects the bus number to be used for actual
* PCI configuration access
*
* PCI1: no special handling necessary, always starts at bus 0
* PCI2: internal config accesses or config accesses to root bus
*       on this controller are only initiated if bus is 0
* PCIe: internal config accesses are only initiated if bus number
*       matches the bus set in Primary Bus Number register
*       for exact behaviour see MPC8548ERM.pdf chapter 18.3.7.1.1
*
* host controller for the selected bus
*
* NOMANUAL
* RETURNS: bus number to be used for access
*
*/
static int setBusSpecDev0Bus(
	int busNo,
	int devNo
)
{
#ifdef MEN_BOARD_HAS_PCIE
	if( busNo == sysPciEMinBus && devNo == 0 ){
		/* want to hit the root device */
		return 0; /* bus has to match Primary Bus Number */
	} else
#endif /* MEN_BOARD_HAS_PCIE */

#ifdef MEN_BOARD_HAS_PCI2
	if( busNo == sysPci2MinBus ) {
		/* want to hit the root device or device on root bus of PCI2 */
		return 0; /* bus has to be 0 */
	} else
#endif /* MEN_BOARD_HAS_PCI2 */

	{
		return busNo; /* no special handling */
	}
}
#endif /* MEN_BOARD_HAS_PCIE || MEN_BOARD_HAS_PCI2 */

/*******************************************************************************
*
* sysPciSpecialCycle - generate a special cycle with a message
*
* This routine generates a special cycle with a message.
*
* NOMANUAL
*
* RETURNS: OK
*/

STATUS sysPciSpecialCycle(
	int busNo,
	UINT32 message
) {
	int deviceNo = 0x0000001f;
	int funcNo = 0x00000007;
	UINT32 pciConfAddr;
	UINT32 pciConfData;
  #if defined(MEN_BOARD_HAS_PCIE) || defined(MEN_BOARD_HAS_PCI2)
	int busNoAcc = setBusSpecDev0Bus(busNo,deviceNo); /* actual bus number to be accessed */
  #else /* MEN_BOARD_HAS_PCIE || MEN_BOARD_HAS_PCI2 */
	int busNoAcc = busNo; /* actual nus number to be accessed */
  #endif /* MEN_BOARD_HAS_PCIE || MEN_BOARD_HAS_PCI2 */

	setBusSpecConfRegs( busNo, &pciConfAddr, &pciConfData );

	pciRegWrite((UINT32*)pciConfAddr,
				(UINT32) pciConfigBdfPack(busNoAcc, deviceNo, funcNo) |
				0x80000000);

	PCI_OUT_LONG(pciConfData, message);

	return (OK);
}

/*******************************************************************************
*
* sysPciConfigRead - read from the PCI configuration space
*
* This routine reads either a byte, word or a long word specified by
* the argument <width>, from the PCI configuration space
* This routine works around a problem in the hardware which hangs
* PCI bus if device no 12 is accessed from the PCI configuration space.
*
* NOMANUAL
*
* RETURNS: OK, or ERROR if this library is not initialized
*/

STATUS sysPciConfigRead(
	int busNo,					/* bus number */
	int deviceNo,				/* device number */
	int funcNo,					/* function number */
	int offset,					/* offset into the configuration space */
	int width,					/* width to be read */
	void *pData					/* data read from the offset */
) {
	UINT8 retValByte = 0;
	UINT16 retValWord = 0;
	UINT32 retValLong = 0;
	STATUS retStat = ERROR;
	UINT32 pciConfAddr;
	UINT32 pciConfData;
  #if defined(MEN_BOARD_HAS_PCIE) || defined(MEN_BOARD_HAS_PCI2)
	int busNoAcc = setBusSpecDev0Bus(busNo,deviceNo); /* actual bus number to be accessed */
  #else /* MEN_BOARD_HAS_PCIE || MEN_BOARD_HAS_PCI2 */
	int busNoAcc = busNo; /* actual nus number to be accessed */
  #endif /* MEN_BOARD_HAS_PCIE || MEN_BOARD_HAS_PCI2 */

	if ((busNo == 0) && (deviceNo == 0x1f) ) /* simulator doesn't like this device being used */
		return (ERROR);

	setBusSpecConfRegs( busNo, &pciConfAddr, &pciConfData );

	switch (width) {
	case 1:					/* byte */
		pciRegWrite((UINT32*)pciConfAddr,
					(UINT32) pciConfigBdfPack(busNoAcc, deviceNo, funcNo) |
					(offset & 0xfc) | 0x80000000);

		retValByte = PCI_IN_BYTE(pciConfData + (offset & 0x3));
		*((UINT8 *) pData) = retValByte;
		retStat = OK;
		break;

	case 2:					/* word */
		pciRegWrite((UINT32*)pciConfAddr,
					(UINT32) pciConfigBdfPack(busNoAcc, deviceNo, funcNo) |
					(offset & 0xfc) | 0x80000000);

		retValWord = PCI_IN_WORD(pciConfData + (offset & 0x2));
		*((UINT16 *) pData) = retValWord;
		retStat = OK;
		break;

	case 4:					/* long */
		pciRegWrite((UINT32*)pciConfAddr,
					(UINT32) pciConfigBdfPack(busNoAcc, deviceNo, funcNo) |
					(offset & 0xfc) | 0x80000000);
		retValLong = PCI_IN_LONG(pciConfData);
		*((UINT32 *) pData) = retValLong;
		retStat = OK;
		break;

	default:
		retStat = ERROR;
		break;
	}

	/* printf("sysPciConfigRead 0x%x (%d/%d/%d): 0x%x:0x%x\n",pciConfAddr,busNo,deviceNo,funcNo,offset,*((UINT32 *) pData)); */
	return (retStat);
}

/*******************************************************************************
*
* sysPciConfigWrite - write to the PCI configuration space
*
* This routine writes either a byte, word or a long word specified by
* the argument <width>, to the PCI configuration space
* This routine works around a problem in the hardware which hangs
* PCI bus if device no 12 is accessed from the PCI configuration space.
*
* NOMANUAL
*
* RETURNS: OK, or ERROR if this library is not initialized
*/

STATUS sysPciConfigWrite(
	int busNo,					/* bus number */
	int deviceNo,				/* device number */
	int funcNo,					/* function number */
	int offset,					/* offset into the configuration space */
	int width,					/* width to write */
	ULONG data					/* data to write */
) {
	UINT32 pciConfAddr;
	UINT32 pciConfData;
  #if defined(MEN_BOARD_HAS_PCIE) || defined(MEN_BOARD_HAS_PCI2)
	int busNoAcc = setBusSpecDev0Bus(busNo,deviceNo); /* actual bus number to be accessed */
  #else /* MEN_BOARD_HAS_PCIE || MEN_BOARD_HAS_PCI2 */
	int busNoAcc = busNo; /* actual nus number to be accessed */
  #endif /* MEN_BOARD_HAS_PCIE || MEN_BOARD_HAS_PCI2 */

	if ((busNo == 0) && (deviceNo == 0x1f) ) /* simulator doesn't like this device being used */
		return (ERROR);

	setBusSpecConfRegs( busNo, &pciConfAddr, &pciConfData );

	/* printf("sysPciConfigWrite 0x%x (%d/%d/%d): 0x%x:0x%x\n",pciConfAddr,busNo,deviceNo,funcNo,offset,data); */
	switch (width) {
	case 1:					/* byte */
		pciRegWrite((UINT32*)pciConfAddr,
					(UINT32) pciConfigBdfPack(busNoAcc, deviceNo, funcNo) |
					(offset & 0xfc) | 0x80000000);
		PCI_OUT_BYTE((pciConfData + (offset & 0x3)), data);
		break;

	case 2:					/* word */
		pciRegWrite((UINT32*)pciConfAddr,
					(UINT32) pciConfigBdfPack(busNoAcc, deviceNo, funcNo) |
					(offset & 0xfc) | 0x80000000);
		PCI_OUT_WORD((pciConfData + (offset & 0x2)), data);
		break;

	case 4:					/* long */
		pciRegWrite((UINT32*)pciConfAddr,
					(UINT32) pciConfigBdfPack(busNoAcc, deviceNo, funcNo) |
					(offset & 0xfc) | 0x80000000);
		PCI_OUT_LONG(pciConfData, data);
		break;

	default:
		return (ERROR);

	}
	return (OK);
}


#endif							/* INCLUDE_PCI */
