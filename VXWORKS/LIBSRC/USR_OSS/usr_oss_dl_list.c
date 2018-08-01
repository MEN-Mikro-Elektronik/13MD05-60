/*********************  P r o g r a m  -  M o d u l e ***********************
 *  
 *         Name: dl_list.c
 *      Project: USR_OSS
 *
 *      $Author: Franke $
 *        $Date: 1999/08/31 10:53:39 $
 *    $Revision: 1.1 $
 *
 *  Description: functions for double linked lists
 *               (functionality cloned from AMIGA ROM kernel)
 *     Required:  
 *     Switches: 
 *
 *---------------------------[ Public Functions ]----------------------------
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: usr_oss_dl_list.c,v $
 * Revision 1.1  1999/08/31 10:53:39  Franke
 * Initial Revision
 *
 * Revision 1.1  1999/04/23 13:28:21  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

const char UOS_DLL_RCSid[]="$Id: usr_oss_dl_list.c,v 1.1 1999/08/31 10:53:39 Franke Exp $";

#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_err.h>
#define DBG_MYLEVEL         UOS_DbgLev
#include <MEN/dbg.h>
#include "usr_oss_intern.h"

/******************************** UOS_DL_NewList *****************************
 *
 *  Description: Initialize list header (empty list)
 *                         
 *---------------------------------------------------------------------------
 *  Input......:  l - ptr to list structure 
 *  Output.....:  returns: same as input
 *  Globals....:  ---
 ****************************************************************************/
UOS_DL_LIST *UOS_DL_NewList( UOS_DL_LIST *l )
{
	l->head = (UOS_DL_NODE *)&l->tail;
	l->tailpred = (UOS_DL_NODE *)&l->head;
	l->tail = (UOS_DL_NODE *)NULL;
	return l;
}

/********************************* UOS_DL_Remove *****************************
 *
 *  Description:  remove a node from a list
 *                         
 *---------------------------------------------------------------------------
 *  Input......:  n - node to remove
 *  Output.....:  returns: same as input
 *  Globals....:  ---
 ****************************************************************************/
UOS_DL_NODE *UOS_DL_Remove( UOS_DL_NODE *n )
{
	n->next->prev = n->prev;
	n->prev->next = n->next;
	return n;
}

/********************************* UOS_DL_RemHead ****************************
 *
 *  Description: remove a node from the head of the list 
 *                         
 *---------------------------------------------------------------------------
 *  Input......:  l - ptr to list header
 *  Output.....:  returns: removed node ( NULL if list was empty )
 *  Globals....:  ---
 ****************************************************************************/
UOS_DL_NODE *UOS_DL_RemHead( UOS_DL_LIST *l )
{
	UOS_DL_NODE *n;
	if( l->head->next == NULL ) return NULL; /* list empty */

	n = l->head;
	l->head = n->next;
	n->next->prev = (UOS_DL_NODE *)&(l->head);

	return n;
}

/******************************** UOS_DL_AddTail *****************************
 *
 *  Description:  add a node at the tail to the list
 *---------------------------------------------------------------------------
 *  Input......:  l - ptr to list functions
 *				  n - node to add
 *  Output.....:  returns: added node
 *  Globals....:  ---
 ****************************************************************************/
UOS_DL_NODE *UOS_DL_AddTail( UOS_DL_LIST *l, UOS_DL_NODE *n )
{
	n->prev = l->tailpred;
	l->tailpred = n;
	n->next = n->prev->next;
	n->prev->next = n;
	return n;
}
	
