/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  iswitch_tests.c
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
#include <MEN/men_typs.h>
#include <MEN/ep02_cfg.h>
#include <MEN/eeprod.h>
#include <drv/multi/ppc860Siu.h>
#include "ipproto.h"
#include <fioLib.h>
#include <taskLib.h>
#include "tickLib.h"
#include <sysLib.h>
#include "trs.h"
#include "aes.h"
#include "aes_eeprom.h"
#include "ee9346.h"
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

/* Globals */
/*
# Here the tests are added to the test menues.
The test matrix has a u_int32 for each test menue.
If a test is part of the menue a bit is set accordingly.

Compare to fi_tmenu_type in f302_id.h */
const u_int32 matrix_test_matrix[]=
{
	/* dummy */
	0,
	/* id_cpu_pse - CPU board mit PSE */
	(1<<tc_pbvlan),
	/* id_cpu - CPU board */
	(1<<tc_pbvlan),
	/* id_cpu_pse_ul - CPU board mit PSE und GBUL */
	(1<<tc_pbvlan),
	/* id_cpu_pci - CPU board mit PCIUL */
	(1<<tc_pbvlan),
	/* id_fp_f - Frontboard F302 */
	(1<<tc_pbvlan),
	/* id_fp_r - Frontboard RS */
	(1<<tc_pbvlan),
	/* id_fp_r_poe - Frontboard RS mit PoE */
	(1<<tc_pbvlan),
	/* id_fp_r_ul_poe - Frontboard RS mit PoE+GBUL */
	(1<<tc_pbvlan),
	/* id_um_poe - Unmanaged systems w/ poe */
	(1<<tc_ethernet) | (1<<tc_poe_pse),
	/* id_um_ul_poe - Unmanaged systems w/ GBUL w/ poe*/
	(1<<tc_poe_pse) | (1<<tc_uplink),
	/* id_um_fp_r - unmanaged frontboard - tested w/ uboot */
	(1<<tc_pbvlan),
	/* id_um_fp_r_ul - unmanaged frontboard w/ ul- tested w/ uboot */
	(1<<tc_pbvlan),
	/* id_um - Unmanaged systems */
	(1<<tc_ethernet)
};
extern char cli_buffer[]; /* console I/O buffer	*/

/* this table must comply the "test cases" enum in iswitch_tests.h */
const matrix_testcase_desc matrix_testcases[MAX_TESTS] =
{
	{"Portbased VLANs", iSwitch_LoopSetup_pbvlan},
	{"Ethernet", iSwitch_LoopTest_eth},
	{"Ethernet+UL", iSwitch_LoopTest_ul},
	{"PoE PSE", iSwitch_LoopTest_PSE},
};

#define PORTS_MAX	0xb
/* port based vlan maps */
typedef u_int16 vlan_map[PORTS_MAX];

static vlan_map vmap_LNPC = {
	1<<0,
	1<<2,
	1<<1,
	1<<4,
	1<<3,
	1<<6,
	1<<5,
	1<<9,
	0,
	1<<7};

static vlan_map vmap_NP = {
	1<<1,
	1<<0,
	1<<3,
	1<<2,
	1<<5,
	1<<4,
	1<<7,
	1<<6,
	0,
	1<<0};

static vlan_map vmap_NPCC = {
	1<<1,
	1<<0,
	1<<3,
	1<<2,
	1<<5,
	1<<4,
	1<<9,
	1<<9,
	0,
	1<<6 | 1<<7};

static vlan_map vmap_CL = {
	1<<9,
	1<<1,
	1<<1,
	1<<4,
	1<<3,
	1<<6,
	1<<5,
	1<<9,
	0,
	1<<7};
	
static vlan_map vmap_DEF = {
	0x7FF & ~(1<<0),
	0x7FF & ~(1<<1),
	0x7FF & ~(1<<2),
	0x7FF & ~(1<<3),
	0x7FF & ~(1<<4),
	0x7FF & ~(1<<5),
	0x7FF & ~(1<<6),
	0x7FF & ~(1<<7),
	0x7FF & ~(1<<8),
	0x7FF & ~(1<<9)};

