/****************************************************************************
 *
 *       Author: aw
 *        $Date: 2009/07/27 11:12:08 $
 *    $Revision: 1.5 $
 *
 *  Description: Testsoftware of project critical software
 *
 *     Required: 
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: gc_test_sw.c,v $
 * Revision 1.5  2009/07/27 11:12:08  MKolpak
 * R: 1. PCI test w/ F206N not working
 *    2. no USB test
 * M: 1. Added init. for second Cham. Dev.
 *    2. USB test implemented
 *
 * Revision 1.4  2009/03/09 11:02:37  AWanka
 * cosmetics
 *
 * Revision 1.3  2009/03/06 17:06:54  AWanka
 * R: For the qualification test a CAN test and Com 2 test was needed.
 * M: Added GC_CanTestA_07320 and GC_Com2Test
 *
 * Revision 1.2  2009/02/10 17:00:50  AWanka
 * R: The initialization of the Z17 sets the direction register to 0
 * M: Set the direction register back to the old value after initialization
 *
 * Revision 1.1  2009/01/22 09:31:57  AWanka
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

/*-----------------------------------------+
|  INCLUDES                                |
+-----------------------------------------*/
#include "string.h"
#include "intLib.h"
#include "sysLib.h"
#include "usrFsLib.h"
#include "usrConfig.h"
#include "stdio.h"
#include "ioLib.h"
#include "drv/hdisk/ataDrv.h"
#include <sys/times.h>
#include <random.h>
#include <nfsDriver.h>
#include <MEN/men_typs.h>
#include <MEN/oss.h>
#include <MEN/desc.h>
#include <MEN/os2m.h>
#include <MEN/mk.h>
#include <MEN/em09_cfg.h>
#include <MEN/mdis_api.h>
#include <MEN/mscan_api.h>
#include <MEN/chameleon.h>
#include <MEN/cham_z069.h>
#include <MEN/sys_smb.h>
#include <MEN/z17_drv.h>
#include "sysLib_men.h"
#include "gc_test_sw.h"


/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
#define Z001_SMB_BUS_NUMBER         2
#define SecCHAM_FPGA_PCI_BUS        7

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/


/*-----------------------------------------+
|  GLOBALS                                 |
+-----------------------------------------*/
u_int8 G_eeTestStart = 0;
u_int8 G_SecChamInit = 0;
LOCAL CHAMELEONV2_HANDLE *G_SecChamHdl = NULL; /* chameleon for test device */
LOCAL CHAM_FUNCTBL		 G_SecChamFktTbl;	  /* CHAM V2 function table */
int32   G_ChamErr = 0;


/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/

IMPORT void microtime (struct timeval * pTv);



void gc_test_sw(void)
{
}

/**********************************************************************/
/** Find and initialize a chameleon device on SecCHAM_FPGA_PCI_BUS
 *  
 *  \param  -
 *
 *  \return OK or ERROR
 */
STATUS InitSecChameleon( int debugLevel )
{

	u_int32 chamPciBus = (SecCHAM_FPGA_PCI_BUS);
	u_int32 chamPciDev = 0;
	u_int32 chamPciFun = 0;

    if(G_SecChamInit)
        return OK;

    if( (G_ChamErr = CHAM_InitMem( &G_SecChamFktTbl ))){
        if(debugLevel != 2) 
		    printf("*** InitSecChameleon: CHAM_InitMem fail\n");
		return ERROR;
	}
    
    /* we only search throuch device 0-63 */
    for(chamPciDev = 0; chamPciDev < 64; chamPciDev++)
    {
        if( (G_ChamErr = G_SecChamFktTbl.InitPci( 
                                            NULL,
                                            chamPciBus,
                                            chamPciDev,
                                            chamPciFun,
                                            &G_SecChamHdl ))
            == CHAMELEON_OK )
        {
            break;
        }
    }
    /* we couldn't find a cham dev */
    if(G_ChamErr)
    {   
        if(debugLevel != 2) 
        {
            printf(" InitSecChameleon: No cham. dev. found on bus# %u (0x%X)\n", 
                chamPciBus,
                G_ChamErr);
        }
        return ERROR;
    }
    
    G_SecChamInit = 1; /* Init done */
    return OK;
}




/**********************************************************************/
/** Routine to execute the loopback test
 *
 *  This routine performs an internal loopback test on the CAN interfaces.
 *
 *  \param port         \IN 1: CAN S1
 *                          2: CAN A
 *                          3: CAN J
 *                          4: CAN Display
 *                          5: CAN B
 *                          6: CAN S2
 *  \param speed        \IN 50: 50kBaud, 500: 500kBaud
 *
 *  \return OK or ERROR
 */
int GC_CanLoopbackTest(int port, int speed)
{
    int bitrateCode;
    extern DESC_SPEC CAN_1, CAN_2, CAN_3, CAN_4, CAN_5, CAN_6, F503;
    char params[12];
    
    if(speed == GC_CAN_SPEED_MIN_KBAUD){
        bitrateCode = MSCAN_BR_50K;
    }
    else if(speed == GC_CAN_SPEED_MAX_KBAUD){
        bitrateCode = MSCAN_BR_500K;
    }  
    else{
        printf("Error: wrong baud rate\n");
        return ERROR;
    }
             
    OSS_PciAddrTranslationInit( EM09CFG_PCI_IO_START, 0x00000000 );
    OSS_SetIrqNum0( 0 );
    
    MK_SetIntConnectRtn( intConnect );
    MK_SetIntEnableRtn( intEnable );
    
    /*---- Startup & Create Devices ----*/
    if( OS2M_DrvInstall() != OK )
        return ERROR;
        
    sprintf(params, "-b=%d %s%d", bitrateCode, "/can/", port );
    
    switch(port){
        case 1:  
            OS2M_DevCreate( "/can/1", &CAN_1, &F503 );
            break;
        case 2:
            OS2M_DevCreate( "/can/2", &CAN_2, &F503 );
            break;
        case 3:
            OS2M_DevCreate( "/can/3", &CAN_3, &F503 );
            break;
        case 4:
            OS2M_DevCreate( "/can/4", &CAN_4, &F503 );
            break;
        case 5:
            OS2M_DevCreate( "/can/5", &CAN_5, &F503 );
            break;
        case 6:
            OS2M_DevCreate( "/can/6", &CAN_6, &F503 );
            break;
        default:
            printf("Error: wrong Port number\n");
            return ERROR;
            break;
    }

    /* example: mscan_loopb "-b=50 /can/1" */
    mscan_loopb(params);
    return OK;    
}

