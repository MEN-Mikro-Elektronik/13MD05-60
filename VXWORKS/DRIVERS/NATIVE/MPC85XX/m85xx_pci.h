#ifdef INCLUDE_PCI

STATUS sysPciSpecialCycle(
	int busNo,
	UINT32 message
);
STATUS sysPciConfigRead(
	int busNo,
	int deviceNo,
	int funcNo,
	int offset,
	int width,
	void *pData
);
STATUS sysPciConfigWrite(
	int busNo,
	int deviceNo,
	int funcNo,
	int offset,
	int width,
	ULONG data
);

#endif /* INCLUDE_PCI */

