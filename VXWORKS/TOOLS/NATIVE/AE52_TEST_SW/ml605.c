
/********************* P r o g r a m - M o d u l e ***********************/
/*!
* \file ml605.c
*
* \author maximilian.kolpak@men.de
* $Date: 2012/04/13 09:37:16 $
* $Revision: 1.1 $
*
* \brief This is the driver for the ml605 IP core.
*
* Remember that Xilinx uses MSB numbering (0 = MSB).
*
* Switches: -
*/
/*-------------------------------[ History ]---------------------------------
*
* $Log: ml605.c,v $
* Revision 1.1  2012/04/13 09:37:16  MKolpak
* Initial Revision
*
*
*---------------------------------------------------------------------------
* (c) Copyright 2011 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <vxWorks.h>
#include <sysLib.h>
#include <spinLockLib.h>

#include <MEN/men_typs.h>
#include <MEN/oss.h>

#include "ml605.h"


/* error reporting and debugging */
#define DBG_MYLEVEL	g_ml605_verbosity
#include <MEN/dbg.h>        /* debug module */

#undef DEBUG_LOCAL
#undef DEBUG_ACCESS

#ifdef DEBUG_LOCAL
	#define debug(fmt,args...) printf("%s - ",__FUNCTION__);printf (fmt ,##args)
	#define locate() printf("%s: line %u\n",__FILE__,__LINE__);OSS_Delay(NULL,1)	
	#define error(fmt,args...) printf("### %s - line %u: ",__FUNCTION__,__LINE__); \
								printf (fmt ,##args);OSS_Delay(NULL,1)
#else
	#define debug(fmt,args...) if(DBG_MYLEVEL)DBG_Write(DBH,"%s - "fmt,__FUNCTION__,##args)
	#define locate()
	#define error(fmt,args...) DBG_Write(DBH,"### %s - line %u: "fmt,__FUNCTION__,__LINE__,##args);
#endif

extern DBG_HANDLE * sysDbgHdlP;
#define DBH sysDbgHdlP


/* access macros */
#define _ML605SWAP_W_D32(ma,offs,val) (*(volatile u_int32*)((u_int8*)(ma)+(offs))=OSS_Swap32(val))
#define _ML605SWAP_R_D32(ma,offs,var) (var=OSS_Swap32(*(volatile u_int32*)((u_int8*)(ma)+(offs))))
#define _ML605_W_D32(ma,offs,val) (*(volatile u_int32*)((u_int8*)(ma)+(offs))=(val))
#define _ML605_R_D32(ma,offs,var) (var=*(volatile u_int32*)((u_int8*)(ma)+(offs)))
#define _ML605_W_D8(ma, offs,val) (*(volatile u_int8* )((u_int8*)(ma)+(offs))=val)
#define _ML605_R_D8(ma,offs,var) (var=*(volatile u_int8* )((u_int8*)(ma)+(offs)))

/* bit reversing macros */
#define _ML605SWAP_WR_D32(ma,offs,val) (*(volatile u_int32*)((u_int8*)(ma)+(offs))=OSS_Swap32(bit_rev32(val)))
#define _ML605SWAP_RR_D32(ma,offs,var) (var=bit_rev32(OSS_Swap32(*(volatile u_int32*)((u_int8*)(ma)+(offs)))))
#define _ML605_WR_D32(ma,offs,val) (*(volatile u_int32*)((u_int8*)(ma)+(offs))=bit_rev32(val))
#define _ML605_RR_D32(ma,offs,var) (var=bit_rev32(*(volatile u_int32*)((u_int8*)(ma)+(offs))))


#ifdef DEBUG_ACCESS
#define ML605SWAP_WRITE_D32(ma, offs,val) _ML605SWAP_W_D32(ma,offs,val); \
			debug("W32 A:0x%08X D:0x%08X\n",(ma)+(offs),val)
#define ML605SWAP_READ_D32(ma,offs,var) _ML605SWAP_R_D32(ma,offs,var); \
			debug("R32 A:0x%08X D:0x%08X\n",(ma)+(offs),var)
#define ML605_WRITE_D32(ma,offs,val) _ML605_W_D32(ma,offs,val); \
			debug("W32 A:0x%08X D:0x%08X\n",(ma)+(offs),val)
#define ML605_READ_D32(ma,offs,var) _ML605_R_D32(ma,offs,var); \
			debug("R32 A:0x%08X D:0x%08X\n",(ma)+(offs),var)
#define ML605_WRITE_D8(ma, offs,val) _ML605_W_D8(ma,offs,val); \
				debug("W8 A:0x%08X D:0x%08X\n",(ma)+(offs),val)
#define ML605_READ_D8(ma,offs,var) _ML605_R_D8(ma,offs,var); \
				debug("R8 A:0x%08X D:0x%08X\n",(ma)+(offs),var)
/* bit reversing macros */
#define ML605SWAP_WRITE_R32(ma, offs,val) _ML605SWAP_WR_D32(ma,offs,val); \
				debug("W32R A:0x%08X D:0x%08X\n",(ma)+(offs),val)
#define ML605SWAP_READ_R32(ma,offs,var) _ML605SWAP_RR_D32(ma,offs,var); \
				debug("R32R A:0x%08X D:0x%08X\n",(ma)+(offs),var)
#define ML605_WRITE_R32(ma,offs,val) _ML605_WR_D32(ma,offs,val); \
				debug("W32R A:0x%08X D:0x%08X\n",(ma)+(offs),val)
#define ML605_READ_R32(ma,offs,var) _ML605_RR_D32(ma,offs,var); \
				debug("R32R A:0x%08X D:0x%08X\n",(ma)+(offs),var)
#else
#define ML605SWAP_WRITE_D32(ma,offs,val)	_ML605SWAP_W_D32(ma,offs,val)
#define ML605SWAP_READ_D32(ma,offs,var)		_ML605SWAP_R_D32(ma,offs,var)
#define ML605_WRITE_D32(ma,offs,val)		_ML605_W_D32(ma,offs,val)
#define ML605_READ_D32(ma,offs,var)			_ML605_R_D32(ma,offs,var)
#define ML605_WRITE_D8(ma, offs,val) 		_ML605_W_D8(ma, offs,val)
#define ML605_READ_D8(ma,offs,var) 			_ML605_R_D8(ma,offs,var)
/* bit reversing macros */
#define ML605SWAP_WRITE_R32(ma,offs,val)	_ML605SWAP_WR_D32(ma,offs,val)
#define ML605SWAP_READ_R32(ma,offs,var)		_ML605SWAP_RR_D32(ma,offs,var)
#define ML605_WRITE_R32(ma,offs,val)		_ML605_WR_D32(ma,offs,val)
#define ML605_READ_R32(ma,offs,var)			_ML605_RR_D32(ma,offs,var)
#endif


/* ### locals */
static spinlockTask_t sysMLSpinlock;
uint32_t	g_ml605_verbosity;
float g_voltages[ML605_SYSMON_VOLTAGES_MAX] = {0};
/* voltage dividers calculated from resistors */
float voltage_dividers[ML605_SYSMON_VOLTAGES_MAX] = 
{
	0.5, 			/* 1V */
	0.5, 			/* 1V_A*/
	0.416666667, 	/* 1_2V*/
	0.416666667, 	/* 1_2V_A*/
	0.277372263, 	/* 1_8V*/
	0.200031999, 	/* 2_5V*/
	0.15151978, 	/* 3_3V*/
	0.09809974, 	/* 5_1V*/
	-0.041500853, 	/* N12V*/
	0.041686632, 	/* P12V*/
	-0.03333333, 	/* N15V*/
	0.033349444 	/* P15V*/
};

/* ### forward declarations #### */
static uint32_t ml605_SCL_error();
static uint32_t ml605_LRU5_8_error();


/* ### definitions #### */

int ml605_init(void)
{
	
	/* Create debug handle	*/
	DBGINIT((NULL,&DBH));
	g_ml605_verbosity = DBG_OFF;
	DBG_Write(DBH,"ml605_init()\n");
	
	ml605_sysmon_init();
	ml605_gpio_init();
	ml605_LRU5_8_init(0);
	ml605_SCL_init();
	/* init rand for tests */
	srand(OSS_TickGet(NULL));
	debug("done\n");

	return OK;
}

/* 
 * System monitor 
 * ==============

System monitor
==============
(see xps_sysmon_adc.pdf - Continuous Cycling Of Sequence Mode Example)
- enable automatic channel sequencer
cl a0000304 00002000
- select channels in sequence register 0/1
	C_BASEADDR + 0x320       Sequence Register 0 R/W 0x0
	SYSMON Sequence register 0 (ADC channel selection)
	C_BASEADDR + 0x324       Sequence Register 1 R/W 0x0
	SYSMON Sequence register 1 (ADC channel selection)
cl0 a0000320 00000100
cl0 a0000324 0000ffff
- restart automatic channel sequencer
cl a0000304 00002000
- on request read voltages (12) and internal temperature
  and return in structure
V=((ADCval>>6)/1024)*3V
T=((ADCval>>6)*503,975/1024)-273,15

*/
int ml605_sysmon_init(void)
{
	uint32_t i = 0, d1 = 0, d2 = 0;
	
	/* reset sysmon */
	ML605_WRITE_D32(
		ML605_UNIT_ADDR_SYSMON,
		ML605_SYSMON_REG_SW_RES,
		ML605_SYSMON_WORD_SW_RES);
	/* wait at least 16 clocks */
	OSS_MikroDelay(NULL,1);	
	/* enable automatic channel sequencer */
	ML605_WRITE_D32(
		ML605_UNIT_ADDR_SYSMON,
		ML605_SYSMON_REG_CONFIG1,
		ML605_SYSMON_WORD_CCSM);
	/* select channels in sequence register 0/1 */
	ML605_WRITE_D32(
		ML605_UNIT_ADDR_SYSMON,
		ML605_SYSMON_REG_CHAN_SEL0,
		ML605_SYSMON_WORD_CHAN_SEL0);
	ML605_WRITE_D32(
		ML605_UNIT_ADDR_SYSMON,
		ML605_SYSMON_REG_CHAN_SEL1,
		ML605_SYSMON_WORD_CHAN_SEL1);
	/* restart automatic channel sequencer */
	ML605_WRITE_D32(
		ML605_UNIT_ADDR_SYSMON,
		ML605_SYSMON_REG_CONFIG1,
		ML605_SYSMON_WORD_CCSM);
	/* OSS_MikroDelay(NULL,500); */
	OSS_Delay(NULL,1);	/* delay one tick to read voltages*/
	/* check sequencing - the channel bits must change */
	ML605_READ_D32(
		ML605_UNIT_ADDR_SYSMON,
		ML605_SYSMON_REG_STATUS,
		d1);
	i = 0;
	do {
		ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_STATUS,
			d2);
		if(d1 != d2)
		{
			break;
		}
		i++;
		OSS_MikroDelay(NULL,i);	/* change pause */
	}while(i < ML605_SYSMON_SEQ_TIMEOUT);
	if(i < ML605_SYSMON_SEQ_TIMEOUT)
	{
		debug("seq. ok (cycle %u - d1=%u d2=%u)\n",i,d1,d2);
		return OK;
	}else
	{
		error("not sequencing (cycle %u)\n",i);
		return E_ERR_ML605_SYSMON_NOT_SEQ;
	}
}

