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
** WORKFILE::  host_dep.h
**---------------------------------------------------------------------
** TASK::
   Application Programmers Interface (API) for PC104 access.
   Definitions of host dependencies.

**---------------------------------------------------------------------
** AUTHOR::    REINWALD_LU
** CREATED::   30.04.1997
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
** HISTORY::   AUTHOR::   Baddam Tirumal Reddy
               REVISION:: 2.04.00
**---------------------------------------------------------------------

**********************************************************************/

#ifndef HOST_DEP_H_
#define HOST_DEP_H_


/*---------- basic definitions --------------------------------------*/
#ifndef NULL
#define NULL 0L
#endif

/*---------- basic type definitions for application -----------------*/
typedef short          SWORD16;
typedef unsigned short UNSIGNED16;
typedef unsigned char  UNSIGNED8;
typedef unsigned long  UNSIGNED32;

#define SIZEOFWORD64   (sizeof(UNSIGNED8)*8)


/*---------- check for correct memory-model on INTEL-Machines -------*/
#ifdef M_I86
#if !defined(M_I86LM)

#error  Please change memory-model to LARGE

#endif
#endif  /* ifdef M_I86 */     


/*---------- basic type definitions for dual port ram access --------*/
#define VOL volatile
typedef unsigned char      TM_TYPE_BYTE;
typedef unsigned short     TM_TYPE_WORD;
typedef VOL unsigned short TM_TYPE_RWORD;  /*Always accessed directly*/


/*---------- basic type definitions for NSDB access -----------------*/
typedef unsigned char  OCTET;
typedef unsigned short WORD16;
typedef unsigned long  WORD32;


/*---------- processor little/big endian ----------------------------*/
/* activate the define for Bigendian CPUs like PPC, 	*/
/* leave it commented out for x86 intels 				*/

#define BIGENDIAN /* not defined for little endian */


/*---------- processor uses segmentation ----------------------------*/

/* ts: - comment out for virtual MM OSes like NT, Linux */
/*     - leave active for real mode operation (MSDOS)  	 */

/* #define O_SEG */  /* Special Case: Intel Processors, segmentation */


/*---------- address arithmetics ------------------------------------*/

#if defined (O_SEG)
/*  On Intel-80x86-based systems:
    The addition is restricted to segment part only.  The offset part
    is not affected.  Therefore, granularity is limited to 16 bytes or
    integral multiple of it. */

#define LCX_PTR_ADD_SEG(ptr, ofs)  \
    ((void*) (((UNSIGNED32) (ptr)) +  \
    ((((UNSIGNED32) (ofs)) & 0xFFFF0L) << 12)))

#else
/*  Linear addressing: pure address addition takes place. */

#define LCX_PTR_ADD_SEG(ptr, ofs)  \
    ((void*) (((UNSIGNED32) (ptr)) + ((UNSIGNED32) (ofs)) ))

#endif


/*---------- pointer conversion and access --------------------------*/

/* macro makes pointer of type "type" out of UNSIGNED32 content of arg,
   which is also a pointer */
#define P_OF_CONTENT(type,arg)   ((type)*((UNSIGNED32*)(arg)))

/* macro makes type "type" out of UNSIGNED32 casted arg */
#define P_OF_ULONG(type,arg)     ((type) (UNSIGNED32)(arg))

/* macro returns UNSIGNED32 casted pointer */
#define PTR2ULONG(ptr)           ((UNSIGNED32)(ptr))

/*---------- swap operations ----------------------------------------*/

/* NOTE: The argument to the macro SWAP should be of type UNSIGNED16 */
#define SWAP16(a)  ((((a) & 0x00ff) << 8) | ((UNSIGNED16)((a) & 0xff00) >> 8))

/* NOTE: The arguments to the macro SWAP32_INC should be pointers to
         UNSIGNED16
   NOTE: the construction "do { ... } while(0)" is used to prevent strange
         syntax errors in conjunction with the use of ";" */
#define SWAP32(dest,src)  \
    do {  \
        UNSIGNED16 tmp;  \
        tmp = SWAP16(*(UNSIGNED16*)(src));  \
        *(UNSIGNED16*)(dest) = SWAP16(*(((UNSIGNED16*)(src))+1));  \
        *(((UNSIGNED16*)(dest))+1) = tmp;  \
    } while(0)


/*---------- Transfer data from Net to Host and back ----------------*/

#ifdef BIGENDIAN
#define NtoH16(x)       (x)             /* Net to Host 16 bit */
#define NtoH32(d,s)  \
    do {  \
        *(UNSIGNED32*)(d) = *(UNSIGNED32*)(s);  \
    } while(0)                          /* Net to Host 32 bit */
#define HtoN16(x)       (x)             /* Host to Net 16 bit */
#define HtoN32(d,s)  \
    do {  \
        *(UNSIGNED32*)(d) = *(UNSIGNED32*)(s);  \
    } while(0)                          /* Host to Net 32 bit */
#else
#define NtoH16(x)       SWAP16(x)       /* Net to Host 16 bit */
#define NtoH32(d,s)     SWAP32(d,s)     /* Net to Host 32 bit */
#define HtoN16(x)       SWAP16(x)       /* Host to Net 16 bit */
#define HtoN32(d,s)     SWAP32(d,s)     /* Host to Net 32 bit */
#endif


/*---------- timeout after trying to switch line --------------------*/
/* This is an arbitrary value big enough to allow enough time for
   switching, even for fast CPUs */
#define LCI_LS_LIMIT    10


/*---------- pre- and postfix for access of traffic store -----------*/
#define PREFIX_WRITE_TS
#define POSTFIX_WRITE_TS
#define PREFIX_READ_TS
#define POSTFIX_READ_TS

/*---------- function prototypes for access on traffic store --------*/

void writeWordToTS(
    void *addr,             /* Address */
    TM_TYPE_WORD val        /* Value to write */
);
/*{
    The function writes the value to the address which is located in TS.
    Swapping is performed for different endianess.
}*/


TM_TYPE_WORD readWordFromTS(
    void* addr              /* Address */
);
/*{
    The function reads a value from the address which is located in TS.
    Swapping is performed for different endianess.
}*/


/*---------- function prototypes for interrupt-manipulation ---------*/ 
void MVB_INT_ENABLE(void); 
void MVB_INT_DISABLE(void);


/*---------- function prototypes for wait-functions ---------*/ 
void Wait2MS(void); 
void Wait100US(void);

/*---------- macro for the calling of the watchdog-retrigger-function ---------*/ 
#define SRVWDT()  

/*---------- function prototypes for MVB-M ---------*/ 
#if defined (MVB_M)
UNSIGNED8 ReadMVBMConfig(
	UNSIGNED8          *buffer,
    UNSIGNED16         bytes
);
/*{
    The function reads the MVBM-Configuration out from the nonviolant memory.
    The function is host-specific. 
	Because of this it has to be implemented by the user.
    Return values:  MVBM_OK
                    MVBM_CONFIG_NOT_AVAIL : configuration could not be read because of any reason
}*/

UNSIGNED8 SaveMVBMConfig(
	UNSIGNED8          *buffer,
    UNSIGNED16         bytes
);
/*{
    The function saves the MVBM-Configuration into the nonviolant memory.
    The function is host-specific. 
	Because of this it has to be implemented by the user.
    Return values:  MVBM_OK
                    MVBM_CONFIG_NOT_SAVED : saving was not possible because of any reason
}*/
#endif




#endif
