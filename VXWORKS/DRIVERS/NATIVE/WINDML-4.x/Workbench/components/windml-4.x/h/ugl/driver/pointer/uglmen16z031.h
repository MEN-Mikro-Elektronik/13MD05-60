/***********************  I n c l u d e  -  F i l e  ************************
 *
 *        \file  uglmen16z031.h
 *
 *      \author  Christian.Schuster@men.de
 *        $Date: 2005/12/19 19:25:23 $
 *    $Revision: 1.1 $
 *
 *       \brief  Header for the MEN 16Z031_SPI touch screen handler
 *
 *    \switches  none
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: uglmen16z031.h,v $
 * Revision 1.1  2005/12/19 19:25:23  cs
 * Initial Revision
 *
 * Revision 1.1  2005/07/19 13:57:18  CSchuster
 * Initial Revision
 *
 *
 * cloned from Wind River uwrsbcpxa250ts.h Revision 01a
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

/* uwrsbcpxa250ts.h - wrSbcPxa250 device header file */

/* Copyright 2002 Wind River Systems, Inc. All Rights Reserved */

/*
modification history
--------------------
01a,04oct02,rfm  created.
*/

/*
DESCRIPTION

This file provides data definitions for MEN 16Z031_SPI touch screen handler

*/
#ifndef __INCMEN16Z031H
#define __INCMEN16Z031H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef SYS_POINTER_NAME
#	define SYS_POINTER_NAME        "/touchScreen/0"
#endif /* SYS_POINTER_NAME */

#ifndef SYS_POINTER_DRIVER
#	define SYS_POINTER_DRIVER      &uglMen16z031TsDriver
#endif /* SYS_POINTER_NAME */

extern UGL_INPUT_DRV uglMen16z031TsDriver;

#ifdef __cplusplus
}
#endif

#endif /* __INCMEN16Z031H */

