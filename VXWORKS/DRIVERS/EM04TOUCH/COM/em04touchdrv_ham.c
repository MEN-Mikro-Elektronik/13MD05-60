/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  em04touchdrv.c
 *
 *      \author  kp
 *        $Date: 2013/11/06 14:12:01 $
 *    $Revision: 2.8 $
 *
 *        \brief driver for the touch panel (ADS78XX)
 *
 * Supports ADS78XX touch controller connected to MEN FPGA Z031_SPI unit
 *
 *     Switches: OS9000		to compile module for OS9000 use
 *				 MENMON		to compile under MENMON
 *				 USE_IRQ	to compile driver to work with interrupts
 *               EM04TOUCH_REC_CYCLES     number of additional read cycles
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: em04touchdrv_ham.c,v $
 * Revision 2.8  2013/11/06 14:12:01  ts
 * R: leftover printf of X/Y position from touch tests appeared
 * M: commented out printf with X/Y position
 *
 * Revision 2.7  2012/03/02 11:11:59  ts
 * R: calibration values and consideration is calibration method specific
 * M: move functions xxxRawToPix to em04touchcalib.c
 *
 * Revision 2.6  2006/10/13 22:32:55  cs
 * added: (for 5-wire touch 1024x768, as last changes)
 *   - discard first and last value measured
 *   - use 3 values to detect first valid position
 *   - generate pressed events only on next PEN Irq, if touch still pressed
 *
 * Revision 2.5  2006/09/14 11:42:47  cs
 * fixed:
 *    - during recovery cycles measure VBat (enables voltage to touch planes)
 *
 * Revision 2.4  2006/06/02 09:47:11  cs
 * added option to support longer recovery times on touch (additional reads)
 *
 * Revision 2.3  2006/01/17 13:42:25  cs
 * fix: if values are outside of screen don't generate TouchEvent
 *      until now values where set to screen edge!
 *
 * Revision 2.2  2005/06/23 16:12:15  kp
 * Copyright line changed (sbo)
 *
 * Revision 2.1  2005/02/21 10:28:53  kp
 * support high level temperature measurement (degrees celsius)
 *
 * Revision 2.0  2005/02/18 16:09:19  kp
 * complete reimplementation
 * Fixes two main problems:
 * 1) Each second SPI transfer sent FFFFxx to SPI controller causing unexact
 * measurment (e.g. with Danielsen Touch)
 * 2) Each SPI transfer blocks PCI bus for approx 768us, causing unnecessary
 * load of CPU due to incorrect interaction with Z031 unit
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

/*--------------------------------------*/
/*    INCLUDES                          */
/*--------------------------------------*/
#include "em04touch_int_ham.h"

/*--------------------------------------*/
/*	TYPEDEFS     	                    */
/*--------------------------------------*/

/*--------------------------------------*/
/*	GLOBALS     	                    */
/*--------------------------------------*/

/*--------------------------------------*/
/*	DEFINES			            		*/
/*--------------------------------------*/
#define ABS(x) ((x)>=0 ? (x):(-(x)))

#define EM04_TEMPCONV_U_mV	((EM04_TOUCH_TEMPREF_VOLTAGE * EM04_TOUCH_FPAF) / \
	EM04_TOUCH_12BIT_CONV_VAL)
#define EM04_BATTCONV_U_mV	((EM04_TOUCH_BATTREF_VOLTAGE * EM04_TOUCH_FPAF) / \
	EM04_TOUCH_12BIT_CONV_VAL)


/*--------------------------------------*/
/*	PROTOTYPES     	                    */
/*--------------------------------------*/
static void TouchEnablePenIrq( EM04_TOUCH_HANDLE *th );
static void TouchDisablePenIrq( EM04_TOUCH_HANDLE *th );
static void TouchAckPenIrq( EM04_TOUCH_HANDLE *th );
static int TouchPenAsserted( EM04_TOUCH_HANDLE *th );
static void TouchEnableSpiIrq( EM04_TOUCH_HANDLE *th );
static void TouchDisableSpiIrq( EM04_TOUCH_HANDLE *th );
static void TouchAckSpiIrq( EM04_TOUCH_HANDLE *th );
static void TouchStEnPenEnter( EM04_TOUCH_HANDLE *th );
static void TouchStXYEnter( EM04_TOUCH_HANDLE *th, EM04_TOUCH_STATE state );
static void TouchStXYSpiXfer(
	EM04_TOUCH_HANDLE *th,
	EM04_TOUCH_STATE state,
	u_int32 subState );