/**********************************************************************/
/** Routine to execute the can test
 *
 *  This routine performs a test on the CAN interfaces. The test board
 *  A_07320 has to be connected to the CAN interface under test. One CAN sends 
 *  4 frames and the other 5 CAN ports receives the frames.
 *
 *  \param port         \IN number of the CAN port, which sends the frames
 *							1: CAN S1
 *                          2: CAN A
 *                          3: CAN J
 *                          4: CAN Display
 *                          5: CAN B
 *                          6: CAN S2
 *  \param speed        \IN 50: 50kBaud, 500: 500kBaud
 *  \param runs         \IN number of passes
 *  \param debugLevel   \IN 0: print error messages
 *                          1: no outputs
 *
 *  \return  all  0: OK
 *           Bit  0: send failed
 *           Bit  8: receive CAN S1 failed
 *           Bit  9: receive CAN A  failed             
 *           Bit 10: receive CAN J  failed
 *           Bit 11: receive CAN Display failed
 *           Bit 12: receive CAN B failed
 *           Bit 13: receive CAN S2 failed
 *           Bit 16: MSCAN init failed
 *
 */
u_int32 GC_CanTestA_07320(int port, int speed, int runs, int debugLevel)
{
    int bitrateCode;
    extern DESC_SPEC CAN_1, CAN_2, CAN_3, CAN_4, CAN_5, CAN_6, F503;
    
    if(speed == GC_CAN_SPEED_MIN_KBAUD){
        bitrateCode = 6;
    }
    else if(speed == GC_CAN_SPEED_MAX_KBAUD){
        bitrateCode = MSCAN_BR_500K;
    }  
    else{
        printf("Error: wrong baud rate\n");
        return ERROR;
    }    
        
    OSS_PciAddrTranslationInit( EM09CFG_PCI_IO_START, 0x00000000 );
    OSS_SetIrqNum0( 0 );
    
    
    
    MK_SetIntConnectRtn( intConnect );
    MK_SetIntEnableRtn( intEnable );
    
    /*---- Startup & Create Devices ----*/
    if( OS2M_DrvInstall() != OK )
        return ERROR;
        
    OS2M_DevCreate( "/can/1", &CAN_1, &F503 );
    OS2M_DevCreate( "/can/2", &CAN_2, &F503 );
    OS2M_DevCreate( "/can/3", &CAN_3, &F503 );
    OS2M_DevCreate( "/can/4", &CAN_4, &F503 );
    OS2M_DevCreate( "/can/5", &CAN_5, &F503 );
    OS2M_DevCreate( "/can/6", &CAN_6, &F503 );
    
    return GC_CanLoopframes(port, bitrateCode, runs, debugLevel);    
}

/**********************************************************************/
/** Routine to execute the CANalyzer test
 *
 *  This routine performs a test on the CAN interfaces. The CANalyzer has to
 *  be connected to the CAN interface under test.
 *
 *  \param port         \IN 1: CAN S1
 *                          2: CAN A
 *                          3: CAN J
 *                          4: CAN Display
 *                          5: CAN B
 *                          6: CAN S2
 *  \param speed        \IN 50: 50kBaud, 500: 500kBaud
 *
 *  \return OK or ERROR
 */
int GC_CanTest(int port, int speed)
{
    int bitrateCode;
    extern DESC_SPEC CAN_1, CAN_2, CAN_3, CAN_4, CAN_5, CAN_6, F503;
    char params[12];
    
    if(speed == GC_CAN_SPEED_MIN_KBAUD){
        bitrateCode = 6;
    }
    else if(speed == GC_CAN_SPEED_MAX_KBAUD){
        bitrateCode = MSCAN_BR_500K;
    }  
	else if(speed == GC_CAN_SPEED_250_KBAUD){
        bitrateCode = MSCAN_BR_250K;
    }
    else{
        printf("Error: wrong baud rate\n");
        return ERROR;
    }    
        
    OSS_PciAddrTranslationInit( EM09CFG_PCI_IO_START, 0x00000000 );
    OSS_SetIrqNum0( 0 );
    
    
    
    MK_SetIntConnectRtn( intConnect );
    MK_SetIntEnableRtn( intEnable );
    
    /*---- Startup & Create Devices ----*/
    if( OS2M_DrvInstall() != OK )
        return ERROR;
        
    sprintf(params, "-b=%d %s%d", bitrateCode, "/can/", port );
        
    switch(port){
        case 1:  
            OS2M_DevCreate( "/can/1", &CAN_1, &F503 );
            break;
        case 2:
            OS2M_DevCreate( "/can/2", &CAN_2, &F503 );
            break;
        case 3:
            OS2M_DevCreate( "/can/3", &CAN_3, &F503 );
            break;
        case 4:
            OS2M_DevCreate( "/can/4", &CAN_4, &F503 );
            break;
        case 5:
            OS2M_DevCreate( "/can/5", &CAN_5, &F503 );
            break;
        case 6:
            OS2M_DevCreate( "/can/6", &CAN_6, &F503 );
            break;
    }
    
    /* example: mscan_alyzer "-b=50 /can/1" */
    mscan_alyzer(params);
    
    return OK;    
}

