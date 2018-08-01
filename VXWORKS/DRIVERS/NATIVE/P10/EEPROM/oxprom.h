/***********************  I n c l u d e  -  F i l e  ************************
 *  
 *         Name: microwire.h
 *
 *       Author: uf
 *        $Date: 2000/03/14 14:35:01 $
 *    $Revision: 1.1 $
 * 
 *  Description: microwire bus protocol interface
 *     Switches: MCRW_COMPILE - for module compilation
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oxprom.h,v $
 * Revision 1.1  2000/03/14 14:35:01  loesel
 * Initial Revision
 *
 * Revision 1.2  2000/02/29 14:52:36  loesel
 * write mode modified
 *
 * Revision 1.1  2000/02/28 15:38:21  loesel
 * Initial Revision
 *
 * 
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
#ifndef _MCRW_H
#  define _MCRW_H

#  ifdef __cplusplus
      extern "C" {
#  endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/


/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define MCRW_ERR_NO           		0   /* OK */
#define MCRW_ERR_DESCRIPTOR   		1   /* descriptor is wrong */
#define MCRW_ERR_NO_MEM       		2   /* no memory */
#define MCRW_ERR_UNK_CODE      		3   /* unknown status code */
#define MCRW_ERR_ERASE      		4   /* erase */
#define MCRW_ERR_WRITE      		5   /* write */
#define MCRW_ERR_WRITE_VERIFY  		6   /* verify failed */
#define MCRW_ERR_ADDR      		    7   /* address in EEPROM */
#define MCRW_ERR_BUF 		    	8   /* buffer not aligned */
#define MCRW_ERR_BUF_SIZE 		    9   /* buffer size */

#define MCRW_DESC_PORT_FLAG_SIZE_MASK		0x07
#define MCRW_DESC_PORT_FLAG_SIZE_8			0x01
#define MCRW_DESC_PORT_FLAG_SIZE_16			0x02
#define MCRW_DESC_PORT_FLAG_SIZE_32			0x04

#define MCRW_DESC_PORT_FLAG_POLARITY_HIGH	0x10
#define MCRW_DESC_PORT_FLAG_READABLE_REG	0x20

#define MCRW_DESC_PORT_FLAG_OUT_IN_ONE_REG  0x01  /* all out bits are in one register */

#define MCRW_IOCTL_BUS_CLOCK				0x01  /* set/getstat to change the bus clock */
#define MCRW_IOCTL_ADDR_LENGTH				0x02  /* set/getstat to change the address length */


/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/


#  ifdef __cplusplus
      }
#  endif

#endif/*_MCRW_H*/

