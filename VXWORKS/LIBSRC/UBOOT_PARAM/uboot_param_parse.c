/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: uboot_param_parse.c
 *      Project: MEN MM50 BSP
 *
 *       Author: cs
 *        $Date: 2011/11/19 13:54:44 $
 *    $Revision: 1.2 $
 *
 *  Description: UBOOT parameter string parse function library
 *
 *     Required:  -
 *     Switches:  -
 *
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: uboot_param_parse.c,v $
 * Revision 1.2  2011/11/19 13:54:44  ts
 * R: compiler warning about comparing pointer with integer
 * M: changed compare in if-statement to NULL instead 0
 *
 * Revision 1.1  2010/03/27 00:27:36  cs
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#include <stdio.h>
#include <string.h>

#include <MEN/uboot_param_parse.h>

const char * G_UbootParamStart = 0;
unsigned int G_UbootParamSize  = 0;

/**********************************************************************/
/** sysUbootParamInit - initialize UBOOT param parse lib
 *
 * RETURN: N/A
 */
void sysUbootParamInit(
	const char* ubootParamStart,
	unsigned int ubootParamSize
){
	G_UbootParamStart = ubootParamStart;
	G_UbootParamSize  = ubootParamSize;
	return;
}

/**********************************************************************/
/** sysUbootParamParse - locate specific string for specific parameter
 *
 * RETURN: NULL or pointer to string
 */
const char* sysUbootParamParse(
	const char*parseStr
){
	const char* tmpPtr = G_UbootParamStart;
	int   curLen = 0;

	/* sanity check */
	if( G_UbootParamStart == NULL || G_UbootParamSize == 0 )
		return (NULL);

	while( strlen(tmpPtr) != 0 ) {
		/* printf("0x%06x, %d  ", tmpPtr, strlen(tmpPtr)); */
		/* paranoia check: only consider max 0x1000 characters
		 *                 a single parameter is to be shorter than 0x200 chars */
		if( tmpPtr >= (G_UbootParamStart+G_UbootParamSize) ||
			strlen(tmpPtr) > 0x200 )
		{
			return( NULL );
		}
		/* find "=", compute length of this string */
		curLen = (int)strchr((const char *)tmpPtr, '=') - (int)tmpPtr;
		if( curLen < 0 || curLen > 0x200 )
			return( NULL );

		/* printf("%s\n",tmpPtr); */

		/* if we look for a string longer than currently found string
		 * we might look for a derivate or similiar string, continue
		 */
		if( curLen == strlen(parseStr) &&
			!strncmp(tmpPtr, parseStr, curLen ))
			return( tmpPtr + (curLen + 1)); /* return actual value only, without name and = */

		tmpPtr += (strlen(tmpPtr)+1);
	}
	return( NULL );
}

/**********************************************************************/
/** sysUbootParamToRawEth - UBOOT param string to ethernet MAC number
 *
 * RETURN: -1 = ERROR; 0 = OK;
 */
int sysUbootParamToRawEth(
	void *dest,
	const char *src
){
	unsigned char *p = (unsigned char*)dest;
    unsigned int temp[6], i;

	for( i=0; i<6; ++i ) {
		p[i] = 0xff;			/* set default */
	}

	if( 6 != sscanf(src, UBOOT_PARAM_MAC_ADDR_FORMAT, &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5])) {
		return -1;
	}

	for( i=0; i<6; ++i ) {
		p[i] = (unsigned char)temp[i];
	}

	/*printf("Ethernet Address: %02x:%02x:%02x:%02x:%02x:%02x ",
	 *		pEnet[0],pEnet[1],pEnet[2],pEnet[3],pEnet[4],pEnet[5] ); */

	return 0;
}

/**********************************************************************/
/** sysUbootParamIntGet - get integer value from UBOOT param string
 *
 * RETURN: retreived or default value;
 */
int sysUbootParamIntGet(
	const char *parName,
	unsigned int radix,
	unsigned int defVal
){
    unsigned int retVal = defVal;
    const char *tmpStr = NULL;


    tmpStr = sysUbootParamParse(parName);
    if( tmpStr == NULL ) {
    	return defVal;
	}

	retVal = strtoul(tmpStr, NULL, radix);

	/*printf("sysUbootParamIntGet: %s=0x%x\n",parName, retVal); */

	return retVal;
}

