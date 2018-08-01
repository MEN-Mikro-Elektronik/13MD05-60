/*********************  P r o g r a m  -  M o d u l e ***********************
 *  
 *         Name: oss_dl_list.c
 *      Project: OSS
 *
 *      $Author: ufranke $
 *        $Date: 2009/03/31 09:37:31 $
 *    $Revision: 1.2 $
 *
 *  Description: Functions for double linked lists
 *               (functionality cloned from AMIGA ROM kernel)
 *     Required:  
 *     Switches: 
 *
 *---------------------------[ Public Functions ]----------------------------
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_dl_list.c,v $
 * Revision 1.2  2009/03/31 09:37:31  ufranke
 * cosmetics
 *
 * Revision 1.1  1999/08/30 11:03:33  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
#include <MEN/men_typs.h>

#include <vxWorks.h>
#include <taskLib.h>

#include <MEN/mdis_err.h>
#include <MEN/oss.h>

/******************************** OSS_DL_NewList *****************************
 *
 *  Description: Initialize list header (empty list)
 *                         
 *---------------------------------------------------------------------------
 *  Input......:  l - ptr to list structure 
 *  Output.....:  returns: same as input
 *  Globals....:  ---
 ****************************************************************************/
OSS_DL_LIST *OSS_DL_NewList( OSS_DL_LIST *l )
{
	l->head = (OSS_DL_NODE *)&l->tail;
	l->tailpred = (OSS_DL_NODE *)&l->head;
	l->tail = (OSS_DL_NODE *)NULL;
	return l;
}

/********************************* OSS_DL_Remove *****************************
 *
 *  Description:  remove a node from a list
 *
 *---------------------------------------------------------------------------
 *  Input......:  n node to remove
 *  Output.....:  returns: same as input
 *  Globals....:  ---
 ****************************************************************************/
OSS_DL_NODE *OSS_DL_Remove( OSS_DL_NODE *n )
{
	n->next->prev = n->prev;
	n->prev->next = n->next;
	return n;
}

/********************************* OSS_DL_RemHead ****************************
 *
 *  Description: remove a node from the head of the list 
 *                         
 *---------------------------------------------------------------------------
 *  Input......:  l - ptr to list header
 *  Output.....:  returns: removed node ( NULL if list was empty )
 *  Globals....:  ---
 ****************************************************************************/
OSS_DL_NODE *OSS_DL_RemHead( OSS_DL_LIST *l )
{
	OSS_DL_NODE *n;
	if( l->head->next == NULL ) return NULL; /* list empty */

	n = l->head;
	l->head = n->next;
	n->next->prev = (OSS_DL_NODE *)&(l->head);

	return n;
}

/******************************** OSS_DL_AddTail *****************************
 *
 *  Description:  add a node at the tail to the list
 *---------------------------------------------------------------------------
 *  Input......:  l - ptr to list functions
 *				  n - node to add
 *  Output.....:  returns: added node
 *  Globals....:  ---
 ****************************************************************************/
OSS_DL_NODE *OSS_DL_AddTail( OSS_DL_LIST *l, OSS_DL_NODE *n )
{
	n->prev = l->tailpred;
	l->tailpred = n;
	n->next = n->prev->next;
	n->prev->next = n;
	return n;
}
	