/**********************************************************************/
/** Routine to test cut-off outputs
 *
 *  This routine controls the cut-off outputs RS422 transceivers that are used
 *  to shut down another GC.
 *
 *  \param port         \IN 1: Cut-Off Output +1
 *                          2: Cut-Off Output -1
 *                          3: Both Cut-Off Outputs
 *  \param state        \IN 0: output off --> "low", 1: output on --> "high"
 *
 *  \return OK or ERROR
 */
int GC_CutOffOut( int port, int state )
{
    int32 gpioReg;
	int32 directionReg = GC_GPIO_OUTPUTS;
    int path;
    extern DESC_SPEC Z17_SW_1, F503;
    
    if(port > 3 || port < 0)
        return ERROR;
    
    if(state > 1 || state < 0)
        return ERROR;  
        
    OSS_PciAddrTranslationInit( EM09CFG_PCI_IO_START, 0x00000000 );
    OSS_SetIrqNum0( 0 );
    
    MK_SetIntConnectRtn( intConnect );
    MK_SetIntEnableRtn( intEnable );
    
    /*---- Startup & Create Devices ----*/
    if( OS2M_DrvInstall() != OK )
        return ERROR;
        
    OS2M_DevCreate( "/z17/0", &Z17_SW_1, &F503 );
    
    if ((path = M_open("/z17/0")) < 0) {
		return ERROR;
	}
	
	if(M_setstat( path, Z17_DIRECTION, directionReg) != OK){
		return ERROR;
	}

    if(M_read( path, &gpioReg) != OK)
        return ERROR;
    
    if(state == 0){
        if( port & 1 )
            gpioReg = gpioReg & ~GC_CUT_OFF_OUT_0;
        if( port & 2 )
            gpioReg = gpioReg & ~GC_CUT_OFF_OUT_1;
    }
    else {
        if( port & 1 )
            gpioReg = gpioReg | GC_CUT_OFF_OUT_0;
        if( port & 2 )
            gpioReg = gpioReg | GC_CUT_OFF_OUT_1;
    }

	gpioReg &= directionReg;

	if(M_write( path, gpioReg ) != OK){  
		return ERROR;
	}

	if (M_close(path) < 0){
		return ERROR;
	}
    
    return OK;
}

/**********************************************************************/
/** Routine to get the status of CAN power supply
 *
 *  This routine reads the status of the CAN power supply input voltage 
 *  monitoring circuit.
 *
 *  \return     \OUT  ERROR or 
 *                    0: CAN external power supply off or
 *                    1: CAN external power supply on
 */
int GC_CanPower( void )
{
    int32 gpioReg;
	int32 directionReg = GC_GPIO_OUTPUTS;
    int path;
    extern DESC_SPEC Z17_SW_1, F503;
        
    OSS_PciAddrTranslationInit( EM09CFG_PCI_IO_START, 0x00000000 );
    OSS_SetIrqNum0( 0 );
    
    MK_SetIntConnectRtn( intConnect );
    MK_SetIntEnableRtn( intEnable );
    
    /*---- Startup & Create Devices ----*/
    if( OS2M_DrvInstall() != OK )
        return ERROR;
        
    OS2M_DevCreate( "/z17/0", &Z17_SW_1, &F503 );
    
    if ((path = M_open("/z17/0")) < 0) {
		printf("M_open failed\n");
		return ERROR;
	}
	if(M_setstat( path, Z17_DIRECTION, directionReg) != OK){
		printf("ERROR\n");
        return ERROR;
	}

    if(M_read( path, &gpioReg) != OK)
        return ERROR;	

	if (M_close(path) < 0){
		printf("M_close failed\n");
		return ERROR;
	}
    
    return (gpioReg & GC_CAN_EN) >> GC_CAN_EN_BIT;
}

/**********************************************************************/
/** Routine to get the status of the ID selection pins
 *
 *  This routine reads the singular address of the GC within the GC system.
 *
 *  \return     \OUT  ERROR or 
 *                    0: ID0 and ID1 pin connected to GND or
 *                    1: ID0 pin connected to VCC, ID1 to GND
 *                    2: ID0 pin connected to GND, ID1 to VCC
 *                    3: ID0 and ID1 pin conntected to VCC
 */
int GC_IdSel( void )
{
    int32 gpioReg;
    int path;
	int32 directionReg = GC_GPIO_OUTPUTS;
    extern DESC_SPEC Z17_SW_1, F503;
        
    OSS_PciAddrTranslationInit( EM09CFG_PCI_IO_START, 0x00000000 );
    OSS_SetIrqNum0( 0 );
    
     
    MK_SetIntConnectRtn( intConnect );
    MK_SetIntEnableRtn( intEnable );
    
    /*---- Startup & Create Devices ----*/
    if( OS2M_DrvInstall() != OK )
        return ERROR;
        
    OS2M_DevCreate( "/z17/0", &Z17_SW_1, &F503 );
    
    if ((path = M_open("/z17/0")) == ERROR) {
		return ERROR;
	}
	if(M_setstat( path, Z17_DIRECTION, directionReg) != OK){
		return ERROR;
	}

    if(M_read( path, &gpioReg) != OK)
        return ERROR;	

	if (M_close(path) < 0){
		return ERROR;
	}
    
    return ((gpioReg & (GC_ID_SEL_0|GC_ID_SEL_1)) >> GC_ID_SEL_0_BIT);    
}

/**********************************************************************/
/** Routine to get the status of the mode selection pins
 *
 *  This routine reads the maintenance mode of the GC.
 *
 *  \return     \OUT  ERROR or 
 *                    0: ModeSel0 and ModeSel1 pin connected to GND or
 *                    1: ModeSel0 pin connected to VCC, ModeSel1 to GND
 *                    2: ModeSel0 pin connected to GND, ModeSel1 to VCC
 *                    3: ModeSel0 and ModeSel1 pin conntected to VCC
 */