int ml605_sysmon_ctemp_get(float * temp)
{
	uint32_t d = 0;
	
	/* read core temperature */
	ML605_READ_D32(
		ML605_UNIT_ADDR_SYSMON,
		ML605_SYSMON_REG_CHIP_TEMP,
		d);
	debug("read 0x%08X\n",d);
	if(temp)
		*temp = (((d>>6)*503.975)/1024)-273.15;
	debug("%.2f캜\n",(((d>>6)*503.975)/1024)-273.15);
	
	return OK;
}

/* reads voltages from FPGA */
/* read aux0: -> sysInLong 0xc4000240 */
#define ML605_SYSMON_VOLTAGE_CALC(x) ((float)(x>>6)/1024)
int ml605_sysmon_voltages_read(void)
{
	uint32_t d = 0, i= 0;
	float f = 0;
	
	/* E_ML605_SYSMON_1V */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_1V,d);

	g_voltages[E_ML605_SYSMON_1V] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_1V];
	
	
	/* E_ML605_SYSMON_1V_A */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_1V_A,d);
	g_voltages[E_ML605_SYSMON_1V_A] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_1V_A];
	
	/* E_ML605_SYSMON_1_2V */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_1_2V,d);
	g_voltages[E_ML605_SYSMON_1_2V] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_1_2V];
	
	/* E_ML605_SYSMON_1_2V_A */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_1_2V_A,d);
	g_voltages[E_ML605_SYSMON_1_2V_A] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_1_2V_A];
	
	/* E_ML605_SYSMON_1_8V */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_1_8V,d);
	g_voltages[E_ML605_SYSMON_1_8V] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_1_8V];
	
	/* E_ML605_SYSMON_2_5V */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_2_5V,d);
	g_voltages[E_ML605_SYSMON_2_5V] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_2_5V];
	
	/* E_ML605_SYSMON_3_3V */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_3_3V,d);
	g_voltages[E_ML605_SYSMON_3_3V] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_3_3V];
	
	/* E_ML605_SYSMON_5_1V */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_5_1V,d);
	g_voltages[E_ML605_SYSMON_5_1V] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_5_1V];
	
	/* ML605_SYSMON_REG_N12V */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_N12V,d);
	g_voltages[E_ML605_SYSMON_N12V] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_N12V];
	
	/* E_ML605_SYSMON_P12V */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_P12V,d);
	g_voltages[E_ML605_SYSMON_P12V] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_P12V];
	
	/* E_ML605_SYSMON_N15V */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_N15V,d);
	g_voltages[E_ML605_SYSMON_N15V] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_N15V];
	
	/* E_ML605_SYSMON_P15V */
	ML605_READ_D32(
			ML605_UNIT_ADDR_SYSMON,
			ML605_SYSMON_REG_P15V,d);
	g_voltages[E_ML605_SYSMON_P15V] = 
			ML605_SYSMON_VOLTAGE_CALC(d)/voltage_dividers[E_ML605_SYSMON_P15V];;

	for(i = 0;i<ML605_SYSMON_VOLTAGES_MAX;i++)
	{
		debug(" %-10s: %5.2fV\n",g_voltage_names[i],g_voltages[i]);
	}
	
	return OK;
}

