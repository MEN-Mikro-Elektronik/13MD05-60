/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  d6_check.h
 *
 *      \author  sv
 *        $Date: 2007/03/05 13:26:54 $
 *    $Revision: 1.6 $
 *
 *        \brief
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: d6_status.h,v $
 * Revision 1.6  2007/03/05 13:26:54  svogel
 * cosmetics
 *
 * Revision 1.5  2006/06/09 12:20:28  SVogel
 * cosmetics
 *
 * Revision 1.4  2006/06/07 10:19:01  SVogel
 * Changes for distribution.
 *
 * Revision 1.3  2006/03/14 12:32:47  SVogel
 * added pci information to BMC handle
 *
 * Revision 1.2  2006/02/20 08:54:52  SVogel
 * added
 * + additional watchdog functionality
 * + function comments
 *
 * Revision 1.1  2006/02/08 16:48:42  SVogel
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef	_D6_CHECK_H
#define	_D6_CHECK_H

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------+
|   DEFINES                             |
+---------------------------------------*/
#define D6S_VERSION_S					((u_int8 *)"01.05")

/*---------
  |  PCI  |
  ---------*/
/* pci device definition */
#define D6S_PCI_VENDOR_ID               0x1172  /**< men vendor id */
#define D6S_PCI_DEVICE_ID               0x4d45  /**< men FPGA device id */
#define D6S_PCI_MEN_SUB_VENDOR_ID       0x001f  /**< subsystem vendor id */
#define D6S_PCI_ALCATEL_SUB_VENDOR_ID   0x0034  /**< subsystem vendor id */
#define D6S_PCI_SUBSYSTEM_ID            0x5a14  /**< subsystem id */

/* pci configuartion space - address definitions */
#define D6S_PCI_HEADER_TYPE             0x0e    /**< pci header */
#define D6S_PCI_HEADER_TYPE_MASK        0x7f    /**< pci header type mask */
#define D6S_PCI_HEADER_PCI_PCI          0x01    /**< pci to pci header */
#define D6S_PCI_SECONDARY_BUS           0x19    /**< secondary bus */
#define D6S_PCI_PRIMARY_BUS             0x18    /**< primary bus */
#define D6S_PCI_HEADER_MULTI_FUNC       0x80    /**< multi header function */
#define D6S_PCI_DEVICE_VENDOR_ADDR      0x00    /**< device vendor address */
#define D6S_PCI_BAR_0_ADDR              0x10    /**< bar0 */
#define D6S_PCI_BAR_OFFSET              0x04    /**< offset to next bar */
#define D6S_PCI_BAR_1_ADDR              0x14    /**< bar1 */
#define D6S_PCI_BAR_2_ADDR              0x18    /**< bar2 */
#define D6S_PCI_BAR_3_ADDR              0x1C    /**< bar3 */
#define D6S_PCI_BAR_4_ADDR              0x20    /**< bar4 */
#define D6S_PCI_BAR_5_ADDR              0x24    /**< bar5 */
#define D6S_PCI_SUBSYSTEM_ID_ADDR       0x2c    /**< subsystem id */
#define D6S_NO_OF_BARS                  6       /**< number of bars */
/* pci bar to use */
#define D6S_PCI_BAR                     D6S_BAR_0
#define D6S_PCI_BAR_INDEX               0

/*------------------
  | FPGA interface |
  ------------------*/
#define D6S_MEMORY_OFFSET               0x400
#define D6S_MEMORY_SIZE                 256     /**< size of send/receive
                                                     memory */
/*------------------------------------------------------------------------
  |   7   |     6    |     5    |   4    |    3    |  2  |   1   | 0     |
  ------------------------------------------------------------------------
  | start | reserved | reserved |bmcirq | comerr | timeout | ack | ready |
  ------------------------------------------------------------------------*/
#define D6S_READY_FLAG                  0x01    /**< FPGA finished command */
#define D6S_ACK_FLAG                    0x02    /**< acknowledge flag */
#define D6S_TIMEOUT_FLAG                0x04    /**< timeout flag */
#define D6S_COMMERR_FLAG                0x08    /**< communication error flag */
#define D6S_BMCIRQ_FLAG                 0x10    /**< BMC interrupt flag */
#define D6S_START_FLAG                  0x80    /**< start command
                                                     transmission */

