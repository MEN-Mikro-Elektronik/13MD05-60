/****************************************************************************
 ************                                                    ************
 ************               MEMBENCH                             ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: kp
 *        $Date: 2005/12/05 14:01:05 $
 *    $Revision: 1.2 $
 *
 *  Description: Memory benchmark
 *                      
 *     Required: 
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: membench.c,v $
 * Revision 1.2  2005/12/05 14:01:05  dpfeuffer
 * membench2() entry added
 *
 * Revision 1.1  2005/06/27 11:38:02  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/testutil.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define REV "1.0"

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/

/* these variables can be modified from shell */
u_int32 membenchMinBlockSz = 1 * 1024; 
u_int32 membenchMaxBlockSz = 2048 * 1024;
u_int32 membenchTotMem = 65536 * 1024;
	
/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
/* assembler routines */
extern void membenchRead( u_int8 *mem, u_int32 blockSz );
extern void membenchWrite( u_int8 *mem, u_int32 blockSz );
extern void membenchReg( u_int32 blockSz );

int membench( int extAddr )
{
	u_int32 blockSz;
	u_int32 accessed, startTick, endTick;
	u_int8 *mem;

	/*--------------------------+
	|  allocate system memory   |
	+--------------------------*/
	if( extAddr == 0 ){
		mem = (u_int8 *)malloc(membenchMaxBlockSz);
		if( !mem ){
			perror("Failed to allocate memory\n");
			return 1;
		}
	}
	else {
		mem = (u_int8 *)extAddr;
	}

	/*---------------+
	|  Do benchmark  |
	+---------------*/
	printf("Memory bench @ 0x%08x, min=%d max=%d tot=%d\n", 
		   mem, membenchMinBlockSz, membenchMaxBlockSz, membenchTotMem );

	for( blockSz=membenchMinBlockSz; 
		 blockSz <= membenchMaxBlockSz; blockSz<<=1 ){

		printf("Blocksize %5d KByte: ", blockSz/1024 ); fflush(stdout);

		/*-------------+
		|  Reg Bench  |
		+-------------*/
		startTick = tu_get_tick();

		for(accessed=0; accessed<membenchTotMem; accessed+=blockSz )
			membenchReg( blockSz );
		
		endTick = tu_get_tick();

		printf("RegCopy: %s\t", 
			   tu_bytes_per_sec( accessed, endTick-startTick));
		fflush(stdout);

		/*-------------+
		|  Read Bench  |
		+-------------*/
		startTick = tu_get_tick();

		for(accessed=0; accessed<membenchTotMem; accessed+=blockSz )
			membenchRead( mem, blockSz );
		
		endTick = tu_get_tick();

		printf("Read: %s\t", tu_bytes_per_sec( accessed, endTick-startTick));
		fflush(stdout);

		/*--------------+
		|  Write Bench  |
		+--------------*/
		startTick = tu_get_tick();

		for(accessed=0; accessed<membenchTotMem; accessed+=blockSz )
			membenchWrite( mem, blockSz );
		
		endTick = tu_get_tick();

		printf("Write: %s\n", tu_bytes_per_sec( accessed, endTick-startTick));
	}
	if( extAddr == 0 )
		free(mem);

	return 0;
}

int membench2( int extAddr, int minBlk, int maxBlk, int totMem )
{
	membenchMinBlockSz = minBlk; 
	membenchMaxBlockSz = maxBlk;
	membenchTotMem = totMem;
	
	return membench( extAddr );
}

#if 0
void ReadBench( u_int8 *mem, u_int32 blockSz )
{
	register u_int32 *memP = (u_int32 *)mem;
	register volatile u_int32 sink;
	
	blockSz/=32;

	while( blockSz-- ){
		sink = *memP++;	
		sink = *memP++;	
		sink = *memP++;	
		sink = *memP++;	
		sink = *memP++;	
		sink = *memP++;	
		sink = *memP++;	
		sink = *memP++;	
	}
}

void RegBench( u_int32 blockSz )
{
	register volatile u_int32 r1,r2;
	
	blockSz/=32;

	while( blockSz-- ){
		r1 = r2;
		r2 = r1;
		r1 = r2;
		r2 = r1;
		r1 = r2;
		r2 = r1;
		r1 = r2;
		r2 = r1;
	}
}

void WriteBench( u_int8 *mem, u_int32 blockSz )
{
	register u_int32 *memP = (u_int32 *)mem;
	register u_int32 source = 0xa55aa55a;
	
	blockSz/=32;

	while( blockSz-- ){
		*memP++ = source;	
		*memP++ = source;	
		*memP++ = source;	
		*memP++ = source;	
		*memP++ = source;	
		*memP++ = source;	
		*memP++ = source;	
		*memP++ = source;	
	}
}
#endif

