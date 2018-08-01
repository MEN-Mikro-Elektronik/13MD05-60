/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  iswitch_id.c
 *
 *      \author  mko
 *        $Date: 2009/01/26 16:23:32 $
 *    $Revision: 1.7 $
 *
 *        \brief Article identification module
 *
 *     Switches: 
 */
/*---------------------------[ Public Functions ]----------------------------
 *  
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: iswitch_id.c,v $
 *
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/


#include <stdio.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/eeprod.h>
#include <sys/types.h>
#include "iswitch_id.h"

/* Debugging */
#undef DEBUG

#ifdef DEBUG
	#define debug(fmt,args...) printf("%s:%u - ",__FILE__,__LINE__);printf (fmt ,##args)
#else
	#define debug(fmt,args...)
#endif


/* Feature matrix
{pci_uplink, poe, gb_uplink, name, model, type (EEPROD), test menu}
*/
const id_board_desc article_matrix[] = 
{
	/* n/def */
	{0, 0,	0,	"dummy",	0,	art_else, 0},
	/* CPU boards */
	{1, 0,	0,	"F302P",	0,	art_cpu_board, id_cpu_pci},
	{1, 0,	0,	"F302P",	6,	art_cpu_board, id_cpu_pci},
	{0, 1,	0,	"F302P",	2,	art_cpu_board, id_cpu_pse},
	{0, 1,	0,	"F302P",	7,	art_cpu_board, id_cpu_pse},   
	{0, 1,	0,	"AE31-", 	0,	art_cpu_board, id_cpu_pse},
	{0, 1,	1,	"AE31-", 	1,	art_cpu_board, id_cpu_pse_ul},
	{0, 0,	0,	"AE34-", 	0,	art_cpu_board, id_cpu},
	{0, 0,	0,	"AE31-", 	4,	art_cpu_board, id_cpu},
	/* Systems */
	{0, 1,	0,	"SF02-",	0,   art_system, id_cpu_pse},
	{0, 1,	0,	"SF03-",	0,   art_system, id_cpu_pse},
	{0, 1,	0,	"SF03-",	1,   art_system, id_cpu_pse},
	{1, 0,	0,	"F302-",	0,   art_system, id_cpu_pci},
	{1, 0,	0,	"F302-",	2,   art_system, id_cpu_pci},
	{0, 1,	0,	"RS01-",	0,   art_system, id_cpu_pse},
	{0, 1,	0,	"RS01-",	4,   art_system, id_cpu_pse},
	{0, 1,	1,	"RS01-",	1,   art_system, id_cpu_pse_ul},
	{0, 1,	1,	"RS01-",	5,   art_system, id_cpu_pse_ul},
	{0, 0,	0,	"RS03-",	0,   art_system, id_cpu},
	{0, 0,	0,	"RS01-",	6,   art_system, id_cpu},
	/* Frontboards (no EEPROD) */
	{0, 0,	0,	"F302B",	0,	art_else, id_fp_r_poe},
	{0, 0,	0,	"F302B",	1,	art_else, id_fp_r_poe},
	{0, 0,	0,	"F302D",	0,	art_else, id_fp_r_poe},
	{0, 0,	0,	"F302D",	1,	art_else, id_fp_r_poe},
	{0, 1,	0,	"AE28-", 	0,	art_else, id_fp_r_poe},
	{0, 1,	0,	"AE28-", 	4,	art_else, id_fp_r_poe},
	{0, 1,	1,	"AE28-", 	1,	art_else, id_fp_r_ul_poe},
	{0, 0,	1,	"AE28-", 	5,	art_else, id_fp_r_ul_poe},
	{0, 0,	0,	"AE28-", 	8,	art_else, id_fp_r},
	/* Frontboards unmanaged - test with uboot */
	{0, 1,	0,	"AE28-", 	2,	 art_else, id_um_fp_r},
	{0, 1,	0,	"AE28-", 	6,	 art_else, id_um_fp_r},
	{0, 1,	1,	"AE28-", 	3,	 art_else, id_um_fp_r_ul},
	{0, 1,	1,	"AE28-", 	7,	 art_else, id_um_fp_r_ul},
	/* Unmanaged (no EEPROD) - test with testbox */
	{0, 1,	0,	"AE31-",	2,	art_else, id_um_poe},
	{0, 1,	1,	"AE31-",	3,	art_else, id_um_ul_poe},
	{0, 0,	0,	"AE34-",	1,	art_else, id_um},
	{0, 1,	0,	"RS02-",	0,	art_system_else, id_um_poe},
	{0, 1,	1,	"RS02-",	1,	art_system_else, id_um_ul_poe},
	{0, 1,	0,	"RS02-",	4,	art_system_else, id_um_poe},
	{0, 1,	1,	"RS02-",	5,	art_system_else, id_um_ul_poe},
	{0, 1,	0,	"RS04-",	0,	art_system_else, id_um},
	{0, 1,	0,	"SF01-",	0,	art_system_else, id_um_poe},
	{0, 1,	0,	"SF04-",	0,	art_system_else, id_um_poe},
	{0, 1,	0,	"F302P",	4,	art_else, id_um_poe},
	{0, 1,	0,	"F302P",	8,	art_else, id_um_poe},
	/* empty */
	{0xFF, 0,  0,  "end",	   0,  art_else, 0}
};
	


