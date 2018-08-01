/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  em04touchcalib.c
 *
 *      \author  rla
 *        $Date: 2012/09/21 17:41:12 $
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
 * $Log: em04touchcalib_ham.c,v $
 * Revision 2.4  2012/09/21 17:41:12  ts
 * R: diab compiler complained about const out of range
 * M: casting constant passed as return value with (int)
 *
 * Revision 2.3  2012/09/21 17:17:51  ts
 * R: diab compiler fails about variables declared inside code
 * M: moved variable declaration to begin of function
 *
 * Revision 2.2  2012/03/02 17:18:57  ts
 * R: warning about C3 specific calibration was left
 * M: removed warning
 *
 * Revision 2.1  2012/02/24 19:39:39  ts
 * Initial checkin for customer specific Calibration method
 *
 * R: 1. calibration values and consideration is calibration method specific
 *    2. default/error state in switch statement was missing
 * M: 1. move functions xxxRawToPix from em04touchdrv.c to here
 *    2. add default statement to EM04TOUCH_Calibrate() mode switch
 *
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
#include "em04touch_int_ham.h"
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
static EM04TOUCH_ERRCODE EM04TOUCH_GetPixFromRaw( EM04TOUCH_HANDLE *thP, EM04_TOUCH_POS *rawP, EM04_TOUCH_POS *calibP );


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

			/* This should always be called first during driver initialization */
			th->getPixFromRaw = EM04TOUCH_GetPixFromRaw;
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
#ifdef EM04TOUCH_HAMILTON_DO_C3_CORRECTION
		case EM04TOUCH_CALIB_LL:
			error = CalibTouch(th, mode );
			break;
		case EM04TOUCH_CALIB_UR:
			error = CalibTouch(th, mode );
			break;
#endif /* EM04TOUCH_HAMILTON_DO_C3_CORRECTION */
		case EM04TOUCH_CALIB_CALC:
			error = CalibCalc(th);
			break;
		default:
			error = (int)EM04TOUCH_ERROR;
			break;
	}/* end switch */

	return error;
}

/**********************************************************************/
/** Retreive calibrated values from raw Pixel values.
 *
 *  \param thp		pointer to EM04_TOUCH_HANDLE structure
 *  \param rawP		pointer to raw Pixel
 *  \param calibP	pointer to calibrated Pixel
 *
 *  \return EM04TOUCH_SUCCESS or error code
 */
