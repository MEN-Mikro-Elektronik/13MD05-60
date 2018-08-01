/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  em04touch_int.h
 *
 *      \author  kp
 *        $Date: 2012/09/22 11:44:54 $
 *    $Revision: 2.2 $
 *
 *  	 \brief  internal header file for EM04 touch driver
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: em04touch_int_ham.h,v $
 * Revision 2.2  2012/09/22 11:44:54  ts
 * R: diab compiler warned about comma at end of enumerator list
 * M: removed comma
 *
 * Revision 2.1  2012/02/24 19:38:45  ts
 * Initial checkin for customer specific Calibration method
 *
 * R: calibration values and consideration is calibration method specific
 * M: move functions xxxRawToPix from em04touchdrv.c to em04touchcalib.c
 *
 * Revision 2.7  2007/03/03 02:10:47  cs
 * changed:
 *    - for enabling interrupt use different command
 *      SPIDATA_NOP now really NOP
 *
 * Revision 2.6  2006/10/13 22:32:16  cs
 * changed:
 *   - reduced EM04_TOUCH_DEF_THOLD to 20
 *   - position/measured value state now extra typedef EM04_TOUCH_POSSTATE
 * added variables for saving 2nd last value and its state in handle
 *
 * Revision 2.5  2006/06/01 17:44:46  cs
 * added EM04TOUCH_STATE for touch recovery
 *
 * Revision 2.4  2005/06/23 16:12:11  kp
 * Copyright line changed (sbo)
 *
 * Revision 2.3  2005/05/11 16:32:44  kp
 * allow negative calibration data for swapped touch panels
 *
 * Revision 2.2  2005/04/12 16:56:23  kp
 * cosmetic
 *
 * Revision 2.1  2005/02/21 10:28:11  kp
 * documentation update.
 * Port to OS-9
 *
 * Revision 2.0  2005/02/18 16:06:58  kp
 * complete reimplementation
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef _EM04TOUCH_INT_H
# define _EM04TOUCH_INT_H