u_int32  article_find(EEPROD2 * pEEPROD)
{
	u_int32  idx = 0; 

	/* check name and model*/
	idx = 0;
	while(article_matrix[idx++].pci_uplink != 0xff)
	{
		if(idx == 0xFFFF)	/* prevent endless loop */
			break;
		debug("%u - comparing idx %u\n",__LINE__,idx);
		if(strcmp(pEEPROD->pd_hwName, article_matrix[idx].name)) /* name */
			continue;
		debug("%u - matching name \"%s\"\n",__LINE__,article_matrix[idx].name);
		if(pEEPROD->pd_model != article_matrix[idx].model)  /* model */
			continue;
		debug("%u - matching model %u\n",__LINE__,article_matrix[idx].model);
		/* match */
		return idx;
	}
	return 0;
}

/**********************************************************************/
/** Routine to print an EEPROD structure
 *
 *	This routine prints an EEPROD structure 
 *
 *	\param		pEEPROD -	pointer to EEPROD structure
 *
 *	\return 	OK/ERROR
 */
u_int32  eeprod_print(EEPROD2 * pEEPROD)
{
	u_int8 day = 0, month = 0;
	u_int16 year = 0;

	if(pEEPROD == NULL)
		return 1;

	printf( " HW-Name    = %s\n"
			" Model      = %02u\n"
			" Serial#    = %06u\n"		   
			" Revision   = %02u.%02u.%02u\n", 
			pEEPROD->pd_hwName,
			pEEPROD->pd_model,
			(u_int32) pEEPROD->pd_serial,
			pEEPROD->pd_revision[0],
			pEEPROD->pd_revision[1],
			pEEPROD->pd_revision[2]);
	/*
  - Bits 15..9 (7 bits) contain the year since 1990 in binary format.
			   This allows a range from 1990..2117
  - Bits  8..5 (4 bits) contain the month in binary format. (1..12)
  - Bits  4..0 (5 bits) contain the day of month in binary format (1..31)
  */  
	/* ProDat */
	day = pEEPROD->pd_prodat & 0x001F;
	month = (pEEPROD->pd_prodat >> 5)  & 0x000F;
	year = (pEEPROD->pd_prodat >> 9)  & 0x007F;
	year += EEPROD2_DATE_YEAR_BIAS;
	printf(" ProDat     = %02u.%02u.%04u\n", day, month, year);
	/* RepDat */
	day = pEEPROD->pd_repdat& 0x001F;
	month = (pEEPROD->pd_repdat>> 5)  & 0x000F;
	year = (pEEPROD->pd_repdat>> 9)  & 0x007F;
	year += EEPROD2_DATE_YEAR_BIAS;
	printf(" RepDat     = %02u.%02u.%04u\n", day, month, year);

	return 0;
}