/* command byte definition */
#define D6S_WDG_EN                      0x00    /**< enable watchdog */
#define D6S_WDG_TIMEOUT_PRESCALER       0x01    /**< timeout prescaler */
#define D6S_WDG_TIMEOUT_VAL             0x02    /**< timeout value */
#define D6S_WDG_TRIGGER                 0x03    /**< watchdog trigger */
#define D6S_WDG_PATTERN_1               0x55    /**< watchdog trigger
                                                     pattern 1 */
#define D6S_WDG_PATTERN_2               0xAA    /**< watchdog trigger
                                                     pattern 2 */

#define D6S_RST_CAUSE                   0x10    /**< reset cause 1..4 */
#define D6S_RST_CAUSE_SRC               0x11    /**< reset source 1..4 */
#define D6S_RST_CAUSE_VAL               0x12    /**< source value 1..4 */
#define D6S_RST_TIMESTAMP_VAL           0x13    /**< reset timestamp 1..4 */

#define D6S_ADC_SENSE_VAL               0x20    /**< senses 1..3 */
#define D6S_ADC_VOLTAGE_VAL             0x21    /**< voltages 1..14 */
#define D6S_ADC_TEMP_VAL                0x22    /**< temperature 1..4 */
#define D6S_ADC_VID_VAL                 0x23    /**< VID in % */

#define D6S_BRD_STATUS_REG              0x30    /**< status register */
#define D6S_BRD_VERSION_STRING          0x31    /**< firmware version */
#define D6S_BRD_CPCI_ADDR               0x32    /**< C-PCI address */
#define D6S_BRD_PERM_STATUS_REG         0x33    /**< permanent status
                                                     register */
#define D6S_BRD_FIRMWARE_CHECKSUM       0x34    /**< firmware checksum */
#define D6S_BRD_PWR_ON_ERR              0x35    /**< power on error */

/* read/write command definition */
#define D6S_WR_EN                       0x80    /**< write enable */
#define D6S_RD_EN                       0x00    /**< read enable */

#define D6S_NO_RESETS					4       /**< numer of resets */

#define D6S_NO_ADC_SENSES               3       /**< number of adc senses */
#define D6S_NO_ADC_VOLTAGES             14      /**< number of adc voltages*/
#define D6S_NO_ADC_TEMPS                4       /**< number of adc
                                                     temperatures */

#define D6S_BYTES_FW_VERS               4       /**< bytes firmware version */
#define D6S_BYTES_WDG                   1       /**< bytes watchdog */
#define D6S_BYTES_RESET_CAUSE           2       /**< bytes reset cause */
#define D6S_BYTES_RESET_SOURCE          1       /**< bytes reset source */
#define D6S_BYTES_SOURCE_VALUE          2       /**< bytes source value */
#define D6S_BYTES_RESET_TIMESTAMP       4       /**< bytes reset timestamp */
#define D6S_BYTES_SENSE                 2       /**< bytes senses */
#define D6S_BYTES_VOLTAGES              2       /**< bytes voltages */
#define D6S_BYTES_TEMP                  2       /**< bytes temperatures */
#define D6S_BYTES_VID                   1       /**< bytes VID */
#define D6S_BYTES_STATUS_REG            2       /**< bytes status register */
#define D6S_BYTES_FIRMWARE_VERSION      4       /**< bytes firmware version */
#define D6S_BYTES_CPCI_ADDR             1       /**< bytes C-PCI address */
#define D6S_BYTES_PERM_STATUS_REG       2       /**< bytes permanent status
                                                     register */
#define D6S_BYTES_FIRMWARE_CHECKSUM     2       /**< bytes firmware checksum */
#define D6S_BYTES_PWR_ON_ERR            2       /**< bytes power on error */

#define D6S_SENSE_3V_ID                 0		/**< 3V sense id */
#define D6S_SENSE_5V_ID                 1		/**< 5V sense id */
#define D6S_SENSE_12V_ID                2		/**< 12V sense id */

#define D6S_VOLT_L5V_ID                 0		/**< L5V id */
#define D6S_VOLT_L3V_ID                 1		/**< L3V id */
#define D6S_VOLT_3V_ID                  2		/**< 3V id */
#define D6S_VOLT_5V_ID                  3		/**< 5V id */
#define D6S_VOLT_12V_ID                 4		/**< 12V id */
#define D6S_VOLT_2_5V_ID                5		/**< 2.5V id */
#define D6S_VOLT_1_2V_ID                6		/**< 1.2V id */
#define D6S_VOLT_1_5V_ID                7		/**< 1.5V id */
#define D6S_VOLT_1_8V_ID                8		/**< 1.8V id */
#define D6S_VOLT_1_1V_ID                9		/**< 1.1V id */
#define D6S_VOLT_0_9V_ID                10		/**< 0.9V id */
#define D6S_VOLT_VTT_ID                 11		/**< VTT id */
#define D6S_VOLT_CORE_ID                12		/**< CORE id */
#define D6S_VOLT_DDR_ID                 13		/**< DDR id */

