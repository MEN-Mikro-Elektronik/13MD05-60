/* mgt5200IntrCtl.h - MGT5200 interrupt controller driver header file */
/***********************  I n c l u d e  -  F i l e  ************************
 *  
 *         Name: mgt5200IntrCtl.h
 *
 *       Author: kp
 *        $Date: 2004/03/01 09:50:56 $
 *    $Revision: 1.1 $
 * 
 *  Description: MGT5200 interrupt controller driver header file
 *                      
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mgt5200IntrCtl.h,v $
 * Revision 1.1  2004/03/01 09:50:56  UFranke
 * Initial Revision - Tornado 2.2
 *
 * Revision 1.1  2003/09/02 10:56:36  UFranke
 * Initial Revision
 *
 * Revision 1.1  2003/05/28 13:52:26  UFranke
 * Initial Revision
 *
 * Revision 1.3  2003/01/22 12:42:32  kp
 * support CAN interrupts
 *
 * Revision 1.2  2002/10/15 10:36:29  kp
 * added SMARTCOMM and ethernet interrupt vectors
 *
 * Revision 1.1  2002/10/01 09:58:10  UFranke
 * alpha without Ethernet
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

#ifndef _MGT5200INTRCTL_H
#define _MGT5200INTRCTL_H

#ifdef __cplusplus
	extern "C" {
#endif

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
/* interrupt vector definitions */

#define INT_VEC_MAIN_BEGIN	0x00 	/* start of main vectors */
#define INT_VEC_MAIN_END	0x10

#define INT_VEC_CRIT_BEGIN	0x20 	/* start of critical vectors */
#define INT_VEC_CRIT_END	0x23

#define INT_VEC_PER_BEGIN	0x30	/* start of peripheral vectors */
#define INT_VEC_PER_END		0x45

#define MGT5200_INT_VEC_MAX	0x45	/* highest vector controlled by MGT5200 */

/* main */
#define INT_VEC_SLICE_TMR_2	(INT_VEC_MAIN_BEGIN+0x00)
#define INT_VEC_IRQ_1		(INT_VEC_MAIN_BEGIN+0x01)
#define INT_VEC_IRQ_2		(INT_VEC_MAIN_BEGIN+0x02)
#define INT_VEC_IRQ_3		(INT_VEC_MAIN_BEGIN+0x03)
#define INT_VEC_LO_INT		(INT_VEC_MAIN_BEGIN+0x04)
#define INT_VEC_RTC_PINT	(INT_VEC_MAIN_BEGIN+0x05)
#define INT_VEC_RTC_SINT	(INT_VEC_MAIN_BEGIN+0x06)
#define INT_VEC_GPIO_STD	(INT_VEC_MAIN_BEGIN+0x07)
#define INT_VEC_GPIO_WKUP	(INT_VEC_MAIN_BEGIN+0x08)
#define INT_VEC_TMR_0		(INT_VEC_MAIN_BEGIN+0x09)
#define INT_VEC_TMR_1		(INT_VEC_MAIN_BEGIN+0x0A)
#define INT_VEC_TMR_2		(INT_VEC_MAIN_BEGIN+0x0B)
#define INT_VEC_TMR_3		(INT_VEC_MAIN_BEGIN+0x0C)
#define INT_VEC_TMR_4		(INT_VEC_MAIN_BEGIN+0x0D)
#define INT_VEC_TMR_5		(INT_VEC_MAIN_BEGIN+0x0E)
#define INT_VEC_TMR_6		(INT_VEC_MAIN_BEGIN+0x0F)
#define INT_VEC_TMR_7		(INT_VEC_MAIN_BEGIN+0x10)

/* critical */
#define INT_VEC_IRQ_0		(INT_VEC_CRIT_BEGIN+0x00)
#define INT_VEC_SLTMR1		(INT_VEC_CRIT_BEGIN+0x01)
#define INT_VEC_HI_INT		(INT_VEC_CRIT_BEGIN+0x02)
#define INT_VEC_CCS			(INT_VEC_CRIT_BEGIN+0x03)

/* peripheral */
#define	INT_VEC_SMARTCOMM	(INT_VEC_PER_BEGIN+0x00)		/* 48 decimal */
#define	INT_VEC_PSC1		(INT_VEC_PER_BEGIN+0x01)		/* 49 decimal */
#define	INT_VEC_PSC2		(INT_VEC_PER_BEGIN+0x02)		/* 50 decimal */
#define	INT_VEC_PSC3		(INT_VEC_PER_BEGIN+0x03)
#define	INT_VEC_IRDA		(INT_VEC_PER_BEGIN+0x04)
#define	INT_VEC_ETHERNET	(INT_VEC_PER_BEGIN+0x05)
#define	INT_VEC_USB			(INT_VEC_PER_BEGIN+0x06)
#define	INT_VEC_ATA			(INT_VEC_PER_BEGIN+0x07)
#define	INT_VEC_PCI_CTRL	(INT_VEC_PER_BEGIN+0x08)
#define	INT_VEC_PCI_RX		(INT_VEC_PER_BEGIN+0x09)
#define	INT_VEC_PCI_TX		(INT_VEC_PER_BEGIN+0x0A)
#define	INT_VEC_RESV1		(INT_VEC_PER_BEGIN+0x0B)
#define	INT_VEC_RESV2		(INT_VEC_PER_BEGIN+0x0C)
#define	INT_VEC_SPI_MODF	(INT_VEC_PER_BEGIN+0x0D)		/* 62 decimal */
#define	INT_VEC_SPI_SPIF	(INT_VEC_PER_BEGIN+0x0E)
#define	INT_VEC_IIC_1		(INT_VEC_PER_BEGIN+0x0F)
#define	INT_VEC_IIC_2		(INT_VEC_PER_BEGIN+0x10)
#define	INT_VEC_CAN_1		(INT_VEC_PER_BEGIN+0x11)
#define	INT_VEC_CAN_2		(INT_VEC_PER_BEGIN+0x12)
#define	INT_VEC_IR_RX		(INT_VEC_PER_BEGIN+0x13)
#define	INT_VEC_IR_TX		(INT_VEC_PER_BEGIN+0x14)
#define	INT_VEC_XLB_ARB		(INT_VEC_PER_BEGIN+0x15)



/* Bestcomm Interrupts (to share a common table */

#define INUM_SDMA_FIRST		83
#define INUM_SDMA_TEA		83
#define INUM_SDMA_TASK0		84
#define INUM_SDMA_TASK1		85
#define INUM_SDMA_TASK2		86
#define INUM_SDMA_TASK3		87
#define INUM_SDMA_TASK4		88
#define INUM_SDMA_TASK5		89
#define INUM_SDMA_TASK6		90
#define INUM_SDMA_TASK7		91
#define INUM_SDMA_TASK8		92
#define INUM_SDMA_TASK9		93
#define INUM_SDMA_TASK10	94
#define INUM_SDMA_TASK11	95
#define INUM_SDMA_TASK12	96
#define INUM_SDMA_TASK13	97
#define INUM_SDMA_TASK14	98
#define INUM_SDMA_TASK15	99
#define INUM_SDMA_LAST		99


#define IV_ETH			INUM_TO_IVEC (INT_VEC_ETHERNET)
#define SDMA_TASKNO_TO_INUM(TASKNO) (INUM_SDMA_TASK0+TASKNO)
#define SDMA_TASKNO_TO_IVEC(TASKNO) INUM_TO_IVEC(SDMA_TASKNO_TO_INUM(TASKNO))


/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
STATUS mgt5200IntInit (void);


#ifdef __cplusplus
	}
#endif

#endif	/* _MGT5200INTRCTL_H */
