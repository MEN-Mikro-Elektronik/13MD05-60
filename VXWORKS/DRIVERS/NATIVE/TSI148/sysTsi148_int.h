/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *		  \file	 sysTsi148_Int.h
 *
 *		\author	 cs
 *		  $Date: 2008/07/29 22:23:06 $
 *	  $Revision: 1.1 $
 *
 *		 \brief	 Tundra	TSI148 PCI2VME bridge internal header file
 *
 *	   Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysTsi148_int.h,v $
 * Revision 1.1  2008/07/29 22:23:06  cs
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2007-2008 by MEN Mikro	Elektronik GmbH, Nuremberg,	Germany
 ****************************************************************************/


#ifndef	_SYSTSI148_INT_H
#define	_SYSTSI148_INT_H

#ifdef __cplusplus
	extern "C" {
#endif

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#ifndef SYSTSI148_INT_PRIORITIES
	/*
	 * Define the priority list for the Tempe interrupt support.  List items
	 * correspond to bit postions in the interrupt enable register.  The
	 * first item in the list is the first one scanned when a Tempe interrupt
	 * occurs.
	 */

	#define SYSTSI148_INT_PRIORITIES \
		TSI148_INTEX_IRQ1,  TSI148_INTEX_IRQ2, \
		TSI148_INTEX_IRQ3,  TSI148_INTEX_IRQ4, \
		TSI148_INTEX_IRQ5,  TSI148_INTEX_IRQ6, \
		TSI148_INTEX_IRQ7,  TSI148_INTEX_ACFL, \
		TSI148_INTEX_SYSFL, TSI148_INTEX_IACK, \
		TSI148_INTEX_VIEEN, TSI148_INTEX_VERR, \
		TSI148_INTEX_PERR,  TSI148_INTEX_MB0, \
		TSI148_INTEX_MB1,   TSI148_INTEX_MB2, \
		TSI148_INTEX_MB3,   TSI148_INTEX_LM0, \
		TSI148_INTEX_LM1,   TSI148_INTEX_LM2, \
		TSI148_INTEX_LM3,   TSI148_INTEX_DMA0, \
		TSI148_INTEX_DMA1,  0xff	/* end of list */

#endif /* SYSTSI148_INT_PRIORITIES */


/*********************************
*   LITTLE HELPERS               *
*********************************/

/* MEN macros to access TSI148 regs */
#define OFFSETOF( _type, _part ) ((int)((char*)&((_type*)0)->_part-(char*)0))

#ifndef OSS_Swap8
	/* will not be existable in system, is only needed to simplify the below macros */
	#define OSS_Swap8(_byte) _byte
#endif /* !OSS_Swap8 */
#ifndef OSS_SWAP8
	/* will not be existable in system, is only needed to simplify the below macros */
	#define OSS_SWAP8(_byte) _byte
#endif /* !OSS_SWAP8 */

#define ASS64(_high,_low)		(((UINT64)(_high)<<32)+((UINT64)(_low)))
#define DISASS64(_val64,_high,_low) { _high = (u_int32)(((_val64)>>32) & 0x00000000ffffffffLL); \
				       				  _low  = (u_int32)( (_val64)      & 0x00000000ffffffffLL); }

#if (_BYTE_ORDER == _LITTLE_ENDIAN)

	#define SYSTSI148_CTRL_READ(_size,_reg)   			\
			OSS_Swap##_size(MREAD_D##_size((MACCESS)G_hdl.base,OFFSETOF(TSI148_CRG, _reg)))

	#define SYSTSI148_CTRL_WRITE( _size, _reg, _value ) \
			MWRITE_D##_size((MACCESS)G_hdl.base,OFFSETOF(TSI148_CRG, _reg), OSS_SWAP##_size(_value));

	#define SYSTSI148_CTRL_SETMASK( _size, _reg, _mask ) \
			MSETMASK_D##_size((MACCESS)G_hdl.base, OFFSETOF(TSI148_CRG, _reg), OSS_SWAP##_size(_mask) )

	#define SYSTSI148_CTRL_CLRMASK( _size, _reg, _mask ) \
			MCLRMASK_D##_size((MACCESS)G_hdl.base, OFFSETOF(TSI148_CRG, _reg), OSS_SWAP##_size(_mask) )

	#define SYSTSI148_PCFS_READ(_size,_reg)   			\
			MREAD_D##_size((MACCESS)G_hdl.base,OFFSETOF(TSI148_CRG,_reg))

	#define SYSTSI148_PCFS_WRITE( _size, _reg, _value ) \
			MWRITE_D##_size((MACCESS)G_hdl.base,OFFSETOF(TSI148_CRG, _reg), (_value));

