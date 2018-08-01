/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  iswitch_sertest.c
 *
 *      \author  mko
 *        $Date: 2009/09/21 11:11:11 $
 *    $Revision: 0.1 $
 *
 *        \brief Test routines for the iSwitch series testbox
 *
 *     Switches: 
 */
/*---------------------------[ Public Functions ]----------------------------
 *  
 *-------------------------------[ History ]---------------------------------
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/


#include <stdio.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/eeprod.h>
#include "iswitch_tests.h"
#include "iswitch_id.h"

/* Debugging */
#undef DEBUG

#ifdef DEBUG
	#define debug(fmt,args...) printf("%s:%u - ",__FILE__,__LINE__);printf (fmt ,##args)
#else
	#define debug(fmt,args...)
#endif

#define CHECK_ERROR(x) if((ret = (x))){printf("***%s: %u - (0x%x/%u)\n",__FILE__,__LINE__,ret,ret);}

/* VERSION - increment after changes*/

/*  V3
	- included RS2-00/01
	V4
	- included RS4-00
	- included unmanaged w/o poe
	V5
	- changed needle adapter GB-UL test

*/

#define MATRIX_VERSION	5

/* Test UI */
#define TESTPROMPT	"\n--> "
#define CONSOLE_BUFFERSIZE	256
#undef INCLUDE_IDPROM

enum
{
	matrix_start,
	matrix_read_code,
	matrix_read_pd,
	matrix_test,
	matrix_end
};

enum
{
	pd_start,
	pd_ask,
	pd_ser,
	pd_prod,
	pd_rep,
	pd_check,
};

enum
{
	tst_start,
	tst_ask,
	tst_test,
};

enum
{
	day,
	month,
	year
};

/*
# Definitions describing the barcode:
*/
#define BC_ART_NAME_LEN	7
#define BC_MODEL_LEN	2
#define BC_REV_LEN		6
#define BC_DIVIDER		'_'
#define BC_REVDIV		'.'
/* BC_LEN is sum of above plus one '_' */
#define BC_LEN			(BC_ART_NAME_LEN+BC_MODEL_LEN+BC_REV_LEN+1)

#define is_digit(c)	((c) >= '0' && (c) <= '9')

/* according to fi_tmenu_type in f302_tests.h */
extern const u_int32 matrix_test_matrix[];
extern const matrix_testcase_desc matrix_testcases[];

char cli_buffer[CONSOLE_BUFFERSIZE]; /* console I/O buffer */
static u_int8 matrix_state_main = matrix_start;
static u_int32 art = 0;

static int matrix_bc2eeprod(char * code, EEPROD2 * pEEPROD);
int matrix_read_line(void);