int GC_ModeSel( void )
{
    int32 gpioReg;
    int path;
	int32 directionReg = GC_GPIO_OUTPUTS;
    extern DESC_SPEC Z17_SW_1, F503;
        
    OSS_PciAddrTranslationInit( EM09CFG_PCI_IO_START, 0x00000000 );
    OSS_SetIrqNum0( 0 );
    
    
    MK_SetIntConnectRtn( intConnect );
    MK_SetIntEnableRtn( intEnable );
    
    /*---- Startup & Create Devices ----*/
    if( OS2M_DrvInstall() != OK )
        return ERROR;
        
    OS2M_DevCreate( "/z17/0", &Z17_SW_1, &F503 );
    
    if ((path = M_open("/z17/0")) < 0) {
		return ERROR;
	}
	if(M_setstat( path, Z17_DIRECTION, directionReg) != OK){
		return ERROR;
	}

    if(M_read( path, &gpioReg) != OK)
        return ERROR;	

	if (M_close(path) < 0){
		return ERROR;
	}
    
    return ((gpioReg & (GC_MODE_SEL_0 | GC_MODE_SEL_1)) >> GC_MODE_SEL_0_BIT);
}

/**********************************************************************/
/** Routine to perform a I2C read operation on the bus connected to Z001
 *
 *  \param address      \IN address of I2C device
 *  \param offset       \IN offset/register at I2C device
 *  \param debugLevel   \IN 0: outputs enabled
 *                          1: no outputs
 *
 *  \return     \OUT  ERROR or 
 *                    read value
 */
int GC_SmbRead( int address, int offset, int debugLevel )
{
    u_int8 data;
    int32 retVal = 0;
    
    if( (retVal = sysSmbWriteByte( Z001_SMB_BUS_NUMBER, address, offset )) 
        != 0 ){
		if(debugLevel == 0){
        	printf("Smb write error!(%d)\n",retVal);
		}
        return retVal;
    }
    
    if( (retVal = sysSmbReadByte( Z001_SMB_BUS_NUMBER, address, &data )) 
        != 0 ){
		if(debugLevel == 0){
        	printf("Smb read error!(%d)\n",retVal);
		}
        return retVal;
    }
    
    return data;    
}

/**********************************************************************/
/** Routine to perform a I2C write operation on the bus connected to Z001
 *
 *  \param address      \IN address of I2C device
 *  \param offset       \IN offset/register at I2C device
 *  \param data         \IN data to be written in the device register
 *  \param debugLevel   \IN 0: outputs enabled
 *                          1: no outputs
 *
 *  \return     \OUT  ERROR or OK
 */
int GC_SmbWrite( int address, int offset, int data, int debugLevel )
{
    int32 retVal = 0;
    
    retVal = sysSmbWriteTwoByte( Z001_SMB_BUS_NUMBER, address, offset, data);
    if( retVal != 0){
		if(debugLevel == 0){
        	printf("Smb write error! (%d)\n",retVal);
		}
        return retVal;
    }

    return OK;    
}

/**********************************************************************/
/** Routine to write a value and verify it over SMB to AD78.
 *
 *  \param value      \IN value to be written and verify
 *  \param debugLevel \IN 0: outputs enabled
 *                        1: no outputs
 *
 *  \return     \OUT  ERROR or OK
 */
static int AD78WriteVerify(int value, int debugLevel)
{
    int retValue;
    
    if( (retValue = GC_SmbWrite( SMB_ADDRESS_AD78, 
                                 AD78_WDOG_TOUT_REG, 
                                 value, debugLevel )
        ) != OK ){
        return ERROR;
    }
    taskDelay(DELAY_TIME_SMB_WRITE);
    if( (retValue = GC_SmbRead( SMB_ADDRESS_AD78, 
                                AD78_WDOG_TOUT_REG, debugLevel )
        ) < 0 ){
        return ERROR;
    }
    if(retValue != value){
		if(debugLevel == 0){
        	printf("error: read value: 0x%x\n", retValue);
		}
        return ERROR;
    }
        
    return OK;
}

/**********************************************************************/
/** Routine to perform a register test on the AD78 over SMB Z001.
 *
 *  This routine reads first the current watchdog limit value, than 
 *  changes this twice with different values and write back the formerly 
 *  value. All write accesses are verified.
 *
 *  \param passes      \IN number of passes
 *  \param debugLevel  \IN 0: outputs enabled
 *                         1: no outputs 
 *
 *  \return     \OUT  ERROR or OK
 */
static int I2cAD78Test(int passes, int debugLevel)
{
    int pass = 0;
    int currentValue;
    
    if( (currentValue = GC_SmbRead( SMB_ADDRESS_AD78, AD78_WDOG_TOUT_REG,
     	debugLevel )) < 0 ){
        return ERROR;
    }
      
    for(pass = 0; pass < passes; pass++)
    {
		if(debugLevel == 0){
        	printf("Write and verify testvalue: 0x%x\t", AD78_TEST_VALUE_1);
		}
        if(AD78WriteVerify(AD78_TEST_VALUE_1, debugLevel) == ERROR)
            return ERROR;
		if(debugLevel == 0){
        	printf("OK\n");
       		printf("Write and verify testvalue: 0x%x\t", AD78_TEST_VALUE_2);
		}
        if(AD78WriteVerify(AD78_TEST_VALUE_2, debugLevel) == ERROR)
            return ERROR;
		if(debugLevel == 0){
        	printf("OK\n");
		}
    }
    if(AD78WriteVerify(currentValue, debugLevel) == ERROR)
        return ERROR;
    
    return OK;        
}


/**********************************************************************/
/** Routine to perform a memory test on production EEPROM on F503 over SMB.
 *  Only two bytes are tested.
 *
 *  The old values on EEPROM are written back after test completed.
 *
 *  \param passes      \IN number of passes
 *  \param debugLevel  \IN 0: outputs enabled
 *                         1: no outputs
 *
 *  \return     \OUT  OK or ERROR
 */
