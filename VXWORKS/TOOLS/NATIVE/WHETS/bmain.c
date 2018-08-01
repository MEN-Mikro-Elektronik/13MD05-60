
/* Danieli bmain benchmark for usage with ElinOS */

#define MAXBENCH	100000

#define SINGLE
#ifdef SINGLE
#define FLOATPOINT float
#else
#define FLOATPOINT double
#endif


#ifdef ISLINUX

#include <stdio.h>
#include <math.h>
#include <sys/time.h>
FLOATPOINT arr1[MAXBENCH];
FLOATPOINT arr2[MAXBENCH];
FLOATPOINT arr3[MAXBENCH];

void start_bench()
{
int i;

 struct timeval t1, t2;
     gettimeofday(&t1, NULL);
     for( i=0; i < MAXBENCH; i++){
        arr1[i] = (1+i)/100.1;
        arr2[i] = (1+i) * 1.123;
        arr3[i] = (arr1[i] + arr2[i])/(arr1[i] * arr2[i]);
     }
     gettimeofday(&t2, NULL);
     printf( "%d loops execution time start: %ld [us]\n", 
			 MAXBENCH, 
			 (t2.tv_sec-t1.tv_sec)*1000000+(t2.tv_usec - t1.tv_usec ));
}

int main(void)
{

	printf("BMAIN build %s %s\n", __DATE__, __TIME__ );

	printf("starting ");
#ifdef SINGLE
	printf("single ");
#else
	printf("double ");
#endif
	printf("float benchmark:\n");
	start_bench();
	printf("finished.\n");
	return(0);
}

#else  /*VXWORKS*/

#include "vxWorks.h"
#include "stdio.h"
#include "semLib.h"
#include "tickLib.h"
#include "wdLib.h"
#include "tickLib.h"

#define MAXBENCH	100000

FLOATPOINT arr1[MAXBENCH];
FLOATPOINT arr2[MAXBENCH];
FLOATPOINT arr3[MAXBENCH];

STATUS sysClkRateSet(int);

void start_bench()
{
int i;
	int startT,endT;

 	printf("starting ");
#ifdef SINGLE
	printf("single ");
#else
	printf("double ");
#endif
	printf("float benchmark:\n");

     
     startT = tickGet();
     sysClkRateSet(1000);

     for( i=0; i < MAXBENCH; i++){
        arr1[i] = (1+i)/100.1;
        arr2[i] = (1+i) * 1.123;
        arr3[i] = (arr1[i] + arr2[i])/(arr1[i] * arr2[i]);
     }

     endT = tickGet();
     printf( "%d loops execution time start: %ld [ms]\n", 
			 MAXBENCH, endT - startT );
}

#endif
