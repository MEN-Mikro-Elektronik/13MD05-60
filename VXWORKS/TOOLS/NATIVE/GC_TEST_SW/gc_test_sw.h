/***********************  I n c l u d e  -  F i l e  ***********************/
/*!
 *        \file  gc_test_sw.h
 *
 *      \author  aw
 *        $Date: 2009/03/06 17:07:34 $
 *    $Revision: 1.3 $
 *
 *       \brief  Header file for testsoftware of critical software
 *
 *    \switches
 *
 *
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: gc_test_sw.h,v $
 * Revision 1.3  2009/03/06 17:07:34  AWanka
 * R: For the qualification test a CAN test and Com 2 test was needed.
 * M: Added GC_CanTestA_07320 and GC_Com2Test
 *
 * Revision 1.2  2009/02/10 17:04:52  AWanka
 * R: The initialization of the Z17 sets the direction register to 0
 * M: Set the direction register back to the old value after initialization
 *
 * Revision 1.1  2009/01/22 09:32:43  AWanka
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef _GC_TEST_SW_H
#define _GC_TEST_SW_H

#ifdef __cplusplus
      extern "C" {
#endif

/*-----------------------------------------+
|  INCLUDES                                |
+-----------------------------------------*/


/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
                                
#define GC_MODE_SEL_0_BIT       0      
#define GC_MODE_SEL_1_BIT       1      
#define GC_ID_SEL_0_BIT         2      
#define GC_ID_SEL_1_BIT         3      
#define GC_CAN_EN_BIT           4
#define GC_LEV_SHIFT_BIT        5
#define GC_CUT_OFF_OUT_0_BIT    6
#define GC_CUT_OFF_OUT_1_BIT    7

#define GC_MODE_SEL_0           ( 1 << GC_MODE_SEL_0_BIT    )
#define GC_MODE_SEL_1           ( 1 << GC_MODE_SEL_1_BIT    )
#define GC_ID_SEL_0             ( 1 << GC_ID_SEL_0_BIT      )
#define GC_ID_SEL_1             ( 1 << GC_ID_SEL_1_BIT      )
#define GC_CAN_EN               ( 1 << GC_CAN_EN_BIT        )
#define GC_LEV_SHIFT            ( 1 << GC_LEV_SHIFT_BIT     )
#define GC_CUT_OFF_OUT_0        ( 1 << GC_CUT_OFF_OUT_0_BIT )
#define GC_CUT_OFF_OUT_1        ( 1 << GC_CUT_OFF_OUT_1_BIT )

#define GC_GPIO_OUTPUTS			( GC_CAN_EN | GC_LEV_SHIFT | GC_CUT_OFF_OUT_0 | \
                                  GC_CUT_OFF_OUT_1 )

#define GC_CAN_SPEED_MIN_KBAUD      50
#define GC_CAN_SPEED_MAX_KBAUD     500
#define GC_CAN_SPEED_250_KBAUD	   250		

#define AD78_TEST                   1
#define EEPROM_TEST                 0

#define SMB_ADDRESS_AD78            0x12
#define SMB_ADDRESS_EEPROM          0xAC

#define I2C_INITIALIZED             1

#define AD78_WDOG_TOUT_REG          0x07
#define AD78_TEMP_REG               0x12
#define AD78_TEST_VALUE_1           0xAA
#define AD78_TEST_VALUE_2           0x55

#define DELAY_TIME_SMB_WRITE    (sysClkRateGet()/20)

#define EEPROM_SIZE             512

#define GC_NUM_NET_IF           3

#define GC_ETH_DRV_NAME				"mottsec"

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/


/*-----------------------------------------+
|  GLOBALS                                 |
+-----------------------------------------*/


/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
int GC_Loopframes(int, int, int, int, int (*)(void), int );
IMPORT int fpga_load(char *);             
IMPORT int mscan_loopb(char *);
IMPORT int mscan_alyzer(char *);
u_int32 GC_CanLoopframes( u_int8, u_int32, int, int );
IMPORT int mtest(char *);
IMPORT int hdtest(char *);                  




#ifdef __cplusplus
      }
#endif

#endif /* _GC_TEST_SW_H */



