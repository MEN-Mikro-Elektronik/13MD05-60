/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  sys_lm81.h
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2005/07/05 12:28:37 $
 *    $Revision: 2.1 $
 *
 *  	 \brief  LM81 export routines of MEN VxWorks BSPs
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sys_lm81.h,v $
 * Revision 2.1  2005/07/05 12:28:37  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _SYS_LM81_H
#define _SYS_LM81_H

#ifdef __cplusplus
	extern "C" {
#endif


/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/
typedef struct {
	const char *descr;			/* description of input */
	u_int16 imask;				/* imask or status bit mask*/
	int32 fullScale;			/* in mV */
} LM81_ANALOG_INPUT_DESC;

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

/* 
 * the defines below select the input channel to measure 
 * must be in sync with lm81.h!
 */
#ifndef LM81_2_5V
#define	LM81_2_5V 	0x20		/**< 2.5V analog in register */
#define	LM81_VCCP1 	0x21		/**< VCCP1 V analog in register */
#define	LM81_3_3V	0x22		/**< 3.3V analog in register */
#define	LM81_5V 	0x23		/**< 5V analog in register */
#define	LM81_12V	0x24		/**< 12V analog in register */
#define	LM81_VCCP2	0x25		/**< VCCP2 V analog in register */
#endif /* LM81_2_5V */

#define SYS_LM81_BAD_TEMPERATURE		10000
#define SYS_LM81_BAD_VOLTAGE 			0x7fffffff

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
extern STATUS sysLm81Init(void);
extern int sysLm81Voltage( int channel );
extern int sysLm81Temp(void);

#ifdef __cplusplus
	}
#endif

#endif /* SYS_LM81_H */

