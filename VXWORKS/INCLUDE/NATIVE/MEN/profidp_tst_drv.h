/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: profidp_tst_drv.h
 *
 *       Author: kp
 *        $Date: 2000/06/15 15:08:39 $
 *    $Revision: 2.1 $
 *
 *  Description: Header file for PROFIDP_TST driver 
 *               - PROFIDP specific status codes
 *               - PROFIDP function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: profidp_tst_drv.h,v $
 * Revision 2.1  2000/06/15 15:08:39  gromann
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _PROFIDP_TST_DRV_H
#define _PROFIDP_TST_DRV_H

#ifdef __cplusplus
      extern "C" {
#endif


/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* none */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* none */

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
#ifdef _LL_DRV_
#ifndef _ONE_NAMESPACE_PER_DRIVER_
	extern void PROFIDP_TST_GetEntry(LL_ENTRY* drvP);
#endif
#endif /* _LL_DRV_ */


#ifdef __cplusplus
      }
#endif

#endif /* _PROFIDP_TST_DRV_H */
