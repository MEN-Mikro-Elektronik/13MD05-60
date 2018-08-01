/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  em04touchcalib.c
 *
 *      \author  rla
 *        $Date: 2006/06/02 09:47:01 $
 *    $Revision: 2.4 $
 *
 *        \brief  routines for the touch panel calibration (ADS78XX)
 *
 *            This module supplies routines to perform a calibration
 *				  - EM04TOUCH_CALIB_UL
 *					performs a calibration of the upper left edge
 *				  - EM04TOUCH_CALIB_LR
 *					performs a calibration of the lower right edge
 *				  - EM04TOUCH_CALIB_CALC
 *					calculate calibration parameters for offset and delta
 *
 *            Load stored calibration parameters form EEPROM
 *				  - EM04TOUCH_CALIB_LOAD
 *
 *            Save stored calibration parameters to EEPROM
 *				  - EM04TOUCH_CALIB_SAVE
 *					store settings in EEPROM
 *
 *     Switches: OS9000		to compile module for OS9000 use
 *				 MENMON		to compile under MENMON
 *				 USE_IRQ	to compile driver to work with interrupts
 *				 EM04TOUCH_USE_OLD_SYSPARAM to include sysparam.h
 *               EM04TOUCH_CALIB_REDUCED_SCREEN  perform touch calibration on
                                                 reduced screen size (94%)
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: em04touchcalib.c,v $
 * Revision 2.4  2006/06/02 09:47:01  cs
 * added support for calibration on reduced screen size
 *
 * Revision 2.3  2006/03/01 17:27:39  UFRANKE
 * cosmetics
 *
 * Revision 2.2  2005/06/23 16:12:13  kp
 * Copyright line changed (sbo)
 *
 * Revision 2.1  2005/05/11 16:32:47  kp
 * allow negative calibration data for swapped touch panels
 *
 * Revision 2.0  2005/02/18 16:07:09  kp
 * complete reimplementation
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2002-2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

/*--------------------------------------*/
/*    INCLUDES                          */
/*--------------------------------------*/
#include "em04touch_int.h"

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define EM04_TOUCH_CALIB_STD_SCALE	0xFFF /**< max measuring value */
#define ABS(x) ((x)>=0 ? (x):(-(x)))

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static EM04TOUCH_ERRCODE CalibTouch		( EM04_TOUCH_HANDLE *th, int mode );
static EM04TOUCH_ERRCODE CalibSaveEEprom( EM04_TOUCH_HANDLE *th );
static EM04TOUCH_ERRCODE CalibLoadEEprom( EM04_TOUCH_HANDLE *th );
static EM04TOUCH_ERRCODE CalibCalc		( EM04_TOUCH_HANDLE *th);
static void CalibSetDefaults( EM04_TOUCH_HANDLE *th );


/**********************************************************************/
/** Main Routine for calibrate the touch panel.
 *  This routine can be called to calibrate the connected touch panel
 *  It calculates the x and y scale and offset values for the current
 *  used hardware and stores/loads the calibration parameters to/from
 *  EEPROM
 *
 *  \param th		pointer to EM04_TOUCH_HANDLE structure
 *  \param mode		calibration mode
 *					- EM04TOUCH_CALIB_LOAD  	load settings from EEPROM
 *					- EM04TOUCH_CALIB_SAVE 		do calibration
 *					- EM04TOUCH_CALIB_UL		find upper left egde
 *					- EM04TOUCH_CALIB_LR		find lower right edge
 *					- EM04TOUCH_CALIB_CALC		calculate new calibration parameter
 *
 *  \return EM04TOUCH_SUCCESS or error code
 */
EM04TOUCH_ERRCODE EM04TOUCH_Calibrate(EM04TOUCH_HANDLE *thp, int mode)
{
	EM04_TOUCH_HANDLE *th = (EM04_TOUCH_HANDLE*)thp;
	int32 error = 0;

	DBGWRT_1((DBH,"em04touchdrv:calibrate mode = %d\n",mode ));

	switch(mode)
	{
		case EM04TOUCH_CALIB_LOAD:
			/* Load calibration data from EEPROM if failed load standard */
			if ( (error = CalibLoadEEprom(th)) != EM04TOUCH_SUCCESS)
				CalibSetDefaults( th );
			break;

		case EM04TOUCH_CALIB_SAVE:
			/* Save calibration data in EEPROM */
			error = CalibSaveEEprom(th);
			break;

		case EM04TOUCH_CALIB_UL:
			error = CalibTouch(th, mode );
			break;
		case EM04TOUCH_CALIB_LR:
			error = CalibTouch(th, mode );
			break;
		case EM04TOUCH_CALIB_CALC:
			error = CalibCalc(th);
			break;
	}/* end switch */

	return error;
}

