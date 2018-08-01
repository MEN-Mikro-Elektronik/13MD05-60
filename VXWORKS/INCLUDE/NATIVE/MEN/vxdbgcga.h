/***********************  I n c l u d e  -  F i l e  ************************/
/*!  
 *        \file  vxdbgcga.h
 *
 *      \author  men
 *        $Date: 2009/07/30 11:05:58 $
 *    $Revision: 2.2 $
 * 
 *  	 \brief  Header file for X86 pre STDIO direct VGA/CGA RAM output
 *                      
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: vxdbgcga.h,v $
 * Revision 2.2  2009/07/30 11:05:58  ufranke
 * added
 *  + line mode
 *
 * Revision 2.1  2009/05/19 13:12:25  ufranke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009..2009 by MEN Mikro Elektronik GmbH, Nueremberg, Germany 
 ****************************************************************************/

#ifndef _VXDBGCGA_H
#define _VXDBGCGA_H

#ifdef __cplusplus
	extern "C" {
#endif


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
extern int VXDBGCGA_Write( char *frmt, ... );
extern void VXDBGCGA_FirstLineMode( int enable );



#ifdef __cplusplus
	}
#endif

#endif	/* _VXDBGCGA_H */