/* reads voltages sets the passed pointer to voltages array */
int ml605_sysmon_voltages_get(float ** ppV)
{
	/* read voltages */
	if(!ml605_sysmon_voltages_read())
	{
		if(ppV)
			*ppV = g_voltages;
		return OK;
	}
	else 
		return E_ERR_ML605_SYSMON_VOLT_READ;
}

/* print voltages to buffer */
int ml605_sysmon_voltages_print(char * p)
{
	int i = 0;

	/* read voltages */
	if(ml605_sysmon_voltages_read())
		return ERROR;

	if(p == NULL)
		for(i = 0;i<ML605_SYSMON_VOLTAGES_MAX;i++)
		{
			printf("%8s: %7.3f V\n",g_voltage_names[i],g_voltages[i]);
		}
	else
		for(i = 0;i<ML605_SYSMON_VOLTAGES_MAX;i++)
		{
			sprintf(p,"%8s: %7.3f V\n",g_voltage_names[i],g_voltages[i]);
		}

	return OK;
}

/*
GPIOs
=====
(see xps_gpio.pdf)
For input ports when the channel is not configured for interrupt:
1. Configure the port as input by writing the corresponding bit in GPIOx_TRI 
   register with the value of 1.
2. Read the corresponding bit in GPIOx_DATA register. 
For output ports:
1. Configure the port as output by writing the corresponding bit in GPIOx_TRI 
   register with a value of 0.
2. Write the corresponding bit in GPIOx_DATA register. 
*/
int ml605_gpio_init(void)
{
	/* configure tristates */
	/* G0/C1 */
	ML605_WRITE_R32(
		ML605_UNIT_ADDR_GPIO_0,
		ML605_GPIO_REG_TRI,
		ML605_GPIO_0_WORD_C1_TRI);
	/* G1/C1 */
	ML605_WRITE_R32(
		ML605_UNIT_ADDR_GPIO_1,
		ML605_GPIO_REG_TRI,
		ML605_GPIO_1_WORD_C1_TRI);
	/* G1/C2 */
	ML605_WRITE_R32(
		ML605_UNIT_ADDR_GPIO_1,
		ML605_GPIO_REG_TRI2,
		ML605_GPIO_1_WORD_C2_TRI);
	
	/* all off */
	ML605_WRITE_R32(
		ML605_UNIT_ADDR_GPIO_0,
		ML605_GPIO_REG_DATA,
		0);
	ML605_WRITE_R32(
		ML605_UNIT_ADDR_GPIO_0,
		ML605_GPIO_REG_DATA2,
		0);
	ML605_WRITE_R32(
		ML605_UNIT_ADDR_GPIO_1,
		ML605_GPIO_REG_DATA,
		0);
	ML605_WRITE_R32(
		ML605_UNIT_ADDR_GPIO_1,
		ML605_GPIO_REG_DATA2,
		0);

    spinLockTaskInit( &sysMLSpinlock, 0);
	
	debug("done\n");
	
	return OK;
	
}