static EM04TOUCH_ERRCODE EM04TOUCH_GetPixFromRaw( EM04TOUCH_HANDLE *thP, EM04_TOUCH_POS *rawP, EM04_TOUCH_POS *calibP )
{
	EM04_TOUCH_HANDLE *th = (EM04_TOUCH_HANDLE *)thP;
	int tmp_int=0;
	int pixX = 0, pixY = 0;
	DBGWRT_3((DBH,"em04touchdrv:raw2pix pos (rawX/rawY)= %d/%d\n",(int)rawP->x, (int)rawP->y ));
	DBGWRT_4((DBH,"em04touchdrv:raw2pix pos (rawX/rawY)= %d/%d cal: xo/xd=%d/%d; yo/yd=%d/%d; ham_corrX1/ham_corrX2=%d/%d; ham_corrY1/ham_corrY2=%d/%d;\n",
												(int)rawP->x, (int)rawP->y,
												(int)th->calibdat.xoffset, (int)th->calibdat.xdelta,
												(int)th->calibdat.yoffset, (int)th->calibdat.ydelta,
												(int)th->calibdat.ham_corr.x1, (int)th->calibdat.ham_corr.x2,
												(int)th->calibdat.ham_corr.y1, (int)th->calibdat.ham_corr.y2 ));

	pixX = rawP->x - th->calibdat.xoffset;
	pixX = pixX * th->init.ds_xres;
	calibP->x = pixX / th->calibdat.xdelta;

	pixY = rawP->y - th->calibdat.yoffset;
	pixY = pixY * th->init.ds_yres;
	calibP->y = pixY / th->calibdat.ydelta;

#ifdef EM04TOUCH_HAMILTON_DO_C3_CORRECTION
    
    /* offset on right side depends on y-position */
    tmp_int = (calibP->y * th->calibdat.ham_corr.x2 / th->init.ds_yres);
    calibP->x = calibP->x - tmp_int;
	
    /* factor on left side includes the offset above 
	   and also depends on y-position */
 	tmp_int = th->calibdat.ham_corr.x1 + th->calibdat.ham_corr.x2;
	tmp_int = calibP->y * tmp_int / th->init.ds_yres;
	tmp_int = th->init.ds_xres + th->calibdat.ham_corr.x1 - tmp_int;
	if(tmp_int != 0)
	{
		calibP->x = calibP->x * th->init.ds_xres / tmp_int;
	}

    /* offset on bottom side depends on x-position */
    tmp_int = (calibP->x * th->calibdat.ham_corr.y1 / th->init.ds_xres);
    calibP->y = calibP->y - tmp_int;

    /* factor on top side includes the offset above 
	   and also depends on x-position */
 	tmp_int = th->calibdat.ham_corr.y1 + th->calibdat.ham_corr.y2;
	tmp_int = calibP->x * tmp_int / th->init.ds_xres;
	tmp_int = th->init.ds_yres + th->calibdat.ham_corr.y2 - tmp_int;
	if(tmp_int != 0)
	{
		calibP->y = calibP->y * th->init.ds_yres / tmp_int;
	}
	
#endif /* EM04TOUCH_HAMILTON_DO_C3_CORRECTION */

	return( EM04TOUCH_SUCCESS );
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
	int ham_corrX1=0, ham_corrX2=0, ham_corrY1=0, ham_corrY2=0;

	xdelta = th->calibdat.xdelta;
	ydelta = th->calibdat.ydelta;
	xoffset = th->calibdat.xoffset;
	yoffset = th->calibdat.yoffset;
	ham_corrX1 = th->calibdat.ham_corr.x1;
	ham_corrX2 = th->calibdat.ham_corr.x2;
	ham_corrY1 = th->calibdat.ham_corr.y1;
	ham_corrY2 = th->calibdat.ham_corr.y2;

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

	sprintf(param,"%d",ham_corrX1);
	error = SysParamSet("c-ham_corrX1", param, NULL);
	if (error )
	{
		DBGWRT_ERR((DBH, "EM04TouchCalib:: SysParamSet (ham_corrX1) failed, "
					"error=0x%x\n",error));
		return EM04TOUCH_ERROR;
	}
	sprintf(param,"%d",ham_corrX2);
	error = SysParamSet("c-ham_corrX2", param, NULL);
	if (error )
	{
		DBGWRT_ERR((DBH, "EM04TouchCalib:: SysParamSet (ham_corrX2) failed, "
					"error=0x%x\n",error));
		return EM04TOUCH_ERROR;
	}
	sprintf(param,"%d",ham_corrY1);
	error = SysParamSet("c-ham_corrY1", param, NULL);
	if (error )
	{
		DBGWRT_ERR((DBH, "EM04TouchCalib:: SysParamSet (ham_corrY1) failed, "
					"error=0x%x\n",error));
		return EM04TOUCH_ERROR;
	}
	sprintf(param,"%d",ham_corrY2);
	error = SysParamSet("c-ham_corrY2", param, NULL);
	if (error )
	{
		DBGWRT_ERR((DBH, "EM04TouchCalib:: SysParamSet (ham_corrY2) failed, "
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
	th->calibdat.ham_corr.x1 = 0;
	th->calibdat.ham_corr.x2 = 0;
	th->calibdat.ham_corr.y1 = 0;
	th->calibdat.ham_corr.y2 = 0;
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
	char param_ham_corrX1[16];
	char param_ham_corrX2[16];
	char param_ham_corrY1[16];
	char param_ham_corrY2[16];

	int itmp[8];
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

	error = SysParamGet("c-ham_corrX1", param_ham_corrX1, (int)16);
	if (error)
	{
		DBGWRT_ERR((DBH, "EM04TouchCalib:: SysParamGet(ham_corrX1) failed, "
					"error=0x%x\n",error));
		return EM04TOUCH_ERROR;
	}

	error = SysParamGet("c-ham_corrX2", param_ham_corrX2, (int)16);
	if (error)
	{
		DBGWRT_ERR((DBH, "EM04TouchCalib:: SysParamGet(ham_corrX2) failed, "
					"error=0x%x\n",error));
		return EM04TOUCH_ERROR;
	}

	error = SysParamGet("c-ham_corrY1", param_ham_corrY1, (int)16);
	if (error)
	{
		DBGWRT_ERR((DBH, "EM04TouchCalib:: SysParamGet(ham_corrY1) failed, "
					"error=0x%x\n",error));
		return EM04TOUCH_ERROR;
	}

	error = SysParamGet("c-ham_corrY2", param_ham_corrY2, (int)16);
	if (error)
	{
		DBGWRT_ERR((DBH, "EM04TouchCalib:: SysParamGet(ham_corrY2) failed, "
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

		/* convert ASCII value to integer */
		itmp[j] = (int)strtol(ctmp,(char**)NULL,10);
		h = h + (i+1);

		/*clear ctmp */
		for (c=0;c<10;c++)
			ctmp[c] = 0x00;

	}/* end for j */

	/* get ham_corrX1 from string */
	{
		for (i=0; i<10; i++)
		{
			if( param_ham_corrX1[i] == '\0' )
			{
				ctmp[i] = '\0';
				break;
			}
			else
			{
				ctmp[i] = param_ham_corrX1[i];
			}
		}/* end for i */

		/* convert ASCII value to integer */
		itmp[4] = (int)strtol(ctmp,(char**)NULL,10);

		/*clear ctmp */
		for (c=0;c<10;c++)
			ctmp[c] = 0x00;
	}
	/* get ham_corrX2 from string */
	{
		for (i=0; i<10; i++)
		{
			if( param_ham_corrX2[i] == '\0' )
			{
				ctmp[i] = '\0';
				break;
			}
			else
			{
				ctmp[i] = param_ham_corrX2[i];
			}
		}/* end for i */

		/* convert ASCII value to integer */
		itmp[5] = (int)strtol(ctmp,(char**)NULL,10);

		/*clear ctmp */
		for (c=0;c<10;c++)
			ctmp[c] = 0x00;
	}
	/* get ham_corrY1 from string */
	{
		for (i=0; i<10; i++)
		{
			if( param_ham_corrY1[i] == '\0' )
			{
				ctmp[i] = '\0';
				break;
			}
			else
			{
				ctmp[i] = param_ham_corrY1[i];
			}
		}/* end for i */

		/* convert ASCII value to integer */
		itmp[6] = (int)strtol(ctmp,(char**)NULL,10);

		/*clear ctmp */
		for (c=0;c<10;c++)
			ctmp[c] = 0x00;
	}
	/* get ham_corrY2 from string */
	{
		for (i=0; i<10; i++)
		{
			if( param_ham_corrY2[i] == '\0' )
			{
				ctmp[i] = '\0';
				break;
			}
			else
			{
				ctmp[i] = param_ham_corrY2[i];
			}
		}/* end for i */

		/* convert ASCII value to integer */
		itmp[7] = (int)strtol(ctmp,(char**)NULL,10);

		/*clear ctmp */
		for (c=0;c<10;c++)
			ctmp[c] = 0x00;
	}

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

	if( ABS(itmp[4]) < EM04_TOUCH_CALIB_STD_SCALE)
		th->calibdat.ham_corr.x1 = itmp[4];
	else
	{
		th->calibdat.ham_corr.x1 = 0;
		error = 1;
	}

	if( ABS(itmp[5]) < EM04_TOUCH_CALIB_STD_SCALE)
		th->calibdat.ham_corr.x2 = itmp[5];
	else
	{
		th->calibdat.ham_corr.x2 = 0;
		error = 1;
	}

	if( ABS(itmp[6]) < EM04_TOUCH_CALIB_STD_SCALE)
		th->calibdat.ham_corr.y1 = itmp[6];
	else
	{
		th->calibdat.ham_corr.y1 = 0;
		error = 1;
	}

	if( ABS(itmp[7]) < EM04_TOUCH_CALIB_STD_SCALE)
		th->calibdat.ham_corr.y2 = itmp[7];
	else
	{
		th->calibdat.ham_corr.y2 = 0;
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
#ifdef EM04TOUCH_HAMILTON_DO_C3_CORRECTION
	EM04_TOUCH_HAM_CORR savedHamCorr = th->calibdat.ham_corr;
#endif /* EM04TOUCH_HAMILTON_DO_C3_CORRECTION */

	/* during calibration, apply default parameters */
	CalibSetDefaults( th );

	if( EM04TOUCH_ActionPerform( th,
								 TOUCH_ACT_CALIBEDGE, OSS_SEM_WAITFOREVER )
		== EM04TOUCH_SUCCESS ){

		switch( mode )
		{
			case( EM04TOUCH_CALIB_UL ):
			{
				th->calibdat.pos_ul = th->calibdat.current;
				break;
			}
			case( EM04TOUCH_CALIB_LL ):
			{
				th->calibdat.pos_ll = th->calibdat.current;
				break;
			}
			case( EM04TOUCH_CALIB_UR ):
			{
				th->calibdat.pos_ur = th->calibdat.current;
				break;
			}
			default:
			{
				th->calibdat.pos_lr = th->calibdat.current;
				break;
			}
		}
		rv = 0;
	}

	/* restore previous calib data */
	th->calibdat.xdelta = savedCalibDat.xdelta;
	th->calibdat.ydelta = savedCalibDat.ydelta;
	th->calibdat.xoffset= savedCalibDat.xoffset;
	th->calibdat.yoffset= savedCalibDat.yoffset;
#ifdef EM04TOUCH_HAMILTON_DO_C3_CORRECTION
	th->calibdat.ham_corr.x1 =savedHamCorr.x1;
	th->calibdat.ham_corr.x2 =savedHamCorr.x2;
	th->calibdat.ham_corr.y1 =savedHamCorr.y1;
	th->calibdat.ham_corr.y2 =savedHamCorr.y2;
#endif /* EM04TOUCH_HAMILTON_DO_C3_CORRECTION */

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
	th->calibdat.ydelta += th->calibdat.ydelta * 6/100;	/* add the 6% */

	th->calibdat.pos_ul.x += th->calibdat.xdelta / 50;   /* +2% of width */
	th->calibdat.pos_ul.y += th->calibdat.ydelta * 3/100;/* +3% of height */
	th->calibdat.pos_lr.x -= th->calibdat.xdelta / 50;	 /* -2% of width */
	th->calibdat.pos_lr.y -= th->calibdat.ydelta * 3/100;/* -3% of height */
#endif /* EM04TOUCH_CALIB_REDUCED_SCREEN */

	/* calculate offset */
	th->calibdat.xoffset = th->calibdat.pos_lr.x;
	th->calibdat.yoffset = th->calibdat.pos_lr.y;

#ifdef EM04TOUCH_HAMILTON_DO_C3_CORRECTION
	/* calculate values for additional HAM C3 calibration correction */

	/* use new calibration values to calculate actual 
	   pixel values of points upper right and lower left */
	/* from there calculate currection necessary to get to given point */
	{
		EM04_TOUCH_POS llPix, urPix, rawPos;
		int retVal;

		/* eliminate this correction first */
		th->calibdat.ham_corr.x1 = 0;
		th->calibdat.ham_corr.x2 = 0;
		th->calibdat.ham_corr.y1 = 0;
		th->calibdat.ham_corr.y2 = 0;

		/* calculate actual pixel values */
		rawPos = th->calibdat.pos_ll;
		retVal = th->getPixFromRaw(th, &rawPos, &llPix);
		if( EM04TOUCH_SUCCESS != retVal ){
			DBGWRT_2((DBH," ERROR calculating pixel values for ll corner %d/%d\n", rawPos.x, rawPos.y ));
		}

		/* calculate actual pixel values */
		rawPos = th->calibdat.pos_ur;
		retVal = th->getPixFromRaw(th, &rawPos, &urPix);
		if( EM04TOUCH_SUCCESS != retVal ){
			DBGWRT_2((DBH," ERROR calculating pixel values for ur corner %d/%d\n", rawPos.x, rawPos.y ));
		}
											    /*tft position X of LL cross*/
		th->calibdat.ham_corr.x1 = ((llPix.x - (th->init.ds_xres * 98/100 )) * 100) / 97;
											    /* tft position X of UR cross */
		th->calibdat.ham_corr.x2 = ((urPix.x - (th->init.ds_xres *  2/100 )) * 100) / 97;
											    /*tft position Y of LL cross*/
		th->calibdat.ham_corr.y1 = ((llPix.y - (th->init.ds_yres *  3/100 )) * 100) / 98;
											    /*tft position Y of UR cross*/
		th->calibdat.ham_corr.y2 = ((urPix.y - (th->init.ds_yres * 97/100 )) * 100) / 98;

		DBGWRT_1((DBH,"em04touchdrv:calibrate::calc ham_corrX1/2=%d/%d\n", th->calibdat.ham_corr.x1, th->calibdat.ham_corr.x2));
	}
#endif /* EM04TOUCH_HAMILTON_DO_C3_CORRECTION */

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