int matrix_main(void)
{
	int len = 0, test_ret = 0, i = 0;
	u_int32 sel = 0, test_err = 0;
	u_int8 testmenu = 0;
	u_int8 bit = 0;
	u_int8 state_pd = pd_start;
	u_int8 state_test = tst_start;
	u_int16 date[3] = {0};
	EEPROD2 eeprod_new = {0};
	EEPROD2 * pEeprod_conf = {0};
	char code[BC_LEN+1] = {0};
	matrix_testcase_desc testcase = {0};

	printf(
		"\n\n\n\n\n"
		"####################################\n"
		"##  Matrix - Serientestmodul VX%02u ##\n"
		"####################################\n"
		,MATRIX_VERSION);  
 
	printf("Enter q to quit\n\n");

	/* check for NULLz */
	bit = 0;						
	while(bit++ < MAX_TESTS)
	{
		if(matrix_testcases[bit].name != NULL)
		{	
			if(matrix_testcases[bit].testfunc == NULL)
			{	
				debug("%u - testcase %u NULL!\n",__LINE__,bit);
				return ERROR;
			}
		}
	}

	/* # main test loop # */
	matrix_state_main = matrix_start;
	while(1)
	{
		switch(matrix_state_main)
		{
			case matrix_start:
				matrix_state_main = matrix_read_code;
				break;
			case matrix_read_code:	
				printf("Code einlesen: ");
				len = matrix_read_line();
				if(len <= 0)
					continue;
				if(len != BC_LEN)
				{
					printf("Länge des Codes muss %u sein (ist %u)\n",BC_LEN,len);
					continue;
				}
				strncpy(code,cli_buffer,BC_LEN);
				/* check if code ok */
				if(matrix_bc2eeprod(code,&eeprod_new) == OK)
				{
					/* print eeprod_new */
					printf("Produktionsdaten aus Code:\n");
					eeprod_print(&eeprod_new);
					/* identify board */
					art = article_find(&eeprod_new);
					if(art == 0)
					{
						printf(
							"Artikel %s%02d nicht gefunden\n",
							eeprod_new.pd_hwName,
							eeprod_new.pd_model
							);
						continue;
					}
#ifdef INCLUDE_IDPROM
					/* is it managed? */
					/* yes - continue ; no - goto tests */
					if(!(article_matrix[art].art_type == art_cpu_board) &&
						!(article_matrix[art].art_type == art_system))
					{
						matrix_state_main = matrix_test;
						continue;
					}
					if(article_matrix[art].art_type == art_cpu_board)
					{	/* cpu-board: first EEPROD */
						pEeprod_conf = &sysconfig.eeprod_cpu;
					}else
					{	/* system: second EEPROD */
						pEeprod_conf = &sysconfig.eeprod_sys;
					}
					
					printf("Gespeicherte Produktionsdaten:\n");
					eeprod_print(pEeprod_conf);
					state_pd = pd_ask;
					matrix_state_main = matrix_read_pd;
					continue;
#else					
					matrix_state_main = matrix_test;
					continue;
#endif					
				}else
				{
					printf("Code ungültig\n");
					matrix_state_main = matrix_start;
				}
				break;
#ifdef INCLUDE_IDPROM				
			case matrix_read_pd:
				switch(state_pd)
				{
					case pd_start:
						state_pd = pd_ask;
						break;
					case pd_ask:
						printf("Produktionsdaten ändern? (j/n): ");
						len = matrix_read_line();
						if (len <= 0)
							continue;
						if(len > 1)
							continue;
						if(cli_buffer[0] == 'j')
						{
							state_pd = pd_ser;
						}else if(cli_buffer[0] == 'n')
						{
							matrix_state_main = matrix_test;
							continue;
						}else
							continue;
					case pd_ser:
						printf("Seriennummer eingeben [1..999999]: ");
						len = matrix_read_line();
						if (len <= 0)
							continue;
						/* guess it will not be more than 999999 */
						if(len > 6)
							continue;
						else
						{
							eeprod_new.pd_serial = 
								strtoul(cli_buffer,NULL,10);
							state_pd = pd_prod;
						}
					case pd_prod:
						printf("Produktionsdatum eingeben (tt.mm.jjjj): ");
						len = matrix_read_line();
						if (
							(len != 10) || 
							(cli_buffer[2] != '.') || 
							(cli_buffer[5] != '.')
							)
							continue;
						else
						{
							date[day] = 
								(u_int8) strtoul(cli_buffer,NULL,10);
							date[month] = 
								(u_int8) strtoul(cli_buffer+3,NULL,10);
							date[year] = 
								(u_int16) strtoul(cli_buffer+6,NULL,10);
							if((len = eeprod_check_date(date[day],date[month],date[year])))
							{
								debug("%u - eeprod_check_date = %u",__LINE__,len);
								continue;
							}
							if((len = eeprod_set_prodat(&eeprod_new,date[day],date[month],date[year])))
							{
								debug("%u - eeprod_set_prodat = %u",__LINE__,len);
								continue;
							}
							state_pd = pd_rep;
						}
					case pd_rep:
						printf("Reparaturdatum eingeben (tt.mm.jjjj oder leer): ");
						len = matrix_read_line();
						if(len == 0)
						{
							state_pd = pd_check;
							continue;
						}else if(
							(len != 10) || 
							(cli_buffer[2] != '.') || 
							(cli_buffer[5] != '.')
							)
							continue;
						else
						{
							date[day] = 
								(u_int8) strtoul(cli_buffer,NULL,10);
							date[month] = 
								(u_int8) strtoul(cli_buffer+3,NULL,10);
							date[year] = 
								(u_int16) strtoul(cli_buffer+6,NULL,10);
							if((len = eeprod_check_date(date[day],date[month],date[year])))
							{
								debug("%u - eeprod_check_date = %u",__LINE__,len);
								continue;
							}
							if((len = eeprod_set_repdat(&eeprod_new,date[day],date[month],date[year])))
							{
								debug("%u - eeprod_set_repdat = %u",__LINE__,len);
								continue;
							}
							state_pd = pd_rep;
						}
					case pd_check:
						/* print data and ask to write */
						eeprod_print(&eeprod_new);
						printf("Produktionsdaten schreiben? (j/n): ");
						len = matrix_read_line();
						if (len <= 0)
							continue;
						if(len > 1)
							continue;
						if(cli_buffer[0] == 'j')
						{
							/* copy to sysconf */
							memcpy(pEeprod_conf,&eeprod_new,sizeof(EEPROD2));
							if((len = eeprod_write(pEeprod_conf)))
							{
								printf("***%u - eeprod_write (%u)",__LINE__,len);
								continue;
							}
							matrix_state_main = matrix_test;
						}else if(cli_buffer[0] == 'n')
						{
							matrix_state_main = matrix_test;
							continue;
						}else
							continue;
				}
#endif
			case matrix_test:
				switch(state_test)
				{
					case tst_start:
						testmenu = 
							article_matrix[(u_int32) art].board_type;
						/* print available tests and ask */
						bit = 0;						
						printf("\n### %s%u - Verfügbare Tests: ###\n\n",
							article_matrix[(u_int32) art].name,
							article_matrix[(u_int32) art].model);
						debug(
							"%u - art: %u, testmenu: %u\n",
							__LINE__,
							art,
							testmenu
							);
						printf("a\tall\n\n");
						for(bit = 0; bit < MAX_TESTS; bit++)
						{
							if(matrix_test_matrix[testmenu] & (1 << bit)) 
							{
								printf("%d\t%s\n",bit, matrix_testcases[bit].name);
							}
						}
						state_test = tst_ask;
						break;
					case tst_ask:
						len = matrix_read_line();
						if (len <= 0)
							continue;
						if(len > 2)
							continue;
						state_test = tst_test;
						if(is_digit(cli_buffer[0]))
						{
							/* two digits */
							if(len == 2)
							{
								/* sainity checks */
								if(!(is_digit(cli_buffer[0]) || 
									(cli_buffer[0] > (MAX_TESTS / 10))))
									continue;
								bit = (cli_buffer[0] - '0') * 10;
								bit += cli_buffer[1] - '0'; /* atoi */
							}else
								bit = cli_buffer[0] - '0'; /* atoi */
							sel = (1<<bit);
							/* test must be available */
							sel &= matrix_test_matrix[testmenu];
							if(sel == 0)
								state_test = tst_start;
						}else if(cli_buffer[0] == 'a')
						{
							sel = matrix_test_matrix[testmenu];
						}/* nothing usable */
						else
							state_test = tst_start;
					case tst_test:
						printf("\n###Starte tests: ###\n\n");
						test_err = 0;
						for(bit = 0;bit < MAX_TESTS;bit++)
						{
							if(sel & (1 << bit)) 
							{
								/* valid test? */
								if(matrix_testcases[bit].name == NULL)
								{
									debug("%u - test name NULL\n",__LINE__);
									break;
								}
								/* print testname */
								printf("\nTest %s running...\n",matrix_testcases[bit].name);
								/* execute */
								testcase.testfunc = matrix_testcases[bit].testfunc;
								test_ret = testcase.testfunc(art);
								printf("%s\n\n", test_ret ? "## FEHLER ##" : "## OK ##");
								test_err |= test_ret ? (1<<bit) : 0;
								/* print summary */
								for(i = 0; i < MAX_TESTS; i++)
								{	
									/* print only performed tests */
									if(sel & (1<<i)) 
									{
										if(i > bit)
											continue;
										printf("Test %-30s %s\n",matrix_testcases[i].name, (test_err & (1<<i)) ? "FEHLER" : "OK");
									}
								}
							}
						}
						printf("\n### Ende ###\n\n");
						/* print result */			
						#if 0
						for(bit = 0;bit < MAX_TESTS;bit++)
						{	
							/* print only performed tests */
							if(sel & (1<<bit)) 
							{
								printf("Test %-30s %s\n",fi_testcases[bit].name, (test_err & (1<<bit)) ? "FEHLER" : "OK");
							}
						}
						putc('\n');
						#endif
						printf("Eine Taste drücken um fortzufahren.");
						getc(stdin);
						state_test = tst_start;
				}
				break;
			case matrix_end:
				printf("Verlasse Test. Neustart mit \"matrix_main\".\n");
				return OK;
			default:
				printf("***%u - matrix_state_mismatch!\n",__LINE__);
				return ERROR;
		}
	}  
	return OK;
}