static int EepromTest2Bytes(int passes, int debugLevel)
{
    int numErrors = 0;
	int retVal = 0; 
    int error = 0; 
    int readValue = 0;
	int rescuedValue = 0;
    u_int32 offset;
    u_int8 testValue;
	u_int8 cnt = 0;

	for(cnt = 0; cnt<2; cnt++){
		offset = (EEPROM_SIZE - 1) & tickGet();
		testValue = (u_int8)tickGet();
    	
		/* read from EEPROM */
    	rescuedValue = GC_SmbRead( SMB_ADDRESS_EEPROM, offset, debugLevel );  
    	if(rescuedValue == ERROR)
    	{
    	    error = 0;
    	    numErrors++;
    	}    
    	
    	taskDelay( DELAY_TIME_SMB_WRITE );  
    	
    	/* write to EEPROM */
		error = GC_SmbWrite(SMB_ADDRESS_EEPROM, offset, testValue, debugLevel);  
		if(error)
		{
		    error = 0;
		    numErrors++;
		}
		taskDelay( DELAY_TIME_SMB_WRITE );
    	
		/* read and verify */
		readValue = GC_SmbRead( SMB_ADDRESS_EEPROM, offset, debugLevel );
    	if( (testValue != readValue) || (readValue < 0) )
    	{
    	    numErrors++;    
    	}
    	taskDelay( DELAY_TIME_SMB_WRITE );
    	
		/* Write back rescued value */
		error = GC_SmbWrite( SMB_ADDRESS_EEPROM, offset, rescuedValue, 
                             debugLevel );  
    	if(error)
    	{
    	    error = 0;
    	    numErrors++;
    	}      
    	taskDelay( DELAY_TIME_SMB_WRITE ); 
	}

    if(numErrors != 0)
		retVal = ERROR;

    return retVal;
}

/**********************************************************************/
/** Routine to perform a memory test on production EEPROM on F503 over SMB.
 *
 *  The old values on EEPROM are written back after test completed.
 *
 *  \param passes      \IN number of passes
 *  \param debugLevel  \IN 0: outputs enabled
 *                         1: no outputs
 *
 *  \return     \OUT  OK or ERROR
 */
static int EepromTest(int passes, int debugLevel)
{
    int cnt = 0;
	int retVal = 0;
    int numErrors = 0; 
    int error = 0; 
    int readValue = 0;
    u_int8 cntPasses = 0;
    char buf[EEPROM_SIZE];
  
	if(debugLevel == 0){
    	printf("Read first 512 Bytes of EEPROM");
	}
    for( cnt = 0; cnt < EEPROM_SIZE; cnt++ ){
		if(debugLevel == 0){
        	if(cnt%48 == 0)
            	printf("\n", cnt);
        	printf(".");
		}
        readValue = GC_SmbRead( SMB_ADDRESS_EEPROM, cnt, debugLevel );  
        if(readValue == ERROR)
        {
            error = 0;
            numErrors++;
        }    
        else
            buf[cnt] = (char)readValue;
        taskDelay( DELAY_TIME_SMB_WRITE );  
    }
   
    
    for( cntPasses = 0; cntPasses < passes; cntPasses++ ){
        /* write to EEPROM */
		if(debugLevel == 0){
        	printf("\n\nWrite test pattern to EEPROM");
		}
        for(cnt = 0; cnt < EEPROM_SIZE; cnt++ )
        {
			if(debugLevel == 0){
            	if(cnt%16 == 0)
                	printf("\n0x%08x:", cnt);
            	printf(" %02x",(cnt+G_eeTestStart) & 0xff );
			}
                
            error = GC_SmbWrite( SMB_ADDRESS_EEPROM, cnt, 
                                 (cnt+G_eeTestStart) & 0xff, debugLevel );  
            if(error)
            {
                error = 0;
                numErrors++;
            }
            taskDelay( DELAY_TIME_SMB_WRITE );
        }
        
        /* read and verify EEPROM content */
		if(debugLevel == 0){
        	printf("\n\nVerify written test pattern");
		}
        for(cnt = 0; cnt < EEPROM_SIZE; cnt++ )
        {
			if(debugLevel == 0){
            	if(cnt%48 == 0)
                	printf("\n", cnt);
            	printf(".");
			}
            readValue = GC_SmbRead( SMB_ADDRESS_EEPROM, cnt, debugLevel );
            if((((cnt+G_eeTestStart) & 0xff) != readValue) || (readValue < 0))
            {
                numErrors++;    
				if(debugLevel == 0){
                	printf("*** read error, 0x%x, 0x%x\n", (cnt+G_eeTestStart), 
                       	readValue);
				}
            }
            taskDelay( DELAY_TIME_SMB_WRITE );
        }
        /* prevent test with same pattern for every run */
        G_eeTestStart++;
    }
    
	if(debugLevel == 0){
    	printf("\n\nWrite back rescued values");
	}
    for( cnt = 0; cnt < EEPROM_SIZE; cnt++ ){
		if(debugLevel == 0){
        	if(cnt%48 == 0)
            	printf("\n", cnt);
        	printf(".");
		}
        error = GC_SmbWrite( SMB_ADDRESS_EEPROM, cnt, buf[cnt], debugLevel );  
        if(error)
        {
            error = 0;
            numErrors++;
        }      
        taskDelay( DELAY_TIME_SMB_WRITE ); 
    }
    
	if(debugLevel == 0){
    	printf("\n");
	}

	if(numErrors != 0)
		retVal = ERROR;

    return retVal;
}

/**********************************************************************/
/** Routine to perform a memory test on the EEPROM or a register test on the
 *  AD78 over SMB Z001.
 *
 *  \param location    \IN 0: write to production data EEPROM
 *                         1: write to AD78
 *  \param passes      \IN number of passes
 *  \param debugLevel  \IN 0: outputs enabled
 *                         1: no outputs
 *  \param testCase    \IN only available for location EEPROM
 *                         0: test the complete EEPROM
 *                         1: test two random Bytes
 *
 *  \return     \OUT  ERROR or OK
 */