#define D6S_ENV_TMP_ID                   0		/**< environment temperature */
#define D6S_HOT_TMP_ID                   1		/**< hot temperature */
#define D6S_DIE_TMP_ID                   2		/**< dei temperature */
#define D6S_CPU_TMP_ID                   3		/**< cpu temperature */

/* BMC status register */
#define D6S_TMP_WARN                    0x0001	/**< temperature warning */
#define D6S_VOLT_WARN                   0x0002	/**< voltage warning */
#define D6S_SENSE_WARN                  0x0004	/**< sense warning */
#define D6S_S3_WARN                     0x0008	/**< state S3 */
#define D6S_IERR_WARN                   0x0010	/**< internal cpu error */
#define D6S_PROC_HOT_WARN               0x0020	/**< processor hot warning */
#define D6S_FERR_PBE                    0x0040	/**< floating point error */
#define D6S_SUS_STAT                    0x0080	/**< suspend status of ich */
#define D6S_CPU_PWR                     0x0100	/**< cpu running */
#define D6S_PSI                         0x0200	/**< state of PSI input */
#define D6S_STPCLK                      0x0400	/**< state of STPCLK input */
#define D6S_XMC1                        0x0800	/**< xmc reset out 0 */
#define D6S_XMC2                        0x1000	/**< xmc reset out 1 */
#define D6S_PROG_MODE                   0x2000  /**< programming mode */
#define D6S_HOT_SWITCH                  0x4000  /**< hot switch */

/* BMC permanent status register */
#define D6S_PWR                         0x0001	/**< power on */
#define D6S_PUSH_BUTTON                 0x0002	/**< push button */
#define D6S_SW                          0x0004	/**< software */
#define D6S_WDG                         0x0008	/**< watchdog */
#define D6S_HOT_SWITCH_PERM             0x0010	/**< hot switch */
#define D6S_FRONT_SIDE                  0x0020	/**< front side button */
#define D6S_PROC_HOT                    0x0040	/**< processor hot */
#define D6S_S4                          0x0080	/**< state S4 */
#define D6S_S5                          0x0100	/**< state S5 */
#define D6S_ADC_OV                      0x0200	/**< adc over voltage */
#define D6S_ADC_UV                      0x0400	/**< adc undet voltage */
#define D6S_ADC_SHORT                   0x0800	/**< adc shortened */
#define D6S_TEMP                        0x1000	/**< temperature */
#define D6S_IERR                        0x2000	/**< internal cpu error */

#define D6S_READ_CMD                    0x00    /**< read command flag */
#define D6S_WRITE_CMD                   0x01    /**< write command flag */
#define D6S_NOT_BUSY_CMD                0x02    /**<  */
#define D6S_BUSY_CMD                    0x03    /**<  */
#define D6S_ERROR_CMD                   0x04    /**< flag */
#define D6S_READY_CMD                   0x05

#define D6S_OK                          OK      /**< 1 result ok definition */
#define D6S_ERROR                       ERROR   /**< 0 result errorness
                                                     definition */

#define D6S_FALSE                       FALSE   /**< 1 false definition */
#define D6S_TRUE                        TRUE    /**< 0 true definition */

#define D6S_STOP                        0xF0    /**< stop cyclic measurement */

#define D6S_WDG_INDEX					2
#define D6S_RST_CAUSE_INDEX				13
#define D6S_RST_SRC_INDEX				21
#define D6S_PWRON_INDEX                 14
#define D6S_MAX_PERM_STAT_REG           14
#define D6S_MAX_STAT_REG                15

#define D6S_TASK_DELAY_MS               1        /**<  taskdelay value in msec */
#define D6S_ACK_TIMEOUT_CNT             10       /**<  timeout for acknowledge in msec */
#define D6S_RPT_CNT                     1000     /**<  repeat counter value */
#define D6S_MEAS_TIMEOUT_CNT            5000     /**<  timeout for initializing
                                                       measurement */