/* dir will be reversed to MSB order! */
int ml605_gpio_dir_set(uint32_t unit, uint32_t channel, uint32_t dir )
{
    spinLockTaskTake( &sysMLSpinlock );
	ML605_WRITE_R32(
		unit ? ML605_UNIT_ADDR_GPIO_1 : ML605_UNIT_ADDR_GPIO_0,
		channel ? ML605_GPIO_REG_TRI2 : ML605_GPIO_REG_TRI,
		dir);
    spinLockTaskGive( &sysMLSpinlock );
	debug("set DIR of GPIO%u channel %u to 0x%08X\n",unit,channel,dir);

	return OK;
}

/* dir will be reversed from MSB order! */
int ml605_gpio_dir_get(int unit, int channel, uint32_t * dir )
{	
	uint32_t d = 0;

    spinLockTaskTake( &sysMLSpinlock );
	ML605_READ_R32(
		unit ? ML605_UNIT_ADDR_GPIO_1 : ML605_UNIT_ADDR_GPIO_0,
		channel ? ML605_GPIO_REG_TRI2: ML605_GPIO_REG_TRI,
		d);
    
	if(dir)
		*dir = d;
    spinLockTaskGive( &sysMLSpinlock );

	debug("GPIO%u dir channel %u = 0x%08X\n",unit,channel,d);

	return OK;
}