int GC_I2cTest( int location, int passes, int debugLevel, int testCase )
{
    int retVal = 0;
    
    if(location == AD78_TEST)
        retVal = I2cAD78Test( passes, debugLevel );
    else if(location == EEPROM_TEST){
		if(testCase)
        	retVal = EepromTest2Bytes( passes, debugLevel );
		else
			retVal = EepromTest( passes, debugLevel );
    }
    else{
		if(debugLevel == 0){
        	printf("Error: wrong location \n");
		}
        return ERROR;    
    }
        
    return retVal;
}

/**********************************************************************/
/** Routine to perform a memory test on the DDR2 of the F503.
 *
 *  \param passes      \IN number of passes
 *  \param testCase    \IN 0: test the ddr2 complete
 *                         1: test only four Bytes (random access)
 *  \param displayMode \IN 0: console screen
 *                         1: sequential output (errors only)
 *
 *  \return     \OUT  OK or ERROR
 */
int GC_SdramTest( int passes, int testCase, int displayMode )
{
    char params[500];
    CHAMELEONV2_UNIT    unit;
	CHAMELEONV2_FIND    chamFind;
	int retVal;
	u_int32 startAddr = 0;
	u_int32 endAddr   = 0;
    
	chamFind.variant  =	-1;
	chamFind.instance =	 0;	
	chamFind.busId	  =	-1;
	chamFind.group	  =	-1;
	chamFind.bootAddr =	-1;
	chamFind.devId	  =	43;
	
	if(	sysChameleonV2UnitFind(	chamFind, 0, &unit )
		!= CHAMELEON_OK	)
	{
		return(	ERROR );
	}

	if( testCase == 1 ){
		startAddr = ((u_int32)(unit.size - 1) & (u_int32)tickGet() & 0xfffffffC) + (u_int32)unit.addr;
		endAddr = startAddr + 4;
	}
	else{
		startAddr = (u_int32)unit.addr;
		endAddr   = (u_int32)unit.addr + unit.size;
	}
	
    sprintf(params, "-n=%d 0x%x 0x%x -t=bwlBWL -o=%d", passes, startAddr, 
            endAddr, displayMode);
    
    retVal = mtest(params);
	if(retVal != OK)
		retVal = ERROR;

	return retVal;
}

/**********************************************************************/
/** Routine to perform a memory test of 4 Bytes on the DDR2 of the XM50.
 *
 *  \param passes      \IN number of passes
 *  \param displayMode \IN 0: console screen
 *                         1: sequential output (errors only)
 *
 *  \return     \OUT  OK or ERROR
 */
int GC_SdramXM50Test( int passes, int displayMode )
{
	char params[500];
    int retVal;
	u_int32 *startAddr = NULL;
	u_int32 endAddr;
	u_int32 size = 0x10;

	startAddr = (u_int32 *)malloc(size);
	if(startAddr == NULL)
		return ERROR;
	endAddr = (u_int32)startAddr + size;
	
    sprintf(params, "-n=%d 0x%x 0x%x -t=bwlBWL -o=%d", passes, startAddr, 
            endAddr, displayMode);
    
    retVal = mtest(params);
	free(startAddr);
	if(retVal != OK)
		retVal = ERROR;

	return retVal;
}

/**********************************************************************/
/** Routine to perform a Com 2 loopback test. Testadapter must be plugged in.
 *  At the testadapter Rx and Tx are connected. The test sends and receives
 *  100 Bytes. 
 *
 *  \param passes      \IN number of passes
 *  \param baudRate    \IN baudrate for tests 
 *
 *  \return     \OUT  OK or ERROR
 */
int GC_Com2Test(int passes, int baudRate)
{
	int fd;
	u_int32 nbrOfTestValues = 100;
	char sendValue[nbrOfTestValues];
	char receiveValue[nbrOfTestValues];
	u_int32 numberOfUnreadBytes = 0;
	u_int32	run = 0;
	u_int32 n = 0;
	int32 retVal = OK;
	struct timeval now;

	fd = open ("/tyCo/3", O_RDWR, 0644);

	if (fd < 0) {
		return ERROR;
	}

	if( ioctl (fd, FIOBAUDRATE, baudRate) != OK ){ /* RS232 Mode */
		retVal = ERROR;
		goto ABORT;
	}

	for(run = 0; run < passes; run++ ){
		/* re-initialize the random value */
		microtime (&now);  
		srand ((u_long)now.tv_usec);
		for(n=0; n < nbrOfTestValues; n++){
			sendValue[n] = (char)rand();
			write(fd, &sendValue[n], 1);
		}
		taskDelay(200);
		for(n=0; n < nbrOfTestValues; n++){
			ioctl(fd, FIONREAD, (int)&numberOfUnreadBytes);
			if( numberOfUnreadBytes >= 1 ){
				/* read byte */
				if(read(fd, &receiveValue[n], 1) == ERROR ){
					retVal = ERROR;
					goto ABORT;
				}
			}
			else{
				/* not any bytes read */
				retVal = ERROR;
				goto ABORT;
			}
		}
		/* verify received values */
		for(n=0; n < nbrOfTestValues; n++){
			if(sendValue[n]!=receiveValue[n]){
				retVal = ERROR;
				goto ABORT;
			}
		}
	}

ABORT:
	close(fd);

	return retVal;
}

/**********************************************************************/
/** Routine to perform a memory test on the Solid State Disc of the F503.
 *
 *  \param passes      \IN number of passes
 *  \param displayMode \IN 0: console screen
 *                         1: sequential output
 *
 *  \return     \OUT  ERROR or OK
 */
