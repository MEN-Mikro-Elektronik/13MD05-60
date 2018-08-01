/* switch CPU_IS_FAST for higher loop counts */

#include "stdio.h"
#include "stdlib.h"
#include "memLib.h"
#include "sysLib.h"
#include "tickLib.h"
#include "taskLib.h"
#include "string.h"

#if (CPU == PENTIUM)
	#include "cacheLib.h"
	/* Invalidate cache, with writeback */ 
	#define FLUSH_CPU_CACHE()	__asm __volatile ("WBINVD")	
	#define MB_PER_SEC
#else
	#define FLUSH_CPU_CACHE()
#endif

int MEM_PerfMax=0;

void blockCopy( char *srcP, char *dstP, int size, int retryCount )
{
int  i;
#if OLD_ONE
char *dP;
char *sP;
int  nbrCpy; 
#endif

	for( i=0; i<retryCount; i++ )
	{
#if OLD_ONE
		dP = dstP;
		sP = srcP;
		nbrCpy = size;
		while( nbrCpy-- )
		{
			*dP++ = *sP++;
		}/*while*/
#else
		bcopy( srcP, dstP, size );
#endif

	/* Invalidate cache, with writeback */ 
	FLUSH_CPU_CACHE();

	}/*for*/
}/*blockCopy*/


int mem_perf( char *addr, int size, int retryCount, int silent )
{
char *startP = NULL;
int	 tickRate;
char *srcP;
char *dstP;
int  startTick;
int  endTick;
float diffSec;
float effectiveMb;
float MBperSec;
int   MEM_PerfCurr;
float scale=1;
int	  position;
int   i;

	size = size * 1024;

	if( addr == NULL )
	{
		startP = memalign( 2, size*2 );
		if( !startP )
		{
			printf("***ERROR - memalign()\n");
			return( ERROR );
		}/*if*/
	}
	else
	{
		startP = addr;
		printf(" %08x:  ", (int)startP );
	}/*if*/

	tickRate = sysClkRateGet();
	srcP = startP;
	dstP = startP + size;


/*	printf("====== BLOCK COPY %d times =========\n", retryCount );
	printf(" src=%08x dst=%08x size=%dkB\n", (int)srcP, (int)dstP, (size/1024) );*/

	taskLock();
	startTick = tickGet();
	blockCopy( srcP, dstP, size, retryCount ); 
	endTick   = tickGet();
	taskUnlock();

/*	printf(" tickRate= %d end= %d start=%d\n", tickRate, endTick, startTick );*/
	diffSec = ( endTick - startTick );
	diffSec /= tickRate;
	if( diffSec == 0.0 )
	{
		printf("***ERROR - retryCount to less (diffSec=%f)\n", diffSec);
		goto CLEANUP;
	}/*if*/
   
	effectiveMb  = size;
	effectiveMb  = effectiveMb/0x100000;
	effectiveMb *= retryCount;

	MBperSec = effectiveMb / diffSec;
	MEM_PerfCurr = MBperSec;
	if( MEM_PerfMax < MEM_PerfCurr )
	{
		MEM_PerfMax = MEM_PerfCurr;
		scale = MEM_PerfMax / 25;
	}
	if( !silent )
	{
		printf(" %8d  %1.3f |", (size/1024), MBperSec );
		position = MEM_PerfCurr / scale;
		for( i=1; i<position;i++)
			printf(" ");
		printf("*\n");
	}/*if*/

	if( addr == NULL )
		free( startP );
	return( OK );

CLEANUP:
	if( addr == NULL )
		free( startP );
	return( ERROR );
}/*mem_perf*/