/* dat will be reversed to MSB order! */

int ml605_gpio_dat_set(int unit, int channel, uint32_t dat )
{
    spinLockTaskTake( &sysMLSpinlock );
	ML605_WRITE_R32(
		unit ? ML605_UNIT_ADDR_GPIO_1 : ML605_UNIT_ADDR_GPIO_0,
		channel ? ML605_GPIO_REG_DATA2: ML605_GPIO_REG_DATA,
		dat);
    spinLockTaskGive( &sysMLSpinlock );
	debug("set GPIO%u channel %u to 0x%08X\n",unit,channel,dat);

	return OK;
}

/* dat will be reversed from MSB order! */
int ml605_gpio_dat_get(int unit, int channel, uint32_t * dat )
{	
	uint32_t d = 0;
	spinLockTaskTake( &sysMLSpinlock );
	ML605_READ_R32(
		unit ? ML605_UNIT_ADDR_GPIO_1 : ML605_UNIT_ADDR_GPIO_0,
		channel ? ML605_GPIO_REG_DATA2: ML605_GPIO_REG_DATA,
		d);
    if(dat)
		*dat = d;
    spinLockTaskGive( &sysMLSpinlock );
	debug("GPIO%u channel %u = 0x%08X\n",unit,channel,d);

	

	return OK;
}


