/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  d6_check.c
 *
 *      \author  sv
 *        $Date: 2007/03/05 13:26:43 $
 *    $Revision: 1.7 $
 *
 *        \brief Call d6Status to get detailed information about this module.
 *               make CPU=<platform> TOOL=gnu
 *
 *     Switches: -
 */
/*---------------------------[ Public Functions ]----------------------------
 *  - D6S_Init
 *  - D6S_DeInit
 *  - D6S_Watchdog
 *  - D6S_Adc
 *  - D6S_ResetInfo
 *  - D6S_BoardInfo
 *  - D6S_BmcStatusRegister
 *  - D6S_StartMeasurement
 *  - D6S_StopMeasurement
 *  - D6S_ShowBoardInfo
 *  - D6S_ShowStatRegs
 *  - D6S_ShowWatchdog
 *  - D6S_ShowRstHistory
 *  - D6S_ShowVoltages
 *  - D6S_ShowTemperatures
 *  - D6S_ShowCurrents
 *  - D6S_TriggerWatchdog
 *  - d6Status (example application, call 'd6Status' to get more information)
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: d6_status.c,v $
 * Revision 1.7  2007/03/05 13:26:43  svogel
 * cosmetics
 *
 * Revision 1.6  2006/06/20 15:47:39  SVogel
 * cosmetics
 *
 * Revision 1.5  2006/06/09 12:20:19  SVogel
 * cosmetics
 *
 * Revision 1.4  2006/06/07 10:18:05  SVogel
 * Changes for distribution.
 *
 * Revision 1.3  2006/03/14 12:32:40  SVogel
 * added pci information to BMC handle
 *
 * Revision 1.2  2006/02/20 08:54:44  SVogel
 * added
 * + additional watchdog functionality
 * + function comments
 *
 * Revision 1.1  2006/02/08 16:48:40  SVogel
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
/*
 *
 */

 /*! \page dummy
  \menimages
*/

 static char *G_d6cRcsId="$Id: d6_status.c,v 1.7 2007/03/05 13:26:43 svogel Exp $ Exp $ build " __DATE__" "__TIME__ ;
/*--------------------------------------*/
/*    INCLUDES                          */
/*--------------------------------------*/
/* VxWorks standard libraries */
#include "vxWorks.h"            /* always first, CPU type, family ,
                                   big/litte endian, etc. */
/* #include "sysLib.h"*/
#include "taskLib.h"
#include "sysLib.h"
/* Standard VxWorks driver libraries */
#include "drv/pci/pciConfigLib.h"   /* for VxWorks PCI functions */

/* Standard ANSI libraries */
#include <stdio.h>
#include <string.h>

#ifndef VXWORKS
#define VXWORKS
#endif

/* MEN specific libraries */
#include <MEN/men_typs.h>
#include <MEN/oss.h>

#include "d6_status.h"
#include <time.h>

/*--------------------------------------+
|   DEFINES                             |
+---------------------------------------*/

/*--------------------------------------+
|   CONSTANTS                           |
+---------------------------------------*/
BMC_HANDLE G_bmcHdl = NULL;        /* used for main function d6Check */
u_int8     G_options = 0;          /*             "                  */
/*--------------------------------------+
|   GLOBALS                             |
+---------------------------------------*/


/*--------------------------------------*/
/*    LOCAL PROTOTYPES                  */
/*--------------------------------------*/
static u_int8   LocSearchForPciDevice(D6S_PCI_TS *pciDevP, u_int8 startBus);
static void     LocMsecDelay(u_int16 value);
static void     LocGetByte(u_int8 *valueP);
static void     LocProcessTask(BMC_HANDLE *bmcHdlP);
static u_int8   LocReadFrame(BMC_HANDLE *bmcHdlP);
static u_int8   LocWriteFrame(BMC_HANDLE *bmcHdlP);
static u_int8   LocInitWatchdog(BMC_HANDLE *bmcHdlP);
static u_int8   LocInitReset(BMC_HANDLE *bmcHdlP);
static u_int8   LocInitAdc(BMC_HANDLE *bmcHdlP);
static u_int8   LocInitBoardInfo(BMC_HANDLE *bmcHdlP);
static u_int8   LocInitCmdResources(BMC_HANDLE *bmcHdlP);
static void     LocReceiveMsg(BMC_HANDLE *bmcHdlP);
static void     LocReadMsg(BMC_HANDLE *bmcHdlP);
static void     LocWriteMsg(BMC_HANDLE *bmcHdlP);
static void		LocGetValue(BMC_HANDLE *bmcHdlP, u_int8 *valueP, u_int16 noBytes);
static void     LocMessageTimeout(BMC_HANDLE *bmcHdlP);
static void     LocMeasureTask(BMC_HANDLE *bmcHdlP);
static void		LocUsage(void);
static u_int8	LocCheckAck(BMC_HANDLE *bmcHdlP, u_int32 *valP, u_int16 repeatCnt);
static u_int8	LocCheckError(BMC_HANDLE *bmcHdlP, u_int32 *valP);
static u_int8	LocShowAll(BMC_HANDLE *bmcHdlP);
static u_int8	LocInitApi(BMC_HANDLE *bmcHdlP, u_int8 manufacturer);
static u_int8	LocDeInitApi(void);
static u_int8	LocTimeoutMeasRdy(BMC_HANDLE *bmcHdlP);
static u_int8	LocSetWatchdogValue(BMC_HANDLE *bmcHdlP, enum D6S_EWDG watchdog);
static u_int8   LocShowStatusRegister(u_int8 maxItems, u_int16 *regP,
                                      D6S_STAT_STRING *statusInfoP);
static u_int8	LocInitPciInterface(BMC_HANDLE *bmcHdlP, D6S_PCI_TS *pciDevP,
                                    u_int8 manufacturer);
static u_int8	LocMakeWatchdogCmd(BMC_HANDLE *bmcHdlP, enum D6S_EWDG watchdog,
                                   u_int8 **measTypeP, u_int8 *cmdP);
static u_int8	LocMakeAdcCmd(BMC_HANDLE *bmcHdlP, enum D6S_ADC adcType,
                            u_int16 **measTypeP, u_int8 *noChanP, u_int8 *cmdP);
static u_int8   LocMakeRstCmd(BMC_HANDLE *bmcHdlP, enum D6S_RST reset, u_int8 *cmdP,
                            u_int16 **measTypeP);

/**********************************************************************/
 /** routine to search for a PCI device
  *
  *  This routine searches for PCI device by its vender, device,
  *  subventor and subdevice ID.
  *
  *  \param pciDevP			PCI device structure
  *  \param startBus		bus number to start search
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8	LocSearchForPciDevice(D6S_PCI_TS *pciDevP, u_int8 startBus)
{
    u_int8  i;
    u_int8  tmpBus          = 0;
    u_int8  tmpDevice       = 0;
    u_int8  tmpFunction     = 0;
    u_int8  tmpHeader       = 0;
    u_int8  tmpPrimaryBus   = 0;
    u_int8  tmpSecondaryBus = 0;
    UINT32 tmpBaseId       = 0;
    UINT32 tmpSubId        = 0;

    /* scan the pci bus */
    for( tmpBus = startBus; tmpBus < 0xff; tmpBus++ )
    {
        for( tmpDevice = 0; tmpDevice < 0xff; tmpDevice++ )
        {
            for( tmpFunction = 0; tmpFunction < 0xff; tmpFunction++ )
            {
                tmpBaseId = 0;
                tmpSubId = 0;
                tmpHeader = 0;
                tmpPrimaryBus = 0;
                tmpSecondaryBus = 0;

                /* read out vendor id */
                pciConfigInLong (tmpBus, tmpDevice, tmpFunction,
                    D6S_PCI_DEVICE_VENDOR_ADDR, &tmpBaseId);

                pciConfigInLong (tmpBus, tmpDevice, tmpFunction,
                    D6S_PCI_SUBSYSTEM_ID_ADDR, &tmpSubId);

                if( ((tmpBaseId & 0x0000ffff) != pciDevP->vendorId) ||
                    (((tmpBaseId & 0xffff0000) >> 16) != pciDevP->deviceId) ||
                    ((tmpSubId & 0x0000ffff) != pciDevP->subVendorId) ||
                    (((tmpSubId & 0xffff0000) >> 16) != pciDevP->subId) )
                {
                    /* try to scan another bus to find device
                       - wrong device or vendor id detected */
                    break;
                }/* end if */

                pciDevP->bus    = tmpBus;
                pciDevP->device = tmpDevice;

                /*------------------------------+
                | read out header type          |
                | skip if not PCI_PCI bridge    |
                +-------------------------------*/
                pciConfigInByte (   tmpBus,
                                    tmpDevice,
                                    tmpFunction,
                                    D6S_PCI_HEADER_TYPE,
                                    &tmpHeader);

                /* PCI to PCI bridge header found */
                if( (tmpHeader & D6S_PCI_HEADER_TYPE_MASK) ==
                     D6S_PCI_HEADER_PCI_PCI)
                {
                    /* read out secondary bus number */
                    pciConfigInByte (   tmpBus,
                                        tmpDevice,
                                        tmpFunction,
                                        D6S_PCI_SECONDARY_BUS,
                                        &tmpSecondaryBus);

                    /* secondary bus available - != 0xff ? */
                    if( tmpSecondaryBus != 0xff ) /* == bus */
                    {
                        pciConfigInByte (   tmpBus,
                                            tmpDevice,
                                            tmpFunction,
                                            D6S_PCI_PRIMARY_BUS,
                                            &tmpPrimaryBus);
                         /* bus number of PCI to PCI bridge */
                        pciDevP->bus    = tmpPrimaryBus;
                        pciDevP->device = tmpDevice;

                    }/* end if */
                }/* end if */
                else
                {
					/* store all bar addresses */
					 for( i=0; i<D6S_NO_OF_BARS; i++ )
					 {
					     pciConfigInLong( tmpBus,
					                      tmpDevice,
					                      tmpFunction,
					                      (D6S_PCI_BAR_0_ADDR+
					                      i*D6S_PCI_BAR_OFFSET),
					                      (UINT32 *)&pciDevP->bar[i]);
					 }

                	return D6S_OK;
				}

				if( (tmpHeader & D6S_PCI_HEADER_MULTI_FUNC) !=
				                     D6S_PCI_HEADER_MULTI_FUNC)
				{/* no multi function device, so we can stop this loop */
				                    break;
                }/* end if */
              }/* end for */
        }/* end for */
    }/* end for */
    return D6S_ERROR;
}/* LocSearchForPciDevice */

