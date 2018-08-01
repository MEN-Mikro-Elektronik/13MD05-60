/******************************************************************************
*
* sysTsi148OutboundAttrShow - displays human readable attributes of TSI148 Outbound Window
*
* This routine displays human readable attributes of TSI148 Outbound Window
*
* RETURNS: N/A
*/
static void sysTsi148OutboundAttrShow(
	u_int32 otatReg
)
{
	switch( otatReg&TSI148_OTAT_AMODE_MASK )
	{
	case TSI148_OTAT_AMODE_A16: printf("A16; "); break;
	case TSI148_OTAT_AMODE_A24: printf("A24; "); break;
	case TSI148_OTAT_AMODE_A32: printf("A32; "); break;
	case TSI148_OTAT_AMODE_A64: printf("A64; "); break;
	default:					printf("AERR; ");
	}

	switch( otatReg&TSI148_OTAT_DBW_MASK )
	{
	case TSI148_OTAT_DBW_16: printf("D16; "); break;
	case TSI148_OTAT_DBW_32: printf("D32; "); break;
	default:				 printf("DERR; ");
	}

	if( 0 != (otatReg & TSI148_OTAT_SUP) )
		printf("SUP ");
	else
		printf("NPRIV ");

	if( 0 != (otatReg & TSI148_OTAT_PGM) )
		printf("PGM");
	else
		printf("DATA");
	printf("; ");

	switch( otatReg&TSI148_OTAT_TM_MASK )
	{
	case TSI148_OTAT_TM_BLT:	printf("BLT; "); break;
	case TSI148_OTAT_TM_MBLT:	printf("MBLT; "); break;
	case TSI148_OTAT_TM_2EVME:	printf("2EVME; "); break;
	case TSI148_OTAT_TM_2ESSTB:	printf("2ESSTB; "); break;
	case TSI148_OTAT_TM_2ESST:
	{
		printf("2ESST");
		switch( otatReg&TSI148_OTAT_2ESSTM_MASK )
		{
		case TSI148_OTAT_2ESSTM_160: printf("160; ");	break;
		case TSI148_OTAT_2ESSTM_267: printf("267; ");	break;
		default:					 printf("320; ");
		}
		break;
	}
	default:					printf("SCT; ");
	}

	printf("MRPF:%s; ", ((otatReg&TSI148_OTAT_MRPFD) == TSI148_OTAT_MRPFD) ? "disabled" : "enabled ");
	switch( otatReg&TSI148_OTAT_PFS_MASK )
	{
	case TSI148_OTAT_PFS_2: printf("PFS2; "); break;
	case TSI148_OTAT_PFS_4: printf("PFS4; "); break;
	case TSI148_OTAT_PFS_8: printf("PFS8; "); break;
	default:				printf("PFS16; ");
	}

	return;
}

/******************************************************************************
*
* sysTsi148OutboundShow - display human readable information about TSI148 Outbound Window
*
* This routine display human readable information about a TSI148 Outbound Window
*
* RETURNS: N/A
*/
void sysTsi148OutboundShow(
	int index
)
{
	TSI148_OUTBOUND outbImg;
	u_int64 reg64;
	u_int32 reg32u, reg32l;

	if( G_hdl.isInit != 2 ) {
		printf(" no VME bridge initialized\n");
		return;
	}

	sysTsi148OutboundWinGetImg( index, &outbImg );

	if( 0 == (outbImg.otat & TSI148_OTAT_EN) ) {
		printf( "\nOutbound Window %d disabled\n", index );
		return;
	} else {
		printf( "\nOutbound Window %d\n",index );
	}

	/* It is enabled */
	/* get VME address */
	reg64 = ASS64(outbImg.otsau, outbImg.otsal) +
			ASS64(outbImg.otofu, outbImg.otofl);
	DISASS64(reg64, reg32u, reg32l);

	printf("    PCI Base: 0x%08x_%08x;    VME Base: 0x%08x_%08x\n",
			outbImg.otsau, outbImg.otsal, reg32u, reg32l);

	/* get size of window */
	reg64 = ASS64(outbImg.oteau, outbImg.oteal) -
			ASS64(outbImg.otsau, outbImg.otsal) +
			(u_int64)0x10000; /* bits A15 - A0 are not compared */
	DISASS64(reg64, reg32u, reg32l);
	printf("    Size:     0x%08x_%08x;     2ESST Broadcast Select: 0x%06x\n    Attributes: ", reg32u, reg32l, outbImg.otbs);
	sysTsi148OutboundAttrShow( outbImg.otat );

	printf( "\n" );

	return;
}