int matrix_read_line(void)
{
	int len = 0;
	/* read from cli */
	printf(TESTPROMPT);
	gets(cli_buffer);
	len = strlen(cli_buffer);

	debug("%u - reading: \"%s\"\n",__LINE__,cli_buffer);
	
	if((len == 1) &&
		(cli_buffer[0] == 'q'))
	{
		matrix_state_main = matrix_end;
		return 0;
	}

	return len;
}

static int matrix_bc2eeprod(char * code, EEPROD2 * pEEPROD)
{
	char * charpnt;
	
	if(code == NULL)
	{
		printf("***%u - code NULL!\n",__LINE__);
		return ERROR;
	}
	if(pEEPROD == NULL)
	{
		printf("***%u - pEEPROD NULL!\n",__LINE__);
		return ERROR;
	}
	if(strlen(code) != BC_LEN)
	{
		printf("Code zu kurz (soll: %u; ist: %u)\n",BC_LEN,strlen(code));
		return ERROR;
	}
	/* for BC_DIVIDER */
	if(code[BC_ART_NAME_LEN+BC_MODEL_LEN] != BC_DIVIDER)
	{
		printf(
			"Code ungültig: Erwarte '%c' an Stelle %u (ist '%c')\n",
			BC_DIVIDER,
			BC_ART_NAME_LEN+BC_MODEL_LEN,
			code[BC_ART_NAME_LEN+BC_MODEL_LEN] 
			);
		return ERROR;
	}
	/* copy name - without article group*/
	strncpy(pEEPROD->pd_hwName,code+2,BC_ART_NAME_LEN-2);
	/* get model */
	pEEPROD->pd_model = (u_int8) strtoul(code+BC_ART_NAME_LEN,NULL,10);
	/* check for BC_REVDIV */
	charpnt = code + strlen(code) - 3;
	if(*charpnt != BC_REVDIV)
	{
		printf(
			"Code ungültig: Erwarte '%c' an Stelle %u (ist '%c')\n",
			BC_REVDIV,
			strlen(code) - 3,
			*charpnt 
			);
		return ERROR;
	}
	/* get revision */
	pEEPROD->pd_revision[0] = (u_int8) strtoul(charpnt-2,NULL,10);
	pEEPROD->pd_revision[1] = (u_int8) strtoul(charpnt+1,NULL,10);
	/* we made it */
	return OK;
}