/**********************************************************************/
 /** routine to stop programm execution
  *
  *  This routine stops the programm execution for a specific time (msecs).
  *
  *  \param value		msecs to stop programm execution
  *
  *  \return no return value
 */
static void LocMsecDelay(u_int16 value)
{
#define L_DIVISOR        1000

    u_int32 clkRate = 0;
    u_int32 delayFactor = 1;

    clkRate = sysClkRateGet() * value;

    delayFactor = (u_int32)(clkRate / L_DIVISOR);

    if( (clkRate % L_DIVISOR) > 0 )
    {
        delayFactor += 1;
    }

    if( delayFactor == 0 )
    {
        delayFactor = 1;
    }

    taskDelay(delayFactor);
}/* LocMsecDelay */

/**********************************************************************/
 /** routine to get a byte from stdin
  *
  *  This routine gets a byte from the standard input.
  *
  *  \param valueP		value from stdin
  *
  *  \return no return value
 */
static void LocGetByte(u_int8 *valueP)
{
#define L_BUFFSIZE      5
    char buffer[L_BUFFSIZE];
    u_int8 input = 0;
    u_int8 size = 0;

    memset(buffer, '\0', sizeof(buffer));

    printf("Type value and hit enter (dezimal): ");

    while( size < 3 )
    {
        input = getchar();

        if( input != '\n' )
        {
            buffer[size++] = input;
        }
        else
        {
            break;
        }/* end if */
    }/* end while */

    *valueP = (u_int8)(atoi(buffer));
}/* LocGetByte */

/**********************************************************************/
 /** routine to read a frame of the BMC
  *
  *  This routine is used to read a frame from the BMC.
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocReadFrame(BMC_HANDLE *bmcHdlP)
{
   u_int16 i;
   D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    /*
        read data from memory
    */
    bmcP->cmd.length = bmcP->memAddrP[0];

#ifdef DEBUG
    printf("read frame (length=%d): 0x%x ", bmcP->cmd.length, bmcP->memAddrP[0]);
#endif
    if( (bmcP->cmd.length < D6S_MEMORY_SIZE) &&
        (bmcP->cmd.length > 0) )
    {
        for( i=0; i<bmcP->cmd.length; i++ )
        {
            /* get all received bytes */
            bmcP->cmd.buffer[i] = bmcP->memAddrP[i+1];
#ifdef DEBUG
            printf("0x%x ", bmcP->cmd.buffer[i]);
#endif
        }/* end for */
#ifdef DEBUG
        printf("\n");
#endif
    } /* end if */
    else
    {
        printf("**** Read frame ERROR (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }

    return D6S_OK;
}/* LocReadFrame */

/**********************************************************************/
 /** routine to write a frame to the BMC
  *
  *  This routine is used to write a frame to the BMC.
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocWriteFrame(BMC_HANDLE *bmcHdlP)
{
    u_int16 i;
    D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }

    bmcP = (D6S_BMC_TS *)bmcHdlP;
    /*
        write data to memory
    */
#ifdef DEBUG
    printf("write frame (length=%d): ", bmcP->cmd.length);
#endif
    if( (bmcP->cmd.length < D6S_MEMORY_SIZE) &&
        (bmcP->cmd.length > 0) )
    {
        bmcP->memAddrP[0] = bmcP->cmd.length;
#ifdef DEBUG
        printf("0x%x ", bmcP->memAddrP[0]);
#endif
        for( i=1; i<bmcP->cmd.length+1; i++ )
        {
            bmcP->memAddrP[i] = bmcP->cmd.buffer[i-1];
#ifdef DEBUG
            printf("0x%x ", bmcP->memAddrP[i]);
#endif
        }/* end for */
#ifdef DEBUG
        printf("\n");
#endif
        bmcP->cmd.length = 0;
        /* write start to status register */
        bmcP->bmcStatus = D6S_START_FLAG;
        *(bmcP->statCtrlRegP) = bmcP->bmcStatus;
    }/* end if */
    else
    {
        printf("**** Write frame error (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }

    return D6S_OK;
}/* LocWriteFrame */

/**********************************************************************/
 /** routine to handle a message timeout
  *
  *  This routine handles the timeout for a send/receive message. The
  *  internal FPGA state is used to break the while loop. If the FPGA
  *  state is not available or invalid a repetition counter stops the
  *  loop with a error message.
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return no return value
 */
static void LocMessageTimeout(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;
    u_int16     repeatCnt = 0;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return ;
    }

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    while( bmcP->cmd.rwCmd != D6S_NOT_BUSY_CMD )
    {
        if( bmcP->cmd.rwCmd == D6S_ERROR_CMD )
        {
            bmcP->cmd.rwCmd = D6S_NOT_BUSY_CMD;
            printf("**** Message error !\n");
            return ;
        }/* end if */

        LocMsecDelay(D6S_TASK_DELAY_MS);

        if( repeatCnt++ == D6S_RPT_CNT )
        {
            printf("**** Message timeout !\n");
            bmcP->cmd.rwCmd = D6S_NOT_BUSY_CMD;
            return ;
        }/* end if */
    }/* end while */
}/* LocMessageTimeout */

/**********************************************************************/
 /** routine to check the FPGA acknowledge flag
  *
  *  This routine checks the BMC status register if a command was rightly
  *  acknowledged.If no acknowledge was received an error will be written
  *  to stdout.
  *
  *  \param bmcHdlP		BMC handle
  *  \param valP		new delay value [msec]
  *  \param repeatCnt	repeat counter to wait for no acknowledge
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocCheckAck(BMC_HANDLE *bmcHdlP, u_int32 *valP, u_int16 repeatCnt)
{
    D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    if( ((bmcP->bmcStatus&D6S_ACK_FLAG) == D6S_ACK_FLAG) &&
         (bmcP->cmd.rwCmd == D6S_BUSY_CMD) )
    {
        bmcP->cmd.rwCmd = D6S_NOT_BUSY_CMD;
        *valP = D6S_TASK_DELAY_MS;
    }/* end if */

    if( ((bmcP->bmcStatus&D6S_ACK_FLAG) != D6S_ACK_FLAG) &&
         (bmcP->cmd.rwCmd == D6S_BUSY_CMD) )
    {
        if( repeatCnt == D6S_RPT_CNT )
        {
            bmcP->cmd.rwCmd = D6S_NOT_BUSY_CMD;
            bmcP->bmcStatus = D6S_ACK_FLAG;
            printf("No ack received!\n");
        }/* end if */
    }/* end if */

    return D6S_OK;
}/* LocCheckAck */

/**********************************************************************/
 /** routine to check the FPGA error flag
  *
  *  This routine checks the BMC status register if an error occured in
  *  transmission.
  *
  *  \param bmcHdlP		BMC handle
  *  \param valP		timeout value, if error occured
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocCheckError(BMC_HANDLE *bmcHdlP, u_int32 *valP)
{
    D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    if( ((bmcP->bmcStatus&D6S_TIMEOUT_FLAG) == D6S_TIMEOUT_FLAG) ||
        ((bmcP->bmcStatus&D6S_COMMERR_FLAG) == D6S_COMMERR_FLAG) )
    {
        bmcP->cmd.rwCmd = D6S_ERROR_CMD;
        *valP = D6S_TASK_DELAY_MS;
    }/* end if */

    return D6S_OK;
}/* LocCheckError */