#ifdef __cplusplus
	extern "C" {
#endif

#include <MEN/men_typs.h>
#include <MEN/oss.h>
#include <MEN/mdis_err.h>
#include <MEN/dbg.h>
#include <MEN/maccess.h>

#ifdef MENMON
# include <mmglob.h>
#endif

#ifdef OS9000
# include <MEN/os_men.h>
# include <MEN/drvsupp.h>
#endif

#include <MEN/em04touch.h>
#include <MEN/sysparam2.h>

/*! \mainpage

  This documentation describes the general issues about the touch driver
  based on a ADS78XX touch controller.

  \section SEC1 Public Functions

  The driver provides 6 routines to handle the different request on the
  touch controller
   - EM04TOUCH_Init()		to initialize the controller and data structure
   - EM04TOUCH_Exit()		to deinitialize the driver
   - EM04TOUCH_Irq()		to handle the interrupt requests
   - EM04TOUCH_MeasureAuxInp()	to measure temperature and battery voltage
   - EM04TOUCH_Calibrate()	to perform a calibration for the touch panel
   - EM04TOUCH_GetID()		to ask for the touch identity code

   \section SEC2 Functional Description ???
   This section gives you an overview about the functionality of the
   supported routines.

   \subsection INIT Init routine

   The driver can be initialize by calling the EM04TOUCH_Init()
   routine.  It will create the EM04_TOUCH_HANDLE, enable the
   interrupts in the FPGA register and prepare the touch controller
   for working by transferring the \c EM04_TOUCH_SPI_NOP_CMD to the
   controller. Now the hardware is prepared to generate an interrupt
   if the touchpanel is touched.

   \subsection IRQ Interrupt routine

   If an touch interrupt occurred the following steps will be done
   		- raw X1 position is measured 3 times
   		- raw Y position is measured 3 times
   		- raw X2 position is measured 3 times
   		- raw X position will be verified
   		- X/Y position will be converted into logical co ordinates
   		- switches for PENIRQ will be set and interrupts enabled again
   		- touchevent will be called


   \endcode

   During a position measurement of \c X or \c Y the PENIRQ will be disabled.
   Directly after the measurement is finished and the value is converted into
   logical co ordinates the PENIRQ is enabled and checked again, so that the
   information if the touch is still pressed or has just released is added
   to (EM04TOUCH_EVENT::event) data structure. If the touch is released the
   last valid position will be reported.

   \subsection TEMP Temperature measurement

   The routine EM04TOUCH_MeasureAuxInp() called with \c
   device=EM04TOUCH_DEV_TEMP will initiate the corresponding
   measurement and wait until it will get the result of the
   measurement. The PENIRQ will be disabled during measurement so no
   touch events are reported.

   Temperature is measured by measuring the voltage over the built
   in diode with two different currents. The difference between these
   two voltages can be used to compute the temperature.

   A total number of 25 such measurements are used to compute the
   mean value, so one EM04TOUCH_DEV_TEMP call takes approx. 500ms to execute.

   The unit of the returned value is in 1/10 °C (Degree Celsius).

   \subsection BATT Battery voltage measurement

   The routine EM04TOUCH_MeasureAuxInp() called with \c
   device=EM04TOUCH_DEV_BATT will initiate the corresponding
   measurement and wait until it will get the result of the
   measurement. The PENIRQ will be disabled during measurement so no
   touch events are reported.

   The unit of measured value is in mV (10^-3 Volt)

   \subsection TOUCHID Get touch identity code

   The routine EM04TOUCH_GetID() can be called to get the touch
   identity code of the touch controller, for example to verify the
   type of controller and SPI communication between controller and
   system. For the used touch controller ADS 7873 the identity code is
   0x800 (100000000000). Not every controller will support this
   feature.


   \subsection CALI Calibration

   The routine EM04TOUCH_Calibrate() performs a calibration of the
   touch panel to determine the hardware specific parameters of the
   touch panel. Depending on the given parameter the routine will
   \b load/store calibration parameter form/into EEPROM or find the
   upper left/lower right edges of the touch panel or calculate the
   calibration parameter.

   The calibration should be performed after following steps.

       - find upper left edge (\c mode=EM04TOUCH_CALIB_UL)
       - find lower right edge (\c mode=EM04TOUCH_CALIB_LR)
       - calculate parameters (\c mode=EM04TOUCH_CALIB_CALC)
       - save parameters to EEPROM (\c mode=EM04TOUCH_CALIB_SAVE)

   The calibration parameter give information about the measurable
   area of the used touch controller and store it in the
   EM04_TOUCH_CAL data structure.

   \code
    upper left
      +----------------------------------------------------+----
      |(0/0) ------>     ( physical touch area )           |   |yoffset
      | | +--------------------------------------------+   |------
      | | |                                            |   |     |
      | | |                                            |   |     |
      | | |                                            |   |     |
      | V |    ( measurable touch area )               |   |     |ydelta
      |   |                                            |   |     |
      |   |                                            |   |     |
      |   |                                            |   |     |
      |   +--------------------------------------------+   |------
      |                                                    |
      +----------------------------------------------------+ lower right
      |   |                                            |
      |<->|xoffset                                     |
          |                                            |
          |<----------------- xdelta ----------------->|
   \endcode

	If the touch is pressed the first valid measured value will
	be taken to calculate the calibration parameters. If the will
	be released the next calibration step can be initiated.
    During the calibration no touchevent data is reported to the OS.

    If no calibration data could be load from EEPROM or could be calculated
    the touch panel will be calibrated with defaults \c x/yoffset=0
    and \c x/ydelta=0xFFF (max value of a 12bit conversion).

*/

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

/*! \defgroup ADS78XXBITFIELD ADS78XX fields

	BURR-BROWN Touch Screen Controller Bit fields of CMD Byte
*/
/**@{*/
#define ADS78XX_START_BIT (1U<<7)	/**< Startbit HIGH if cmd is sent */
#define ADS78XX_A2        (1U<<6)	/**< A2 Channel select bit */
#define ADS78XX_A1        (1U<<5)	/**< A1 Channel select bit */
#define ADS78XX_A0        (1U<<4)	/**< A0 Channel select bit */
#define ADS78XX_MODE      (1U<<3)	/**< HIGH = 8, LOW = 12 bits conversion */
#define ADS78XX_SER_DFR   (1U<<2)	/**< LOW = differential mode            */
#define ADS78XX_PD1       (1U<<1)	/**< PD1,PD0 = 11  PENIRQ disable       */
#define ADS78XX_PD0       (1U<<0)	/**< PD1,PD0 = 00  PENIRQ enable    	*/
/**@}*/

/*! \defgroup SPIDATA SPI CMD byte data defines

	define for the CTRL byte to initiate X, Y, battery, temperature measuring
		- measure x and y in differential mode, PENIRQ disable
		- measure temperature in singel ended mode, PENIRQ enable
		- measure battery voltage in singel ended mdoe, PENIRQ disable
*/
/**@{*/
#define SPIDATA_ASKX    (ADS78XX_START_BIT   | \
                         ADS78XX_A2          | \
                         ADS78XX_A1        *0| \
                         ADS78XX_A0          | \
                         ADS78XX_MODE      *0| \
                         ADS78XX_SER_DFR   *0| \
                         ADS78XX_PD1         | \
                         ADS78XX_PD0            )
                         /**< CTRL Byte Ask for X conversion. Disable PENIRQ */