static void TouchSpiStart( EM04_TOUCH_HANDLE *th, u_int32 cmd, int phase );
static u_int16 TouchSpiXferValueGet( EM04_TOUCH_HANDLE *th );
static void TouchPenIrq( EM04_TOUCH_HANDLE *th);
static void TouchSpiIrq( EM04_TOUCH_HANDLE *th );
static int TouchValidXYRawValue( u_int16 raw );
static int TouchValidXYPixelValues(
	EM04_TOUCH_HANDLE *th,
	EM04_TOUCH_POS *pos,
	u_int16 x2 );
static void TouchStXYMeasFinished( EM04_TOUCH_HANDLE *th, u_int16 x2 );
static void TouchEventGenerate(
	EM04_TOUCH_HANDLE *th,
	u_int8 pressFlag,
	u_int16 x,
	u_int16 y );
static void TouchStIdleEnter( EM04_TOUCH_HANDLE *th );
static void TouchCriticalSectionLeave( EM04_TOUCH_HANDLE *th );
static void TouchCriticalSectionEnter( EM04_TOUCH_HANDLE *th );
static EM04TOUCH_ERRCODE TouchActionRequest(
	EM04_TOUCH_HANDLE *th,
	EM04_TOUCH_ACTION action );
static EM04TOUCH_ERRCODE TouchActionWait(
	EM04_TOUCH_HANDLE *th,
	int32 time);
static void TouchActionKick( EM04_TOUCH_HANDLE *th );
static void TouchSignalWaiter( EM04_TOUCH_HANDLE *th );
static int32 TouchMeasureTemperature( EM04TOUCH_HANDLE *thp );


/**********************************************************************/
/** Initialize touch driver
 *  The routine creates the EM04_TOUCH_HANDLE, enables the internal
 *  FPGA interrupts and prepare the hardware for operation
 *
 *  \param thP			pointer to EM04_TOUCH_HANDLE pointer
 *  \param tih			pointer to EM04TOUCH_INIT structure
 *
 *  \return 0=ok, or error code (EM04TOUCH_xxx)
 */
EM04TOUCH_ERRCODE EM04TOUCH_Init(
	EM04TOUCH_INIT *tih,
 	EM04TOUCH_HANDLE **thP)
{
	EM04_TOUCH_HANDLE *th;
	MACCESS ma;
	u_int32 hGotSize;
	int32 error =0;

	/*------------------+
    |  allocate handle  |
    +------------------*/
	if( (th = (EM04_TOUCH_HANDLE *)
	   OSS_MemGet( tih->osh, sizeof(EM04_TOUCH_HANDLE), &hGotSize )) == NULL )
		return EM04TOUCH_OUT_OF_RESOURCE;

	OSS_MemFill( tih->osh, sizeof(*th), (char*)th, 0 );

	th->osh = tih->osh;
	th->hGotSize = hGotSize;
	th->init = *tih;
	*thP = th;

	DBGWRT_1((DBH,"em04touchdrv:Init touchhandle=0x%x size=%d Byte\n",
			  th,hGotSize));

	/*-------------------+
    |  get base address  |
    +-------------------*/
	ma = th->init.base;
	DBGWRT_1((DBH,"em04touchdrv:Init baseaddr=0x%x\n",ma));
	DBGWRT_1((DBH,"em04touchdrv:Init xres=%d, yres=%d\n",
			  tih->ds_xres,tih->ds_yres));

	/*-------------------+
    |  create semaphore  |
    +-------------------*/
	if( (error = OSS_SemCreate(th->osh, OSS_SEM_BIN, 0, &th->irqSem)) )
	{
		DBGWRT_ERR((DBH,"*** TOUCH::Init: can't create IRQ sem err=%d ***\n",
				error ));
		return EM04TOUCH_OUT_OF_RESOURCE;
	}

	/*--------------------------------+
    |  set touch threshold parameter  |
    +---------------------------------*/

    if (th->init.ds_thresh == 0)
    	th->init.ds_thresh = EM04_TOUCH_DEF_THOLD;


	DBGWRT_1((DBH,"em04touchdrv:Init pixel threshold=%d, flags=%x\n",
			  th->init.ds_thresh, th->init.ds_flags));

    /* load calibration parameter */
	EM04TOUCH_Calibrate(th, EM04TOUCH_CALIB_LOAD);

	th->ierShadow = 0;
	MWRITE_D8( ma, Z031_IER_REG, th->ierShadow );

	/* send pen irq enable cmd */
	TouchStEnPenEnter( th );

	return 0;
}

/**********************************************************************/
/** Routine to deinitialise the touch controller.
 *  The routine destroys the EM04_TOUCH_HANDLE
 *  \param thP			pointer to EM04_TOUCH_HANDLE pointer
 *
 */
void EM04TOUCH_Exit( EM04TOUCH_HANDLE **thP )
{
	EM04_TOUCH_HANDLE *th = (EM04_TOUCH_HANDLE*)*thP;

	TouchDisablePenIrq( th );
	TouchDisableSpiIrq( th );

	/* destroy semaphore */
	if( th->irqSem ) OSS_SemRemove( th->osh, &th->irqSem );

	/*--- free handle ---*/
	OSS_MemFree( th->osh, (char *)th, th->hGotSize);

	*thP = NULL;
}