/******************************************************************************
*
* sysTsi148InboundAttrShow - displays human readable attributes of TSI148 Inbound Window
*
* This routine displays human readable attributes of TSI148 Inbound Window
*
* RETURNS: N/A
*/
static void sysTsi148InboundAttrShow(
	u_int32 itatReg
)
{
	switch( itatReg&TSI148_ITAT_AS_MASK )
	{
	case TSI148_ITAT_AS_A16: printf("A16;"); break;
	case TSI148_ITAT_AS_A24: printf("A24;"); break;
	case TSI148_ITAT_AS_A32: printf("A32;"); break;
	case TSI148_ITAT_AS_A64: printf("A64;"); break;
	default:				 printf("AERR;");
	}

	if( 0 != (itatReg & TSI148_ITAT_SUPR) )  printf(" SUPR");
	if( 0 != (itatReg & TSI148_ITAT_NPRIV) ) printf(" NPRIV");
	if( 0 != (itatReg & TSI148_ITAT_PGM) )	 printf(" PGM");
	if( 0 != (itatReg & TSI148_ITAT_DATA) )	 printf(" DATA");
	printf( "; " );

	if( 0 != (itatReg & TSI148_ITAT_2ESSTB) )	printf(" 2ESSTB");
	if( 0 != (itatReg & TSI148_ITAT_2ESST) ){
		printf(" 2ESST");
		switch( itatReg&TSI148_ITAT_2ESSTM_MASK )
		{
		case TSI148_ITAT_2ESSTM_160: printf("160");	break;
		case TSI148_ITAT_2ESSTM_267: printf("267");	break;
		default:					 printf("320");
		}
	}
	if( 0 != (itatReg & TSI148_ITAT_2EVME) )	printf(" 2EVME");
	if( 0 != (itatReg & TSI148_ITAT_MBLT) )		printf(" MBLT");
	if( 0 != (itatReg & TSI148_ITAT_BLT) )		printf(" BLT");
	printf( ";" );

	switch( itatReg&TSI148_ITAT_VFS_MASK )
	{
	case TSI148_ITAT_VFS_64:  printf(" VFS=64;"); break;
	case TSI148_ITAT_VFS_128: printf(" VFS=128;"); break;
	case TSI148_ITAT_VFS_256: printf(" VFS=256;"); break;
	default:				  printf(" VFS=512;");
	}

	printf( " THRESHOLD %sabled; ", (0 != (itatReg & TSI148_ITAT_TH)) ? "en" : "dis");
}