/*
LRU5/8
======

This unit uses LSB numbering and bytes must be swapped!
Hail the FPGA developer...
*/
int ml605_LRU5_8_init(uint32_t dir)
{
	uint32_t d = 0;

	/* set CLR */
	ML605_WRITE_D8(
		ML605_UNIT_ADDR_LRU5_8,
		ML605_LRU5_8_REG_CTRL,
		1<<ML605_LRU5_8_BIT_CTRL_CLEAR);
	
	/* reset CLR and set DIR */
	ML605_WRITE_D8(
		ML605_UNIT_ADDR_LRU5_8,
		ML605_LRU5_8_REG_CTRL,
		(dir ? 1<<ML605_LRU5_8_BIT_CTRL_DIR : 0));

	/* wait to clear */
	OSS_MikroDelay(NULL,1);	
	
	ML605_READ_D8(
		ML605_UNIT_ADDR_LRU5_8,
		ML605_LRU5_8_REG_FIFO_STAT,
		d);

	if(!((d & 1<<ML605_LRU5_8_BIT_FSTAT_TX_EMPTY) &&
		(d & 1<<ML605_LRU5_8_BIT_FSTAT_RX_EMPTY)))
	{
		error("can't empty FIFOs (0x%08X)\n",d);
		return E_ERR_ML605_LRU_EMPT_FIFOS;
	}

	/* check for errors */
	if((d = ml605_LRU5_8_error()))
	{
		error("error condition start (0x%08X)\n",d);
		return d;
	}

	debug("done\n");
	return OK;
}

/*
This test fills and receives the FIFO once.
To get permanent load consider filling the FIFO with interrupt routine.
*/
int ml605_LRU5_8_test(void)
{
	uint8_t d = 0;
	uint8_t dir = 0;
	uint32_t i = 0;
	uint8_t t[ML605_LRU5_8_FIFO_SIZE] = {0};
	uint8_t r[ML605_LRU5_8_FIFO_SIZE] = {0};
	int ret = OK;


	for(dir = 0; dir < 2; dir++)
	{
		/* init unit */
		if(ml605_LRU5_8_init(dir))
			return E_ERR_ML605_LRU_INIT;

		/* fill test array */
		for(i = 0; i < ML605_LRU5_8_FIFO_SIZE; i++)
		{
			t[i] = rand() % UCHAR_MAX;
			r[i] = 0;
		}

		/* fill first TX FIFO */
		ML605_WRITE_D8(
				ML605_UNIT_ADDR_LRU5_8,
				ML605_LRU5_8_REG_DAT,
				t[0]);

		/* wait to fill */
		OSS_MikroDelay(NULL,1);	
		
		/* check FIFO */
		ML605_READ_D8(
			ML605_UNIT_ADDR_LRU5_8,
			ML605_LRU5_8_REG_FIFO_STAT,
			d);
		if(d & 1<<ML605_LRU5_8_BIT_FSTAT_TX_EMPTY)
		{
			error("can't fill FIFO (0x%X)\n",d);
			return E_ERR_ML605_LRU_FILL_FIFOS;
		}

		/* fill rest of TX FIFO */
		for(i = 1; i < ML605_LRU5_8_FIFO_SIZE; i++)
		{
			ML605_WRITE_D8(
				ML605_UNIT_ADDR_LRU5_8,
				ML605_LRU5_8_REG_DAT,
				t[i]);
		}

		/* wait for RX FIFO full */
		i = 0;
		
		do {
			ML605_READ_D8(
				ML605_UNIT_ADDR_LRU5_8,
				ML605_LRU5_8_REG_FIFO_STAT,
				d);
			if(d & 1<<ML605_LRU5_8_BIT_FSTAT_RX_FULL)
			{
				break;
			}
			OSS_MikroDelay(NULL,1);	
			i++;
		}while(i < ML605_LRU5_8_RX_TIMEOUT);
		
		if(i >= ML605_LRU5_8_RX_TIMEOUT)
		{
			error("RX timeout (%u탎 - stat:0x%X)\n",i,d);
			return E_ERR_ML605_LRU_RX_TIMEOUT;
		}
			
		debug("RX FULL after %u탎\n",i);

		/* get RX data */
		for(i = 0; i < ML605_LRU5_8_FIFO_SIZE; i++)
		{
			ML605_READ_D8(
				ML605_UNIT_ADDR_LRU5_8,
				ML605_LRU5_8_REG_DAT,
				r[i]);
		}

		/* check for errors */
		if((d = ml605_LRU5_8_error()))
		{
			error("error after RX (0x%08X)\n",d);
			return d;
		}

		/* check test array */
		for(i = 0; i < ML605_LRU5_8_FIFO_SIZE; i++)
		{
			if(r[i] != t[i])
			{
				error("RX[%u] data mismatch (0x%02X!=0x%02X)\n",i,t[i],r[i]);
				ret = E_ERR_ML605_LRU_RX_DATA_MISMATCH;
			}else
				debug("RX[%u] 0x%02X=0x%02X\n",i,t[i],r[i]);
		}
	}
	return ret;
}

