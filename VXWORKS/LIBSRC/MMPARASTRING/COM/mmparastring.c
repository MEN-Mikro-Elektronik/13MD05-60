/*********************  P r o g r a m  -  M o d u l e ***********************
 *  
 *         Name: mmparastring.c
 *      Project: many
 *
 *       Author: kp
 *        $Date: 2002/06/05 15:46:07 $
 *    $Revision: 1.5 $
 *
 *  Description: Routines to parse MENMON parameter string
 *                      
 *	Some MENMONs provide an ASCII string in memory containing board and
 *	other parameters (first implemented on MEN B11). This library allows
 *	you to parse these parameters easily. Can be used also to parse parameters
 *	in VxWorks bootline
 *                      
 *     Required: -
 *     Switches: MENMON
 *
 *---------------------------[ Public Functions ]----------------------------
 *  MMPARA_SearchKey
 *	MMPARA_StringToInt
 *  MMPARA_StringToIp
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mmparastring.c,v $
 * Revision 1.5  2002/06/05 15:46:07  kp
 * Bug fix in MMPARA_SearchKey: go back to EatBlanks when scanned key
 * contains a blank
 *
 * Revision 1.4  2002/02/15 13:28:33  kp
 * modified MMPARA_SearchKey() to be able to handle composed values
 * a la KERPAR='p1=x p2=y'
 *
 * Revision 1.3  2001/04/25 08:44:43  kp
 * Bug fix in MMPARA_StringToIp:
 * return value advanced beyound end of string!
 *
 * Revision 1.2  2000/12/22 14:31:24  kp
 * adapted to MENMON
 *
 * Revision 1.1  2000/12/19 15:10:15  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
static const char RCSid[]="$Id: mmparastring.c,v 1.5 2002/06/05 15:46:07 kp Exp $";

#include <MEN/men_typs.h>
#include <MEN/mmparastring.h>

#ifdef MENMON
# include <mmglob.h>
#else
# include <string.h>
#endif

/********************************* MMPARA_SearchKey **************************
 *
 *  Description: Search for a parameter in bootline or menmon parameters
 *			   
 *			     Copies the string found into the supplied buffer.
 *				 dst must be large enough...	
 *---------------------------------------------------------------------------
 *  Input......: key				parameter name (e.g. "cbr=")
 *				 src				source string containing parameters
 *				 dst				destination string 
 *  Output.....: returns:			<dst> or NULL if parameter not found
 *  Globals....: -
 ****************************************************************************/
char *MMPARA_SearchKey( char *key, char *src, char *dst )
{
	int keylen;
	char *orgDst = dst;
	enum { EatBlanks_, LookupKey, BadKey, GetValue } state=EatBlanks_;
	int lookupIdx=0, haveKeyMatch=0, apostrophCount=0, found=0;
		   
	if( (src == NULL) || (dst == NULL) || (key == NULL) )
		return NULL;

	keylen = strlen(key);

	while( *src && !found ){
		/*mm_usr_printf("src=%c state=%d\n", *src, state );*/

		switch( state ){
		case EatBlanks_:
			if( *src  != ' ' ){
				state = LookupKey;
				lookupIdx=0;
				src--;
			}
			break;

		case LookupKey:
			if( *src == key[lookupIdx] ){
				lookupIdx++;
			}
			else {
				src--;
				state=BadKey;
				break;
			}
			if( *src == '=' ){
				
				if( lookupIdx == keylen )
					haveKeyMatch = 1;
				else
					haveKeyMatch = 0;

				state = GetValue;
				apostrophCount=0;
			}
			break;

		case BadKey:
			if( *src == '=' ){
				haveKeyMatch = 0;
				apostrophCount=0;
				state = GetValue;
			}
			else if(*src == ' ')
				state = EatBlanks_;
			break;

		case GetValue:
			if( *src == '\'' ){				
				apostrophCount ^= 1;
			}

			else if( *src == ' ' && apostrophCount == 0  ){
				state = EatBlanks_;
				if( haveKeyMatch ) 
					found=1;
				break;
			}

			if( haveKeyMatch )
				*dst++ = *src;

			break;
		}

		src++;
	}
	*dst = '\0';

	if( haveKeyMatch ){
		/*mm_usr_printf("FOUND: <%s>\n", orgDst );*/
		return orgDst;
	}
	/*mm_usr_printf("NOT FOUND\n");*/
	return NULL;
}

/********************************* MMPARA_StringToInt ***********************
 *
 *  Description: Convert string to integer accordung to "base"
 *			   
 *  If an illegal character is found, conversion is aborted and the return
 *	value points to the unsuccessfully parsed character
 *  Will handle positive integers only
 *---------------------------------------------------------------------------
 *  Input......: base			radix (2,10,16)
 *				 expr		    string to parse
 *  Output.....: returns:		ptr to first unsuccessfully parsed char
 *				 *valP			value parsed
 *  Globals....: -
 ****************************************************************************/
char *MMPARA_StringToInt( int base, char *expr, u_int32 *valP)
{
	u_int32 tmp=0, new;

	while( *expr ){
		if( *expr >= '0' && *expr <= '9' )
			new = *expr - '0';
		else if( *expr >= 'a' && *expr <= 'f' )
			new = *expr - 'a' + 10;
		else if( *expr >= 'A' && *expr <= 'F' )
			new = *expr - 'A' + 10;
		else
			goto ex;
		
		if( new >= base )
			goto ex;

		tmp = (tmp*base) + new;
		expr++;
	}
ex:
	*valP = tmp;
	return expr;
}

/********************************* MMPARA_StringToIp *************************
 *
 *  Description: Converts string to IP address
 *			   
 *	String must be in dotted notation (e.g. 192.1.1.1) and may contain
 *	additional chars (e.g 192.1.1.1:ffffff00) which are not parsed		   
 *---------------------------------------------------------------------------
 *  Input......: str			string to parse
 *  Output.....: returns		ptr into string to next char after IP address
 *								or NULL if invalid
 *				 *ipP			converted IP address
 *  Globals....: -
 ****************************************************************************/
char *MMPARA_StringToIp( char *str, u_int32 *ipP )
{
	int num,i;
	u_int32 ip=0;

	for(i=0; i<4; i++){
		num = 0;

		while(1){
			if( *str >= '0' && *str <= '9' )
				num = num * 10 + (*str - '0');
			else {
				if( i== 3 ){
					break;
				}
				else {
					if( *str == '.' ){
						str++; 
						break;
					}
					else
						return NULL;
				}
			}
			str++;	
		}
		ip |= num << (8*(3-i));
	}
	*ipP = ip;
	return str;
}


