/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  systemtest_misc.c
 *
 *      \author  René Lange
 *        $Date: 2012/04/13 09:37:13 $
 *    $Revision: 1.1 $
 *
 *        \brief  utility routines for stress test
 *
 *     Switches: -
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: systemtest_misc.c,v $
 * Revision 1.1  2012/04/13 09:37:13  MKolpak
 * Initial Revision
 *
 * Revision 1.4  2010/03/09 16:29:48  RLange
 * R: Improved performance
 * M: Changed calling convention for writeLogging (now task)
 *
 * Revision 1.3  2010/02/23 11:29:32  RLange
 * R: Improvements Testbench
 * M: added second logging path
 *    added serial number to logging filename
 *    added logging ACCU info
 *    added logging Stuck errors
 *
 * Revision 1.2  2010/01/22 13:11:11  RLange
 * R: Support Testboard
 * M: Added routines to handle absolute value
 *    Adapted calculation for PIC voltages and currents
 *
 * Revision 1.1  2009/12/10 15:30:05  RLange
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009..2010 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
 ****************************************************************************/

/*--------------------------------------------------------------+
 |	INCLUDE														|
 +-------------------------------------------------------------*/
#define STRESS_CYCLE_TIME	100 /* test cycle in ms */

/*--------------------------------------------------------------+
 |	EXTERNALS													|
 +-------------------------------------------------------------*/



/*--------------------------------------------------------------+
 |	INCLUDES  													|
 +-------------------------------------------------------------*/

enum
{
	STRESS_DUMMY_STATE_RUNNING,
	STRESS_DUMMY_STATE_LOCKED
};

 /* 
 	this dummy reports is to check datarate calculation, error reporting and
 	stuck checking.
 	It will report ~1kB/s for 10s, lock for 10 s. Error state will be toggled.
 */
 void stressDummy( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
 {
	 u_int32 delay = (100 - load) * sysClkRateGet() / 100;
	 u_int32 tick = 0;
	 int tmp = 0;
	 u_int32 i = 0;
	 u_int8 state = STRESS_DUMMY_STATE_RUNNING;
	 int fh = -1;
	 
 
	 taskDelay( sysClkRateGet() + 5 * h->id );
	 h->startTick= tickGet();
 
	 while( h->enabled )
	 {
#ifdef _WRS_CONFIG_SMP                
				 h->cpu = vxCpuIndexGet();
#endif
 
#if 0	
		 /* recalculate delay timing */
		 if( h->load_act != load )
		 {
			 load = h->load_act;
			 if( load == 99 )
			 {
				 delay = 1;
			 }
			 else
			 {
				 delay = (100 - load) * sysClkRateGet() / 100;
			 }
		 }
#endif
		 /* were alive! */
		 h->alive = 1;

		/**********************/
		/* Run test		   */
		/**********************/
		if(i < 10)
		{
			if(i == 0)
			{
				stressDbg(NULL,STRESS_DBGLEV_3, "Dummy running");
				state = STRESS_DUMMY_STATE_RUNNING;
			}
			i++;
		}else
		{
			h->errCnt++;
			state = STRESS_DUMMY_STATE_LOCKED;
			i = 0;
		}
		switch(state)
		{
			case STRESS_DUMMY_STATE_RUNNING:
				h->bytenum += 1000;
				tick = tickGet();
				while(tickGet() < tick + sysClkRateGet())
				{
					tmp++;	/* loop 1s */
				}
				break;
			case STRESS_DUMMY_STATE_LOCKED:
				stressDbg(NULL,STRESS_DBGLEV_3, "Dummy locked");
				taskDelay(10000);
				break;

		}
		 
		while( h->halt && h->enabled )
		{
			taskDelay(100);
		};

		 h->loopCnt++;
	 }
 
	 h->isFinished = 1;
 
	 return;
 }

 
 /* 
	Load task funktion
 */
 void stressLoad( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
 {	 
	 taskDelay( sysClkRateGet() + 5 * h->id );
	 h->startTick= tickGet();
 
	 while( h->enabled )
	 {
#ifdef _WRS_CONFIG_SMP                
				 h->cpu = vxCpuIndexGet();
#endif
 
		 /* were alive! */

		h->bytenum++;
		if(!(h->bytenum%100000))
		{
			h->alive = 1;
			h->loopCnt++;
		}
		 
		while( h->halt && h->enabled )
		{
			taskDelay(100);
		};
	 }
 
	 h->isFinished = 1;
 
	 return;
 }
 
 
 

