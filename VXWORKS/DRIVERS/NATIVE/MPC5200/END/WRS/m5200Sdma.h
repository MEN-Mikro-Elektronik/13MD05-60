/* m5200Sdma.h - Motorola MPC5200 Smartcomm DMA controller interface header */

/* Copyright 1990-2003 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,17Jull03,pkr written
*/

#ifndef __INCm5200Sdmah
#define __INCm5200Sdmah

/* includes */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sdma_register_set {
    volatile UINT32 taskBar;           /*  MBAR_SDMA + 0x00 sdTpb */
    volatile UINT32 currentPointer;    /*  MBAR_SDMA + 0x04 sdMdeComplex */
    volatile UINT32 endPointer;        /*  MBAR_SDMA + 0x08 sdMdeComplex */
    volatile UINT32 variablePointer;   /*  MBAR_SDMA + 0x0c sdMdeComplex */

/*  The following are Priority Task Decoder (ptd) UINTs in sdma/rtl_v/sdPtd.v. */
/*  The ptd UINTister map below is from the smartcomm spec, table 3-2, page 3-54. */

/*  The spec shows the ptd map as 20 words, but sdPtd.v has only implemented 19. */
/*  The word commented out below is the one which is not implemented. */

/*   volatile UINT8  IntVect;    // MBAR_SDMA + 0xXX sdPtd read only */
/*   volatile UINT8  res0[3];    // MBAR_SDMA + 0xXX sdPtd read only */
    volatile UINT8  IntVect1;   /*  MBAR_SDMA + 0x10 sdPtd */
    volatile UINT8  IntVect2;   /*  MBAR_SDMA + 0x11 sdPtd */
    volatile UINT16 PtdCntrl;   /*  MBAR_SDMA + 0x12 sdPtd */
    
    volatile UINT32 IntPend;    /*  MBAR_SDMA + 0x14 sdPtd */
    volatile UINT32 IntMask;    /*  MBAR_SDMA + 0x18 sdPtd */
    
    volatile UINT32 TCR01;      /*  MBAR_SDMA + 0x1c sdPtd */
    volatile UINT32 TCR23;      /*  MBAR_SDMA + 0x20 sdPtd */
    volatile UINT32 TCR45;      /*  MBAR_SDMA + 0x24 sdPtd */
    volatile UINT32 TCR67;      /*  MBAR_SDMA + 0x28 sdPtd */
    volatile UINT32 TCR89;      /*  MBAR_SDMA + 0x2c sdPtd */
    volatile UINT32 TCRAB;      /*  MBAR_SDMA + 0x30 sdPtd */
    volatile UINT32 TCRCD;      /*  MBAR_SDMA + 0x34 sdPtd */
    volatile UINT32 TCREF;      /*  MBAR_SDMA + 0x38 sdPtd */
    
#if 0
    volatile UINT8  IPR0;       /*  MBAR_SDMA + 0x3c sdPtd */
    volatile UINT8  IPR1;       /*  MBAR_SDMA + 0x3d sdPtd */
    volatile UINT8  IPR2;       /*  MBAR_SDMA + 0x3e sdPtd */
    volatile UINT8  IPR3;       /*  MBAR_SDMA + 0x3f sdPtd */
    volatile UINT8  IPR4;       /*  MBAR_SDMA + 0x40 sdPtd */
    volatile UINT8  IPR5;       /*  MBAR_SDMA + 0x41 sdPtd */
    volatile UINT8  IPR6;       /*  MBAR_SDMA + 0x42 sdPtd */
    volatile UINT8  IPR7;       /*  MBAR_SDMA + 0x43 sdPtd */
    volatile UINT8  IPR8;       /*  MBAR_SDMA + 0x44 sdPtd */
    volatile UINT8  IPR9;       /*  MBAR_SDMA + 0x45 sdPtd */
    volatile UINT8  IPR10;      /*  MBAR_SDMA + 0x46 sdPtd */
    volatile UINT8  IPR11;      /*  MBAR_SDMA + 0x47 sdPtd */
    volatile UINT8  IPR12;      /*  MBAR_SDMA + 0x48 sdPtd */
    volatile UINT8  IPR13;      /*  MBAR_SDMA + 0x49 sdPtd */
    volatile UINT8  IPR14;      /*  MBAR_SDMA + 0x4a sdPtd */
    volatile UINT8  IPR15;      /*  MBAR_SDMA + 0x4b sdPtd */
    volatile UINT8  IPR16;      /*  MBAR_SDMA + 0x4c sdPtd */
    volatile UINT8  IPR17;      /*  MBAR_SDMA + 0x4d sdPtd */
    volatile UINT8  IPR18;      /*  MBAR_SDMA + 0x4e sdPtd */
    volatile UINT8  IPR19;      /*  MBAR_SDMA + 0x4f sdPtd */
    volatile UINT8  IPR20;      /*  MBAR_SDMA + 0x50 sdPtd */
    volatile UINT8  IPR21;      /*  MBAR_SDMA + 0x51 sdPtd */
    volatile UINT8  IPR22;      /*  MBAR_SDMA + 0x52 sdPtd */
    volatile UINT8  IPR23;      /*  MBAR_SDMA + 0x53 sdPtd */
    volatile UINT8  IPR24;      /*  MBAR_SDMA + 0x54 sdPtd */
    volatile UINT8  IPR25;      /*  MBAR_SDMA + 0x55 sdPtd */
    volatile UINT8  IPR26;      /*  MBAR_SDMA + 0x56 sdPtd */
    volatile UINT8  IPR27;      /*  MBAR_SDMA + 0x57 sdPtd */
    volatile UINT8  IPR28;      /*  MBAR_SDMA + 0x58 sdPtd */
    volatile UINT8  IPR29;      /*  MBAR_SDMA + 0x59 sdPtd */
    volatile UINT8  IPR30;      /*  MBAR_SDMA + 0x5a sdPtd */
    volatile UINT8  IPR31;      /*  MBAR_SDMA + 0x5b sdPtd */
#else
    volatile UINT8  IPR[32];	/*  MBAR_SDMA + 0x3c ..
    								MBAR_SDMA + 0x5b sdPtd */
#endif

    volatile UINT32 res1;       /*  reserved */
    volatile UINT32 res2;       /*  reserved */
    volatile UINT32 res3;       /*  reserved */
    volatile UINT32 MDEDebug;   /*  MBAR_SDMA + 0x68 sdMdeComplex */
    volatile UINT32 ADSDebug;   /*  MBAR_SDMA + 0x6c sdAdsTop */
    volatile UINT32 Value1;     /*  MBAR_SDMA + 0x70 sdDbg */
    volatile UINT32 Value2;     /*  MBAR_SDMA + 0x74 sdDbg */
    volatile UINT32 Control;    /*  MBAR_SDMA + 0x78 sdDbg */
    volatile UINT32 Status;     /*  MBAR_SDMA + 0x7c sdDbg */
    volatile UINT32 EU00;       /*  MBAR_SDMA + 0x80 sdMac macer UINT */
    volatile UINT32 EU01;       /*  MBAR_SDMA + 0x84 sdMac macemr UINT */
    volatile UINT32 EU02;       /*  MBAR_SDMA + 0x88 unused */
    volatile UINT32 EU03;       /*  MBAR_SDMA + 0x8c unused */
    volatile UINT32 EU04;       /*  MBAR_SDMA + 0x90 unused */
    volatile UINT32 EU05;       /*  MBAR_SDMA + 0x94 unused */
    volatile UINT32 EU06;       /*  MBAR_SDMA + 0x98 unused */
    volatile UINT32 EU07;       /*  MBAR_SDMA + 0x9c unused */
    volatile UINT32 EU10;       /*  MBAR_SDMA + 0xa0 unused */
    volatile UINT32 EU11;       /*  MBAR_SDMA + 0xa4 unused */
    volatile UINT32 EU12;       /*  MBAR_SDMA + 0xa8 unused */
    volatile UINT32 EU13;       /*  MBAR_SDMA + 0xac unused */
    volatile UINT32 EU14;       /*  MBAR_SDMA + 0xb0 unused */
    volatile UINT32 EU15;       /*  MBAR_SDMA + 0xb4 unused */
    volatile UINT32 EU16;       /*  MBAR_SDMA + 0xb8 unused */
    volatile UINT32 EU17;       /*  MBAR_SDMA + 0xbc unused */
    volatile UINT32 EU20;       /*  MBAR_SDMA + 0xc0 unused */
    volatile UINT32 EU21;       /*  MBAR_SDMA + 0xc4 unused */
    volatile UINT32 EU22;       /*  MBAR_SDMA + 0xc8 unused */
    volatile UINT32 EU23;       /*  MBAR_SDMA + 0xcc unused */
    volatile UINT32 EU24;       /*  MBAR_SDMA + 0xd0 unused */
    volatile UINT32 EU25;       /*  MBAR_SDMA + 0xd4 unused */
    volatile UINT32 EU26;       /*  MBAR_SDMA + 0xd8 unused */
    volatile UINT32 EU27;       /*  MBAR_SDMA + 0xdc unused */
    volatile UINT32 EU30;       /*  MBAR_SDMA + 0xe0 unused */
    volatile UINT32 EU31;       /*  MBAR_SDMA + 0xe4 unused */
    volatile UINT32 EU32;       /*  MBAR_SDMA + 0xe8 unused */
    volatile UINT32 EU33;       /*  MBAR_SDMA + 0xec unused */
    volatile UINT32 EU34;       /*  MBAR_SDMA + 0xf0 unused */
    volatile UINT32 EU35;       /*  MBAR_SDMA + 0xf4 unused */
    volatile UINT32 EU36;       /*  MBAR_SDMA + 0xf8 unused */
   volatile UINT32 EU37;       /*  MBAR_SDMA + 0xfc unused */
} sdma_regs;

extern sdma_regs *sysSDMA;

#define SDMA_PTDCNTRL_PE	1

#ifdef __cplusplus
}
#endif

#endif /* __INCm5200Sdmah */