/******************************************************************************
*
* sysTsi148InboundShow - displays human readable information about TSI148 Inbound Window
*
* This routine displays human readable information about a TSI148 Inbound Window
*
* RETURNS: N/A
*/
void sysTsi148InboundShow(
	int index
)
{
	TSI148_INBOUND inbImg;
	u_int64 reg64;
	u_int32 reg32u, reg32l;

	if( G_hdl.isInit != 2 ) {
		printf(" no VME bridge initialized\n");
		return;
	}

	sysTsi148InboundWinGetImg( index, &inbImg );

	if( 0 == ( inbImg.itat & TSI148_ITAT_EN ) ){
		printf( "\nInbound Window %d disabled\n", index );
		return;
	} else {
		printf( "\nInbound Window %d\n",index );
	}

	/* get PCI address */
	switch( inbImg.itat & TSI148_ITAT_AS_MASK )
	{
	case TSI148_ITAT_AS_A16:
		reg64 = (ASS64(inbImg.itsau, inbImg.itsal) & (u_int32)0x000000000000ffffLL) +
				ASS64(inbImg.itofu, inbImg.itofl);
		break;
	case TSI148_ITAT_AS_A24:
		reg64 = (ASS64(inbImg.itsau, inbImg.itsal) & (u_int32)0x0000000000ffffffLL) +
				ASS64(inbImg.itofu, inbImg.itofl);
		break;
	case TSI148_ITAT_AS_A32:
		reg64 = (ASS64(inbImg.itsau, inbImg.itsal) & (u_int32)0x00000000ffffffffLL) +
				ASS64(inbImg.itofu, inbImg.itofl);
		break;
	default: /* A64 */
		reg64 = ASS64(inbImg.itsau, inbImg.itsal) +
				ASS64(inbImg.itofu, inbImg.itofl);
	}
	DISASS64(reg64, reg32u, reg32l);

	printf("    VME Base: 0x%08x_%08x;    PCI Base: 0x%08x_%08x\n",
			inbImg.itsau, inbImg.itsal, reg32u, reg32l);

	/* get size of window */
	switch( inbImg.itat & TSI148_ITAT_AS_MASK )
	{
	case TSI148_ITAT_AS_A16:
		reg64 = (ASS64(inbImg.iteau, inbImg.iteal) | (u_int64)0x000000000000000fLL) -
				ASS64(inbImg.itsau, inbImg.itsal) +
				(u_int64)0x01LL;
		break;
	case TSI148_ITAT_AS_A24:
		reg64 = (ASS64(inbImg.iteau, inbImg.iteal) | (u_int64)0x0000000000000fffLL) -
				ASS64(inbImg.itsau, inbImg.itsal) +
				(u_int64)0x01LL;
		break;
	default:
		reg64 = (ASS64(inbImg.iteau, inbImg.iteal) | (u_int64)0x000000000000ffffLL) -
				ASS64(inbImg.itsau, inbImg.itsal) +
				(u_int64)0x01LL;
	}
	DISASS64(reg64, reg32u, reg32l);
	printf("    Size:     0x%08x_%08x;\n    Attributes: ", reg32u, reg32l);

	sysTsi148InboundAttrShow( inbImg.itat );
	printf("\n");

	return;
}

