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
** WORKFILE::  config.h
**---------------------------------------------------------------------
** TASK::
   Application Programmers Interface (API) for PC104 access.
   Configuration parameters.


TS: local copy for PP01 (ppc) build

**---------------------------------------------------------------------
** AUTHOR::    REINWALD_LU
** CREATED::   30.04.1997
**---------------------------------------------------------------------
** CONTENTS::
      functions:
          none
**---------------------------------------------------------------------
** NOTES::     -

**=====================================================================
** HISTORY::   AUTHOR::   Graf Ralf
               REVISION:: 2.2
               MODTIME::  Apr 15 1999
**---------------------------------------------------------------------
  
**********************************************************************/

#ifndef CONFIG_H_
#define CONFIG_H_


/*---------- class mode  of software --------------------------------*/
/* No one defined: Class 1.1
   Only one of the following may be defined */
 /*#define CLASS13 */   /* Class 1.3 */
 /*#define CLASS12 */   /* Class 1.2 */

/*---------- Definition of MVBM --------------------------------*/
/* May only be defined in Class 1.2 or Class 1.3 */
#if defined(CLASS12) || defined(CLASS13)
/* #define MVB_M  */
#endif


/*---------- value for supervision register STSR --------------------*/
/* Value defines supervision time interval in milliseconds.
   The user is responsible to avoid TM access overloading due to
   supervising too many docks within a too small time interval.

   Possible values:
   TM_STSR_INTERV_OFF
   TM_STSR_INTERV_1MS
   TM_STSR_INTERV_2MS
   TM_STSR_INTERV_4MS
   TM_STSR_INTERV_8MS
   TM_STSR_INTERV_16MS
   TM_STSR_INTERV_32MS
   TM_STSR_INTERV_64MS
   TM_STSR_INTERV_128MS
   TM_STSR_INTERV_256MS */
#define TM_STSR_INTERV          TM_STSR_INTERV_OFF

/* Number of docks to be supervised.
   User must assure that this value does not exceed the number of
   available docks in the data area. Bus traffic overflow can occur
   by wrong combination of intervall and number of docks ! */
#define TM_STSR_DOCKS           0

/*---------- MVBC in Intel or Motorola mode -------------------------*/
/* In this application the MVBC will always work in Intel Mode.
   The value can be 0 (Motorola) or 1 (Intel). */

#define LCX_INTEL_MODE          0

/*---------- number of managed traffic stores -----------------------*/
#define LCX_TM_COUNT            1


/*---------- traffic memory access waitstates -----------------------*/
/* Fill values for all traffic stores (LCX_TM_COUNT) between brackets.
   Example: { TM_SCR_WS_0, TM_SCR_WS_1, TM_SCR_WS_1 }
   Possible values:
   TM_SCR_WS_3
   TM_SCR_WS_2
   TM_SCR_WS_1
   TM_SCR_WS_0 */
#define LCX_WAITSTATES          { TM_SCR_WS_3 }

/*---------- traffic memory arbitration mode -----------------------*/
/* Fill values for all traffic stores (LCX_TM_COUNT) between brackets.
   Example: { TM_SCR_WS_0, TM_SCR_WS_1, TM_SCR_WS_1 }
   Possible values:
   TM_SCR_ARB_3
   TM_SCR_ARB_2
   TM_SCR_ARB_1
   TM_SCR_ARB_0 */
#define LCX_ARB_MODE          { TM_SCR_ARB_0 }


/*---------- Memory Control Mode (MCM) -----------------------*/
/* PC/104 usually works with 64K
   Possible values:
   64K: 2 
   32K: 1
   16K: 0 */
#define MEM_MODE  2

/*---------- maximal port index for Class 1.1 -----------------------*/
/* The value depends on the Memory Control Mode (MCM) of the MVBC
   Possible values:
   TM_MCM_64K: 1023 
   TM_MCM_32K:  255
   TM_MCM_16K:  255 */
#if MEM_MODE >= 2
#define LC_TS_MAX_LA_PORT_INDX  1023
#else
#define LC_TS_MAX_LA_PORT_INDX  255
#endif



#endif