#define D6S_MEN                         0       /**< D6 MEN board */
#define D6S_ALCATEL                     1       /**< D6 ALCATEL board */

/*--------------------------------------+
|   TYPEDEFS                            |
+---------------------------------------*/
/** This structure describes the watchdog functionality of the BMC.
 */
typedef struct {/* D6S_WDG_TS */
    u_int8 enable;                           /**< enable watchdog */
    u_int8 prescaler;                        /**< set prescaler value */
    u_int8 timeout;                          /**< set timeout value */
    u_int8 trigger;                          /**< trigger watchdog */
} D6S_WDG_TS;

/** This structure describes the reset functionality of the BMC.
 */
typedef struct {/* D6S_RST_TS */
    u_int16 cause[D6S_NO_RESETS];           /**< reset cause */
    u_int8  source[D6S_NO_RESETS];          /**< reset source */
    int16   value[D6S_NO_RESETS];           /**< reset value */
    u_int32 timestamp[D6S_NO_RESETS];       /**< reset timestamp */
} D6S_RST_TS;

/** This structure describes the adc functionality of the BMC.
 */
typedef struct {/* D6S_ADC_TS */
    u_int16 senses[D6S_NO_ADC_SENSES];      /**< sense buffer */
    u_int16 voltages[D6S_NO_ADC_VOLTAGES];  /**< voltage buffer */
    int16   temps[D6S_NO_ADC_TEMPS];        /**< temperature buffer */
    u_int16 vid;                            /**< VID voltage */
} D6S_ADC_TS;

/** This structure describes the board information functionality of the BMC.
 */
typedef struct {/* D6S_BRD_TS */
    u_int16 statusReg;                      /**< BMC status register */
    u_int8  fwVersion[D6S_BYTES_FW_VERS];   /**< firmware version */
    u_int8  cpciAddr;                        /**< C-PCI address */
    u_int16 permStatusReg;                   /**< permanent status register */
    u_int16 fwChecksum;                      /**< firmware checksum */
    u_int16 pwrOnErr;                        /**< power on errror */
} D6S_BRD_TS;

/** This structure describes the command control.
 */
typedef struct { /* D6S_CMD_TS */
    u_int8   buffer[D6S_MEMORY_SIZE];       /**< send/receive buffer */
    u_int16  length;                        /**< bytes to transfer */
    u_int8   rwCmd;                         /**< read = READ_CMD,
                                                 write = WRITE_CMD */
    u_int8   bufferStatus;
} D6S_CMD_TS;


/** This structure describes the FPGA PCI resources.
 */
typedef struct { /* D6S_PCI_TS */
    u_int8  bus;                            /**< PCI bus */
    u_int8  device;                         /**< PCI device */
    u_int8  function;                       /**< PCI function */
    u_int16 deviceId;                       /**< PCI device id */
    u_int16 vendorId;                       /**< PCI vendor id */
    u_int8  revision;                       /**< PCI revision */
    u_int32 bar[D6S_NO_OF_BARS];            /**< PCI bar buffer */
    u_int16 subId;                          /**< PCI subsystem id */
    u_int16 subVendorId;                    /**< PCI subsystem vendor id */
} D6S_PCI_TS;

/** This structure describes the BMC functionality.
 */
typedef struct { /* D6S_BMC_TS */
    volatile u_int8 *memAddrP;           /**< memory bar address of
                                              FPGA interface */
    volatile u_int8 *statCtrlRegP;       /**< status control register */

    u_int8      bmcStatus;              /**< should not be read by the user,
                                             take statusReg in D6S_BRD_TS */

    D6S_WDG_TS  wdg;                      /**< watchdog structure */
    D6S_RST_TS  rst;                      /**< reset structure */
    D6S_ADC_TS  adc;                      /**< adc structure */
    D6S_BRD_TS  brd;                      /**< board information structure */
    D6S_CMD_TS  cmd;                      /**< to command structure */
    D6S_PCI_TS  pciDev;                   /**< pci sturcture */
    int32		cmdTaskId;                /**< command process
                                               task id */
	int32		measTaskId;               /**< measurement task id */
    int32       wdgTaskId;                /**< watchdog trigger task */

	u_int8      measRdy;                  /**< measurement ready */
	u_int8      stopCylicMeas;            /**< stop cyclic measurement */
} D6S_BMC_TS;

typedef void *  BMC_HANDLE;      /**< BMC handle */

/** This enumeration describes the reset reasons.
 */
