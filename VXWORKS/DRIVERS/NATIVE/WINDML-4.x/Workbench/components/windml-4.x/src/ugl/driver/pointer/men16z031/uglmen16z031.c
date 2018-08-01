/***********************  I n c l u d e  -  F i l e  ************************
 *
 *        \file  uglmen16z031.c
 *
 *      \author  Christian.Schuster@men.de
 *        $Date: 2015/08/17 09:10:01 $
 *    $Revision: 1.6 $
 *
 *       \brief  WindML MEN 16Z031_SPI touch screen handler
 *
 *    \switches  none
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: uglmen16z031.c,v $
 * Revision 1.6  2015/08/17 09:10:01  ts
 * R: device /graphics(23456789,98765432)/0 not instantiated under 6.8.3, 6.9
 * M: reverted menSysWindMLDevGet/Ctrl back to sysWindML... because MEN specific
 *    functions were not called by Wind ML
 *
 * Revision 1.5  2011/09/07 19:33:01  ts
 * R: windML port to vxWorks 6.8 necessary
 * M: added support for sysWindML functions in 6.8 BSPs
 *
 * Revision 1.4  2007/10/01 10:11:09  CSchuster
 * changed:
 *   - use uglLog of severity UGL_ERR_TYPE_WARN instead of *_FATAL when
 *     uglIoMen16z031TsDrv() fails (e.g. no 16Z031_SPI in FPGA, ...)
 *     reason: root task is suspended on startup when uglLog(..FATAL,) is posted
 *             all usrApplicationInit(), usrApplicationRtpInit() is not executed!
 *
 * Revision 1.3  2007/03/03 01:36:13  cs
 * fixed:
 *    - pass unknown requests to lower levels
 *
 * Revision 1.2  2006/01/26 10:53:45  cs
 * moved sysWindMLDevGet from uglMen16Z031TsOpen to uglIoMen16Z031TsDrv
 * minor changes for compilation in RTP
 *
 * Revision 1.1  2005/12/19 19:25:24  cs
 * Initial Revision
 *
 * Revision 1.1  2005/07/19 13:57:19  CSchuster
 * Initial Revision
 *
 *
 * cloned from Wind River uwrsbcpxa250ts.h Revision 01a
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

/* udwrsbcpxa250ts.c - UGL wrSbcPxa250 Touch Screen Handler */

/* Copyright 2002 Wind River Systems, Inc. All Rights Reserved */


/*
modification history
--------------------
01a,04oct02,rfm  Created
*/


/*

DESCRIPTION

This file provides the wrSbcPxa250 touch screen protocol support for UGL. Note
that this driver depends upon the structure returned by the sysWrSbcPxa250Ts.c
serial driver.

*/

#include <string.h>
#include <stdlib.h>
#include <ioLib.h>

#include <ugl/uglos.h>
#include <ugl/uglmem.h>
#include <ugl/uglinput.h>
#include <ugl/uglMsg.h>
#include <ugl/sysWindML.h>
#include <ugl/ugllog.h>

/* typedefs and defines */
/* !!! keep in sync with sysMenTouchDrv.h and em04touch.h !!! */

/* namespace conflict in vxWorks 6.8 */
#if (_WRS_VXWORKS_MINOR >= 8 )
IMPORT WINDML_DEVICE * sysWindMLDevGet(UINT32 , UINT32 ,UINT32 ,UINT32);
#endif


typedef struct
{
	unsigned char sync;			/**< magic TOUCH_SYNC if data are valid */
	unsigned char penStatus;	/**< status of pen (UP or DOWN) */
	int x;						/**< x co-ordinate of pen */
	int y;						/**< y co-ordinate of pen */
} TOUCH_PACKET;

#define TOUCH_PRESSED			0x85
#define TOUCH_RELEASED			0x87
#define TOUCH_PEN_UP			6
#define TOUCH_PEN_DOWN			5
#define TOUCH_SYNC				0xA5

#define TOUCH_CALIBRATE         0xA4

/* Calibration is done in MENMON (Setup -> Touch Calibration) */
#define MEN16Z031_TS_MULT_X    (1)
#define MEN16Z031_TS_MULT_Y    (1)
#define MEN16Z031_TS_DELTA_X   (0)
#define MEN16Z031_TS_DELTA_Y   (0)

#undef DEBUG_TS

#ifdef DEBUG_TS
#include <stdio.h>
#endif

/****************************** Z031_Swap32 **********************************
 *
 *  Description:  Swap bytes in double word.
 *                cloned from oss_swap.c
 *
 *---------------------------------------------------------------------------
 *  Input......:  dword     double word to swap
 *  Output.....:  return    swapped word
 *  Globals....:  -
 ****************************************************************************/
UGL_UINT32 Z031_Swap32( UGL_UINT32 dword )
{
    return(  (dword>>24) | (dword<<24) |
             ((dword>>8) & 0x0000ff00) |
             ((dword<<8) & 0x00ff0000)   );
}

/*******************************************************************************
*
* uglMen16z031TsOpen - open a MEN 16Z031_SPI touch screen input device
*
* This routine initializes the touch screen controller for use with UGL.
* It opens the device <pDevName> and sets it to the proper mode.
*
*/