/******************************************************************************
*
* sysTsi148Show - displays information about TSI148
*
* This routine displays information about TSI148.
*
* RETURNS: N/A
*/
void sysTsi148Show(
	int verbose
)
{
	int i;

	printf("%s\n", VME2PCI_TSI148_RCSid );

	if( G_hdl.isInit != 2 ) {
		printf(" no VME bridge initialized\n");
		return;
	}

	printf("VME bridge %04x/%04x found at pci b/d/f %02x/%02x/%02x\n",
			TSI148_VEN_ID, TSI148_DEV_ID, G_hdl.bus, G_hdl.dev, G_hdl.func );

	printf("control registers at %08x\n", (int)G_hdl.base );

	printf("Slot 1 function %sabled\n", sysVmeIsSystemController() ? "en" : "dis" );

#ifdef TSI148_TBD
	if( G_hdl.littleEndian == 1 )
		printf("byte order little endian\n");
	else
		printf("byte order big endian\n");
#endif

	/* get Windows */

	printf("======================================================================\n" );
	printf("Master Windows\n");
	printf("======================================================================\n" );
	for (i = 0; i < TSI148_OUTBOUND_NO; i++)
	{
		sysTsi148OutboundShow(i);
	}

	printf("======================================================================\n" );
	printf("Slave Windows\n");
	printf("======================================================================\n" );
	for(i = 0; i < TSI148_INBOUND_NO; i++ ) {
		sysTsi148InboundShow( i );
	}

	if( verbose ){
		printf("\n======================================================================\n" );
		printf("PCFS Register Group\n");
		SYSTSI148_PCFSPRINT (16,pcfs.veni);
		SYSTSI148_PCFSPRINT (16,pcfs.devi);
		SYSTSI148_PCFSPRINT (16,pcfs.cmmd);
		SYSTSI148_PCFSPRINT (16,pcfs.stat);
		SYSTSI148_PCFSPRINT (8, pcfs.revi);
		SYSTSI148_PCFSPRINT (8, pcfs.clsz);
		SYSTSI148_PCFSPRINT (8, pcfs.mlat);
		SYSTSI148_PCFSPRINT (8, pcfs.head);
		SYSTSI148_PCFSPRINT (32,pcfs.mbarl);
		SYSTSI148_PCFSPRINT (32,pcfs.mbaru);
		printf("\n======================================================================\n" );
		printf("LCFR Register Group\n");

		for (i = 0; i < TSI148_OUTBOUND_NO; i++)
		{
			printf("    Outbound Window %d\n",i);
			SYSTSI148_REGPRINT (32,lcsr.outbound[i].otsau);
			SYSTSI148_REGPRINT (32,lcsr.outbound[i].otsal);
			SYSTSI148_REGPRINT (32,lcsr.outbound[i].oteau);
			SYSTSI148_REGPRINT (32,lcsr.outbound[i].oteal);
			SYSTSI148_REGPRINT (32,lcsr.outbound[i].otofu);
			SYSTSI148_REGPRINT (32,lcsr.outbound[i].otofl);
			SYSTSI148_REGPRINT (32,lcsr.outbound[i].otbs);
			SYSTSI148_REGPRINT (32,lcsr.outbound[i].otat);
		}

	  #if 0 /* if enabled will thow VMEbus exception (appears in VEAT) */
		printf("    VMEbus interrupt acknowledge\n");
		SYSTSI148_REGPRINT (32,lcsr.viack[0]);
		SYSTSI148_REGPRINT (32,lcsr.viack[1]);
		SYSTSI148_REGPRINT (32,lcsr.viack[2]);
		SYSTSI148_REGPRINT (32,lcsr.viack[3]);
		SYSTSI148_REGPRINT (32,lcsr.viack[4]);
		SYSTSI148_REGPRINT (32,lcsr.viack[5]);
		SYSTSI148_REGPRINT (32,lcsr.viack[6]);
		SYSTSI148_REGPRINT (32,lcsr.viack[7]);
	  #endif /* 0 */
		printf("    RMW\n");
		SYSTSI148_REGPRINT (32,lcsr.rmwau);
		SYSTSI148_REGPRINT (32,lcsr.rmwal);
		SYSTSI148_REGPRINT (32,lcsr.rmwen);
		SYSTSI148_REGPRINT (32,lcsr.rmwc);
		SYSTSI148_REGPRINT (32,lcsr.rmws);
		printf("    VMEbus control\n");
		SYSTSI148_REGPRINT (32,lcsr.vmctrl);
		SYSTSI148_REGPRINT (32,lcsr.vctrl);
		SYSTSI148_REGPRINT (32,lcsr.vstat);
		printf("    PCI/X status\n");
		SYSTSI148_REGPRINT (32,lcsr.pcsr);
		printf("    VME filter\n");
		SYSTSI148_REGPRINT (32,lcsr.vmefl);
		printf("    VME exception status\n");
		SYSTSI148_REGPRINT (32,lcsr.veau);
		SYSTSI148_REGPRINT (32,lcsr.veal);
		SYSTSI148_REGPRINT (32,lcsr.veat);
		printf("    PCI/X error status\n");
		SYSTSI148_REGPRINT (32,lcsr.edpau);
		SYSTSI148_REGPRINT (32,lcsr.edpal);
		SYSTSI148_REGPRINT (32,lcsr.edpxa);
		SYSTSI148_REGPRINT (32,lcsr.edpxs);
		SYSTSI148_REGPRINT (32,lcsr.edpat);

		for(i = 0; i < TSI148_INBOUND_NO; i++ ) {
			printf("    Inbound Window %d\n",i);
			SYSTSI148_REGPRINT (32,lcsr.inbound[i].itsau);
			SYSTSI148_REGPRINT (32,lcsr.inbound[i].itsal);
			SYSTSI148_REGPRINT (32,lcsr.inbound[i].iteau);
			SYSTSI148_REGPRINT (32,lcsr.inbound[i].iteal);
			SYSTSI148_REGPRINT (32,lcsr.inbound[i].itofu);
			SYSTSI148_REGPRINT (32,lcsr.inbound[i].itofl);
			SYSTSI148_REGPRINT (32,lcsr.inbound[i].itat);
		}

		printf("    Inbound Translation GCSR\n");
		SYSTSI148_REGPRINT (32,lcsr.gbau);
		SYSTSI148_REGPRINT (32,lcsr.gbal);
		SYSTSI148_REGPRINT (32,lcsr.gcsrat);
		printf("    Inbound Translation CRG\n");
		SYSTSI148_REGPRINT (32,lcsr.cbau);
		SYSTSI148_REGPRINT (32,lcsr.cbal);
		SYSTSI148_REGPRINT (32,lcsr.crgat);
		printf("    Inbound Translation CR/CSR\n");
		SYSTSI148_REGPRINT (32,lcsr.crou);
		SYSTSI148_REGPRINT (32,lcsr.crol);
		SYSTSI148_REGPRINT (32,lcsr.crat);
		printf("    Inbound Translation Location Monitor\n");
		SYSTSI148_REGPRINT (32,lcsr.lmbau);
		SYSTSI148_REGPRINT (32,lcsr.lmbal);
		SYSTSI148_REGPRINT (32,lcsr.lmat);
		printf("    VMEbus Interrupt control\n");
		SYSTSI148_REGPRINT (32,lcsr.bcu64);
		SYSTSI148_REGPRINT (32,lcsr.bcl64);
		SYSTSI148_REGPRINT (32,lcsr.bpgtr);
		SYSTSI148_REGPRINT (32,lcsr.bpctr);
		SYSTSI148_REGPRINT (32,lcsr.vicr);
		printf("    Local bus Interrupt control\n");
		SYSTSI148_REGPRINT (32,lcsr.inten);
		SYSTSI148_REGPRINT (32,lcsr.inteo);
		SYSTSI148_REGPRINT (32,lcsr.ints);
		SYSTSI148_REGPRINT (32,lcsr.intc);
		SYSTSI148_REGPRINT (32,lcsr.intm1);
		SYSTSI148_REGPRINT (32,lcsr.intm2);

		printf("\n    DMA controller 0\n");
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dctl);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dsta);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dcsau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dcsal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dcdau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dcdal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dclau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dclal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dsau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dsal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].ddau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].ddal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dsat);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].ddat);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dnlau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dnlal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].dcnt);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[0].ddbs);

		printf("\n    DMA controller 1\n");
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dctl);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dsta);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dcsau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dcsal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dcdau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dcdal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dclau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dclal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dsau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dsal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].ddau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].ddal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dsat);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].ddat);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dnlau);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dnlal);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].dcnt);
		SYSTSI148_REGPRINT (32,lcsr.dmactl[1].ddbs);

		printf("\n======================================================================\n" );
		printf("GCSR Register Group\n");

		SYSTSI148_REGPRINT (16,gcsr.veni);
		SYSTSI148_REGPRINT (16,gcsr.devi);
		printf("    Control\n");
		SYSTSI148_REGPRINT (32,gcsr.gctrl);
		printf("    Semaphores\n");
		SYSTSI148_REGPRINT (8,gcsr.sema[0]);
		SYSTSI148_REGPRINT (8,gcsr.sema[1]);
		SYSTSI148_REGPRINT (8,gcsr.sema[2]);
		SYSTSI148_REGPRINT (8,gcsr.sema[3]);
		SYSTSI148_REGPRINT (8,gcsr.sema[4]);
		SYSTSI148_REGPRINT (8,gcsr.sema[5]);
		SYSTSI148_REGPRINT (8,gcsr.sema[6]);
		SYSTSI148_REGPRINT (8,gcsr.sema[7]);
		printf("    Mail Boxes\n");
		SYSTSI148_REGPRINT (32,gcsr.mbox[0]);
		SYSTSI148_REGPRINT (32,gcsr.mbox[1]);
		SYSTSI148_REGPRINT (32,gcsr.mbox[2]);
		SYSTSI148_REGPRINT (32,gcsr.mbox[3]);

		printf("\n======================================================================\n" );
		printf("CR/CSR Register group\n");

		SYSTSI148_REGPRINT (32,csr.csrbcr);
		SYSTSI148_REGPRINT (32,csr.csrbsr);
		SYSTSI148_REGPRINT (32,csr.cbar);
	}

	printf("======================================================================\n" );
	printf("(P2) \t\t- P2 connector is needed for D32 access\n" );
}
void sysVmeShow(
	int verbose
)
{
	sysTsi148Show( verbose );
}