enum
{
	poe_tbox_c3_c3 = 0,
	poe_tbox_c2_c3 = 1,
	poe_tbox_c3_c2 = 2,
	poe_tbox_c2_c2 = 3
};

enum
{
	poe_tbox_off_off = 0,
	poe_tbox_on_off = 1,
	poe_tbox_off_on = 2,
	poe_tbox_on_on = 3
};

static int pbVlanMap(vlan_map map);
static int EnablePort(u_int8 port);
static int DisablePort(u_int8 port);
static int LoopTest(u_int16 portstatemask, vlan_map map);
static int pdEnable(u_int8 ena);
static int pdClassSet(u_int8 val);
static int iSwitch_LoopSetupMap( int speed, vlan_map map);
static int pdPgGet(u_int8 port);


int iSwitch_LoopSetup_pbvlan( u_int32 art )
{
	int ret = 0;
	vlan_map * map = NULL;

	if(article_matrix[art].art_type == art_system)
		map = &vmap_LNPC;
	else
		map = &vmap_NP;
	
	/* setup vlan map */
	iSwitch_LoopSetupMap(100,*map);
	printf("\n# Test kann gestartet werden.\n");
    getc(stdin);
	pbVlanMap(vmap_DEF);
	
	return ret;
}

/* test for systems w/ ul */

int iSwitch_LoopTest_ul( u_int32 art )
{
	int ret = 0;

	/* test port 0-7 */
    AES_fecUnitTX = MII1;
	AES_fecUnitRX = MII1;
	CHECK_ERROR(LoopTest(0xfff & ~(1<<7),vmap_NPCC));
	/* test port 8 + UL */
    AES_fecUnitTX = MII1;
	if(article_matrix[art].art_type == art_system_else)
		AES_fecUnitRX = MII2;
	else
		AES_fecUnitRX = MII1; /* needle adapter is looped back */
	CHECK_ERROR(LoopTest((1<<7) | (1<<9),NULL));

	pbVlanMap(vmap_DEF);
	
	return ret;
}

/* test for systems w/o ul */
int iSwitch_LoopTest_eth( u_int32 art )
{
	int ret = 0;

    AES_fecUnitTX = MII1;
	AES_fecUnitRX = MII1;
	/* test port 0-8 */
	CHECK_ERROR(LoopTest(0xfff,vmap_LNPC));

	pbVlanMap(vmap_DEF);
	
	return ret;
}