#else /* _LITTLE_ENDIAN */

	#define SYSTSI148_CTRL_READ(_size,_reg)   			\
			MREAD_D##_size((MACCESS)G_hdl.base,OFFSETOF(TSI148_CRG, _reg))

	#define SYSTSI148_CTRL_WRITE(_size,_reg,_value )	\
			MWRITE_D##_size((MACCESS)G_hdl.base,OFFSETOF(TSI148_CRG, _reg), (_value));

	#define SYSTSI148_CTRL_SETMASK( _size, _reg, _mask ) \
			MSETMASK_D##_size((MACCESS)G_hdl.base, OFFSETOF(TSI148_CRG, _reg), (_mask) )

	#define SYSTSI148_CTRL_CLRMASK( _size, _reg, _mask ) \
			MCLRMASK_D##_size((MACCESS)G_hdl.base, OFFSETOF(TSI148_CRG, _reg), (_mask) )

	#define SYSTSI148_PCFS_READ(_size,_reg)   			\
			OSS_Swap##_size(MREAD_D##_size((MACCESS)G_hdl.base,OFFSETOF(TSI148_CRG, _reg)))

	#define SYSTSI148_PCFS_WRITE( _size, _reg, _value ) \
			MWRITE_D##_size((MACCESS)G_hdl.base,OFFSETOF(TSI148_CRG, _reg), OSS_Swap##_size(_value));

#endif /* _LITTLE_ENDIAN */



#define SYSTSI148_DMA_LLDESC_ALIGN8BYTE( _llDescP, _dmaDescP ) 											\
	if( (U_INT32_OR_64)&_dmaDescP->dmaLlDesc & 0x07LL ) { 												\
		_llDescP = (TSI148_DMA_LL_DESC*)(&_dmaDescP->dmaLlDesc + 1); 									\
		TSI148DBG_4((DBH,"sysTsi148Dma: alligned llDesc Addr: 0x%08x\n",(u_int32)_llDescP)); 			\
	} else { 																							\
		_llDescP = (TSI148_DMA_LL_DESC*)&_dmaDescP->dmaLlDesc; 											\
		TSI148DBG_4((DBH,"sysTsi148Dma: llDesc Addr no allignment needed: 0x%08x\n",(u_int32)_llDescP)); \
	}

#define	SYSTSI148_AM_IS_SUP(addr)	((addr & 0x04) == 0x04) /* bit mask 0x04 set */
#define	SYSTSI148_AM_IS_PGM(addr)	((addr & 0x01) == 0x00) /* bit mask 0x01 not set */

#ifdef INCLUDE_SHOW_ROUTINES
	#define SYSTSI148_REGPRINT(_size,_reg) \
			printf( "%-25s @ 0x%08x = 0x%08x\n", #_reg, \
					(G_hdl.base + OFFSETOF(TSI148_CRG, _reg)), \
					SYSTSI148_CTRL_READ(_size,_reg) )

	#define SYSTSI148_PCFSPRINT(_size,_reg) \
			printf( "%-25s @ 0x%08x = 0x%08x\n", #_reg, \
					(G_hdl.base + OFFSETOF(TSI148_CRG, _reg)), \
					SYSTSI148_PCFS_READ(_size,_reg) )
#endif /* INCLUDE_SHOW_ROUTINES */

#ifdef __cplusplus
	}
#endif

#endif	/* _SYSTSI148_INT_H */

