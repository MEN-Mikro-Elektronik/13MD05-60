/* sysMgt5200AuxClk.h - PP01 Aux Clock */

/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: sysMgt5200AuxClk.h
 *
 *       Author: uf
 *        $Date: 2005/09/09 19:48:54 $
 *    $Revision: 1.3 $
 *
 *  Description: Header for for the MGT5x00 aux clock driver
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: sysMgt5200AuxClk.h,v $
 * Revision 1.3  2005/09/09 19:48:54  CSchuster
 * added newline at end of file to avoid compiler warning
 *
 * Revision 1.2  2005/03/14 12:06:15  ufranke
 * cosmetics
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003..2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef __SYS_MGT5200_AUX_INC
#define __SYS_MGT5200_AUX_INC
#ifdef __cplusplus
extern "C"	{
#endif



/*------------------------------------------+
|  DEFINES & CONST                          |
+------------------------------------------*/

/*------------------------------------------+
|   TYPDEFS                                 |
+------------------------------------------*/

/*------------------------------------------+
|   EXTERNALS                               |
+------------------------------------------*/
extern void sysMgt5200AuxClkInit
(
	int ipbClk,
	int usedTimer
);

/*------------------------------------------+
|    PROTOTYPES                             |
+------------------------------------------*/

#ifdef __cplusplus
}
#endif
#endif /*__SYS_MGT5200_AUX_INC*/

