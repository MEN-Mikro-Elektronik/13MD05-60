/**********************************************************************
Copyright (C) Siemens AG 1997 All rights reserved. Confidential

Permission is hereby granted, without written agreement and without
license or royalty fees, to use and modify this software and its
documentation for the use with SIEMENS PC104 cards.

IN NO EVENT SHALL SIEMENS AG BE LIABLE TO ANY PARTY FOR DIRECT,
INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF SIEMENS AG
HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

SIEMENS AG SPECIFICALLY DISCLAIMS ANY WARRANTIES,INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN
"AS IS" BASIS, AND SIEMENS AG HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
**=====================================================================
** PROJECT::   TCN
** MODULE::    PC104 CLASS1.1-1.3
** WORKFILE::  host_dep.c 
**---------------------------------------------------------------------
** TASK::
   Application Programmers Interface (API) for PC104 access.
   Functions which depend on the host-system

**---------------------------------------------------------------------
** AUTHOR::    REINWALD_LU
** CREATED::   13.01.1998
**---------------------------------------------------------------------
** CONTENTS::
      functions:
        writeWordToTS()
        readWordFromTS()
        MVB_INT_ENABLE()
        MVB_INT_DISABLE()
        Wait2MS()
        Wait100US()

	#if defined (MVB_M)
         ReadMVBMConfig()
         SaveMVBMConfig()
	#endif

**---------------------------------------------------------------------
** NOTES::     -

**=====================================================================
** HISTORY::   AUTHOR::   Graf Ralf
               REVISION:: 2.2
               MODTIME::  Apr 15 1999
**---------------------------------------------------------------------
  
**********************************************************************/


#ifdef __KERNEL__  /* we are a module */
# include <linux/module.h>
# include <linux/kernel.h>
# include <linux/types.h>
# include <linux/delay.h>
# include <asm/io.h>
#else	/* we are a user process */
/* # include <stdint.h> */
# include <fcntl.h>
# include <stdlib.h>
# include <string.h>
# include <stdio.h>
# include <errno.h>
# include <sys/mman.h>
#if defined(LINUX)
# include <MEN/men_typs.h>
# include <MEN/usr_utl.h>
#endif
# include <time.h> 
#endif

#include "config.h"
#include "host_dep.h"
#include "dpr_dep.h"
#include "cl1_ret.h"
#include "cl1_1.h"

#if defined(LINUX)

#endif

extern void *G_MvbPciBase;


#if defined (MVB_M) && !defined(__KERNEL__)
#include <stdio.h>
#endif

#if 0
#define DBG_FCTNNAME   printk("--> %s()\n", __FUNCTION__);
#define HSTDBG(x...)  printk(KERN_INFO x);
#else 
#define DBG_FCTNNAME   
#define HSTDBG(x...)
#endif


#ifndef __KERNEL__
static struct timespec	timewait;
#endif


/*---------- access to traffic-store --------------------------------*/
void writeWordToTS(void* addr, TM_TYPE_WORD val)
{

#ifndef MVB_SWAP_BYTES
	*((unsigned short *)addr) = val;
#else
	unsigned short swap = ((val & 0x00ff) << 8) | ((val & 0xff00) >> 8);
#endif

	*((unsigned short *)addr) = swap;

} /* writeWordToTS */




TM_TYPE_WORD readWordFromTS(void* addr)
{

	TM_TYPE_WORD v;
	unsigned short swap = 0x0000;

	v = *((unsigned short*)addr);

#ifndef MVB_SWAP_BYTES
	return v;
#else
	swap = ((v & 0x00ff) << 8) | ((v & 0xff00) >> 8);
	return swap;
#endif

} /* readWordFromTS */




/*---------- interrupt-handling -------------------------------------*/
void MVB_INT_ENABLE(void)
{
	/* put in IRQ enabling code here */


}


void MVB_INT_DISABLE(void)
{

/* #error Please insert function to disable interrupts on hostsystem here  */

}

    
/*---------- waiting-functions --------------------------------------*/
void Wait2MS(void)
{
/* #error Please insert wait fctn ( 2 ms) here to clear MVBC rx buffer  */

	timewait.tv_sec = 0;        /* seconds */
	timewait.tv_nsec =2000000;  /* nanoseconds */
	nanosleep( &timewait, NULL);

}


void Wait100US(void)
{

/* #error Please insert wait fctn ( 2 ms) here to clear MVBC rx buffer  */
	timewait.tv_sec  = 0;         /* seconds */
	timewait.tv_nsec = 100000; /* nanoseconds */
	nanosleep( &timewait, NULL);

}



/*---------- functions for MVB-M ---------*/ 
#if defined (MVB_M)
UNSIGNED8 ReadMVBMConfig(
	UNSIGNED8          *buffer,
    UNSIGNED16         bytes
)
{

	return MVBM_OK;
/* #error  Please insert function to read the MVBM-Configuration  */
/* this should perform some file opening, reading, closing.. */
/* Return values:
				 MVBM_OK
				 MVBM_CONFIG_NOT_AVAIL */
}

UNSIGNED8 SaveMVBMConfig(
	UNSIGNED8          *buffer,
    UNSIGNED16         bytes
)
{

	return MVBM_OK;
/* #error  Please insert function to save the MVBM-Configuration  */
/* Return values:
				 MVBM_OK
				 MVBM_CONFIG_NOT_SAVED */



}
#endif