int iSwitch_LoopTest_PSE( u_int32 art )
{
	int ret = OK;
	vlan_map * map = vmap_DEF;
	u_int16 portmask = 0;
	u_int16	timer_1s = 0;

    AES_fecUnitTX = MII1;
	AES_fecUnitRX = MII1;
	/* turn off PDs */
    if(pdEnable(poe_tbox_off_off)){
		printf("Fehler - Kann PD nicht deaktivieren\n");
		ret = ERROR;
		goto exit;
    	}
	
	/* Uplink? */
	if((article_matrix[art].gb_uplink))
	{
		map = &vmap_NPCC;
		portmask = 0xfff & ~(1<<7);
	}
	else
	{
		map = &vmap_LNPC;
		portmask = 0xfff;
	}
	/* check power good */
	if(!(pdPgGet(0))){
		printf("Fehler - Port 1 PoE aktiv\n");
		ret = ERROR;
		goto exit;
    	}
	if(!(pdPgGet(1))){
		printf("Fehler - Port 2 PoE aktiv\n");
		ret = ERROR;
		goto exit;
    	}
	/* set PD class 15W / off */
	if(pdClassSet(poe_tbox_c3_c2)){
		printf("Fehler - Kann PoE Klassen nicht konfigurieren\n");
		ret = ERROR;
		goto exit;
		}
	/* turn on PD on Port1  */
    if(pdEnable(poe_tbox_on_off)){
		printf("Fehler - Kann PD nicht aktivieren\n");
		ret = ERROR;
		goto exit;
    	}
	/* check power good */
	timer_1s = 5;
	do
	{
		taskDelay(sysClkRateGet());
		if(pdPgGet(0) == OK)
		{
			break;
    	}
	}while(timer_1s--);
	if(pdPgGet(0)){
		printf("Fehler - Port 1 kein PoE\n");
		ret = ERROR;
		goto exit;
    }
	/* test port 0-7 */
	if(LoopTest(portmask,*map)){
		printf("Fehler - Looptest nicht erfolgreich\n");
		ret = ERROR;
		goto exit;
		}
	/* set PD class off / 15W */
	if(pdClassSet(poe_tbox_c2_c3)){
		printf("Fehler - Kann PoE Klassen nicht konfigurieren\n");
		ret = ERROR;
		goto exit;
		}
	/* turn on PD on Port2  */
    if(pdEnable(poe_tbox_off_on)){
		printf("Fehler - Kann PD nicht aktivieren\n");
		ret = ERROR;
		goto exit;
    	}
	/* check power good */
	timer_1s = 5;
	do
	{
		taskDelay(sysClkRateGet());
		if(pdPgGet(1) == OK)
		{
			break;
    	}
	}while(timer_1s--);
	if(pdPgGet(1)){
		printf("Fehler - Port 2 kein PoE\n");
		ret = ERROR;
		goto exit;
    }
	/* test port 0-7 */
	if(LoopTest(portmask, NULL)){
		printf("Fehler - Looptest nicht erfolgreich\n");
		ret = ERROR;
		goto exit;
		}

	/* set PD class 7W / 7W */
	if(pdClassSet(poe_tbox_c2_c2)){
		printf("Fehler - Kann PoE Klassen nicht konfigurieren\n");
		ret = ERROR;
		goto exit;
		}
	/* turn on PD on Port1+2  */
    if(pdEnable(poe_tbox_on_on)){
		printf("Fehler - Kann PD nicht aktivieren\n");
		ret = ERROR;
		goto exit;
    	}
	/* check power good */
	timer_1s = 5;
	do
	{
		taskDelay(sysClkRateGet());
		if(pdPgGet(0) == OK)
		{
			break;
    	}
	}while(timer_1s--);
	if(pdPgGet(0)){
		printf("Fehler - Port 1 kein PoE\n");
		ret = ERROR;
		goto exit;
    }
	taskDelay(sysClkRateGet()); /* one more sec. */
	if(pdPgGet(1)){
		printf("Fehler - Port 2 kein PoE\n");
		ret = ERROR;
		goto exit;
    	}
	/* test port 0-7 */
	if(LoopTest(portmask,NULL)){
		printf("Fehler - Looptest nicht erfolgreich\n");
		ret = ERROR;
		goto exit;
		}

exit:
	/* cleanup */
	pdEnable(poe_tbox_off_off);
	pbVlanMap(vmap_DEF);
	
	return ret;
}


/**********************************************************************/
/** Setup for big looptest over all ports
 * 
 * \param speed \IN 100MBit (100) or 10MBit (10)
 *
 * MII1:
 * CPU -> Port 9 -> Port7 -> cable -> Port 6 -> Port 5 -> cable -> Port 4 ->
 * Port 3 -> cable -> Port 2 -> Port 1
 * Port 0 = loopback
 *
 * MII2: copper loopback cable
 * 
 * \return    \OUT OK 
 */
static int iSwitch_LoopSetupMap( int speed, vlan_map map)
{

    AES_MiiInit();
    
    TRS_RelPxOn( 0, 1 );
    TRS_RelPxOn( 1, 1 );
    TRS_BypPxOn( 0, 0 );
    TRS_BypPxOn( 1, 0 );    

    taskDelay(sysClkRateGet()/10);
    
    /* setup port VLANs */
	pbVlanMap(map);
    
    if( speed == 100 )
    	TRS_MiiWrite( MV88E609X_SMI_PHY( 7 ), 0x00, 0xa100 );
    else
    	TRS_MiiWrite( MV88E609X_SMI_PHY( 7 ), 0x00, 0x8100 );
    
    /* wait for link up */
    while( (TRS_MiiRead( MV88E609X_SMI_PHY( 0 ), 0x11 ) & 0x0400) == 0 )
        taskDelay( 1 );    
   
	return OK;
}