static uint32_t ml605_LRU5_8_error(void)
{
	uint32_t d = 0;
	
	ML605_READ_D32(
			ML605_UNIT_ADDR_LRU5_8,
			ML605_LRU5_8_REG_RXTX_STAT,
			d);

	if(d & 1<<ML605_LRU5_8_BIT_RTSTAT_RXPAR_ERR)
		return E_ERR_ML605_LRU_RX_PARITY;
	else 
		return OK;
}


/*
SCL
===

This unit uses LSB numbering and bytes must be swapped!
*/
int ml605_SCL_init(void)
{
	uint32_t d = 0;

	ML605_WRITE_R32(
		ML605_UNIT_ADDR_SCL,
		ML605_SCL_REG_CTRL,
		(1<<ML605_SCL_BIT_CTRL_CLR_RX) | (1<<ML605_SCL_BIT_CTRL_CLR_TX));
	ML605_WRITE_R32(
		ML605_UNIT_ADDR_SCL,
		ML605_SCL_REG_CTRL,
		0);
	
	ML605_READ_R32(
		ML605_UNIT_ADDR_SCL,
		ML605_SCL_REG_STATUS,
		d);

	

	if((d & 1<<ML605_SCL_BIT_STAT_RX_VALID) ||
		!(d & 1<<ML605_SCL_BIT_STAT_TX_EMP))
	{
		error("can't empty FIFOs (0x%08X)\n",d);
		return E_ERR_ML605_SCL_EMPT_FIFOS;
	}

	if((d = ml605_SCL_error()))
	{
		error("error condition (0x%08X)\n",d);
		return d;
	}

	/* TODO: check status */
	debug("done\n");
	return OK;
}

/* 
This test fills and receives the FIFO once.
To get permanent load consider filling the FIFO with interrupt routine.
*/
int ml605_SCL_test(void)
{
	uint32_t d = 0;
	uint32_t i = 0, j = 0;
	uint32_t t[ML605_SCL_FIFO_SIZE] = {0};
	uint32_t r[ML605_SCL_FIFO_SIZE] = {0};
	int ret = OK;

	
	/* init test arrays */
	for(i = 0; i < ML605_SCL_FIFO_SIZE; i++)
	{
		t[i] = rand()%UCHAR_MAX;
		r[i] = 0;
	}

	/* check for errors */
	if((d = ml605_SCL_error()))
	{
		error("error condition start (0x%08X)\n",d);
		return d;
	}

	/* fill TX FIFO */
	for(i = 0; i < ML605_SCL_FIFO_SIZE; i++)
	{
		ML605_WRITE_D32(
			ML605_UNIT_ADDR_SCL,
			ML605_SCL_REG_TX_FIFO,
			t[i]);
	}

	/* check for errors */
	if((d = ml605_SCL_error()))
	{
		error("error condition TX (0x%08X)\n",d);
		return d;
	}

	/* wait for ML605_SCL_BIT_STAT_RX_VALID and receive*/
	i = 0;
	j = 0;

	do {
		do {
			ML605_READ_R32(
			ML605_UNIT_ADDR_SCL,
			ML605_SCL_REG_STATUS,
			d);
			/* try to read */
			if(d & 1<<ML605_SCL_BIT_STAT_RX_VALID)
			{
				ML605_READ_D32(
						ML605_UNIT_ADDR_SCL,
						ML605_SCL_REG_RX_FIFO,
						r[j]);
				j++;
				if(j == ML605_SCL_FIFO_SIZE)
					break;
			}
		}while(d & 1<<ML605_SCL_BIT_STAT_RX_VALID);
		if(j == ML605_SCL_FIFO_SIZE)
			break;
		OSS_MikroDelay(NULL,10); 
		i++;
	}while(i < ML605_SCL_RX_TIMEOUT);

	/* check for errors */
	if((d = ml605_SCL_error()))
	{
		error("error condition RX (0x%08X)\n",d);
		return d;
	}
	
	if(i >= ML605_SCL_RX_TIMEOUT)
	{
		error("RX timeout (%u탎 - RX%u - stat:0x%X)\n",i,j,d);
		return E_ERR_ML605_SCL_RX_TIMEOUT;
	}
	
	debug("RX done after %u탎\n",i*10);

	/* check test array */
	for(i = 0; i < ML605_SCL_FIFO_SIZE; i++)
	{
		if(t[i] != r[i])
		{
			error("RX[%u] data mismatch (0x%02X!=0x%02X)\n",i,t[i],r[i]);
			ret = E_ERR_ML605_SCL_RX_DATA_MISMATCH;
		}else
			debug("RX[%u] 0x%02X=0x%02X\n",i,t[i],r[i]);
	}

	debug("test successfull\n");
	return ret;
}