enum D6S_RST {
    erst_start = 0,
    erst_cause,
    erst_source,
    erst_value,
    erst_timestamp,
    erst_end = (erst_timestamp+1)
}; /* D6S_RST */

/** This enumeration describes the adc functionality.
 */
enum D6S_ADC {
    eadc_start = 0,                         /**< enumeration start */
    eadc_senses,
    eadc_voltages,
    eadc_temps,
    eadc_vid,
    eadc_end = (eadc_vid+1)                /**< enumeration end */
}; /* D6S_ADC */

/** This enumeration describes the watchdog functionality.
 */
enum D6S_EWDG {
    ewdg_start = 0,                         /**< enumeration start */
    ewdg_enable,                            /**< read watchdog enable */
    ewdg_prescaler,                         /**< read watchdog prescaler */
    ewdg_timeout,                           /**< read watchdog timeout */
    ewdg_trigger_rd,                        /**< read watchdog trigger */
    ewdg_trigger_wr,                        /**< write watchdog trigger */
    ewdg_setPrescaler,                      /**< set watchdog prescaler value */
    ewdg_setTimeout,                        /**< set watchdog timeout value */
    ewdg_end = (ewdg_setTimeout+1)          /**< enumeration end */
}; /* D6S_EWDG */

/** This enumeration describes the board information.
 */
enum D6S_BRD {
	ebrd_start = 0,
    ebrd_statusReg,                      /**< BMC status register */
    ebrd_fwVersion,                       /**< firmware version */
    ebrd_cpciAddr,                        /**< C-PCI address */
    ebrd_permStatusReg,                   /**< permanent status register */
    ebrd_fwChecksum,                      /**< firmware checksum */
    ebrd_pwrOnErr,
    ebrd_end = (ebrd_pwrOnErr+1)
}; /* D6S_BRD */

typedef struct {/* D6S_RST_STRING */
    u_int8  *stringP;
    u_int16 value;
}D6S_RST_STRING;

typedef struct {/* D6S_STAT_STRING */
    u_int8  *stringP;
    u_int16 value;
}D6S_STAT_STRING;

typedef struct {/* D6S_PWRON_STRING */
    u_int8 *string1P;
    u_int8 *string2P;
    u_int16 value;
}D6S_PWRON_STRING;

/*--------------------------------------+
|   CONSTANTS                            |
+---------------------------------------*/

const u_int8 *wdgStatusStr[D6S_WDG_INDEX] = {
								  "DISABLED",		/**< watchdog disabled */
				     		      "ENABLED" };		/**< watchdog enabled */

const D6S_RST_STRING rstCauseStr[D6S_RST_CAUSE_INDEX+1] = {
                                  { "Undefined",                0x0000 },
								  { "Power On",                 0x0001 },
								  { "Push Button",              0x0002 },   
								  { "SW Reset",                 0x0004 },   
								  { "Watchdog",                 0x0008 },
								  { "Hot Switch",               0x0010 },
								  { "Front Side Push Button",   0x0020 },
								  { "PROCHOT Reset",            0x0040 },
								  { "Sleep S4",                 0x0080 },
								  { "Sleep S5",                 0x0100 },
								  { "ADC Over Voltage",         0x00200 },
								  { "ADC Under Voltage",        0x00400 },
								  { "ADC Shortened",            0x00800 },
								  { "Temperature Reset",        0x01000 } };

const u_int8 *rstSourceStr[D6S_RST_SRC_INDEX+1] = {
                                  "",
								  "SENSE_3V",
								  "SENSE_5V",
								  "SENSE_12V",
								  "L3V_AN",
								  "L5V_AN",
								  "V3_AN",
								  "V5_AN",
								  "V12_AN",
								  "ENV_TEMP",
								  "P_2V5",
								  "P_1V5",
								  "SENSE_3V",
								  "SENSE_5V",
								  "P_1V1",
								  "VCC_0V9_I",
								  "VCC_VTT_I",
								  "VCC_CORE_I",
								  "DDR_1V8_I",
								  "HOT_TEMP",
								  "P_1V2",
								  "P_1V8"};