#define SPIDATA_ASKY    (ADS78XX_START_BIT   | \
                         ADS78XX_A2        *0| \
                         ADS78XX_A1        *0| \
                         ADS78XX_A0          | \
                         ADS78XX_MODE      *0| \
                         ADS78XX_SER_DFR   *0| \
                         ADS78XX_PD1         | \
                         ADS78XX_PD0            )
                         /**< CTRL Byte Ask for Y conversion. Disable PENIRQ */


#define SPIDATA_NOP     (ADS78XX_START_BIT   | \
                         ADS78XX_A2          | \
                         ADS78XX_A1          | \
                         ADS78XX_A0        *0| \
                         ADS78XX_MODE      *0| \
                         ADS78XX_SER_DFR   *0| \
                         ADS78XX_PD1       *0| \
                         ADS78XX_PD0       *0   )
                         /**< CRL Byte for IDLE mode Enable PENIRQ */

#define SPIDATA_ASKTEMP0 (ADS78XX_START_BIT   | \
                         ADS78XX_A2        *0| \
                         ADS78XX_A1        *0| \
                         ADS78XX_A0        *0| \
                         ADS78XX_MODE      *0| \
                         ADS78XX_SER_DFR     | \
                         ADS78XX_PD1       *1| \
                         ADS78XX_PD0       *0     )
                         /**< CTRL Byte Ask for TEMP conversion. Disable PENIRQ */

#define SPIDATA_ASKTEMP1 (ADS78XX_START_BIT   | \
                         ADS78XX_A2          | \
                         ADS78XX_A1          | \
                         ADS78XX_A0          | \
                         ADS78XX_MODE      *0| \
                         ADS78XX_SER_DFR     | \
                         ADS78XX_PD1       *1| \
                         ADS78XX_PD0       *0     )
                         /**< CTRL Byte Ask for TEMP conversion. Disable PENIRQ */

#define SPIDATA_ASKBATT (ADS78XX_START_BIT   | \
                         ADS78XX_A2        *0| \
                         ADS78XX_A1          | \
                         ADS78XX_A0        *0| \
                         ADS78XX_MODE      *0| \
                         ADS78XX_SER_DFR     | \
                         ADS78XX_PD1         | \
                         ADS78XX_PD0            )
                         /**< CTRL Byte Ask for BATT conversion. Disable PENIRQ */

#define SPIDATA_ASKID   (ADS78XX_START_BIT   | \
                         ADS78XX_A2          | \
                         ADS78XX_A1          | \
                         ADS78XX_A0        *0| \
                         ADS78XX_MODE      *0| \
                         ADS78XX_SER_DFR   *0| \
                         ADS78XX_PD1         | \
                         ADS78XX_PD0            )
                         /**< CTRL Byte Ask for ID conversion. Disable PENIRQ */


/**@}*/

/*! \defgroup SPI_BLOCK 24bit CTRL byte block defines

	24 bit block for SPI transfer
*/
/**@{*/
#define EM04_TOUCH_SPI_ASKX_CMD    ( (SPIDATA_ASKX << 16) | (SPIDATA_ASKX) )
	/**< 24 bit SPI block to ask for x data */
#define EM04_TOUCH_SPI_ASKY_CMD    ( (SPIDATA_ASKY << 16) | (SPIDATA_ASKY) )
	/**< 24 bit SPI block to ask for y data */
#define EM04_TOUCH_SPI_NOP_CMD    	(SPIDATA_NOP << 16)
	/**< 24 bit SPI block to enable PENIRQ */

#define EM04_TOUCH_SPI_ASKTEMP0_CMD (SPIDATA_ASKTEMP0 << 16)
	/**< 24 bit SPI block to ask for temperature */
