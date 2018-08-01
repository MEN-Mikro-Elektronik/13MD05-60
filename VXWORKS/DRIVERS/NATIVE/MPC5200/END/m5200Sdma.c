/* Copyright 1984-2003 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
  modification history
  --------------------
  01a,17Jull03,pkr written

$Log: m5200Sdma.c,v $
Revision 1.5  2006/02/03 14:08:35  UFRANKE
changed
 - sysSdmaInit()
   initialize all unused initiator priorities to zero


*/

const char *RCSid_m5200Sdma = "$Id: m5200Sdma.c,v 1.5 2006/02/03 14:08:35 UFRANKE Exp $\n";


#include "vxWorks.h"
#include "iv.h"
#include "intLib.h"
#include "config.h"
#include "sysLib.h"
#include "WRS/m5200Sdma.h"
#include "bestcomm_api.h"
#include "task_api/bestcomm_cntrl.h"
#include "task_api/bestcomm_api_mem.h"

sdma_regs *sysSDMA = (sdma_regs *)(SDMA_BASE_ADRS);
uint8	*MBarGlobal = (uint8 *) MBAR_VALUE;

void sdmaTeaInt(int arg);

STATUS sdmaIntSetLevel(int intNum, int level)
{
    /* TBD */
    return OK;
}

void sdmaDemuxInt(int arg)
{
    int pend = sysSDMA->IntPend & ~sysSDMA->IntMask;
    int intNum = -1;
    int i, msk;
    int found=0;

	/*DBG_Write(DBH,">> demux: pend=%08x real=%08x mask=%08x\n", 
	  pend, sysSDMA->IntPend, sysSDMA->IntMask );*/

    if (pend & 0x10000000){
		intNum = INUM_SDMA_TEA;
		sdmaTeaInt(0);
    }
    else {
        for (i = INUM_SDMA_TASK0, msk=1; i <= INUM_SDMA_TASK15; i++, msk<<=1){
			if ((pend & msk) != 0){
				intNum = i;
				sysIntHandlerExec (intNum, 1);
				found++;
				break;
			}
		}
    }
	/*DBG_Write(DBH,"<< demux mask=%08x\n", sysSDMA->IntMask );*/
}

void sdmaTeaInt(int arg)
{
    int taskNum = ((sysSDMA->IntPend)>>24)&0xF;

    sysSDMA->IntPend = 0x10000000; /* clear the event */

    logMsg("SDMA: TEA detected in task %d\n",taskNum,2,3,4,5,6);
}



STATUS sysSdmaInit()
{
	int i;
	
    sysSDMA->taskBar = (unsigned long)SRAM_BASE_ADRS;
 
	sysSDMA->IntMask = 0xffffffff;
	sysSDMA->IntPend = 0xffffffff;

	intConnect (INUM_TO_IVEC(INT_VEC_SMARTCOMM), sdmaDemuxInt, 0);
	intEnable(INT_VEC_SMARTCOMM);
	intConnect (INUM_TO_IVEC(INUM_SDMA_TEA), sdmaTeaInt, 0);
	intEnable(INUM_SDMA_TEA);

    TasksInitAPI ((void *)MBAR_VALUE);
    TasksLoadImage (sysSDMA);

	/* initialize initiator priorities */
	for( i=0; i<32; i++ )
	    sysSDMA->IPR[i] = 0; 

    sysSDMA->IPR[0] = 7; 

    /*
     * Turn off COMM bus prefetch. This affects all data movements on
     * the COMM bus. (Yes, _PE -- prefetch enable -- should probably be 
     * named _PD.)
     */
     
    sysSDMA->PtdCntrl |= SDMA_PTDCNTRL_PE;

    return OK;
}

STATUS sdmaIntEnable(int intNum)
{
    /* range is checked in caller */
    if (intNum == INUM_SDMA_TEA)
        sysSDMA->IntMask &= 0xEFFFFFFF;
    else
		SDMA_INT_ENABLE( SDMA_INT_MASK, (intNum-INUM_SDMA_TASK0));
    return OK;
}

STATUS sdmaIntDisable(int intNum)
{
    /* range is checked in caller */
    if (intNum == INUM_SDMA_TEA)
        sysSDMA->IntMask |= 0x10000000;
    else
		SDMA_INT_DISABLE( SDMA_INT_MASK, (intNum-INUM_SDMA_TASK0));
    return OK;
}
