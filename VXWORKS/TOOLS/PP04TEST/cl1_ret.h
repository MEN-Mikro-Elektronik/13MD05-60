/**********************************************************************Copyright (C) Siemens AG 1997 All rights reserved. Confidential

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
** WORKFILE::  cl1_ret.h
**---------------------------------------------------------------------
** TASK::
   Header for Application Programmers Interface (API) with return codes

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
** HISTORY::   AUTHOR::    Baddam Tirumal Reddy
			   REVISION::  2.04.00
**---------------------------------------------------------------------
  
**********************************************************************/

#ifndef CL1_RET_H_
#define CL1_RET_H_

/*---------- return values ------------------------------------------*/
#define MVB_NO_ERROR                0
#define MVB_WARNING_OLD_DATA        1
#define MVB_UNKNOWN_TRAFFICSTORE    2
#define MVB_ERROR_PORT_UNDEF        3
#define MVB_WARNING_NO_SINK         4
#define MVB_ERROR_PARA              5
#define MVB_ERROR_NO_MVBC           6
#define MVB_ERROR_NO_SRC            7
#define MVB_ERROR_VALIDITY          8
#define MVB_ERROR_MVBC_INIT         9
#define MVB_ERROR_MVBC_STOP        10
#define MVB_ERROR_MVBC_RESET       11
#define MVB_ERROR_DATA             12
#define MVB_ERROR_NSDB             13
#define MVB_ERROR_SIGNAL           14
#define MVB_ERROR_CHECKSUM         15
#define MVB_ERROR_WRONG_SEQ        16
#define MVB_WATCHDOG_NOT_AVAIL	   17

/*---------- link level return codes --------------------------------*/
#define MVB_LPS_OK                           0
#define MVB_LPS_ERROR                       50 /* general error      */
#define MVB_LPS_CONFIG                      51 /* configuration error*/
#define MVB_LPS_UNKNOWN_TS_ID               52 /* ts out of range    */

/*---------- link layer return codes --------------------------------*/
#define MVB_LC_OK                            0
#define MVB_LC_REJECT                       60 /* general error      */

/*---------- return codes of NS -------------------------------------*/
#define NS_OK                                0
#define NS_NOT_FOUND                        70
#define NS_WARNING_ONE_NSDB					71	  /* Using func MVBCGetSpecifiedNsdb() warning,
													 if the multiple NSDB contains only 1 NSDB. */

/*---------- return codes of DB handling ----------------------------*/
#define NS_DB_NOT_AVAIL                     80
#define NS_DB_CORRUPT                       81
#define NS_DB_TRANSFER                      82

/*---------- return codes of MVBM ----------------------------*/
#define MVBM_OK                              0
#define MVBM_CONFIG_UNDEF                   90
#define MVBM_CONFIG_REQ                     91
#define MVBM_CONFIG_NOT_AVAIL               92
#define MVBM_CONFIG_NOT_SAVED               93
#define MVBM_CONFIG_ANNOUNCE                94

#endif