#define EM04_TOUCH_SPI_ASKTEMP1_CMD (SPIDATA_ASKTEMP1 << 16)
	/**< 24 bit SPI block to ask for temperature */
#define EM04_TOUCH_SPI_ASKBATT_CMD (SPIDATA_ASKBATT << 16)
	/**< 24 bit SPI block to ask for battery voltage */
#define EM04_TOUCH_SPI_ASKID_CMD (SPIDATA_ASKID << 16)
	/**< 24 bit SPI block to ask for ID */

/**@}*/

/*! \defgroup TOUCHOFFSET Touch controller register offsets

  offsets for TX/RX data and interrupt registers
*/
/**@{*/
#define Z031_DATA_REG		0x00	/**< TX/RX data register */
#define Z031_IER_REG		0x04	/**< interrupt enable register */
#define Z031_IRQ_REG		0x08	/**< interrupt request register */
/**@}*/

/*! \defgroup INTDEF Touch controller interrupt mask defines

	defines to mask the interrupt
*/
/**@{*/
#define Z031_SPIIRQ_MSK		0x02	/**< SPI interrupt mask */
#define Z031_PENIRQ_MSK		0x01	/**< PENIRQ interrupt mask */
#define Z031_IRQ_ALL_MSK	0x03	/**< all interrupt mask */
/**@}*/

/*! \defgroup MISC miscellaneous touch controller defines
	further defines for touch
*/
/**@{*/
#define EM04_TOUCH_12BIT_MSK	0x00007FF8	/**< used data mask if 12bit converting */
#define EM04_TOUCH_8BIT_MSK		0x000007F8	/**< used data mask if 8bit converting */
#define EM04_TOUCH_DEF_THOLD			20		/**< tolerance value for touch measurement */
/*#define EM04_TOUCH_MAXMOVE_LIMIT		100	*/	/**< tolerance value for touch move */
#define EM04_TOUCH_TEMPREF_VOLTAGE		2500	/**< reference voltage in mV */
#define EM04_TOUCH_BATTREF_VOLTAGE		5000	/**< reference voltage in mV */
#define EM04_TOUCH_12BIT_CONV_VAL		4096	/**< max 12bit conversion value */
#define EM04_TOUCH_AD7873_MIN_TMP_VAL	-40		/**< min Temperature border */
#define EM04_TOUCH_AD7873_MAX_TMP_VAL	85		/**< max Temperature border */
#define EM04_TOUCH_FPAF					1000	/**< fix point arithmetic factor */
#define EM04_TOUCH_CAL_PARAM_SIZE		80		/**< size of parameters in EEPROM */


/*! \defgroup SPILEV SPI interrupt level
	is stored in EM04_TOUCH_HANDLE::irqstate and shows the state of a running
	x or y measurement
*/

/**@{*/
#define EM04_TOUCH_SPII_IDLE		0	/**< irqstate idle define */
#define EM04_TOUCH_SPII_LEVSTART	1	/**< irqstate start define */
#define EM04_TOUCH_SPII_CMDEND		3	/**< irqstate end define, measurement complete */
#define EM04_TOUCH_SPII_LEVEND		/*6*/3	/**< irqstate end define, measurement complete */
/**@}*/


/*! \defgroup DEBUG Debug defines
*/
/**@{*/
#define DBH				th->init.dbh				/**< debug handle */
#ifndef MENMON
# define DBG_MYLEVEL 	th->init.debugLevel			/**< debug level */
#else
# define DBG_MYLEVEL	MMBIOS_dbgLevel
extern u_int32 MMBIOS_dbgLevel;
#endif
/**@}*/


typedef enum {
	TOUCH_ST_IDLE	=0,
	TOUCH_ST_X1		=1,
	TOUCH_ST_Y		=2,
	TOUCH_ST_X2		=3,
	TOUCH_ST_REC	=4,			/* only used to extend measurement */
	TOUCH_ST_ENPEN	=5,			/* enabling pen irq cmd in process */
	TOUCH_ST_GETID	=6,
	TOUCH_ST_BAT	=7,
	TOUCH_ST_TEMP0	=8,
	TOUCH_ST_TEMP1	=9
} EM04_TOUCH_STATE;

/* async action requests from main routine */
typedef enum {
	TOUCH_ACT_NONE	= 0,
	TOUCH_ACT_GETID	= 1,
	TOUCH_ACT_BAT	=2,
	TOUCH_ACT_TEMP0	=3,
	TOUCH_ACT_TEMP1	=4,
	TOUCH_ACT_CALIBEDGE=5
} EM04_TOUCH_ACTION;