UGL_INPUT_DEV_ID uglMen16z031TsOpen
    (
    char * 		 pDevName,      /* name of device */
    UGL_INPUT_DRV *      pDriver	/* input driver */
    )
    {
    UGL_INPUT_DEV * pDevice;

    /* allocate the device descriptor */

    pDevice = (UGL_INPUT_DEV *) UGL_CALLOC (1, sizeof (UGL_INPUT_DEV));
    if (pDevice == UGL_NULL)
        return (UGL_NULL);

    /* open the device */

    pDevice->fd = open (pDevName, 2, 0);
    if (pDevice->fd < 0)
        {
        UGL_FREE (pDevice);
        return (UGL_NULL);
        }

    /* initialize device data */

    pDevice->deviceType = UGL_DEVICE_POINTER_ABSOLUTE;
    pDevice->pDriver = pDriver;

    return (pDevice);
    }

/*******************************************************************************
*
* uglMen16z031TsClose - close an MEN 16Z031_SPI touch screen device
*
*/

UGL_LOCAL UGL_STATUS uglMen16z031TsClose
    (
    UGL_INPUT_DEV * pDevice      /* device control structure */
    )
    {
    /* close the device */
    close (pDevice->fd);

    /* free local storage */
    UGL_FREE (pDevice);

    return UGL_STATUS_OK;
    }

/*******************************************************************************
*
* uglMen16z031TsControl - control  an MEN 16Z031_SPI touch screen device
*
*/

UGL_LOCAL UGL_STATUS uglMen16z031TsControl
    (
    UGL_INPUT_DEV *  pDevice,/* device control structure */
    UGL_DEVICE_REQ   request,     /* request to perform */
    void *           arg          /* argument for request */
    )
    {
    if(pDevice == UGL_NULL)
        return(UGL_STATUS_ERROR);

#ifdef DEBUG_TS
	printf( "uglMen16z031TsControl: %x arg %x\n",request,arg);
#endif

    switch (request)
	{
        case ICR_READ:    /* read PTR data */
            {
            TOUCH_PACKET packet;

            if( (read(pDevice->fd, (char *)&packet,
                    sizeof(TOUCH_PACKET)) ==
                 sizeof(TOUCH_PACKET)) &&
                (packet.sync == TOUCH_SYNC) )
            {
                UGL_MSG ptrMsg;

#ifdef DEBUG_TS
                printf( "WML_z031_touch_ICR_READ: %s (%4d;%4d)\n",
						(packet.penStatus == TOUCH_PEN_DOWN) ? "P" : "R",
						packet.x, packet.y);
#endif

                bzero ((char *)&ptrMsg, sizeof (ptrMsg));
                ptrMsg.type = MSG_RAW_PTR;
                ptrMsg.data.rawPtr.deviceId = pDevice;
                ptrMsg.data.rawPtr.isAbsolute = UGL_TRUE;
                ptrMsg.data.rawPtr.pos.absolute.x = packet.x;
                ptrMsg.data.rawPtr.pos.absolute.y = packet.y;

                if (packet.penStatus == TOUCH_PEN_DOWN)
                    {
                    ptrMsg.data.rawPtr.buttonState = UGL_PTR_BUTTON1;
                    }
                else
                    {
                    ptrMsg.data.rawPtr.buttonState = 0;
                    }

                uglInputMsgPost (pDevice->inputServiceId,
                                 &ptrMsg);
                }
            }
            break;

	case ICR_GET_PTR_TYPE:
	    {
	    if(arg != UGL_NULL)
		{
		*(int *)arg = UGL_PTR_TYPE_TOUCH_SCREEN;
		return(UGL_STATUS_OK);
		}
	    else
		{
		return(UGL_STATUS_ERROR);
		}
	    }
	    break;

	default:
	    {
		/* map known codes */
		if( request == ICR_CALIBRATE )
			request = TOUCH_CALIBRATE;
	    return (ioctl (pDevice->fd, request, (int)arg));
	    }
	}

    return (UGL_STATUS_OK);
    }

#ifdef _WRS_KERNEL
/**************************************************************************
*
* uglIoMen16z031TsDrv - Z031_TOUCH Pointer Driver initialization routine
* dummy, actual initialization is done by EM04Touch driver
*
*/
STATUS uglIoMen16z031TsDrv
    (
    char * devName,
    int param1,
    int param2
    )
    {
	WINDML_DEVICE *	pDev = NULL;

	/* creates device "/touchScreen/0" */

	/*  ts: sysWindMLDevGet renamed to sysWindMLDevGet due 
	 *  to name conflicts in 6.8. sysWindMLDevGet is defined
	 * 	in sysWindML.c 
	 */
	if ( !(pDev = sysWindMLDevGet (WINDML_TOUCHSCREEN_DEVICE, 0, 0, 0)) )	
    {
		uglLog( UGL_WARN_PROBE_FAIL, /* UGL_ERR_TYPE_WARN + x */
				"uglIoMen16z031TsDrv(): failed to create touch device\n",
				0,0,0,0,0);
    	return (UGL_STATUS_ERROR);
    }

    return OK;
    }
#endif /* _WRS_KERNEL */

/* MEN 16Z031_SPI touch screen Driver */

UGL_INPUT_DRV uglMen16z031TsDriver =
    {
    (UGL_INPUT_DEV_OPEN)uglMen16z031TsOpen,
    (UGL_INPUT_DEV_CLOSE)uglMen16z031TsClose,
    (UGL_INPUT_DEV_CONTROL)uglMen16z031TsControl
    };