/**********************************************************************/
/** Routine to get the touch controller ID
 *
 *  \param thp		pointer to EM04_TOUCH_HANDLE structure
 *
 *  \return Touch ID
 */
int EM04TOUCH_GetID(EM04TOUCH_HANDLE *thp)
{
	EM04_TOUCH_HANDLE *th = (EM04_TOUCH_HANDLE*)thp;
	u_int16 id = 0;

	if( EM04TOUCH_ActionPerform( th, TOUCH_ACT_GETID, 1000 ) ==
		EM04TOUCH_SUCCESS )
		id = th->auxValue;

    return id;
}

/**********************************************************************/
/** Routine to measure the temperature and battery
 *  This routine measures the value for temperature or battery depending
 *  on the given device
 *
 *  \param thp		pointer to EM04_TOUCH_HANDLE structure
 *  \param device	EM04TOUCH_DEV_xxxx
 *
 *  \return measured value or 0xFFFFFFFF for error
 */
u_int32 EM04TOUCH_MeasureAuxInp(EM04TOUCH_HANDLE *thp, u_int8 device)
{
	EM04_TOUCH_HANDLE *th = (EM04_TOUCH_HANDLE*)thp;
	EM04_TOUCH_ACTION action = TOUCH_ACT_NONE;
	u_int32 rv = EM04TOUCH_ERROR;

	switch( device ){
	case EM04TOUCH_DEV_TEMP:
		rv = (u_int32)TouchMeasureTemperature( th );
		goto EXIT;
	case EM04TOUCH_DEV_BATT:
		action = TOUCH_ACT_BAT;
		break;
	case EM04TOUCH_DEV_TEMP_RAW_LO:
		action = TOUCH_ACT_TEMP0;
		break;
	case EM04TOUCH_DEV_TEMP_RAW_HI:
		action = TOUCH_ACT_TEMP1;
		break;
	default:
		return EM04TOUCH_ERROR;
	}

	if( EM04TOUCH_ActionPerform( th, action, 1000 ) == EM04TOUCH_SUCCESS )
		rv = th->auxValue;

	switch( device ){
	case EM04TOUCH_DEV_BATT:
		if( rv != EM04TOUCH_ERROR )
			rv = rv * 4 * EM04_BATTCONV_U_mV / EM04_TOUCH_FPAF;
		break;
	default:
		break;
	}
 EXIT:
	return rv;
}

/**********************************************************************/
/** Measure temperature
 *
 *  \param th		touch device handle
 *
 *  \return measured temperature (1/10 degress celsius) or 10000 if failure
 */
static int32 TouchMeasureTemperature( EM04TOUCH_HANDLE *thp )
{
	EM04_TOUCH_HANDLE *th = (EM04_TOUCH_HANDLE*)thp;
	u_int32 loAccum=0, hiAccum=0;
	u_int32 numValidMeasurements = 0;
	int32 dvbe, temp, run;
	u_int32 lo, hi;

	for( run=0; run<50; run++ ){
		lo = EM04TOUCH_MeasureAuxInp( thp,
									  EM04TOUCH_DEV_TEMP_RAW_LO );

		hi = EM04TOUCH_MeasureAuxInp( thp,
									  EM04TOUCH_DEV_TEMP_RAW_HI );

		/*MMCONS_Print("%5d %5d\n", lo, hi );*/

		if( (lo==EM04TOUCH_ERROR) || (hi==EM04TOUCH_ERROR) ){
			continue;
		}

		if( IN_RANGE(lo, 600, 1500 ) &&  IN_RANGE(hi, 600, 1500 )){
			numValidMeasurements++;
			loAccum += lo;
			hiAccum += hi;
		}
		if( numValidMeasurements > 25 )
			break;

		OSS_Delay( th->osh, 10 );
	}
	if( numValidMeasurements == 0 ){
		temp = 10000;
		goto EXIT;
	}
	/*
	 * dvbe = (hiAccum - loAccum) * 24900 * 5 / 4096 / 2 /
	 * numValidMeasurements;
	 */
	dvbe = (hiAccum - loAccum) * 1520 / numValidMeasurements / 100  ;
	temp = dvbe - 2730;
 EXIT:
	DBGWRT_1((DBH,"TouchMeasureTemperature temp=%d run=%d\n", temp, run ));

	return temp;
}

/**********************************************************************/
/** update SPI interrupt enable reg with value in shadow reg
 *
 * kp: My intention was to enable bits in IER only if USE_IRQ defined,
 * but IER bits must be turned on in order to appear in IRQ reg... at least
 * in the first designs of Z031
 */