/**********************************************************************/
/** LowLevel routine to save CALIBRATION data to EEPROM
 *
 *  \param thp		pointer to EM04_TOUCH_HANDLE structure
 *
 *	\return EM04TOUCH_SUCCESS or EM04TOUCH_CALIB_SAVE_ERR
 */
static EM04TOUCH_ERRCODE CalibSaveEEprom( EM04_TOUCH_HANDLE *th )
{
	int error = 0;
	char param[EM04_TOUCH_CAL_PARAM_SIZE];

	int xdelta=0,ydelta=0,xoffset=0,yoffset=0;

	xdelta = th->calibdat.xdelta;
	ydelta = th->calibdat.ydelta;
	xoffset = th->calibdat.xoffset;
	yoffset = th->calibdat.yoffset;

	sprintf(param,"%u,%u,%u,%u",xoffset,xdelta,yoffset,ydelta);

	DBGWRT_1((DBH, "CalibSaveEEprom:: dump param %s\n",param));

#ifndef OS9000
	/* set touch calibration parameter */
	error = SysParamSet("c-tcal", param, NULL);
#else
	error = _osmen_sysparam_set("c-tcal", param, NULL);
#endif

	if (error )
	{
		DBGWRT_ERR((DBH, "EM04TouchCalib:: SysParamSet failed, "
					"error=0x%x\n",error));
		return EM04TOUCH_ERROR;
	}

	return EM04TOUCH_SUCCESS;
}

static void CalibSetDefaults( EM04_TOUCH_HANDLE *th )
{
	/* set default values */
	th->calibdat.xdelta = EM04_TOUCH_CALIB_STD_SCALE;
	th->calibdat.xoffset = 0;
	th->calibdat.ydelta = EM04_TOUCH_CALIB_STD_SCALE;
	th->calibdat.yoffset = 0;
}

/**********************************************************************/
/** LowLevel routine to load CALIBRATION data from EEPROM
 *
 *  \param thp		pointer to EM04_TOUCH_HANDLE structure
 *
 *	\return EM04TOUCH_SUCCESS or EM04TOUCH_CALIB_LOAD_ERR
 */
static EM04TOUCH_ERRCODE CalibLoadEEprom( EM04_TOUCH_HANDLE *th )
{
	int i, h=0, j, c, error = 0;
	char param[EM04_TOUCH_CAL_PARAM_SIZE];
	int itmp[4];
	char ctmp[10];

	/* get touch calibration parameter from EEPORM */
#if defined( OS9000 )
	error = _osmen_sysparam_get("c-tcal", param,
								(int)EM04_TOUCH_CAL_PARAM_SIZE);
#else
	error = SysParamGet("c-tcal", param, (int)EM04_TOUCH_CAL_PARAM_SIZE);
#endif

	if (error)
	{
		DBGWRT_ERR((DBH, "EM04TouchCalib:: SysParamGet failed, "
					"error=0x%x\n",error));
		return EM04TOUCH_ERROR;
	}

	DBGWRT_1((DBH, "CalibLoadEEprom:: dump param %s\n",param));

	/* get data from string */
	for (j=0;j<4;j++)
	{
		for (i=0; i<10; i++)
		{
			if( (param[h+i] == ',') || (param[h+i] == '\0' ) )
			{
				ctmp[i] = '\0';
				break;
			}
			else
			{
				ctmp[i] = param[h+i];
			}
		}/* end for i */

		/* convert ASCII value ti integer */
		itmp[j] = (int)strtol(ctmp,(char**)NULL,10);
		h = h + (i+1);

		/*clear ctmp */
		for (c=0;c<10;c++)
			ctmp[c] = 0x00;

	}/* end for j */

	/* set calibration data */
	if( ABS(itmp[0]) <= EM04_TOUCH_CALIB_STD_SCALE )
		th->calibdat.xoffset = itmp[0];
	else
	{
		th->calibdat.xoffset = 0;
		error = 1;
	}

	if( ABS(itmp[1]) <= EM04_TOUCH_CALIB_STD_SCALE)
		th->calibdat.xdelta = itmp[1];
	else
	{
		th->calibdat.xdelta = EM04_TOUCH_CALIB_STD_SCALE;
		error = 1;
	}

	if( ABS(itmp[2]) <= EM04_TOUCH_CALIB_STD_SCALE)
		th->calibdat.yoffset = itmp[2];
	else
	{
		th->calibdat.yoffset = 0;
		error = 1;
	}

	if( ABS(itmp[3]) <= EM04_TOUCH_CALIB_STD_SCALE)
		th->calibdat.ydelta = itmp[3];
	else
	{
		th->calibdat.ydelta = EM04_TOUCH_CALIB_STD_SCALE;
		error = 1;
	}

	if (error)
		return EM04TOUCH_CALIB_LOAD_ERR;
	else
		return EM04TOUCH_SUCCESS;

}