/**********************************************************************/
/** Setup for big looptest over all ports (LNPC)
 * 
 * \param speed \IN 100MBit (100) or 10MBit (10)
 *
 * MII1:
 * CPU -> Port 9 -> Port7  -> cable -> Port 6 -> Port 5 -> cable -> Port 4 ->
 * Port 3 -> cable -> Port 2 -> Port 1
 * Port 0 = loopback
 *
 * MII2: copper loopback cable
 * 
 * \return    \OUT OK 
 */
int iSwitch_LoopSetupLNPC( int speed )
{

    AES_MiiInit();
    
    TRS_RelPxOn( 0, 1 );
    TRS_RelPxOn( 1, 1 );
    TRS_BypPxOn( 0, 0 );
    TRS_BypPxOn( 1, 0 );    

    taskDelay(sysClkRateGet()/10);
    
    /* setup port VLANs */
	pbVlanMap(vmap_LNPC);
        
    if( speed == 100 )
    	TRS_MiiWrite( MV88E609X_SMI_PHY( 7 ), 0x00, 0xa100 );
    else
    	TRS_MiiWrite( MV88E609X_SMI_PHY( 7 ), 0x00, 0x8100 );
    
    /* wait for link up */
    while( (TRS_MiiRead( MV88E609X_SMI_PHY( 1 ), 0x11 ) & 0x0400) == 0 )
        taskDelay( 1 );    
   
	return OK;
}

/**********************************************************************/
/** Setup for big looptest over all ports (NP)
 * 
 * \param speed \IN 100MBit (100) or 10MBit (10)
 *
 * MII1:
 * CPU -> Port 8 -> Port7 -> cable -> Port 6 -> Port 5 -> cable -> Port 4 ->
 * Port 3 -> cable -> Port 2 -> Port 1
 * Port 0 = loopback
 *
 * MII2: copper loopback cable
 * 
 * \return    \OUT OK 
 */
int iSwitch_LoopSetupNP( int speed )
{
    AES_MiiInit();
    
    TRS_RelPxOn( 0, 1 );
    TRS_RelPxOn( 1, 1 );
    TRS_BypPxOn( 0, 0 );
    TRS_BypPxOn( 1, 0 );    
    	    	
    taskDelay(sysClkRateGet()/10);
    	
    
    /* setup port VLANs */
	pbVlanMap(vmap_NP);
        
    if( speed == 100 )
    	TRS_MiiWrite( MV88E609X_SMI_PHY( 7 ), 0x00, 0xa100 );	
    else
    	TRS_MiiWrite( MV88E609X_SMI_PHY( 7 ), 0x00, 0x8100 );
    
    /* wait for link up */
    while( (TRS_MiiRead( MV88E609X_SMI_PHY( 7 ), 0x11 ) & 0x0400) == 0 )
        taskDelay( 1 );    
   
	return OK;
}

/**********************************************************************/
/** Setup for big looptest over all ports (NPCC)
 * 
 * \param speed \IN 100MBit (100) or 10MBit (10)
 *
 * MII1:
 * CPU -> Port 8; CPU -> Port7 -> cable -> Port 6 -> Port 5 -> cable -> Port 4 ->
 * Port 3 -> cable -> Port 2 -> Port 1
 * Port 0 = loopback
 *
 * MII2: copper loopback cable
 * 
 * \return    \OUT OK 
 */
int iSwitch_LoopSetupNPCC( int speed )
{
    AES_MiiInit();
    
    TRS_RelPxOn( 0, 1 );
    TRS_RelPxOn( 1, 1 );
    TRS_BypPxOn( 0, 0 );
    TRS_BypPxOn( 1, 0 );    
    	    	
    taskDelay(sysClkRateGet()/10);
    
    /* setup port VLANs */
	pbVlanMap(vmap_NPCC);
        
    if( speed == 100 )
    	TRS_MiiWrite( MV88E609X_SMI_PHY( 7 ), 0x00, 0xa100 );
    else
    	TRS_MiiWrite( MV88E609X_SMI_PHY( 7 ), 0x00, 0x8100 );
    
    /* wait for link up */
    while( (TRS_MiiRead( MV88E609X_SMI_PHY( 7 ), 0x11 ) & 0x0400) == 0 )
        taskDelay( 1 );    
   
	return OK;
}