static void TouchShadowToIer( EM04_TOUCH_HANDLE *th )
{
	MWRITE_D8( th->init.base, Z031_IER_REG, th->ierShadow );
}

/**********************************************************************/
/** Acknowledge PenIRQ in FPGA
 */
static void TouchAckPenIrq( EM04_TOUCH_HANDLE *th )
{
	MWRITE_D8( th->init.base, Z031_IRQ_REG, Z031_PENIRQ_MSK );
}


/**********************************************************************/
/** Enable PenIRQ in FPGA (clear any pending IRQ)
 *
 *  PENIRQ should be already enabled in touch controller
 */
static void TouchEnablePenIrq( EM04_TOUCH_HANDLE *th )
{
	TouchAckPenIrq( th );
	th->ierShadow |= Z031_PENIRQ_MSK;
	TouchShadowToIer( th );
}

/**********************************************************************/
/** Disable PenIRQ in FPGA
 */
static void TouchDisablePenIrq( EM04_TOUCH_HANDLE *th )
{
	th->ierShadow &= ~Z031_PENIRQ_MSK;
	TouchShadowToIer( th );
}

/**********************************************************************/
/** Check if PEN asserted (touch pressed)
 * Works only if touch controller has been told to enable PEN irq output
 */
static int TouchPenAsserted( EM04_TOUCH_HANDLE *th )
{
	TouchAckPenIrq( th );
	return !!( MREAD_D8( th->init.base, Z031_IRQ_REG ) & Z031_PENIRQ_MSK );
}



/**********************************************************************/
/** Acknowledge SPI IRQ in FPGA
 */
static void TouchAckSpiIrq( EM04_TOUCH_HANDLE *th )
{
	MWRITE_D8( th->init.base, Z031_IRQ_REG, Z031_SPIIRQ_MSK );
}

/**********************************************************************/
/** Enable "SPI transfered finsished IRQ" in FPGA
 */
static void TouchEnableSpiIrq( EM04_TOUCH_HANDLE *th )
{
	TouchAckSpiIrq( th );
	th->ierShadow |= Z031_SPIIRQ_MSK;
	TouchShadowToIer( th );
}

/**********************************************************************/
/** Disable "SPI transfered finsished IRQ" in FPGA
 */
static void TouchDisableSpiIrq( EM04_TOUCH_HANDLE *th )
{
	th->ierShadow &= ~Z031_SPIIRQ_MSK;
	TouchShadowToIer( th );
}



/**********************************************************************/
/** Start SPI transfer
 *
 * passed \a cmd is shifted according to \a phase
 *
 * cmd shall be in a format that can be transferred to Z031 data reg
 * without modification (in phase 0)
 */
static void TouchSpiStart( EM04_TOUCH_HANDLE *th, u_int32 cmd, int phase )
{
	MACCESS ma = th->init.base;
	if( phase & 1 )
		cmd <<= 8;

	TouchEnableSpiIrq( th );
	DBGWRT_3((DBH," SpiStart Cmd %08x\n", cmd & 0xffffff));

	MWRITE_D32(ma, Z031_DATA_REG, cmd);

	/* NOTE: DO NOT ACCESS ANY Z031 REG now until SPI transfer finished! */
}

/**********************************************************************/
/** Get 12 bits shifted in during last SPI transfer
 * Valid only in EVEN subStates! (0,2...)
 */
static u_int16 TouchSpiXferValueGet( EM04_TOUCH_HANDLE *th )
{
	MACCESS ma = th->init.base;
	u_int16 rv = (u_int16) ((MREAD_D32(ma,Z031_DATA_REG) &
							 EM04_TOUCH_12BIT_MSK) >> 3);
	DBGWRT_3((DBH," SpiValue 0x%04x\n", rv ));
	return rv;
}

/**********************************************************************/
/** Interrupt service routine of the touch driver.
 *
 *  \param thp		pointer to EM04_TOUCH_HANDLE structure
 *
 *	\return EM04TOUCH_SUCCESS = 0 or EM04TOUCH_NOTME = 1
 */
EM04TOUCH_ERRCODE EM04TOUCH_Irq( EM04TOUCH_HANDLE *thp )
{
	EM04_TOUCH_HANDLE *th = (EM04_TOUCH_HANDLE*)thp;
	MACCESS ma = th->init.base;
	u_int8 status = MREAD_D8( ma, Z031_IRQ_REG);

	if( th->maskIrq == TRUE ){
		/* mask request from main routine. Shut off IRQs */
		MWRITE_D8( ma, Z031_IER_REG, 0);
		return EM04TOUCH_SUCCESS;
	}

	status &= th->ierShadow;

	if ( status & Z031_IRQ_ALL_MSK )
	{
		DBGWRT_3((DBH,"EM04TOUCH_Irq irq=%x/%x st=%d/%d\n",
				  status, th->ierShadow, th->state, th->subState));
		/* check if touch was pressed (PENIRQ) */
		if ( status  & Z031_PENIRQ_MSK )
		{
			TouchPenIrq( th );
			return EM04TOUCH_SUCCESS;
		}
		else {
			TouchSpiIrq( th );
			return EM04TOUCH_SUCCESS;
		}
	}
	else
		return EM04TOUCH_NOTME;
}

