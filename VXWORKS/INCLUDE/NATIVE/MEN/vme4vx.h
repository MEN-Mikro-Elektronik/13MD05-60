/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *		\file  vme4vx.h
 *
 *
 * (c) Copyright 2018 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ******************************************************************************/
#ifndef _VME4VX_H
#define _VME4VX_H

#  ifdef __cplusplus
extern "C"
{
#  endif

/*--------------------------------------+
 |	VME4VX_SPC VME space definitions	|
 +--------------------------------------*/
typedef enum
{
    VME4VX_SPC_A16_D16 = 0, /* Short non priviledged (D16) */
    VME4VX_SPC_A24_D64_BLT = 1, /* Extended non priviledged (D64-BLT) */
    VME4VX_SPC_A16_D32 = 2, /* Short non priviledged (D32) */
    /* empty line contained VME4VX_SPC_A16_D32_BLT */
    VME4VX_SPC_A24_D16 = 4, /* Standard non priviledged (D16) */
    VME4VX_SPC_A24_D16_BLT = 5, /* Standard non priviledged (D16-BLT) */
    VME4VX_SPC_A24_D32 = 6, /* Standard non priviledged (D32) */
    VME4VX_SPC_A24_D32_BLT = 7, /* Standard non priviledged (D32-BLT) */
    VME4VX_SPC_A32_D32 = 8, /* Extended non priviledged (D32) */
    VME4VX_SPC_A32_D32_BLT = 9, /* Extended non priviledged (D32-BLT) */
    VME4VX_SPC_A32_D64_BLT = 10, /* Extended non priviledged (D64-BLT) */

    VME4VX_SPC_A24_D16_SWAPPED = 0x11, /* Swapped non priviledged (D16) */
    VME4VX_SPC_A24_D32_SWAPPED = 0x12, /* Swapped non priviledged (D32) */
    VME4VX_SPC_A32_D16_SWAPPED = 0x13, /* Swapped non priviledged (D16) */
    VME4VX_SPC_A32_D32_SWAPPED = 0x14, /* Swapped non priviledged (D32) */
    VME4VX_SPC_A32_D64_SWAPPED = 0x15, /* Swapped non priviledged (D64) */

} VME4VX_SPACE;

/*--------------------------------------+
 |  VME4VX_BUSERR handler parameters    |
 +--------------------------------------*/
typedef struct
{
    uint32_t    busErrorAddrs;
    uint32_t    busErrorAccess;
    void*       parameters;
}VME4VX_BUSERR_PARAMS;

/*------------------------------------------+
 |	VME4VX_IRQVEC VME interrupt vectors		|
 +------------------------------------------*/
/*
 * Numbers 0 to 0xF7 correspond to VME vectors	
 * Numbers above 0xF7 are pseudo vectors for
 * interrupts generated by the VME bridge
 */
#define VME4VX_IRQVEC_BUSERR	0xFF	/* vector for bus error interrupt */
#define VME4VX_IRQVEC_ACFAIL	0xFE	/* vector for ACFAIL interrupt */
/* vector for mailbox _n interrupt caused by VME read access */
#define VME4VX_IRQVEC_MBOX(_n)  (0xF8+(_n))
/* vector for location monitor n interrupt */
#define VME4VX_IRQVEC_LOCMON(_n)    (0xFC+(_n))

#define VME4VX_NUM_VECTORS	0x100

/*--------------------------------------+
 |	VME4VX_IRQLEV VME interrupt levels	|
 +--------------------------------------*/
/* 
 * Numbers 1 to 7 correspond to the VME IRQ lines 1 to 7.
 * Numbers above 7 are special VME interrupts generated by the VME
 * bridge.
 */
#define VME4VX_IRQLEV_UNKNOWN		0	/* IRQ level unknown	*/
#define VME4VX_IRQLEV_1				1	/* VME IRQ level 1		*/
#define VME4VX_IRQLEV_2				2	/* VME IRQ level 2		*/
#define VME4VX_IRQLEV_3				3	/* VME IRQ level 3		*/
#define VME4VX_IRQLEV_4				4	/* VME IRQ level 4		*/
#define VME4VX_IRQLEV_5				5	/* VME IRQ level 5		*/
#define VME4VX_IRQLEV_6				6	/* VME IRQ level 6		*/
#define VME4VX_IRQLEV_7				7	/* VME IRQ level 7		*/
#define VME4VX_IRQLEV_NUM			7

/*-----------------------------------------+
 |     VME4VX_RW_FLAGS for vme4vx tools    |
 +----------------------------------------*/
/* no flags */
#define VME4VX_RW_NOFLAGS      0
/* use DMA engine for non-BLT spaces */
#define VME4VX_RW_USE_SGL_DMA  0x02
#define VME4VX_RW_SRAM_DMA     0x04
#define VME4VX_RW_NOVMEINC     0x08

#endif