/* if map == NULL, port based VLAN configuratinon will not be changed */
static int LoopTest(u_int16 portstatemask, vlan_map map)
{
	int ret = 0, res = 0;
 	u_int8 i = 0;

	CHECK_ERROR(AES_MiiInit());
    
    TRS_RelPxOn( 0, 1 );
    TRS_RelPxOn( 1, 1 );
    TRS_BypPxOn( 0, 0 );
    TRS_BypPxOn( 1, 0 );    
	    	    	
    taskDelay(sysClkRateGet()/10);

	if(map != NULL)
		pbVlanMap(map);
	
	
	CHECK_ERROR(ipAttach(MII1,"motfec"));
	CHECK_ERROR(ipAttach(MII2,"motfec"));
    /* CHECK_ERROR(ifAddrSet("motfec0", "192.1.2.47"));
	CHECK_ERROR(ipDetach(MII2,"motfec"));*/

	taskDelay(sysClkRateGet()); 
	
	for(i=0;i<PORTS_MAX;i++)
	{
		if(portstatemask & (1<<i))
			EnablePort(i);
		else
			DisablePort(i);
	}

	/* wait for link up */
	while( (TRS_MiiRead( MV88E609X_SMI_PHY( 6 ), 0x11 ) & 0x0400) == 0 )
		taskDelay(sysClkRateGet()/10); 

	#ifdef DEBUG
	res = AES_Loopframesv2( 2, 0, 0, NULL, 0);
	#else
	res = AES_Loopframesv2( 2, 0, 0, NULL, 5);
	#endif
/*
	CHECK_ERROR(ipDetach(0,"motfec"));
	CHECK_ERROR(ipAttach(1,"motfec"));
*/
	
	return res;
}


static int pbVlanMap(vlan_map map)
{
	int i = 0;
	int ret = 0;
	
	for(i = 0; i < PORTS_MAX; i++)
	{
		debug("PBVLan 0x%x for port %u\n",map[i],i);
		CHECK_ERROR(TRS_MiiWrite( MV88E609X_SMI_PORT( i ), MV88E609X_PORT_VLANMAP, map[i]));
	}
	return OK;
}

#define MVL_PORT_PC_PORT_STATE_BITS	0x3

static int DisablePort(u_int8 port)
{
	u_int16 dat = 0;
	/* disable port */
	dat = TRS_MiiRead(MV88E609X_SMI_PORT( port ), MV88E609X_PORT_CONTROL);
	dat &= ~MVL_PORT_PC_PORT_STATE_BITS;
	TRS_MiiWrite(
		MV88E609X_SMI_PORT( port ), 
		MV88E609X_PORT_CONTROL,
		dat);
	debug("Port %u disable\n",port);
	return OK;
}

static int EnablePort(u_int8 port)
{
	u_int16 dat = 0;
	/* port to forwarding */
	dat = TRS_MiiRead(MV88E609X_SMI_PORT( port ), MV88E609X_PORT_CONTROL);
	dat |= MVL_PORT_PC_PORT_STATE_BITS;
	TRS_MiiWrite(
		MV88E609X_SMI_PORT( port ), 
		MV88E609X_PORT_CONTROL,
		dat);
	debug("Port %u enable\n",port);
	return OK;
}

static int pdEnable(u_int8 val)
{
	int 	ret = OK;
	
	debug("pdEnable - %u\n",val);
	CHECK_ERROR(TRS_SmbWrite( ADDR_LENGTH_8, A7810_Expander_PDon, 0x0, val));
	return ret;
}

static int pdClassSet(u_int8 val)
{
	int ret = OK;

	CHECK_ERROR(TRS_SmbWrite( ADDR_LENGTH_8, A7810_Expander_PDClass, 0x0, val ));
	debug("pdClassSet - PD1 class %u, PD2 class %u\n",(val&1)?2:3,(val&2)?2:3);
	taskDelay(sysClkRateGet()/10);
	return ret;
}

static int pdPgGet(u_int8 port)
{
	u_int16 dat = 0;
    dat = TRS_SmbRead( ADDR_LENGTH_8, A7810_Expander_PD_PG, 0xff ); 
	debug("pdPgGet -  0x%X\n", dat);
    if (dat & (1<<port))
	{   
		return OK;
    }
    else 
	{
        return ERROR;
    }
}