/**********************************************************************/
 /** routine to process read/write operation
  *
  *  This routine manages the read/write operation to the BMC. It polls
  *  cyclic the BMC status register an performs the required operation.
  *  This function should be called periodically, if not running in a
  *  task !
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return no return value
 */
static void LocProcessTask(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;
    u_int32 delayValue;
    u_int16 repeatCnt = 0;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return ;
    }

    bmcP = (D6S_BMC_TS *)bmcHdlP;
    delayValue = D6S_TASK_DELAY_MS;

    while( bmcP->cmdTaskId != 0 )
    {
        bmcP->bmcStatus = *bmcP->statCtrlRegP;
        /* check, whether the FPGA is ready to perform a command */
        if( (bmcP->bmcStatus&D6S_READY_FLAG) == D6S_READY_FLAG  )
        {
            if( bmcP->cmd.rwCmd == D6S_READ_CMD )
            {
                repeatCnt = 0;
                delayValue = D6S_TASK_DELAY_MS;
                if( LocReadFrame(bmcHdlP) == D6S_OK )
                {
                    bmcP->cmd.rwCmd = D6S_NOT_BUSY_CMD;
                }/* end if */
            }/* end if */
            else if( bmcP->cmd.rwCmd == D6S_WRITE_CMD )
            {
                bmcP->cmd.rwCmd = D6S_BUSY_CMD;
                if( LocWriteFrame(bmcHdlP) == D6S_OK )
                {
                    delayValue = D6S_ACK_TIMEOUT_CNT*D6S_TASK_DELAY_MS;
                }/* end if */
            }/* end if */
        }/* end if */
        /* check for acknowledge */
        LocCheckAck(bmcHdlP, &delayValue, repeatCnt);

        /* check if a timeout occured */
        LocCheckError(bmcHdlP, &delayValue);

        /* stay for a while */
        LocMsecDelay(delayValue);
    }/* while */
    bmcP->cmd.rwCmd = D6S_NOT_BUSY_CMD;
}/* LocProcessTask */

/**********************************************************************/
 /** routine to initiate a write message to the BMC
  *
  *  This routine initiates a write operation to the BMC.
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return no return value
 */
static void LocWriteMsg(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return ;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    bmcP->cmd.rwCmd = D6S_WRITE_CMD;

	LocMessageTimeout(bmcHdlP);
}/* LocSendMsg */

/**********************************************************************/
 /** routine to initiate a read operation of the FPGA BMC memory space
  *
  *  This routine initiates a read operation of the FPGA BMC memory
  *  space..
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return no return value
 */
static void LocReadMsg(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return ;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    memset(bmcP->cmd.buffer, 0, sizeof(bmcP->cmd.buffer));

    bmcP->cmd.rwCmd = D6S_READ_CMD;

	LocMessageTimeout(bmcHdlP);

}/* LocReadMsg */

/**********************************************************************/
 /** routine to read a receive message operation
  *
  *  This routine is used to read a message, sent by the BMC
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return no return value
 */
static void LocReceiveMsg(BMC_HANDLE *bmcHdlP)
{
	D6S_BMC_TS *bmcP = NULL;
	u_int8 cmd = 0;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return ;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    cmd = bmcP->cmd.buffer[0];

	/* write command */
	bmcP->cmd.rwCmd = D6S_WRITE_CMD;

	LocMessageTimeout(bmcHdlP);

	memset(bmcP->cmd.buffer, 0, sizeof(bmcP->cmd.buffer));

	/* get value */
	bmcP->cmd.buffer[0] = cmd;
	bmcP->cmd.rwCmd = D6S_READ_CMD;
	LocMessageTimeout(bmcHdlP);
}/* LocReceiveMsg */

/**********************************************************************/
 /** routine to get a value from the receive buffer
  *
  *  This routine is a generic function to handle the receive byte values.
  *
  *  \param bmcHdlP		BMC handle
  *  \param valueP		destination for received byte(s)
  *  \param noBytes		number of bytes received
  *
  *  \return no return value
 */
static void	LocGetValue(BMC_HANDLE *bmcHdlP, u_int8 *valueP, u_int16 noBytes)
{
	D6S_BMC_TS *bmcP = NULL;
	u_int16 i;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return ;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

	for(i=0; i<noBytes; i++)
	{
		*(valueP+i) = bmcP->cmd.buffer[i];
	}/* end for */
}/* LocGetValue */

/**********************************************************************/
 /** routine to initialize the watchdog structure
  *
  *  This routine is used to set the watchdog structure to its default
  *  values.
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocInitWatchdog(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    bmcP->wdg.enable = 0;
    bmcP->wdg.prescaler = 0;
    bmcP->wdg.timeout = 0;
    bmcP->wdg.trigger = 0;

    return D6S_OK;
}/* LocInitWatchdog */

/**********************************************************************/
 /** routine to initialize the reset structure
  *
  *  This routine is used to set the reset structure to its default
  *  values.
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocInitReset(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    memset((u_int8 *)bmcP->rst.cause, 0, sizeof(bmcP->rst.cause));
    memset((u_int8 *)bmcP->rst.source, 0, sizeof(bmcP->rst.source));
    memset((u_int8 *)bmcP->rst.value, 0, sizeof(bmcP->rst.value));
    memset((u_int8 *)bmcP->rst.timestamp, 0, sizeof(bmcP->rst.timestamp));

   return D6S_OK;
}/* LocInitReset */

/**********************************************************************/
 /** routine to initialize the A/D converter structure
  *
  *  This routine is used to set the A/D converter structure to its
  *  default values.
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocInitAdc(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    memset((u_int8 *)bmcP->adc.senses, 0, sizeof(bmcP->adc.senses));
    memset((u_int8 *)bmcP->adc.voltages, 0, sizeof(bmcP->adc.voltages));
    memset((u_int8 *)bmcP->adc.temps, 0, sizeof(bmcP->adc.temps));

    bmcP->adc.vid = 0;

    return D6S_OK;
}/* LocInitAdc */

/**********************************************************************/
 /** routine to initialize the board information converter structure
  *
  *  This routine is used to set the board information converter
  *  structure to its default values.
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocInitBoardInfo(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    bmcP->brd.statusReg = 0;
    memset(bmcP->brd.fwVersion, 0, sizeof(bmcP->brd.fwVersion));
    bmcP->brd.cpciAddr = 0;
    bmcP->brd.permStatusReg = 0;
    bmcP->brd.fwChecksum = 0;
    bmcP->brd.pwrOnErr = 0;

    return D6S_OK;
}/* LocInitBoardInfo */

/**********************************************************************/
 /** routine to initialize the message command structure
  *
  *  This routine is used to set the message command structure to its
  *  values.
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocInitCmdResources(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    memset(bmcP->cmd.buffer, 0, sizeof(bmcP->cmd.buffer));
    bmcP->cmd.length = 0;
    bmcP->cmd.rwCmd = D6S_NOT_BUSY_CMD;

    return D6S_OK;
}/* LocInitCmdResources */