int GC_SsdTest( int passes, u_int8 displayMode )
{
    char params[500];
	char *hdtestResultP = NULL;
	char *hdtestResultTmpP = NULL;
	u_int32 errors = 0, writeXfer, readXfer;
	u_int32 i = 0;
    
    usrAtaConfig( 0, 0, "/ata0a");
	
	if( displayMode == 1 ){
		/* allocate memory to get the outputs from hdtest */
		hdtestResultP = malloc(sizeof(char) * 100);
		hdtestResultTmpP = hdtestResultP;
		if(hdtestResultP == NULL)
			return ERROR;
		
		for(i = 0; i< 100; i++, hdtestResultTmpP++){
			*hdtestResultTmpP = 0;
		}
		/* run hdtest in raw mode */
    	sprintf(params, "-f=100000 -s=0 -n=%d -r=0x%x /ata0a/test", passes, 
                hdtestResultP);
		hdtest(params);
		/* read out the result from raw mode */
		sscanf(hdtestResultP, "-e=%d -w=%d -r=%d", &errors, &writeXfer, 
               &readXfer);
		free(hdtestResultP);   
	}
	else{
		/* run hdtest in console screen mode */
		sprintf(params, "-f=100000 -s=0 -n=%d /ata0a/test", passes);
		hdtest(params);
	}
	
	if(errors)
		return ERROR;

    return OK;
}

/**********************************************************************/
/** Perfprmance test for USB device
 *
 *  \param passes      \IN number of passes
 *  \param displayMode \IN 0: console screen
 *                         1: sequential output
 *
 *  \return     \OUT  ERROR or OK
 */
int GC_UsbTest( int passes, u_int8 displayMode )
{
    char params[500];
	char *hdtestResultP = NULL;
	u_int32 errors = 0, writeXfer, readXfer;
   
    
	if( displayMode == 1 ){
		/* allocate memory to get the outputs from hdtest */
		hdtestResultP = malloc(100 * sizeof(char));
		if(hdtestResultP == NULL)
			return ERROR;
		
		bzero(hdtestResultP, 100);
		/* run hdtest in raw mode */
    	sprintf(params, "-f=1000 -s=0 -n=%d -r=0x%x /bd0/test", passes, 
                hdtestResultP);
		hdtest(params);
		/* read out the result from raw mode */
		sscanf(hdtestResultP, "-e=%d -w=%d -r=%d", &errors, &writeXfer, 
               &readXfer);
		  
	}
	else{
		/* run hdtest in console screen mode */
		sprintf(params, "-f=1000 -s=0 -n=%d /bd0/test", passes);
		hdtest(params);
	}
	free(hdtestResultP); 
    
	if(errors)
		return ERROR;

    return OK;
}


/**********************************************************************/
/** Routine to perform a memory test over the PCI bus.
 *
 *  \param  passes      \IN number of passes
 *          bus         \IN bus# of destdevice
 *          dev         \IN dev#
 *
 *  \return     \OUT  ERROR or OK
 */
int GC_PciTest( int passes, int debugLevel)
{
    char params[500];
    
    CHAMELEONV2_UNIT    unit;
    CHAMELEONV2_FIND    chamFind;

    if(InitSecChameleon(debugLevel))
        return ERROR;

    chamFind.variant  = -1;
    chamFind.instance =  0; 
    chamFind.busId    = -1;
    chamFind.group    = -1;
    chamFind.bootAddr = -1;
    chamFind.devId    = 43;

    if( (G_ChamErr = G_SecChamFktTbl.InstanceFind( 
                                        G_SecChamHdl,
                                        0,
                                        chamFind,
                                        &unit,
                                        NULL,
                                        NULL))
        != CHAMELEONV2_UNIT_FOUND )
    {
        if(debugLevel != 2) 
            printf("\n\n***G_SecChamFktTbl.InstanceFind (%d)\n",G_ChamErr);
        return ERROR;
    }

    /* optimized to approx 10s test duration */
    sprintf(params, "0x%x 0x%x -t=bwlBWL -n=%d -o=%d", unit.addr, 
            (int) (unit.size / 4) + unit.addr, passes, (debugLevel==2)?1:0);
    
    mtest(params);
    
    return OK;
}

/**********************************************************************/
/** Routine to perform a test on the Ethernet interfaces. 
 *
 *  This routine tests the ethernet interfaces. Therefore a loopback connector
 *  has to be connected to the ethernet port under test.
 *  
 *  \param port         \IN number of port (1..3)
 *  \param duration		\IN duration in seconds
 * \param debugLevel    \IN 1: print error messages only, 0: print all, 
 *                          2: print nothing
 *
 *  \return     \OUT  ERROR or OK
 */
int GC_EthTest( int port, int duration, int debugLevel )
{
    if(GC_Loopframes(	port - 1, duration, 64, 1000, NULL, debugLevel) != OK)
		return ERROR;
    
    return OK;    
}

/**********************************************************************/
/** Routine to execute a performance test for the Ethernet interface. 
 *  
 *  GC_EthPerf makes a nfs mount to the swserver and then executes the 
 *  hdtest over ethernet. To change the network interface use mottsec0,
 *  mottsec1 or mottsec2 at MenMon (ee-vxbline->boot device) and set the same
 *  interface at MenMon>ee-ecl.
 *
 *  \param fileSize         \IN size of the testfile
 *
 *  \return     \OUT  ERROR or OK
 */
int GC_EthPerf( int fileSize )
{
    char params[500];

    printf("To change the network interface use mottsec0, mottsec1 \n");
    printf("or mottsec2 at MenMon (ee-vxbline->boot device) and set\n");
    printf("the same interface at MenMon>ee-ecl\n");

    taskDelay(0x500);

    hostAdd("swserver", "192.1.1.22");
    nfsMount("swserver", "/tftpboot", "/nfs");

    sprintf(params, "/nfs/testfile_F503 -n=1 -d -s=0 -f=%d", fileSize);
    hdtest(params);

    return OK;
}

