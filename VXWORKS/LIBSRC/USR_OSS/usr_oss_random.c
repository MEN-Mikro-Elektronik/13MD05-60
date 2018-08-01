/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: usr_oss_random.c
 *      Project: UOS library
 *
 *       Author: see
 *        $Date: 2008/09/05 11:15:41 $
 *    $Revision: 1.2 $
 *
 *  Description: USER OSS RANDOM - This module creates random integers.
 *               It is a part of the USR_OSS library.
 *
 *     Required: -
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 * 
 * $Log: usr_oss_random.c,v $
 * Revision 1.2  2008/09/05 11:15:41  ufranke
 * R: diab compiler warning
 * M: cosmetics
 *
 * Revision 1.1  1999/08/31 10:53:48  Franke
 * Initial Revision
 *
 * cloned from OS9 Revision 1.4  1999/06/10 08:33:44  kp random.c
 *---------------------------------------------------------------------------
 * (c) Copyright 1999..2008 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

static const char RCSid[]="$Id: usr_oss_random.c,v 1.2 2008/09/05 11:15:41 ufranke Exp $";

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>


/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/

/********************************* UOS_Random *******************************
 *
 *  Description: Create a new random integer value
 *			   
 *---------------------------------------------------------------------------
 *  Input......: old	 initial or last returned random value
 *  Output.....: return  random integer value (0..0xffffffff)
 *  Globals....: -
 ****************************************************************************/
u_int32 UOS_Random(u_int32 old)
{
	register u_int32 a = old;
			 
	a <<= 11;
	a += old;
	a <<= 2;
	old += a;
	old += 13849;
	return old;
}

/********************************* UOS_RandomMap ****************************
 *
 *  Description: Map created integer value into specified range
 *			   
 *			     Maps a random integer value 'val' returned from UOS_Random()
 *               into range ra..re.
 *			   
 *---------------------------------------------------------------------------
 *  Input......: val     integer value
 *               ra		 min value
 *               re      max value
 *  Output.....: return  mapped integer value [ra..re]
 *  Globals....: -
 ****************************************************************************/
u_int32 UOS_RandomMap(u_int32 val, u_int32 ra, u_int32 re)
{
   double  f;
   u_int32 r;

   f = (double)val / 0xffffffff;      			/* make double 0..1 */
   r = (u_int32)((f * (double)(re-ra)) + 0.5 + ra);    	/* expand to set, add offset */

   return(r);
}