/**********************************************************************/
/** handle PENIRQ interrupt
 */
static void TouchPenIrq( EM04_TOUCH_HANDLE *th)
{
	TouchDisablePenIrq( th );

	if( th->state != TOUCH_ST_IDLE ){
		DBGWRT_ERR((DBH,"*** TouchPenIrq unexpected\n"));
	}
	else {
#ifdef EM04TOUCH_REC_CYCLES
		th->recCycle = EM04TOUCH_REC_CYCLES;
#else
		th->recCycle = 0;
#endif /* EM04TOUCH_REC_CYCLES */
		/* start x measurement */
		TouchStXYEnter( th, TOUCH_ST_X1 );
	}
}


/**********************************************************************/
/** handle spi transfer finished interrupt
 */
static void TouchSpiIrq( EM04_TOUCH_HANDLE *th )
{
	u_int16 xferValue;

	TouchAckSpiIrq( th );

	xferValue = TouchSpiXferValueGet( th );

	switch( th->state ){

	case TOUCH_ST_IDLE:
		DBGWRT_ERR((DBH,"*** TouchSpiIrq unexpected\n"));
		break;

	case TOUCH_ST_X1:
	case TOUCH_ST_Y:
	case TOUCH_ST_X2:
	case TOUCH_ST_REC:
		if( ++th->subState < 3 ){
			/* start next SPI transfer */
			TouchStXYSpiXfer( th, th->state, th->subState );
		}
		else {
			/* 3 SPI transfers finished, enter next state */

			/* save/evaluate measured value*/
			switch( th->state ){

			case TOUCH_ST_X1:
				th->currentRaw.x = xferValue;
				th->state = TOUCH_ST_Y;
				TouchStXYEnter( th, th->state );
				break;

			case TOUCH_ST_Y:
				th->currentRaw.y = xferValue;
				th->state = TOUCH_ST_X2;
				TouchStXYEnter( th, th->state );
				break;

			case TOUCH_ST_X2:
				th->state = TOUCH_ST_REC;
				TouchStXYMeasFinished( th, xferValue );
				if( th->recCycle-- > 0 ) {
					th->subState = 3; /* go directly to TOUCH_ST_REC */
					TouchSpiStart( th, EM04_TOUCH_SPI_ASKBATT_CMD, 0 );
				} else
					TouchStEnPenEnter( th );
				break;
			case TOUCH_ST_REC:
				if( th->recCycle-- == 0 )
					TouchStEnPenEnter( th );
				else
				{
					th->subState = 3; /*return to here, not another XY measure*/
					TouchSpiStart( th, EM04_TOUCH_SPI_ASKBATT_CMD, 0 );
				}
				break;
			default:
				break;
			} /* switch */

		}
		break;

	case TOUCH_ST_ENPEN:
		/* go into idle state */
		TouchStIdleEnter( th );
		break;

	case TOUCH_ST_GETID:
	case TOUCH_ST_BAT:
	case TOUCH_ST_TEMP0:
	case TOUCH_ST_TEMP1:
		th->auxValue = xferValue; /* save measured value */
		DBGWRT_2((DBH,"auxVal=%d\n", th->auxValue ));

		/*--- check for finished action ---*/
		if( th->actionWaiter )
			TouchSignalWaiter( th );

		TouchStEnPenEnter( th );
		break;
	}
}

/**********************************************************************/
/** Enter TOUCH_ST_ENPEN state
 */
static void TouchStEnPenEnter( EM04_TOUCH_HANDLE *th )
{
	th->state = TOUCH_ST_ENPEN;
	TouchSpiStart( th, EM04_TOUCH_SPI_NOP_CMD, 0 );
}

/**********************************************************************/
/** Enter TOUCH_ST_X1, Y or X2 state
 */
static void TouchStXYEnter( EM04_TOUCH_HANDLE *th, EM04_TOUCH_STATE state )
{
	th->state = state;
	th->subState = 0;

	TouchStXYSpiXfer( th, state, 0 );
}

/**********************************************************************/
/** next SPI transfer for TOUCH_ST_X1, Y or X2 state
 */
static void TouchStXYSpiXfer(
	EM04_TOUCH_HANDLE *th,
	EM04_TOUCH_STATE state,
	u_int32 subState )
{
	u_int32 cmd;

	cmd = (state == TOUCH_ST_Y) ?
		EM04_TOUCH_SPI_ASKY_CMD : EM04_TOUCH_SPI_ASKX_CMD;

	TouchSpiStart( th, cmd, subState );
}