const D6S_PWRON_STRING  pwrOnStr[D6S_PWRON_INDEX] = {
        { "OK",             NULL,           0x0000 },
        { "MAIN_FAILED",    NULL,           0x0100 },
        { "NORM_FAILED",    NULL,           0x0200 },

        { "SET_FAILED",     NULL,           0x0300 },
        { "SET_FAILED",     "ETH_PWGD",     0x0304 },
        { "SET_FAILED",     "PS_DDR_PG",    0x0305 },
        { "SET_FAILED",     "PS_FPGA_PG",   0x0306 },
        { "SET_FAILED",     "PS_VTT_PG",    0x0308 },
        { "SET_FAILED",     "PS_CPU_PG",    0x0310 },
        { "SET_FAILED",     "FPGA_RESET",   0x0312 },
        { "SET_FAILED",     "SUS_STAT",     0x0317 },
        { "SET_FAILED",     "SLP_S3",       0x0318 },

        { "SENSE_FAILED",   NULL,           0x0400 },
        { "MOSFET_ERROR",   NULL,           0x0500 } };

const D6S_STAT_STRING statusRegStr[D6S_MAX_STAT_REG] = {
								  { "Temp Warning",     0x0001 },
								  { "Voltage Warning",  0x0002 },
								  { "Sense Warning",    0x0004 },
								  { "SLP_S3",           0x0008 },
								  { "IERR",             0x0010 },
								  { "PROC HOT",         0x0020 },
								  { "FERR PBE",         0x0040 },
								  { "SUS STAT",         0x0080 },
								  { "Main CPU Power",   0x0100 },
								  { "PSI",              0x0200 },
								  { "STPCLK",           0x0400 },
								  { "XMC1",             0x0800 },
								  { "XMC2",             0x1000 },
								  { "ProgMode",         0x2000 },
								  { "HotSwitch",        0x4000 } };

const D6S_STAT_STRING permStatusRegStr[D6S_MAX_PERM_STAT_REG] = {
								  { "Power On",                 0x0001 },
								  { "PushButton CPCI",          0x0002 },
								  { "Software",                 0x0004 },
								  { "Watchdog",                 0x0008 },
								  { "Hot Switch",               0x0010 },
								  { "Front Side Pushbutton",    0x0020 },
								  { "Proc Hot",                 0x0040 },
								  { "Sleep S4",                 0x0080 },
								  { "Sleep S5",                 0x0100 },
								  { "ADC_OV",                   0x0200 },
								  { "ADC_UV",                   0x0400 },
								  { "ADC_SHORT",                0x0800 },
								  { "Temperature",              0x1000 },
								  { "IERR",                     0x2000 } };
/*--------------------------------------+
|   PROTOTYPES                          |
+---------------------------------------*/
extern BMC_HANDLE   D6S_Init(u_int8 manufacturer);
extern u_int8       D6S_DeInit(BMC_HANDLE **bmcHdlP);
extern u_int8       D6S_Watchdog(BMC_HANDLE *bmcHdlP, enum D6S_EWDG watchdog,
                    u_int8 value);
extern u_int8       D6S_Adc(BMC_HANDLE *bmcHdlP, enum D6S_ADC adcType);
extern u_int8       D6S_ResetInfo(BMC_HANDLE *bmcHdlP, enum D6S_RST reset);
extern u_int8       D6S_BoardInfo(BMC_HANDLE *bmcHdlP);
extern u_int8       D6S_BmcStatusRegister(BMC_HANDLE *bmcHdlP);
extern int16        D6S_StartMeasurement(BMC_HANDLE *bmcHdlP);
extern int32        D6S_StopMeasurement(BMC_HANDLE *bmcHdlP);
extern u_int8		D6S_ShowBoardInfo(BMC_HANDLE *bmcHdlP);
extern u_int8		D6S_ShowStatRegs(BMC_HANDLE *bmcHdlP);
extern u_int8		D6S_ShowWatchdog(BMC_HANDLE *bmcHdlP);
extern u_int8		D6S_ShowRstHistory(BMC_HANDLE *bmcHdlP);
extern u_int8		D6S_ShowVoltages(BMC_HANDLE *bmcHdlP);
extern u_int8		D6S_ShowTemperatures(BMC_HANDLE *bmcHdlP);
extern u_int8		D6S_ShowCurrents(BMC_HANDLE *bmcHdlP);
extern u_int8       D6S_TriggerWatchdog(BMC_HANDLE *bmcHdlP);

/* module main function */
extern u_int8 		d6Status(u_int8 options, u_int8 manufacturer);
extern u_int8 		d6GetBmcHandle(BMC_HANDLE **bmcHdlP);

#ifdef __cplusplus
}
#endif

#endif	/* _D6_CHECK_H */