void mem_perf_test( void *addr )
{

	printf("MEMORY performance test ( bcopy() Version )\n");
	printf("===========================================\n");
	printf("SIZE[kB]  PERFORMANCE[MB/s]\n");

#ifdef CPU_IS_FAST
    mem_perf( addr,   1, 114000, 1 );
    mem_perf( addr,   1, 114000, 0 );
    mem_perf( addr,   4, 14000, 0 );
    mem_perf( addr,   8, 14000, 0 );
    mem_perf( addr,   9, 12000, 0 );
    mem_perf( addr,  10, 12000, 0 );
    mem_perf( addr,  11, 12000, 0 );
    mem_perf( addr,  14, 12000, 0 );
    mem_perf( addr,  16, 12000, 0 );
    mem_perf( addr,  32, 11000, 0 );
    mem_perf( addr,  64, 11000, 0 );
    mem_perf( addr, 128,  1500, 0 );
    mem_perf( addr, 256,  1500, 0 );
    mem_perf( addr, 270,  1400, 0 );
    mem_perf( addr, 300,  1400, 0 );
    mem_perf( addr, 330,  1400, 0 );
    mem_perf( addr, 512,  1200, 0 );
    mem_perf( addr,1024,   150, 0 );
    mem_perf( addr,2048,   120, 0 );
    mem_perf( addr,4096,   110, 0 );
    mem_perf( addr,2*4096, 110, 0 );
    mem_perf( addr,4*4096, 110, 0 );
#else
    mem_perf( addr,   1, 4000, 1 );
    mem_perf( addr,   1, 4000, 0 );
    mem_perf( addr,   4, 4000, 0 );
    mem_perf( addr,   8, 4000, 0 );
    mem_perf( addr,   9, 2000, 0 );
    mem_perf( addr,  10, 2000, 0 );
    mem_perf( addr,  11, 2000, 0 );
    mem_perf( addr,  14, 2000, 0 );
    mem_perf( addr,  16, 2000, 0 );
    mem_perf( addr,  32, 1000, 0 );
    mem_perf( addr,  64, 1000, 0 );
    mem_perf( addr, 128,  500, 0 );
    mem_perf( addr, 256,  500, 0 );
    mem_perf( addr, 270,  400, 0 );
    mem_perf( addr, 300,  400, 0 );
    mem_perf( addr, 330,  400, 0 );
    mem_perf( addr, 512,  200, 0 );
    mem_perf( addr,1024,   50, 0 );
    mem_perf( addr,2048,   20, 0 );
    mem_perf( addr,4096,   10, 0 );
#endif /* CPU_IS_FAST */

}/*mem_perf_test*/

int vx_memperf
(
	int sizeKB,
	int retryCount,
	char *src,           /* may be NULL - than allocate */
	char *dst,           /* may be NULL - than allocate */
	int minTransferRate, /* KB per ms */
	int maxTransferRate  /* KB per ms */
)
{
	int error = -1;
	int srcAlloc = 0;
	int dstAlloc = 0;
	double milliSec;
	double effectiveKB;
	double KBperMilliSec;
	int	 tickRate;
	int  startTick;
	int  endTick;

	if( sizeKB == 0 )
	{
		printf("vx_memperf <sizeKB> <retryCount> <src> <dst> <minTransferRate> <maxTransferRate>\n"
		       "sizeKB          - block size\n"
		       "retryCount      - number of block copy retries\n"
		       "src 		    - source - NULL force allocation\n"
		       "dst 		    - destination - NULL force allocation\n"
		       "minTransferRate - \n"
		       "maxTransferRate - \n"
		       "returns 0 or -1 if transferrate is out of range\n"
		       );
		goto CLEANUP;
	}/*if*/

	/*-----------------------------+
	| INPUT PARAMETERS             |
	+-----------------------------*/
	sizeKB = sizeKB * 1024; /* to byte */

	if( src == NULL )
	{
		srcAlloc = 1;
		src = memalign( 2, sizeKB );
	}/*if*/
	if( dst == NULL )
	{
		dstAlloc = 1;
		dst = memalign( 2, sizeKB );
	}/*if*/
	if( src == NULL || dst == NULL )
	{
		printf("*** ERROR - no mem\n");
		goto CLEANUP;
	}/*if*/

	tickRate = sysClkRateGet();


	/*------------------------+
	| MEASUREMENT             |
	+------------------------*/
	taskLock();
	startTick = tickGet();
	blockCopy( src, dst, sizeKB, retryCount ); 
	endTick   = tickGet();
	taskUnlock();

	/*------------------------+
	| CHECK VALUES            |
	+------------------------*/
	milliSec = ( endTick - startTick ) * 1000.0;
	milliSec /= tickRate;
	if( milliSec < 300.0 )
	{
		printf("*** ERROR - retryCount to less (milliSec=%f)\n", milliSec );
		goto CLEANUP;
	}/*if*/

	sizeKB /=1024;        					/* back to KB */
    effectiveKB = sizeKB * retryCount; 		/* absolute */
	KBperMilliSec = effectiveKB / milliSec; /* transferrate */

#ifdef MB_PER_SEC
	printf("min / current / max [MB/s]:  %d   %d  %d  - ", 
			minTransferRate, (int)(KBperMilliSec*1000/1024), maxTransferRate );
#else
	printf("min / current / max [kB/ms]:  %d   %d  %d  - ", 
			minTransferRate, (int)KBperMilliSec, maxTransferRate );
#endif
	
	
	if( minTransferRate <= KBperMilliSec
	    && KBperMilliSec <= maxTransferRate )
	{
		printf( "OK\n");
		error = 0;
	}
	else
	{
		printf( "*** ERROR\n");
	}/*if*/
    

CLEANUP:
	if( src && srcAlloc )
		free( src );
	if( dst && dstAlloc )
		free( dst );

	return( error );
}/*vx_memperf*/