static void TouchSetPrevPos( EM04_TOUCH_HANDLE *th, EM04_TOUCH_POS pos )
{
	th->prevPos2nd = th->prevPos;
	th->prevPos = pos;
	/* also save states! */
	th->prevPosState2nd = th->prevPosState;
}

static int TouchPosDiffers(
	EM04_TOUCH_POS p1,
	EM04_TOUCH_POS p2,
	u_int32 thold)
{
	return ( (ABS( (int)p1.x - (int)p2.x ) > thold) ||
			 (ABS( (int)p1.y - (int)p2.y ) > thold));
}


/**********************************************************************/
/** Evaluate completed measurement
 */
static void TouchStXYMeasFinished( EM04_TOUCH_HANDLE *th, u_int16 x2 )
{
	EM04_TOUCH_POS pos;

	/*--- test if raw values look valid ---*/
	if( !TouchValidXYRawValue( th->currentRaw.x ) ||
		!TouchValidXYRawValue( th->currentRaw.y ) ||
		!TouchValidXYRawValue( x2 )){

		DBGWRT_2((DBH," invalid raw values %d %d %d\n",
				  th->currentRaw.x, th->currentRaw.y, x2 ));
		goto EXIT;
	}

	if( !TouchValidXYPixelValues( th, &pos, x2 ) ) {
		DBGWRT_2((DBH," invalid pixel values %d %d %d\n",
				  th->currentRaw.x, th->currentRaw.y, x2 ));
		goto EXIT;
	}
	DBGWRT_2((DBH," pos (%d; %d)\n", pos.x, pos.y ));
	/* printf(" pos (%d; %d)\n", pos.x, pos.y ); */

	/*--- generate press event ---*/
	switch( th->prevPosState ){
	case TOUCH_PPS_IDLE:
		/*
		 * record initial press, measure again and check to see
		 * if both positions are almost identical
		 */
		TouchSetPrevPos( th, pos );
		th->prevPosState = TOUCH_PPS_HAVEINITALVALUE;
		break;

	case TOUCH_PPS_HAVEINITALVALUE:

		/*
		 * get initial press values
		 * valid if three positions measured at the same position
		 */
		if( TouchPosDiffers( pos, th->prevPos, EM04_TOUCH_DEF_THOLD) == FALSE           &&
			TouchPosDiffers( th->prevPos, th->prevPos2nd, EM04_TOUCH_DEF_THOLD) == FALSE )
		{
			TouchEventGenerate( th, EM04TOUCH_PRESSED, pos.x, pos.y );

			/* Initialize so released event can be generated */
			th->prevPosState    = TOUCH_PSS_PRESSEDNODIFF;
			TouchSetPrevPos( th, pos );

			if( th->calib ){
				/* save raw coordinates for calibration purpose */
				th->calib = FALSE;
				th->calibdat.current = th->currentRaw;
				TouchSignalWaiter( th );
			}
		} else {
			TouchSetPrevPos( th, pos );
		}

		break;

	case TOUCH_PSS_PRESSEDNODIFF:
	case TOUCH_PSS_PRESSEDEVENTSEND:
        if( TouchPosDiffers( pos, th->prevPos, th->init.ds_thresh) ){
        	TouchSetPrevPos( th, pos );
			th->prevPosState = TOUCH_PSS_PRESSEDEVENTSEND;
        }
		break;
	}

 EXIT:
	return;
}

/**********************************************************************/
/** Enter idle state
 */
static void TouchStIdleEnter( EM04_TOUCH_HANDLE *th )
{
	TouchEnablePenIrq( th );

	/* check if we need to generate touch release event */
	if( !TouchPenAsserted( th ) ) {
		if( th->prevPosState2nd == TOUCH_PSS_PRESSEDNODIFF ||
		 	th->prevPosState2nd == TOUCH_PSS_PRESSEDEVENTSEND )
		 {
			TouchEventGenerate( th, EM04TOUCH_RELEASED,
								th->prevPos2nd.x, th->prevPos2nd.y);
			/* invalidate all captured values for event generation */
			th->prevPosState = TOUCH_PPS_IDLE;
			th->prevPosState2nd = TOUCH_PPS_IDLE;
		}
	}
	/* check if we need to generate touch pressed event */
	else {
		if( th->prevPosState2nd == TOUCH_PSS_PRESSEDEVENTSEND ){
			/* We have already initial press values.
			 * Generate move events */
			if( th->init.ds_flags & EM04TOUCH_DSFLG_MOVE ){
				TouchEventGenerate( th, EM04TOUCH_PRESSED,
									th->prevPos2nd.x, th->prevPos2nd.y );
			}
			/* invalidate all captured values for event generation */
			th->prevPosState2nd = TOUCH_PSS_PRESSEDNODIFF;
		}
	}



	th->state = TOUCH_ST_IDLE;

	/*--- check for scheduled actions from main routine ---*/
	if( th->pendAction != TOUCH_ACT_NONE)
		TouchActionKick( th ); /* start now */

}

