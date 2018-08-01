/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  systemtest.c
 *
 *      \author  René Lange
 *        $Date: 2012/09/06 15:31:58 $
 *    $Revision: 1.2 $
 *
 *        \brief  stress test routine for SMBbus (EEPROM)
 *
 *     Switches: -
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: systemtest_smb.c,v $
 * Revision 1.2  2012/09/06 15:31:58  MKolpak
 * M: Release version
 *
 * Revision 1.1  2012/04/13 09:37:04  MKolpak
 * Initial Revision
 *
 * Revision 1.2  2010/03/09 16:38:54  RLange
 * R: Improved performance
 * M: replace do by while loops
 *
 * Revision 1.1  2009/12/10 15:30:11  RLange
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009..2010 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
 ****************************************************************************/
#if 0
/*--------------------------------------------------------------+
 |	INCLUDE														|
 +-------------------------------------------------------------*/

/*--------------------------------------------------------------+
 |	EXTERNALS													|
 +-------------------------------------------------------------*/
extern void sysNvWrite(
    ULONG	offset,	/* NVRAM offset to write the byte to */
    UCHAR	data	/* datum byte */
);
extern UCHAR sysNvRead(
    ULONG	offset	/* NVRAM offset to read the byte from */
);

/*--------------------------------------------------------------+
 |	DEFINES 													|
 +-------------------------------------------------------------*/
#define STRESS_EEPROM_SIZE				0x20

/*--------------------------------------------------------------+
 |	GLOBALS														|
 +-------------------------------------------------------------*/

/*--------------------------------------------------------------+
 |	PROTOTYPES													|
 +-------------------------------------------------------------*/

/*--------------------------------------------------------------+
 |	INCLUDES  													|
 +-------------------------------------------------------------*/


/**********************************************************************/
/** Routine for EEPROM stress task.
 *
 *  \param hP   	pointer to STRESS_TEST_HDL
 *  \param load   	cpu load of task
 *
 */
static void stressEeprom( STRESS_TEST_HDL *hP, u_int32 load_nom, u_int32 resvd )
{
	STRESS_TEST_HDL *h = hP;
	u_int32 delay = (100 - load_nom) * sysClkRateGet() / 100;
	u_int32 tick = 0;
	u_int32 bucketSize = STRESS_BUCKET_SIZE;
	u_int32 bucket = 0;
	u_int32 meanVal = 0;
	u_int32 offs = 0xE0; /* Overwrites the last 32 Byte of the VXBLINE !!!*/
	u_int8 cnt = 0;
	u_int8 data = 0xff;
	u_int32 i = 0;

	h->startTick = tickGet();

	while(1){
		if( !h->enabled ){
			return;
		}

		/* recalculate delay timing */
		if( h->load_nom != load_nom ){
			load_nom = h->load_nom;
			if( load_nom == 99 ){
				delay = 1;
			}
			else{
				delay = (100 - load_nom) * sysClkRateGet() / 100;
			}
		}

		for( i = 0; i < STRESS_EEPROM_SIZE; i++ ){
			tick = tickGet();

			if( (h->bytenum + (sizeof(u_int8)*2)) <= (sizeof(u_int8)*2 ) ){
				/* overflow occurred, clear startime to fix Xfer-Rate */
				h->startTick = tickGet();
			}

			h->bytenum += (sizeof(u_int8) * 2);
			data = 0xFF;
			cnt++;

			/* write Byte */
			sysNvWrite(offs + i, cnt);

			/* verify Byte */
			data = sysNvRead(offs + i);

			if( cnt != data ){
				if( h->verbose & STRESS_DBGLEV_1 ){
					stressWrtDbg(h->dbgP, "*** (eeprom) verify ***");
				}
				h->errCnt++;
			}

			/* init bucket if empty */
			if( bucket == 0 ){
				bucket = (tickGet() - tick) * bucketSize;
			}

			meanVal = bucket / bucketSize;
			bucket = (bucket + (tickGet() - tick)) - meanVal;

			h->currExeTime = bucket / bucketSize;
			h->allExeTime += h->currExeTime;

			if( h->verbose & STRESS_DBGLEV_3 ){
				stressWrtDbg(h->dbgP, "eeprom alive");
			}

			taskDelay( delay );

			while( h->halt ){
				taskDelay(1);
			};

		}/* end for */

		while( h->halt ){
			taskDelay(1);
		};
	}/* end while */
}

#endif