static uint32_t ml605_SCL_error(void)
{
	uint32_t d = 0;
	
	ML605_READ_R32(
			ML605_UNIT_ADDR_SCL,
			ML605_SCL_REG_STATUS,
			d);
	
	if(d & 1<<ML605_SCL_BIT_STAT_FRM_ERR)
		return E_ERR_ML605_SCL_FRAMING;
	else if(d & 1<<ML605_SCL_BIT_STAT_PAR_ERR)
		return E_ERR_ML605_SCL_PARITY;
	else if(d & 1<<ML605_SCL_BIT_STAT_OVRN_ERR)
		return E_ERR_ML605_SCL_OVERRUN;
	else return OK;
}

/*
 *	UTILS
 */

/*
 * reverse bits of a u32
 */
uint32_t bit_rev32(uint32_t word)
{
    register uint32_t ret = 0, cnt;

	ret |= word & 1;
    for(cnt=0;cnt<31;cnt++)
    {
    	word >>= 1;
		ret <<= 1;
		ret |= word & 1;
    }
    return ret;
}



#if 0
void ml605_alignment_check(uint8_t * base)
{
	uint8_t * u8_addr = 0;
	uint16_t * u16_addr = 0;
	uint32_t * u32_addr = 0;
	
	int i = 0;

	puts("\n32 bit aligned\n");
	for(i=0;i<5;i++)
	{
		u8_addr = u16_addr = u32_addr = 
			base + 4*i;
		printf("u32_addr 0x%08X = %u\n",u32_addr,*u32_addr);
		printf("u16_addr 0x%08X = %u\n",u16_addr,*u16_addr);
		printf("u8_addr  0x%08X = %u\n",u8_addr, *u8_addr);
	}
#if 0
	puts("\n16 bit aligned\n");
	for(i=0;i<5;i++)
	{
		u8_addr = u16_addr = u32_addr = 
			base + 2*i;
		printf("u32_addr 0x%08X = %u\n",u32_addr,*u32_addr);
		printf("u16_addr 0x%08X = %u\n",u16_addr,*u16_addr);
		printf("u8_addr  0x%08X = %u\n",u8_addr, *u8_addr);
	}

	puts("\n8 bit aligned\n");
	for(i=0;i<5;i++)
	{
		u8_addr = u16_addr = u32_addr = 
			base + i;
		printf("u32_addr 0x%08X = %u\n",u32_addr,*u32_addr);
		printf("u16_addr 0x%08X = %u\n",u16_addr,*u16_addr);
		printf("u8_addr  0x%08X = %u\n",u8_addr, *u8_addr);
	}
#endif
}
#endif