/**********************************************************************/
/** Generate touch events
 *
 */
static void TouchEventGenerate(
	EM04_TOUCH_HANDLE *th,
	u_int8 pressFlag,
	u_int16 x,
	u_int16 y )
{
	if( (th->init.ds_flags & EM04TOUCH_DSFLG_NOSWAP) == 0){
		/* invert coordinates */
		x = th->init.ds_xres - 1 - x;
		y = th->init.ds_yres - 1 - y;
	}
	DBGWRT_1((DBH,"%c (%d; %d)\n", pressFlag == EM04TOUCH_PRESSED ? 'P':'R',
			  x, y ));

	/* invoke user's callback */
	if( th->init.tefunctp ){
		EM04TOUCH_EVENT evt;

		evt.event 	= pressFlag;
		evt.highx 	= (x & 0xff00) >> 8;
		evt.lowx 	= (x & 0xff);
		evt.highy 	= (y & 0xff00) >> 8;
		evt.lowy 	= (y & 0xff);

		th->init.tefunctp( &evt, th->init.data );
	}

}

/**********************************************************************/
/** Try to guess if A/D value looks valid
 */
static int TouchValidXYRawValue( u_int16 raw )
{
	if( raw == 0 || raw == 0xfff)
		return FALSE;
	return TRUE;
}

/**********************************************************************/
/** convert raw values to pixel and check validity
 */
static int TouchValidXYPixelValues(
	EM04_TOUCH_HANDLE *th,
	EM04_TOUCH_POS *pos,
	u_int16 x2 )
{
	EM04_TOUCH_POS tmpPos, tmpPos2, rawPos;
	int retVal = TRUE, retVal2;

	/* calculate actual pixel values */
	rawPos = th->currentRaw;
	retVal2 = th->getPixFromRaw(th, &rawPos, &tmpPos);
	if( EM04TOUCH_SUCCESS != retVal2 ){
		DBGWRT_2((DBH," ERROR calculating pixel values for %d/%d\n", rawPos.x, rawPos.y ));
		retVal = FALSE;
		goto EXIT;
	}

	/* calculate actual pixel value for X2, we assume that Y stayed stable */
	rawPos.x = x2;
	retVal2 = th->getPixFromRaw(th, &rawPos, &tmpPos2);
	if( EM04TOUCH_SUCCESS != retVal2 ){
		DBGWRT_2((DBH," ERROR calculating pixel values for %d/%d\n", rawPos.x, rawPos.y ));
		retVal = FALSE;
		goto EXIT;
	}

	/*--- test if X and X2 are at almost same pos ---*/
	if( ABS( tmpPos.x - tmpPos2.x ) > EM04_TOUCH_DEF_THOLD ){
		DBGWRT_2((DBH," X/X2 too far away %d %d\n", tmpPos.x, tmpPos2.x ));
		retVal = FALSE;
		goto EXIT;
	}

	/*--- test if X, X2 and Y are inside valid screen area ---*/
	/*--- but only if no calibration is in progress        ---*/
	if( (! th->calib) &&
		( (tmpPos.x  < 0 - EM04_TOUCH_DEF_THOLD) ||
		  (tmpPos.x  > (int)th->init.ds_xres + EM04_TOUCH_DEF_THOLD) ||
		  (tmpPos2.x < 0 - EM04_TOUCH_DEF_THOLD) ||
		  (tmpPos2.x > (int)th->init.ds_xres + EM04_TOUCH_DEF_THOLD) ||
		  (tmpPos.y  < 0 - EM04_TOUCH_DEF_THOLD) ||
		  (tmpPos.y  > (int)th->init.ds_yres + EM04_TOUCH_DEF_THOLD) ) ){
		DBGWRT_2((DBH," X/X2 or Y outside valid area %d/%d %d\n", tmpPos.x, tmpPos2.x, tmpPos.y ));
		retVal = FALSE;
		goto EXIT;
	}

	if( tmpPos.x < 0 ) {
		tmpPos.x = 0;
	}
	if( tmpPos.x >= th->init.ds_xres ) {
		tmpPos.x = th->init.ds_xres - 1;
	}

	if( tmpPos2.x < 0 ) {
		tmpPos2.x = 0;
	}
	if( tmpPos2.x >= th->init.ds_xres ) {
		tmpPos2.x = th->init.ds_xres - 1;
	}

	if( tmpPos.y < 0 ) {
		tmpPos.y = 0;
	}
	if( tmpPos.y >= th->init.ds_yres ) {
		tmpPos.y = th->init.ds_yres - 1;
	}

	/* get average value from X/X2 */
	pos->x = (u_int16)((tmpPos.x + tmpPos2.x) / 2);

	pos->y = (u_int16)tmpPos.y;

EXIT:
	return retVal;
}

