/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: sysMenTouchDrv.h
 *
 *       Author: cs
 *        $Date: 2007/03/03 02:11:05 $
 *    $Revision: 2.2 $
 *
 *  Description: Header for systouchDrv.c - Driver for touch panel
 *
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sysMenTouchDrv.h,v $
 * Revision 2.2  2007/03/03 02:11:05  cs
 * added: ioctl TOUCH_CALIBRATE
 *
 * Revision 2.1  2006/03/22 15:05:10  cs
 * Initial Revision
 *
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef _SYSMENTOUCHDRV_H
#define _SYSMENTOUCHDRV_H

#ifdef __cplusplus
	extern "C" {
#endif


/*--------------------------------------*/
/*	TYPEDEFS     	                    */
/*--------------------------------------*/
typedef struct
{
	u_int8 sync;		/**< magic TOUCH_SYNC if data are valid */
	u_int8 penStatus;	/**< status of pen (UP or DOWN) */
	int x;				/**< x co-ordinate of pen */
	int y;				/**< y co-ordinate of pen */
} TOUCH_PACKET;

/*--------------------------------------*/
/* DEFINES      	                    */
/*--------------------------------------*/

#define TOUCH_DEFAULT_XRES		640
#define TOUCH_DEFAULT_YRES		480
#define TOUCH_TRESHOLD_MAX		100
#define TOUCH_TRESHOLD_MIN		0
#define TOUCH_TRESHOLD_DEFAULT	10
#define TOUCH_MOVE_DEFAULT		1

#define TOUCH_TRESHOLD_SET		0xA0
#define TOUCH_GS_VBAT			0xA1
#define TOUCH_GS_TEMP			0xA2
#define TOUCH_GS_TEMP_RAW		0xA3
#define TOUCH_CALIBRATE			0xA4

#define TOUCH_PRESSED			0x85
#define TOUCH_RELEASED			0x87
#define TOUCH_PEN_UP			6
#define TOUCH_PEN_DOWN			5
#define TOUCH_SYNC				0xA5

#ifdef __cplusplus
	}
#endif

#endif	/* _SYSMENTOUCHDRV_H */