/**********************************************************************/
/** LowLevel routine to calibrate upper left and lower right edges
 *
 *  \param thp		pointer to EM04_TOUCH_HANDLE structure
 *
 *	\return EM04TOUCH_SUCCESS
 */
static EM04TOUCH_ERRCODE CalibTouch( EM04_TOUCH_HANDLE *th, int mode )
{
	EM04_TOUCH_CAL savedCalibDat = th->calibdat;
	EM04TOUCH_ERRCODE rv = EM04TOUCH_ERROR;

	/* during calibration, apply default parameters */
	CalibSetDefaults( th );

	if( EM04TOUCH_ActionPerform( th,
								 TOUCH_ACT_CALIBEDGE, OSS_SEM_WAITFOREVER )
		== EM04TOUCH_SUCCESS ){

		if( mode == EM04TOUCH_CALIB_UL )
		{
			th->calibdat.pos_ul = th->calibdat.current;
		}
		else
		{
			th->calibdat.pos_lr = th->calibdat.current;
		}
		rv = 0;
	}

	/* restore previous calib data */
	th->calibdat.xdelta = savedCalibDat.xdelta;
	th->calibdat.ydelta = savedCalibDat.ydelta;
	th->calibdat.xoffset= savedCalibDat.xoffset;
	th->calibdat.yoffset= savedCalibDat.yoffset;

	return rv;
}

/**********************************************************************/
/** LowLevel routine to calculate calibration parameter
 *  There are 4 parameters that will be calculate and stored in
 *  EM04_TOUCH_HANDLE::calibdat
 *      - xoffset	min measured x value(raw)
 *      - yoffset	min measured y value(raw)
 *      - xdelta	max measured x value(raw) - xoffset
 *      - ydelta	max measured y value(raw) - yoffset
 *
 *  \param thp		pointer to EM04TOUCH_HANDLE structure
 *
 *  \return EM04TOUCH_SUCCESS
 *
 */
static EM04TOUCH_ERRCODE CalibCalc( EM04_TOUCH_HANDLE *th)
{
	/* calculate Scale factor */
	th->calibdat.xdelta = th->calibdat.pos_ul.x - th->calibdat.pos_lr.x;
	th->calibdat.ydelta = th->calibdat.pos_ul.y - th->calibdat.pos_lr.y;

#ifdef EM04TOUCH_CALIB_REDUCED_SCREEN
	/* consider reduced calibration size */
	th->calibdat.xdelta += th->calibdat.xdelta / 25;	/* add the 4% */
	th->calibdat.ydelta += th->calibdat.xdelta * 6/100;	/* add the 4% */

	th->calibdat.pos_ul.x += th->calibdat.xdelta / 50;   /* +2% of width */
	th->calibdat.pos_ul.y += th->calibdat.ydelta * 3/100;/* +3% of height */
	th->calibdat.pos_lr.x -= th->calibdat.xdelta / 50;	 /* -2% of width */
	th->calibdat.pos_lr.y -= th->calibdat.ydelta * 3/100;/* -3% of height */
#endif /* EM04TOUCH_CALIB_REDUCED_SCREEN */

	/* calculate offset */
	th->calibdat.xoffset = th->calibdat.pos_lr.x;
	th->calibdat.yoffset = th->calibdat.pos_lr.y;

	DBGWRT_1((DBH,"em04touchdrv:calibrate::calc xdelta=%d\n",
			  th->calibdat.xdelta));
	DBGWRT_1((DBH,"em04touchdrv:calibrate::calc xoffset=%d\n",
			  th->calibdat.xoffset));
	DBGWRT_1((DBH,"em04touchdrv:calibrate::calc ydelta=%d\n",
			  th->calibdat.ydelta));
	DBGWRT_1((DBH,"em04touchdrv:calibrate::calc yoffset=%d\n",
			  th->calibdat.yoffset));

	return EM04TOUCH_SUCCESS;
}