/**********************************************************************/
 /** routine to initialize the PCI structure
  *
  *  This routine is used to set the PCI structure to its default
  *  values. The variable 'manufacturer' is used to set the manufacturer
  *  specific device ID (MEN/ALCATEL).
  *
  *  \param bmcHdlP			BMC handle
  *  \param pciDevP			PCI device structure
  *  \param manufacturer	manufacturer identification
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocInitPciInterface(BMC_HANDLE *bmcHdlP, D6S_PCI_TS *pciDevP,
                                  u_int8 manufacturer)
{
    D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    pciDevP->bus = pciDevP->device = pciDevP->function = 0;
    pciDevP->deviceId = D6S_PCI_DEVICE_ID;
    pciDevP->vendorId = D6S_PCI_VENDOR_ID;
    pciDevP->revision = 0;
    memset((u_int8 *)pciDevP->bar, 0, sizeof(pciDevP->bar));
    pciDevP->subId = D6S_PCI_SUBSYSTEM_ID;

    if( manufacturer == D6S_ALCATEL )
    {
        pciDevP->subVendorId = D6S_PCI_ALCATEL_SUB_VENDOR_ID;
    }
    else
    {
        pciDevP->subVendorId = D6S_PCI_MEN_SUB_VENDOR_ID;
    }/* end if */

    /*--------------------
     | pci configuration |
     ---------------------*/
    if( LocSearchForPciDevice(pciDevP, 0) != D6S_OK )
    {
        D6S_DeInit(&bmcHdlP);
        printf("**** PCI device not found (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    return D6S_OK;
}/* LocInitPciInterface */

/**********************************************************************/
 /** routine to initialize this module
  *
  *  This routine is used to initialize this module and starts the task
  *  to sent or receive a message from the BMC. It is used by the
  *  application 'd6Status'.
  *
  *  \param manufacturer	manufacturer identification
  *
  *  \return D6S_OK or D6S_ERROR
 */
BMC_HANDLE D6S_Init(u_int8 manufacturer)
{
    /*D6S_PCI_TS pciDev;*/      /* FPGA PCI resources */
    D6S_BMC_TS *bmcP = NULL;
    u_int32 gotSize = 0;

    if( (bmcP = (D6S_BMC_TS *)
        OSS_MemGet(NULL, sizeof(D6S_BMC_TS),&gotSize)) == NULL )
    {
        printf("**** Error allocating resources (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return 0;
    }

    OSS_MemFill(NULL, gotSize, (u_int8 *)bmcP, 0);

     /* initialize PCI resource structure */
     if( LocInitPciInterface((BMC_HANDLE)&bmcP, &bmcP->pciDev, manufacturer) !=
         D6S_OK )
     {
        printf("**** PCI initialization error (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return NULL;
     }/* end if */


    LocInitWatchdog((BMC_HANDLE)bmcP);
    LocInitReset((BMC_HANDLE)bmcP);
    LocInitAdc((BMC_HANDLE)bmcP);
    LocInitBoardInfo((BMC_HANDLE)bmcP);
    LocInitCmdResources((BMC_HANDLE)bmcP);

    bmcP->memAddrP = (u_int8 *)( bmcP->pciDev.bar[D6S_PCI_BAR_INDEX] + D6S_MEMORY_OFFSET );
	bmcP->statCtrlRegP = bmcP->memAddrP + D6S_MEMORY_SIZE;

    bmcP->bmcStatus = *bmcP->statCtrlRegP;

	bmcP->measTaskId = 0;
    bmcP->cmdTaskId = 0;
    bmcP->wdgTaskId = 0;

    bmcP->measRdy = D6S_FALSE;
    bmcP->stopCylicMeas = D6S_FALSE;

    /* start task */
   if((bmcP->cmdTaskId = taskSpawn ("tBmcFpga",
			     5,		    /* priority */
			     0,		    /* task options */
			     0x1000, 	/* 4k stack space */
			     (FUNCPTR)LocProcessTask,
			     (int)bmcP, 0, 0, 0, 0, 0, 0, 0, 0, 0 )) == D6S_ERROR)
	{
        D6S_DeInit((BMC_HANDLE)&bmcP);
        printf("**** Error spawning task 'tBmcFpga' (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return  NULL;
	}/* end if */

    return ((BMC_HANDLE)bmcP);
}/* D6S_Init */

/**********************************************************************/
 /** routine to de-initialize this module
  *
  *  This routine is used to de-initialize this module and stops the task
  *  which is managing the send/receive procedure. It is used by the
  *  application 'd6Status'.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return D6S_OK or D6S_ERROR
 */
u_int8 D6S_DeInit( BMC_HANDLE **bmcHdlP )
{
    D6S_BMC_TS *bmcP = NULL;
    u_int16 repeatCnt = 0;

    if( *bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)*bmcHdlP;
    bmcP->cmdTaskId = 0;

    while( bmcP->cmd.rwCmd != D6S_NOT_BUSY_CMD )
    {
        LocMsecDelay(D6S_TASK_DELAY_MS);

        if( repeatCnt++ == D6S_MEAS_TIMEOUT_CNT )
        {
            bmcP->cmd.rwCmd = D6S_NOT_BUSY_CMD;
            printf("**** De-initialization timeout !\n");
            return D6S_ERROR;
        }/* end if */
    }/* end while */

    OSS_MemFree(NULL, (void *)bmcP, sizeof(D6S_BMC_TS));
    bmcP = NULL;

    *bmcHdlP = (BMC_HANDLE)bmcP;

    return D6S_OK;
}/* D6S_DeInit */

/**********************************************************************/
 /** routine to make a watchdog command
  *
  *  This routine makes a watchdog command which will be sent to the BMC.
  *
  *  \param bmcHdlP			BMC handle
  *  \param watchdog		enumeration of watchdog function
  *  \param measTypeP		pointer to measurement structure
  *  \param cmdP			BMC command identifier
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocMakeWatchdogCmd(BMC_HANDLE *bmcHdlP, enum D6S_EWDG watchdog, u_int8 **measTypeP, u_int8 *cmdP)
{
    D6S_BMC_TS *bmcP = NULL;

    if( *bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    switch (watchdog)
    {
        case ewdg_enable:/* command byte */
            *cmdP = D6S_WDG_EN;
            *measTypeP = &bmcP->wdg.enable;
            break;
        case ewdg_prescaler:/* command byte */
            *cmdP = D6S_WDG_TIMEOUT_PRESCALER;
            *measTypeP = &bmcP->wdg.prescaler;
            break;
        case ewdg_timeout:/* command byte */
            *cmdP = D6S_WDG_TIMEOUT_VAL;
            *measTypeP = &bmcP->wdg.timeout;
            break;
        case ewdg_trigger_rd:/* command byte */
            *cmdP = D6S_WDG_TRIGGER;
            *measTypeP = &bmcP->wdg.trigger;
            break;
        case ewdg_trigger_wr:/* command byte */
            *cmdP = D6S_WDG_TRIGGER | D6S_WR_EN;
            *measTypeP = &bmcP->wdg.trigger;
            break;
        case ewdg_setPrescaler:/* command byte */
            *cmdP = D6S_WDG_TIMEOUT_PRESCALER | D6S_WR_EN;
            *measTypeP = &bmcP->wdg.prescaler;
            break;
        case ewdg_setTimeout:/* command byte */
            *cmdP = D6S_WDG_TIMEOUT_VAL | D6S_WR_EN;
            *measTypeP = &bmcP->wdg.timeout;
            break;
        default:
            printf("**** Unsupported watchdog function (%s, line %d) !\n", __FUNCTION__, __LINE__);
            return D6S_ERROR;
    }/* switch */

    return D6S_OK;
}/* LocMakeWatchdogCmd */

/**********************************************************************/
 /** routine to get/set a watchdog operation
  *
  *  This routine gets/sets a watchdog operation.
  *
  *  \param bmcHdlP			BMC handle
  *  \param watchdog		enumeration of watchdog function
  *  \param value			watchdog parameter to set
  *
  *  \return D6S_OK or D6S_ERROR
 */
u_int8 D6S_Watchdog( BMC_HANDLE *bmcHdlP, enum D6S_EWDG watchdog, u_int8 value )
{
    D6S_BMC_TS *bmcP = NULL;
    u_int8 *measTypeP = NULL;
    u_int8 measTemp = 0;
    u_int8 command = 0;
    u_int8 buffer[5];

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    memset(buffer, 0, sizeof(buffer));

    if( LocMakeWatchdogCmd(bmcHdlP, watchdog, (u_int8 **)&measTypeP, &command)
        != D6S_OK )
    {
        printf("**** Error making watchdog command (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP->cmd.length = 1; /* frame length */

    if( measTypeP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP->cmd.buffer[0] = command;
    bmcP->cmd.length = 1; /* frame length */

    if( watchdog >= ewdg_trigger_wr )
    {
        bmcP->cmd.length = 2; /* frame length */
        bmcP->cmd.buffer[1] = value;
        LocWriteMsg(bmcHdlP);
        return D6S_OK;
    }/* end if */

    LocReceiveMsg(bmcHdlP);

    LocGetValue(bmcHdlP, &measTemp, bmcP->cmd.length);
    *measTypeP = measTemp;

    return D6S_OK;
}/* D6S_Watchdog */

/**********************************************************************/
 /** routine to make an A/D converter command
  *
  *  This routine makes an A/D converter command.
  *
  *  \param bmcHdlP			BMC handle
  *  \param adcType			enumeration of A/D converter function
  *  \param measTypeP		pointer to measurement structure
  *  \param noChanP			channel identifier (sense/voltage/temperature
  *                         or voltage ID)
  *  \param cmdP			BMC command identifier
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocMakeAdcCmd(BMC_HANDLE *bmcHdlP, enum D6S_ADC adcType,
                            u_int16 **measTypeP, u_int8 *noChanP, u_int8 *cmdP)
{
    D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    switch(adcType)
    {
        case eadc_senses:
            *cmdP = D6S_ADC_SENSE_VAL;
            *noChanP = D6S_NO_ADC_SENSES;
            *measTypeP = bmcP->adc.senses;
            break;

        case eadc_voltages:
            *cmdP = D6S_ADC_VOLTAGE_VAL;
            *noChanP = D6S_NO_ADC_VOLTAGES;
            *measTypeP = bmcP->adc.voltages;
            break;

        case eadc_temps:
            *cmdP = D6S_ADC_TEMP_VAL;
            *noChanP = D6S_NO_ADC_TEMPS;
            *measTypeP = bmcP->adc.temps;
            break;

        case eadc_vid:
            *cmdP = D6S_ADC_VID_VAL;
            *noChanP = 1;
            *measTypeP = &bmcP->adc.vid;
            break;

        default:
            printf("**** Invalid selection for ADC (%s, line %d) !\n", __FUNCTION__, __LINE__);
            return D6S_ERROR;
    }/* switch */

    if( *measTypeP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
	    return D6S_ERROR;
    }/* end if */

    return D6S_OK;
}/* LocMakeAdcCmd */

/**********************************************************************/
 /** routine to get an A/D converter value
  *
  *  This routine gets an A/D converter value (voltage/sense/temperature
  *  or voltage id).
  *
  *  \param bmcHdlP			BMC handle
  *  \param adcType			enumeration of A/D converter function
  *
  *  \return D6S_OK or D6S_ERROR
 */
u_int8 D6S_Adc ( BMC_HANDLE *bmcHdlP, enum D6S_ADC adcType )
{
    D6S_BMC_TS *bmcP = NULL;
    u_int16 *measTypeP = NULL;
    u_int16 measTemp = 0;
    u_int8 noChannels = 0;
    u_int8 command = 0;
    u_int8 i;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    if( LocMakeAdcCmd(bmcHdlP, adcType, &measTypeP, &noChannels, &command)
        != D6S_OK )
    {
        printf("**** Error making ADC command (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */


    for(i=1; i<(noChannels+1); i++)
    {
        measTemp = 0;

		/* fill buffer with data byte */
		bmcP->cmd.buffer[0] = command;
		if( adcType != eadc_vid )
		{
		    bmcP->cmd.buffer[1] = i;
		    bmcP->cmd.length = 2;
		}
		else
		{
		    bmcP->cmd.length = 1;
		}/* end if */
		LocReceiveMsg(bmcHdlP);

        LocGetValue(bmcHdlP, (u_int8 *)&measTemp, bmcP->cmd.length);

	    if( adcType == eadc_temps )
	    {/* convert to VID voltage */
	        *(measTypeP+i-1) = (int16)measTemp;
	    }
	    else
	    {
	        *(measTypeP+i-1) = measTemp;
	    }/* end if */
    }/* end for */


    return D6S_OK;
}/* D6S_Adc */

/**********************************************************************/
 /** routine to get the board information
  *
  *  This routine gets the board specific information.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return D6S_OK or D6S_ERROR
 */
u_int8 D6S_BoardInfo ( BMC_HANDLE *bmcHdlP )
{
	D6S_BMC_TS *bmcP = NULL;
	u_int32 measTemp = 0;
	u_int8 i;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

	/* read out status and permanent status register */
	D6S_BmcStatusRegister(bmcHdlP);

	/* fill buffer with command and data byte */
	bmcP->cmd.buffer[0] = D6S_BRD_VERSION_STRING; /* command byte */
	bmcP->cmd.length = 1; /* length */
	LocReceiveMsg(bmcHdlP);
	for(i=0; i<bmcP->cmd.length; i++)
	{
		if( i < 4 )
		{
			bmcP->brd.fwVersion[i] = bmcP->cmd.buffer[i];
		}
		else
		{
			printf("**** Illegal number of received bytes (%s, line %d) !\n", __FUNCTION__, __LINE__);
		}/* end if */
	}/* end for */

	measTemp = 0;
	bmcP->cmd.buffer[0] = D6S_BRD_CPCI_ADDR; /* command byte */
	bmcP->cmd.length = 1; /* length */
	LocReceiveMsg(bmcHdlP);
	LocGetValue(bmcHdlP, (u_int8 *)&measTemp, bmcP->cmd.length);
	bmcP->brd.cpciAddr = measTemp;

	measTemp = 0;
	bmcP->cmd.buffer[0] = D6S_BRD_FIRMWARE_CHECKSUM; /* command byte */
	bmcP->cmd.length = 1; /* length */
	LocReceiveMsg(bmcHdlP);
	LocGetValue(bmcHdlP, (u_int8 *)&measTemp, bmcP->cmd.length);
	bmcP->brd.fwChecksum = (u_int16)measTemp;

	measTemp = 0;
	bmcP->cmd.buffer[0] = D6S_BRD_PWR_ON_ERR; /* command byte */
	bmcP->cmd.length = 1; /* length */
	LocReceiveMsg(bmcHdlP);
	LocGetValue(bmcHdlP, (u_int8 *)&measTemp, bmcP->cmd.length);
	bmcP->brd.pwrOnErr =
	    (u_int16)(((measTemp<<8)&0xff00) | ((measTemp>>8)&0x00ff));

	return D6S_OK;
}/* D6S_BoardInfo */

/**********************************************************************/
 /** routine to get the BMC status and permanent status register
  *
  *  This routine gets the BMC status and permanent status register.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return D6S_OK or D6S_ERROR
 */
u_int8 D6S_BmcStatusRegister( BMC_HANDLE *bmcHdlP )
{
	D6S_BMC_TS *bmcP = NULL;
	u_int measTemp = 0;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
	    return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    /* fill buffer with command and data byte */
    measTemp = 0;
	bmcP->cmd.buffer[0] = D6S_BRD_STATUS_REG; /* command byte */
	bmcP->cmd.length = 1; /* length */
	LocReceiveMsg(bmcHdlP);
	LocGetValue(bmcHdlP, (u_int8 *)&measTemp, bmcP->cmd.length);
	bmcP->brd.statusReg = (u_int16)measTemp;

	measTemp = 0;
	bmcP->cmd.buffer[0] = D6S_BRD_PERM_STATUS_REG; /* command byte */
	bmcP->cmd.length = 1; /* length */
	LocReceiveMsg(bmcHdlP);
	LocGetValue(bmcHdlP, (u_int8 *)&measTemp, bmcP->cmd.length);
	bmcP->brd.permStatusReg = (u_int16)measTemp;

	return D6S_OK;
}/* D6S_BmcStatusRegister */

/**********************************************************************/
 /** routine to make a reset command
  *
  *  This routine makes a reset command.
  *
  *  \param bmcHdlP			BMC handle
  *  \param reset			enumeration of reset function
  *  \param cmdP		    BMC command identifier
  *  \param measTypeP		pointer to measurement structure
  *
  *  \return D6S_OK or D6S_ERROR
 */
static u_int8 LocMakeRstCmd(BMC_HANDLE *bmcHdlP, enum D6S_RST reset,
                            u_int8 *cmdP, u_int16 **measTypeP)
{
    D6S_BMC_TS *bmcP = NULL;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    switch(reset)
    {
        case erst_cause:
            *cmdP = D6S_RST_CAUSE;
            *measTypeP = bmcP->rst.cause;
            break;

        case erst_source:
            *cmdP = D6S_RST_CAUSE_SRC;
            *measTypeP = (u_int16 *)bmcP->rst.source;
            break;

        case erst_value:
		    *cmdP = D6S_RST_CAUSE_VAL;
		    *measTypeP = bmcP->rst.value;
            break;

        case erst_timestamp:
            *cmdP = D6S_RST_TIMESTAMP_VAL;
            break;

        default:
            printf("**** Invalid selection for reset information (%s, line %d) !\n", __FUNCTION__, __LINE__);
            return D6S_ERROR;
    }/* switch */

    return D6S_OK;
}/* LocMakeRstCmd */

/**********************************************************************/
 /** routine to get the reset history
  *
  *  This routine gets the reset history.
  *
  *  \param bmcHdlP			BMC handle
  *  \param reset			enumeration of reset function
  *
  *  \return D6S_OK or D6S_ERROR
 */
u_int8 D6S_ResetInfo( BMC_HANDLE *bmcHdlP, enum D6S_RST reset)
{
	D6S_BMC_TS *bmcP = NULL;
	u_int16 *measTypeP = NULL;
    u_int8 noChannels = 0;
    u_int8 command = 0;
    u_int8 i;

    if( bmcHdlP == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    if( LocMakeRstCmd(bmcHdlP, reset, &command, &measTypeP) != D6S_OK )
    {
        printf("**** Error making reset command (%s, line %d) !\n", __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* end if */

    noChannels = D6S_NO_RESETS;

    for(i=1; i<(noChannels+1); i++)
    {
		/* fill buffer with data byte */
		bmcP->cmd.length = 2;
		bmcP->cmd.buffer[0] = command;
		bmcP->cmd.buffer[1] = i;
		LocReceiveMsg(bmcHdlP);

		/* analyse received data */
        /* store cause/source or value */
		if( reset != erst_timestamp )
		{
		    if( measTypeP == NULL )
            {
                printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
                return D6S_ERROR;
            }/* end if */
		    LocGetValue(bmcHdlP, (u_int8 *)&measTypeP[i-1], bmcP->cmd.length);
		}
		else
		{
		    LocGetValue(bmcHdlP, (u_int8 *)&bmcP->rst.timestamp[i-1], bmcP->cmd.length);
		}/* end if */
    }/* end for */

    return D6S_OK;
}/* D6S_ResetInfo */

/**********************************************************************/
 /** routine to proceed a cyclic board parameter measurement
  *
  *  This routine proceeds periodically the measurement of the board
  *  board status parameter. It is used by the example application
  *  'd6Status'.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return no return value
 */
static void LocMeasureTask(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;
    u_int8     i;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return ;
	}/* end if */

    bmcP = (D6S_BMC_TS *)bmcHdlP;

    D6S_BoardInfo(bmcHdlP);

    for(i=(erst_start+1); i<erst_end; i++)
	{
		D6S_ResetInfo(bmcHdlP, i);
	}/* end for */

	for(i=(ewdg_start+1); i<ewdg_trigger_wr; i++)
	{
		D6S_Watchdog(bmcHdlP, i, 0);
	}/* end for */

	i = 0;
    while(  bmcP->measTaskId != 0 )
    {
	    if( bmcP->cmdTaskId != 0 )
	    {/* measurement order */

            if( bmcP->stopCylicMeas == D6S_FALSE )
            {
                D6S_BmcStatusRegister(bmcHdlP);

                LocMsecDelay(D6S_TASK_DELAY_MS);

	    	    if( (i >=(eadc_start+1)) &&
	                (i < eadc_end) )
	            {
                    D6S_Adc(bmcHdlP, i);
	            }/* end if */
	            if( i > eadc_end )
        	    {
	                i = 0;
	            }
	            else
	            {
	                i++;
	            }/* end if */

	            if( i == eadc_start )
	            {
	                bmcP->measRdy = D6S_TRUE;
	            }/* end if */

			    if( ((bmcP->brd.statusReg&D6S_HOT_SWITCH) == D6S_HOT_SWITCH) &&
			        (bmcP->measTaskId != 0) )
                {/* set Error */
                    printf("W A R N I N G: Hot switch detected, system %s",
                           "will shutdown in 10 seconds !\n");
                    bmcP->measTaskId = 0;
                    bmcP->cmdTaskId = 0;
                    bmcP->wdgTaskId = 0;
                    bmcP->stopCylicMeas = TRUE;
                }/* end if */
            }/* end if */
	    }
	    else
	    {
			printf("**** Command process module not started (%s, line %d) !\n", __FUNCTION__, __LINE__);
			bmcP->measRdy = D6S_FALSE;
			bmcP->measTaskId = 0;
		}/* end if */

	    LocMsecDelay(D6S_TASK_DELAY_MS);
    }/* end while */

    bmcP->measRdy = D6S_FALSE;
}/* LocMeasureTask */

/**********************************************************************/
 /** routine to initialize the cyclic measurement
  *
  *  This routine initializes the cyclic measurement of the board
  *  parameter.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  measurement task ID
 */
int16 D6S_StartMeasurement(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return 0;
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;

	if( bmcP->measTaskId != 0 )
	{
		printf("INFO: Measurement task already started (%s, line %d) !\n", __FUNCTION__, __LINE__);
	}
	else
	{/* start task */
    	if((bmcP->measTaskId = taskSpawn ("tMeasD6",
			     5,		    /* priority */
			     0,		    /* task options */
			     0x1000, 	/* 4k stack space */
			     (FUNCPTR)LocMeasureTask,
			     (int)bmcP, 0, 0, 0, 0, 0, 0, 0, 0, 0 )) == D6S_ERROR)
		{
    	    bmcP->measTaskId = 0;
    	    printf("**** Error spawning task 'tMeasD6' (%s, line %d) !\n", __FUNCTION__, __LINE__);
    	    return  bmcP->measTaskId;
		}/* end if */
	}/* end if */

	return bmcP->measTaskId;
}/* D6S_StartMeasurement */

/**********************************************************************/
 /** routine to stop the cyclic measurement
  *
  *  This routine stops the cyclic measurement of the board
  *  parameter.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  measurement task ID
 */
int32 D6S_StopMeasurement(BMC_HANDLE *bmcHdlP)
{
	D6S_BMC_TS *bmcP = NULL;
	u_int16 repeatCnt = 0;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;

    bmcP->stopCylicMeas = D6S_TRUE;
    bmcP->measTaskId = 0;

    while( bmcP->measRdy != D6S_FALSE )
    {
        LocMsecDelay(D6S_TASK_DELAY_MS);
        if( repeatCnt++ == D6S_MEAS_TIMEOUT_CNT )
        {
            bmcP->measRdy = D6S_FALSE;
            break;
        }/* end if */
    }/* end while */

	return bmcP->measTaskId;
}/* D6S_StopMeasurement */

/**********************************************************************/
 /** routine to put the board information to stdout
  *
  *  This routine puts the board information to stdout.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  D6S_OK or D6S_ERROR
 */
u_int8 D6S_ShowBoardInfo(BMC_HANDLE *bmcHdlP)
{
	D6S_BMC_TS *bmcP = NULL;
    u_int16 i;
    u_int16 iPwrOn = 0;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;


	printf("\nB O A R D   I N F O R M A T I O N\n");
	printf("=================================\n");
	printf("firmware version  : %02d.%02d\n",
		bmcP->brd.fwVersion[2], bmcP->brd.fwVersion[3]);
	printf("cpci address      : %d\n",
		bmcP->brd.cpciAddr);
	printf("firmware checksum : 0x%04x\n",
		bmcP->brd.fwChecksum);

	printf("power on error    : 0x%02x\n",
		bmcP->brd.pwrOnErr);

    for( i=0; i<D6S_PWRON_INDEX; i++ )
    {
        if( pwrOnStr[i].value == bmcP->brd.pwrOnErr )
        {
            iPwrOn = i;
            break;
        }/*end if */
    }

    if( pwrOnStr[iPwrOn].string2P != NULL )
    {
        printf("                    STATUS ==> %s: %s\n\n",
            pwrOnStr[iPwrOn].string1P,
            pwrOnStr[iPwrOn].string2P);
    }
    else
    {
        printf("                    ==> %s\n\n",
            pwrOnStr[iPwrOn].string1P);
    }/* end if */

    return D6S_OK;
}/* D6S_ShowBoardInfo */

/**********************************************************************/
 /** routine to make the status register string output
  *
  *  This routine makes the status register string.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  D6S_OK or D6S_ERROR
 */
static u_int8 LocShowStatusRegister(u_int8 maxItems, u_int16 *regP,
                                    D6S_STAT_STRING *statusInfoP)
{
    u_int8 i;

    for(i=0; i<maxItems; i++)
    {
        printf("     %-21s ", statusInfoP[i].stringP);
	    if(  (statusInfoP[i].value&(*regP)) ==
	        statusInfoP[i].value )
		{
		    printf("x\n");
		}
		else
		{
		    printf("o\n");
		}/* end if */
	}/* end for */
	printf("\n");

    return D6S_OK;
}

/**********************************************************************/
 /** routine to put the status register content to stdout
  *
  *  This routine puts the status register content to stdout.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  D6S_OK or D6S_ERROR
 */
u_int8 D6S_ShowStatRegs(BMC_HANDLE *bmcHdlP)
{
	D6S_BMC_TS *bmcP = NULL;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;

	printf("\nS T A T U S   R E G I S T E R\n");
	printf("=============================\n");

	printf("status register          : 0x%04x\n\n",
		 bmcP->brd.statusReg );

    LocShowStatusRegister((u_int8)D6S_MAX_STAT_REG, (u_int16 *)&bmcP->brd.statusReg,
        (D6S_STAT_STRING *)&statusRegStr[0]);

    printf("permanent status register: 0x%04x\n\n",
		 bmcP->brd.permStatusReg );
    LocShowStatusRegister((u_int8)D6S_MAX_PERM_STAT_REG, (u_int16 *)&bmcP->brd.permStatusReg,
        (D6S_STAT_STRING *)&permStatusRegStr[0]);

    return D6S_OK;
}/* D6S_ShowStatRegs */

/**********************************************************************/
 /** routine to put the watchdog parameter to stdout
  *
  *  This routine puts watchdog parameter to stdout.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  D6S_OK or D6S_ERROR
 */
u_int8 D6S_ShowWatchdog(BMC_HANDLE *bmcHdlP)
{
	D6S_BMC_TS *bmcP = NULL;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;

	printf("\nW A T C H D O G\n");
	printf("===============\n");
	if( bmcP->wdg.enable == 0 )
	{
		printf("state: %s\n", wdgStatusStr[0]);
	}
	else
	{
		printf("state: %s\n", wdgStatusStr[1]);
	}/* end if */

	printf("prescaler: %3d    timeout: %3d\n\n",
		bmcP->wdg.prescaler, bmcP->wdg.timeout);

    return D6S_OK;
}/* D6S_ShowWatchdog */

/**********************************************************************/
 /** routine to put the reset history to stdout
  *
  *  This routine puts the reset history to stdout.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  D6S_OK or D6S_ERROR
 */
u_int8 D6S_ShowRstHistory(BMC_HANDLE *bmcHdlP)
{
	D6S_BMC_TS *bmcP = NULL;
	u_int8      i, j;
	u_int16     iCause = 0;
	u_int8      iSource = 0;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;

	printf("R E S E T   H I S T O R Y\n");
	printf("=========================\n");
	printf("        cause               source        value    timestamp\n");
	printf("------------------------------------------------------------\n");

	for(i=0; i<D6S_NO_RESETS; i++)
	{
        iSource = bmcP->rst.source[i];
	    iCause  = 0;

		    for(j=0; j<D6S_RST_SRC_INDEX; j++)
		    {
		        if( rstCauseStr[j].value == bmcP->rst.cause[i] )
		        {
		            iCause = j;
		            break;
		        }/* end if */
		    }/* end for */

            if( (iSource == 0) &&
	            ((rstCauseStr[iCause].value == 0x200) ||
	             (rstCauseStr[iCause].value == 0x400) ||
	             (rstCauseStr[iCause].value == 0x800)) ){
	            iSource = 1;
	        }
	        else if( iSource > 0 ) {
	            iSource += 1;
	        }
	        else {
	            iSource = 0;
	        }/* end if */

			printf("%-21s    %-13s   %5d     %8d\n",
				rstCauseStr[iCause].stringP,
				rstSourceStr[iSource],
				bmcP->rst.value[i],
			    (u_int32)bmcP->rst.timestamp[i]);
	}/* end for */

    printf("\n\n");

    return D6S_OK;
}/* D6S_ShowRstHistory */

/**********************************************************************/
 /** routine to put the board voltages to stdout
  *
  *  This routine puts the board voltages to stdout.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  D6S_OK or D6S_ERROR
 */
u_int8 D6S_ShowVoltages(BMC_HANDLE *bmcHdlP)
{
	D6S_BMC_TS *bmcP = NULL;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;

	printf("\nV O L T A G E\n");
	printf("=============\n");

	printf("3V  = %6d mV    5V  = %6d mV    12V = %6d mV\n",
		bmcP->adc.voltages[D6S_VOLT_L3V_ID],
		bmcP->adc.voltages[D6S_VOLT_L5V_ID],
		bmcP->adc.voltages[D6S_VOLT_12V_ID]);

	printf("L3V = %6d mV    L5V = %6d mV    2V5 = %6d mV\n",
		bmcP->adc.voltages[D6S_VOLT_L3V_ID],
		bmcP->adc.voltages[D6S_VOLT_L5V_ID],
		bmcP->adc.voltages[D6S_VOLT_2_5V_ID]);

	printf("1V2 = %6d mV    1V5 = %6d mV    1V8 = %6d mV\n",
	    bmcP->adc.voltages[D6S_VOLT_1_2V_ID],
		bmcP->adc.voltages[D6S_VOLT_1_5V_ID],
		bmcP->adc.voltages[D6S_VOLT_1_8V_ID]);
	printf("1V1 = %6d mV    0V9 = %6d mV\n",
		bmcP->adc.voltages[D6S_VOLT_1_1V_ID],
		bmcP->adc.voltages[D6S_VOLT_0_9V_ID]);

	printf("VTT  = %6d mV    CORE = %6d mV    DDR  = %6d mV\n",
	bmcP->adc.voltages[D6S_VOLT_VTT_ID],
	bmcP->adc.voltages[D6S_VOLT_CORE_ID],
	bmcP->adc.voltages[D6S_VOLT_DDR_ID]);

	printf("VID  = %5d mV\n", bmcP->adc.vid);

	return D6S_OK;
}/* D6S_ShowVoltages */

/**********************************************************************/
 /** routine to put the board temperatures to stdout
  *
  *  This routine puts the board temperatures to the stdout.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  D6S_OK or D6S_ERROR
 */
u_int8 D6S_ShowTemperatures(BMC_HANDLE *bmcHdlP)
{
	D6S_BMC_TS *bmcP = NULL;
	u_int16 cpuTemp = 0;
	const u_int8 degree = 0xb0;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;

	printf("T E M P E R A T U R E\n");
	printf("=====================\n");
  	printf("ENV = %4d %cC    HOT = %4d %cC    DEI = %4d %cC\n",
  	bmcP->adc.temps[D6S_ENV_TMP_ID], degree,
  	bmcP->adc.temps[D6S_HOT_TMP_ID], degree,
  	bmcP->adc.temps[D6S_DIE_TMP_ID], degree);
	printf("-----------------\n");

	cpuTemp = bmcP->adc.temps[D6S_CPU_TMP_ID];
	printf("| CPU = %3d.%1d %cC |\n",
		(cpuTemp/10),
		(cpuTemp-((cpuTemp/10)*10)), degree);
	printf("-----------------\n");

	return D6S_OK;
}/* D6S_ShowTemperatures */

/**********************************************************************/
 /** routine to put the board currents to stdout
  *
  *  This routine puts the board currents to stdout.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  D6S_OK or D6S_ERROR
 */
u_int8 D6S_ShowCurrents(BMC_HANDLE *bmcHdlP)
{
	D6S_BMC_TS *bmcP = NULL;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;

    printf("C U R R E N T\n");
	printf("=============\n");
	printf("3V  = %5d mA    5V  = %5d mA    12V = %5d mA\n",
	    bmcP->adc.senses[D6S_SENSE_3V_ID],
	    bmcP->adc.senses[D6S_SENSE_5V_ID],
	    bmcP->adc.senses[D6S_SENSE_12V_ID]);

    return D6S_OK;
}/* D6S_ShowCurrents */

/**********************************************************************/
 /** routine to trigger the BMC watchdog
  *
  *  This routine triggers the BMC watchdog.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  D6S_OK or D6S_ERROR
 */
u_int8 D6S_TriggerWatchdog(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;
    u_int8 status = D6S_OK;
    u_int8 triggerPattern = D6S_WDG_PATTERN_2;

	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;
    bmcP->stopCylicMeas = D6S_TRUE;
	printf("W A R N I N G: Watchdog was triggered !\n");
    while( bmcP->wdgTaskId != 0 )
    {
        /*while( (bmcP->cmdTaskId == 0) ||
               (bmcP->measTaskId == 0) )
        {
            LocMsecDelay(D6S_TASK_DELAY_MS);
        }*//* end while */

        if(	triggerPattern == D6S_WDG_PATTERN_1 )
	    {
	        triggerPattern = D6S_WDG_PATTERN_2;
	    }
	    else
	    {/* default/start trigger pattern */
	        triggerPattern = D6S_WDG_PATTERN_1;
	    }/* end if */

        status = D6S_Watchdog(bmcHdlP, ewdg_trigger_wr, triggerPattern);

        LocMsecDelay(D6S_TASK_DELAY_MS);
    }

    return status;
}/* D6S_TriggerWatchdog */

/**********************************************************************/
 /** routine to show all board parameter
  *
  *  This routine puts all board parameter to stdout.
  *
  *  \param bmcHdlP			BMC handle
  *
  *  \return  D6S_OK or D6S_ERROR
 */
static u_int8 LocShowAll(BMC_HANDLE *bmcHdlP)
{
	if( bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

	D6S_ShowBoardInfo(bmcHdlP);
    D6S_ShowVoltages(bmcHdlP);
    D6S_ShowTemperatures(bmcHdlP);
    D6S_ShowCurrents(bmcHdlP);
    D6S_ShowStatRegs(bmcHdlP);
    D6S_ShowWatchdog(bmcHdlP);
    D6S_ShowRstHistory(bmcHdlP);

	return D6S_OK;
}/* LocShowAll */

/**********************************************************************/
 /** routine to initialize the demo application
  *
  *  This routine initializes the demo application.
  *
  *  \param bmcHdlP			BMC handle
  *  \param manufacturer	manufacturer identification
  *
  *  \return  D6S_OK or D6S_ERROR
 */
static u_int8 LocInitApi(BMC_HANDLE *bmcHdlP, u_int8 manufacturer)
{
    BMC_HANDLE tmpHdlP = NULL;

	tmpHdlP = D6S_Init(manufacturer);

	if( tmpHdlP == NULL )
	{
	    printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

    *bmcHdlP = (D6S_BMC_TS *)tmpHdlP;

	D6S_StartMeasurement(*bmcHdlP);

    if( *bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}
    else
    {
        return D6S_OK;
    }/* end if */
}/* LocInitApi */

/**********************************************************************/
 /** routine to de-initialize the demo application
  *
  *  This routine de-initializes the demo application.
  *
  *
  *  \return  D6S_OK or D6S_ERROR
 */
static u_int8 LocDeInitApi(void)
{
    D6S_BMC_TS *bmcP = NULL;

	bmcP = (D6S_BMC_TS *)G_bmcHdl;
	D6S_StopMeasurement((BMC_HANDLE)bmcP);
	D6S_DeInit((BMC_HANDLE)&bmcP);
	G_bmcHdl = NULL;
	G_options = 0;

	return D6S_OK;
}/* LocDeInitApi */

/**********************************************************************/
 /** routine to handle the measurement initialization timeout
  *
  *  This routine handles the measurement initialization timeout.
  *
  *  \param bmcHdlP		BMC handle
  *
  *  \return  D6S_OK or D6S_ERROR
 */
static u_int8 LocTimeoutMeasRdy(BMC_HANDLE *bmcHdlP)
{
    D6S_BMC_TS *bmcP = NULL;
    u_int16 repeatCnt = 0;

    if( *bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;

    while( bmcP->measRdy != D6S_TRUE )
    {
        LocMsecDelay(D6S_TASK_DELAY_MS);

        if( repeatCnt++ == D6S_MEAS_TIMEOUT_CNT )
        {
            bmcP->measRdy = D6S_FALSE;
            printf("**** Measurement ready timeout !\n");
            return D6S_ERROR;
        }/* end if */
    }/* end while */

    return D6S_OK;
}/* LocTimeoutMeasRdy */

/**********************************************************************/
 /** routine to set the watchdog adjustable parameter
  *
  *  This routine sets the watchdog adjustable parameter.
  *
  *  \param bmcHdlP		BMC handle
  *  \param watchdog	enumeration of watchdog function
  *
  *  \return  D6S_OK or D6S_ERROR
 */
static u_int8 LocSetWatchdogValue(BMC_HANDLE *bmcHdlP, enum D6S_EWDG watchdog)
{
    D6S_BMC_TS *bmcP = NULL;
    u_int8      value = 0;

    if( *bmcHdlP == NULL )
	{
		printf("**** Nullpointer assignment (%s, line %d) !\n", __FUNCTION__, __LINE__);
		return D6S_ERROR;
	}/* end if */

	bmcP = (D6S_BMC_TS *)bmcHdlP;

	if( G_options == D6S_ALCATEL )
	{
	    printf("**** Feature not supported !\n");
	}
	else
	{
	    LocGetByte(&value);
	    bmcP->stopCylicMeas = D6S_TRUE;
	    D6S_Watchdog(bmcHdlP, watchdog, value);
	    bmcP->stopCylicMeas = D6S_FALSE;
	}/* end if */

    return D6S_OK;
}

/**********************************************************************/
 /** routine to show the demo application usage
  *
  *  This routine shows the demo application usage.
  *
  *
  *  \return  D6S_OK or D6S_ERROR
 */
static void LocUsage()
{
	printf("Usage: d6Status [opt1], [opt2], [value]\n");
	printf("Function: Show BMC status parameter\n");
	printf("Option 1: 0xF0: stop all used tasks\n");
	printf("          0x00: only measurement (no output)\n");
	printf("          0x01: voltages\n");
	printf("          0x02: senses\n");
	printf("          0x03: temperatures\n");
	printf("          0x04: board information\n");
	printf("          0x05: BMC status registers\n");
	printf("          0x06: watchdog parameter\n");
	printf("          0x07: reset history\n");
	printf("          0x08: trigger watchdog (start/stop)\n");
	printf("          0x09: set watchdog prescaler value\n");
	printf("          0x0A: set watchdog timeout value\n");
	printf("\n");
	printf("          0x0B: get all information\n");
	printf("\n");
	printf("\n");
	printf("Option 2:    0: MEN FPGA \n");
	printf("             1: CUSTOMER FPGA \n");
	printf("\n");
	printf("(c) 2006 by MEN mikro elektronik GmbH\n");
	printf("Version %s\n", D6S_VERSION_S);
	printf("\n");
} /* LocUsage */

/**********************************************************************/
 /** main routine to execute module functionality
  *
  *  This routine executes d6Status functionality.
  *
  * /param options			function option
  * /param manufacturer		manufacturer ID
  *
  *  \return  BMC_HANDLE
 */
u_int8 d6Status(u_int8 options, u_int8 manufacturer)
{
	D6S_BMC_TS *bmcP = NULL;

    G_options = options;

    /* initialize Measurement */
    if( G_bmcHdl == NULL )
    {
        printf("\n");

        if( LocInitApi((BMC_HANDLE)&bmcP, manufacturer) != D6S_OK )
        {
            printf("**** d6Status initialization error (%s, line %d) !\n", __FUNCTION__, __LINE__);
            return D6S_ERROR;
        }/* end if */
        G_bmcHdl = (BMC_HANDLE)bmcP;
    }
    else
    {
        bmcP = (D6S_BMC_TS *)G_bmcHdl;
    }/* end if */

    if( G_bmcHdl == NULL )
    {
        printf("**** Nullpointer assignment (%s, line %d) !\n",
            __FUNCTION__, __LINE__);
        return D6S_ERROR;
    }/* endif */

    if( LocTimeoutMeasRdy((BMC_HANDLE)bmcP) != D6S_OK )
    {
        printf("**** Measurement initialization error(%s, line %d) !\n", __FUNCTION__, __LINE__);
        LocDeInitApi();
        return D6S_ERROR;
    }/* end if */
	switch(options)
	{
	    case 0x00:
	        /* just initialize measurement */
	        break;
		case 0x01:
		    D6S_ShowVoltages((BMC_HANDLE)bmcP);
			break;
		case 0x02:
		    D6S_ShowCurrents((BMC_HANDLE)bmcP);
			break;
		case 0x03:
		    D6S_ShowTemperatures((BMC_HANDLE)bmcP);
			break;
		case 0x04:
		    D6S_ShowBoardInfo((BMC_HANDLE)bmcP);
			break;
		case 0x05:
		    D6S_ShowStatRegs((BMC_HANDLE)bmcP);
			break;
		case 0x06:
		    bmcP->stopCylicMeas = D6S_TRUE;
		    D6S_Watchdog((BMC_HANDLE)bmcP, ewdg_prescaler, 0);
	        D6S_Watchdog((BMC_HANDLE)bmcP, ewdg_timeout, 0);
	        bmcP->stopCylicMeas = D6S_FALSE;
		    D6S_ShowWatchdog((BMC_HANDLE)bmcP);
			break;
		case 0x07:
		    D6S_ShowRstHistory((BMC_HANDLE)bmcP);
			break;
		case 0x08:
		    if( bmcP->wdgTaskId == 0 )
		    {/* start triggering watchdog */
		        /* spawn task */
		        if((bmcP->wdgTaskId = taskSpawn ("tWdgTrig",
			            1,		    /* highest priority */
    			        0,		    /* task options */
			            0x1000, 	/* 4k stack space */
			            (FUNCPTR)D6S_TriggerWatchdog,
			            (int)bmcP, 0, 0, 0, 0, 0, 0, 0, 0, 0 )) == D6S_ERROR )
		        {
    	            bmcP->wdgTaskId = 0;
    	            printf("error spwaning task 'tWdgTrig' (%s, line %d) !\n", __FUNCTION__, __LINE__);
    	            return  D6S_ERROR;
		        }/* end if */
		    }
		    else
		    {
		        bmcP->wdgTaskId = 0;
		    }/* end if */
			break;

	    case 0x09:
	        LocSetWatchdogValue((BMC_HANDLE)bmcP, ewdg_setPrescaler);
	        break;

	    case 0x0A:
	        LocSetWatchdogValue((BMC_HANDLE)bmcP, ewdg_setTimeout);
	        break;

	    case 0xF0:
            if( LocDeInitApi() != D6S_OK )
            {
                printf("**** d6Status de-initialization error (%s, line %d) !\n", __FUNCTION__, __LINE__);
                return D6S_ERROR;
            }/* end if */
	        break;

        case 0x0B:
            LocShowAll((BMC_HANDLE)bmcP);
            break;
        case '?':
		default:
		    LocUsage();
			break;
	}/* end switch */

	if( G_bmcHdl == NULL )
    {
        printf("\nType 'd6Status ?' to see the available options.\n");
    }/* end if */
    return D6S_OK;
}/* d6Check */

u_int8 d6GetBmcHandle(BMC_HANDLE **bmcHdlP)
{
    if( G_bmcHdl == NULL )
    {
        printf("**** Nullpointer assignment !\n");
        return D6S_ERROR;
    }/* end if */

    bmcHdlP = G_bmcHdl;

    return D6S_OK;
}/* D6S_GetBmcHandle */