/**********************************************************************/
/** Routine to read the temperature on the AD78. 
 *
 *  \param debugLevel   \IN 0: outputs enabled
 *                          1: no outputs
 *  
 *  \return     \OUT  ERROR or OK
 */
int GC_PowerTemp( int debugLevel )
{
    int currentValue;

    currentValue = GC_SmbRead( SMB_ADDRESS_AD78, AD78_TEMP_REG, debugLevel );
    if(currentValue == ERROR)
        return ERROR;

	currentValue = ((3*10000*currentValue)/255 - 4800)/156;

    
    if(debugLevel == 0){
    	printf("Measured Temperature: %d Grad Celsius\n", currentValue);
	}    

    return OK;
}

/**********************************************************************/
/** Routine to dump the chameleon table. 
 *  
 *
 *  \return     \OUT  ERROR or OK
 */
void GC_ChamShow( void )
{
    sysChameleonShow();
}


/**********************************************************************/
/** Routine to test functionality of the FPGA internal watchdog circuit.
 *
 *  This routine starts the watchdog and triggers them. After the time has
 *  elapsed the watchdog will not be triggered anymore. A system reset 
 *  shall be asserted. 
 *  
 *  \param timeout     \IN time in seconds
 *
 *  \return     \OUT  ERROR or OK
 */
int GC_WdgTest( int timeout )
{   
    u_int32 cntTime = 0;
    u_int32 timeoutMs = timeout * 1000;
    
    sysChameleonInit();    
    CHAMZ069_Init();
    
    CHAMZ069_WdogSetTout(60);
	
	for(cntTime = 0; cntTime <= timeoutMs; ){
	    CHAMZ069_WdogPing();
	    /* sleep 50ms */
	    taskDelay(sysClkRateGet()/20);
	    cntTime += 50;
	}
	
	while(1)
	    ;
    
    return OK;
}

/**********************************************************************/
/** Routine to partition and format the solid state disc. 
 *  
 *
 *  \return     \OUT  ERROR or OK
 */
int GC_PartitionFormat( void )
{
    BLK_DEV *ataBlkDevP;
    
    ataBlkDevP = ataDevCreate(0,0,0,0);
    if(ataBlkDevP == NULL)
        return ERROR;
        
    /* creates the partition table */
    if( usrFdiskPartCreate( (CBIO_DEV_ID)ataBlkDevP, 1, 0,0,0 ) )
        return ERROR;
        
    /* mount the solid state disc */
    if( usrAtaConfig( 0, 0, "/ata0a" ) )
        return ERROR;
    
    /* format the solid state disc */    
    if( dosFsVolFormat( "/ata0a", DOS_OPT_BLANK, 0 ) )
        return ERROR;

    reboot(0);
    
    return OK;
}

/*****************************************************************************/
/** Reads from stdin until '\n' is noticed
 * 
 * \param readCharactersP \IN  pointer to the read characters from stdin
 *                             inclusive \0
 * \param sizeOfBuffer    \IN  max. possible number of read characters from 
 *                             stdin inclusive \n
 *
 * \return    \OUT OK or ERROR if '\n' wasn't detected at the second read 
 *                 access.
 */
static int GetEntry( char *readCharactersP, u_int8 sizeOfBuffer )
{
    u_int8 cnt = 0;
    char c = 0;
    
    if( sizeOfBuffer == 0 ) {
        return ERROR;
    }
    
    while((c = getc(stdin)) != '\n')
    {
        if((cnt >= sizeOfBuffer - 1)) {
            return ERROR;
        }
        readCharactersP[cnt] = c;
        cnt++;
        
    }
    readCharactersP[cnt] = '\0';    
    
    return 0;
}

/**********************************************************************/
/** Routine to update the FPGA configuration file in the configuration Flash. 
 *  
 * \param fpgaLength    \IN  length of the FPGA file
 *
 *  \return     \OUT  ERROR or OK
 */
int GC_FpgaLoad( int fpgaLength )
{
    char params[500];
    char value[2];
    int fh;   
    CHAMELEONV2_UNIT    unit;
	CHAMELEONV2_FIND    chamFind;
	void *bufferAddress;
    
	chamFind.variant  =	-1;
	chamFind.instance =	 0;	
	chamFind.busId	  =	-1;
	chamFind.group	  =	-1;
	chamFind.bootAddr =	-1;
	chamFind.devId	  =	43;
	
	if(	sysChameleonV2UnitFind(	chamFind, 0, &unit )
		!= CHAMELEON_OK	)
	{
		return(	ERROR );
	}
	
	bufferAddress = unit.addr;
    
    printf("\nTo update the Flash the following things are necessary:\n");
    printf("- Solid State Disc must be partitioned and formatted\n");
    printf("- The FPGA file must have been the extension .d00\n");
    printf("- SERDL the FPGA file at MenMon\n");
    printf("- MenMon>mo 200000 %x <FPGA file length>\n", bufferAddress);
    printf("- The exactly length of the FPGA File must be given\n");
    
    printf("\n\nFor exit please press 'e', otherwise any other key.\n");
    
    if(GetEntry(value, 2) == ERROR)
        return ERROR;
    
    if(value[0] == 'e')
        return 0;
    
    chamFind.variant  =	-1;
	chamFind.instance =	 0;	
	chamFind.busId	  =	-1;
	chamFind.group	  =	-1;
	chamFind.bootAddr =	-1;
	chamFind.devId	  =	126;
	
	if(	sysChameleonV2UnitFind(	chamFind, 0, &unit )
		!= CHAMELEON_OK	)
	{
		return(	ERROR );
	}
    
    
    usrAtaConfig( 0, 0, "/ata0a");
    
    cd("/ata0a");
    fh = open("fpga.bin",0x202,0);
    write(fh, bufferAddress, fpgaLength);
    close(fh);
    
    
    sprintf(params, "-d %x -z -f -w fpga.bin 0 -v", unit.addr);
    fpga_load(params);

    return OK;
}