/* states for read values */
typedef enum {
	TOUCH_PPS_IDLE=0,
	TOUCH_PPS_HAVEINITALVALUE=1,
	TOUCH_PSS_PRESSEDNODIFF=2,
	TOUCH_PSS_PRESSEDEVENTSEND=3
} EM04_TOUCH_POSSTATE;

/*! \brief data structure to store a measured position */
typedef struct {
	int16		x;		/**< x position */
	int16		y;		/**< y position */
} EM04_TOUCH_POS;

#ifdef EM04TOUCH_HAMILTON_DO_C3_CORRECTION
typedef struct {
	int16		x1;		/**< x1 correction param */
	int16		x2;		/**< x2 correction param */
	int16		y1;		/**< y1 correction param */
	int16		y2;		/**< y2 correction param */
} EM04_TOUCH_HAM_CORR;
#endif /* EM04TOUCH_HAMILTON_DO_C3_CORRECTION
*/
/*! \brief data structure to store the calibration parameters */
typedef struct {
	int32		xoffset;		/**< x offset of touch panel */
	int32		xdelta;			/**< x delta of touch panel */
	int32		yoffset;		/**< y offset of touch panel */
	int32		ydelta;			/**< y delta of touch panel */
	EM04_TOUCH_POS	current;	/**< current position */
	EM04_TOUCH_POS	pos_ul;		/**< co ordinates of upper left edge */
	EM04_TOUCH_POS	pos_lr;		/**< co ordinates of lower right edge */
	EM04_TOUCH_POS	pos_ur;		/**< co ordinates of upper right edge */
	EM04_TOUCH_POS	pos_ll;		/**< co ordinates of lower left edge */
  #ifdef EM04TOUCH_HAMILTON_DO_C3_CORRECTION
	EM04_TOUCH_HAM_CORR		ham_corr;
  #endif /* EM04TOUCH_HAMILTON_DO_C3_CORRECTION */
} EM04_TOUCH_CAL;

/*! \brief data structure for touch controller */
typedef struct {
	u_int32			hGotSize;	/**< size of allocated handle */
	EM04_TOUCH_STATE state;		/**< main state machine  */
	u_int32			subState;	/**< substate (counts 0..2 in X,Y and X2)  */
	OSS_HANDLE		*osh;		/**< OSS Handle */
	u_int8 			calib;		/**< if 1 indicator for calibration in progress ??? */
	EM04TOUCH_ERRCODE (*getPixFromRaw)(	EM04TOUCH_HANDLE *thp,
										EM04_TOUCH_POS *rawP,
										EM04_TOUCH_POS *calibP ); /**< Ptr to calibration recalc function */
	EM04_TOUCH_CAL	calibdat;	/**< calibration data structure */
	OSS_SEM_HANDLE	*irqSem;	/**< semaphore to signal waiter */

	EM04_TOUCH_POS	currentRaw;	/**< current RAW A/D values (x/y) */
	EM04_TOUCH_POS	prevPos;	/**< last event position of touch reported */
	EM04_TOUCH_POS	prevPos2nd;	/**< 2nd last event position of touch reported */
	EM04_TOUCH_POSSTATE prevPosState;    /**< state of previous position recorded */
	EM04_TOUCH_POSSTATE prevPosState2nd; /**< state of 2nd last position recorded */
	EM04_TOUCH_ACTION pendAction; /**< scheduled action pending  */
	u_int8			actionWaiter; /**< TRUE if somebody waiting for action  */

	u_int8			ierShadow;	/**< Z031_IER register shadow */
	u_int8			maskIrq;	/**< flag for interrupt routine to turn
								     off interrupts (to avoid spurious irqs)*/
	u_int8			recCycle;	/**< number of recovery cycles to perform */
	u_int16 		auxValue;	/**< value measured for getid,bat,tempx  */
	EM04TOUCH_INIT	init;		/**< init handle */
} EM04_TOUCH_HANDLE;


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
EM04TOUCH_ERRCODE EM04TOUCH_ActionPerform(
	EM04_TOUCH_HANDLE *th,
	EM04_TOUCH_ACTION action,
	int32 time);

#ifdef __cplusplus
	}
#endif

#endif /* _EM04TOUCH_INT_H */