/**********************************************************************/
/** Enter critical section
 */
static void TouchCriticalSectionEnter( EM04_TOUCH_HANDLE *th )
{
	th->maskIrq = TRUE;
}

/**********************************************************************/
/** Leave critical section
 */
static void TouchCriticalSectionLeave( EM04_TOUCH_HANDLE *th )
{
	th->maskIrq = FALSE;
	TouchShadowToIer( th );
}


/**********************************************************************/
/** Request action to measure batt, temp, ID, calibration pos.
 *
 * Does not wait until finished.
 */
static EM04TOUCH_ERRCODE TouchActionRequest(
	EM04_TOUCH_HANDLE *th,
	EM04_TOUCH_ACTION action )
{
	if( th->actionWaiter )
		return EM04TOUCH_ERROR;	/* busy */

	TouchCriticalSectionEnter( th );

	th->pendAction = action;

	if( th->state == TOUCH_ST_IDLE )
		TouchActionKick( th ); /* start now */

	th->actionWaiter = TRUE;

	TouchCriticalSectionLeave( th );

	return EM04TOUCH_SUCCESS;
}


/**********************************************************************/
/** Wait for requested action finished
 *
 * \a time is passed accordung to OSS_SemWait wait time
 */
static EM04TOUCH_ERRCODE TouchActionWait(
	EM04_TOUCH_HANDLE *th,
	int32 time)
{
	EM04TOUCH_ERRCODE rv = EM04TOUCH_ERROR;

#ifdef USE_IRQ
	if( OSS_SemWait( th->osh, th->irqSem, time) != 0 )
		goto EXIT;
#else
	while( th->actionWaiter == TRUE ){
		OSS_Delay(th->osh, 1 );
		if( time == OSS_SEM_WAITFOREVER )
			continue;
		if( time == 0 )
			goto EXIT;
		time--;
	}
#endif
	rv = EM04TOUCH_SUCCESS;
 EXIT:
	th->actionWaiter = FALSE;
	return rv;
}

/**********************************************************************/
/** Request action to measure batt, temp, ID, calibration pos.
 *
 * Wait until finished.
 */
EM04TOUCH_ERRCODE EM04TOUCH_ActionPerform(
	EM04_TOUCH_HANDLE *th,
	EM04_TOUCH_ACTION action,
	int32 time)
{
	EM04TOUCH_ERRCODE rv = EM04TOUCH_ERROR;

	if( TouchActionRequest( th, action ) == EM04TOUCH_SUCCESS )
		rv = TouchActionWait( th, time );

	return rv;
}

/**********************************************************************/
/** Actually start action to measure batt, temp, ID
 *
 * Can be called from IRQ or main routine
 */
static void TouchActionKick( EM04_TOUCH_HANDLE *th )
{
	u_int32 cmd = 0;
	EM04_TOUCH_STATE followingState = TOUCH_ST_IDLE;

	DBGWRT_2((DBH,"TouchActionKick %d\n", th->pendAction ));

	switch( th->pendAction ){
	case TOUCH_ACT_GETID:
		cmd = EM04_TOUCH_SPI_ASKID_CMD;
		followingState = TOUCH_ST_GETID;
		break;
	case TOUCH_ACT_BAT:
		cmd = EM04_TOUCH_SPI_ASKBATT_CMD;
		followingState = TOUCH_ST_BAT;
		break;
	case TOUCH_ACT_TEMP0:
		cmd = EM04_TOUCH_SPI_ASKTEMP0_CMD;
		followingState = TOUCH_ST_TEMP0;
		break;
	case TOUCH_ACT_TEMP1:
		cmd = EM04_TOUCH_SPI_ASKTEMP1_CMD;
		followingState = TOUCH_ST_TEMP1;
		break;
	case TOUCH_ACT_CALIBEDGE:
		th->calib = TRUE;
		break;
	default:
		break;
	}

	if( cmd ){
		TouchSpiStart( th, cmd, 0 );
		th->state = followingState;
	}

	th->pendAction = TOUCH_ACT_NONE;
}


/**********************************************************************/
/** Wakes up/flag waiter for action complete
 *
 */
static void TouchSignalWaiter( EM04_TOUCH_HANDLE *th )
{
	if( th->actionWaiter ){
		th->actionWaiter = FALSE;
#ifdef USE_IRQ
		OSS_SemSignal(th->osh, th->irqSem);
#endif
	}
}




