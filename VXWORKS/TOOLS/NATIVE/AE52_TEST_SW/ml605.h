
/*********************** I n c l u d e - F i l e ************************/
/*!
* \file ml605.h
*
* \author maximilian.kolpak@men.de
* $Date: 2012/04/13 09:37:18 $
* $Revision: 1.1 $
*
* \brief Header file for the ml605 IP core driver.

*
* Switches: -
*/
/*-------------------------------[ History ]---------------------------------
*
* $Log: ml605.h,v $
* Revision 1.1  2012/04/13 09:37:18  MKolpak
* Initial Revision
*
*---------------------------------------------------------------------------
* (c) Copyright 2007 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
****************************************************************************/
#ifndef _ML605_H
#define _ML605_H

#if 0
#define REN(x)	(31-(x))	/* renumber for MSB numbering */
#else
#define REN(x)	(x)
#endif

/* 
 * ============
 * = IO units =
 * ============
 */

#define ML605_IO_UNIT_BASE			0xc0000000


/* 
 * SYSMON 
 * ======
 */

#define ML605_UNIT_OFFSET_SYSMON	0
#define ML605_UNIT_ADDR_SYSMON		(ML605_IO_UNIT_BASE+ML605_UNIT_OFFSET_SYSMON)

#define ML605_SYSMON_REG_SW_RES		0
#define ML605_SYSMON_REG_STATUS		0x4
#define ML605_SYSMON_REG_CHIP_TEMP	0x200
#define ML605_SYSMON_REG_VCC_INT	0x204
#define ML605_SYSMON_REG_VCC_AUX	0x208
#define ML605_SYSMON_REG_V_AUX0		0x240
#define ML605_SYSMON_REG_V_AUX1		0x244
#define ML605_SYSMON_REG_V_AUX2		0x248
#define ML605_SYSMON_REG_V_AUX3		0x24C
#define ML605_SYSMON_REG_V_AUX4		0x250
#define ML605_SYSMON_REG_V_AUX5		0x254
#define ML605_SYSMON_REG_V_AUX6		0x258
#define ML605_SYSMON_REG_V_AUX7		0x25C
#define ML605_SYSMON_REG_V_AUX8		0x260
#define ML605_SYSMON_REG_V_AUX9		0x264
#define ML605_SYSMON_REG_V_AUX10	0x268
#define ML605_SYSMON_REG_V_AUX11	0x26C
#define ML605_SYSMON_REG_V_AUX12	0x270
#define ML605_SYSMON_REG_V_AUX13	0x274
#define ML605_SYSMON_REG_V_AUX14	0x278
#define ML605_SYSMON_REG_V_AUX15	0x27C

#define ML605_SYSMON_REG_CONFIG0	0x300
#define ML605_SYSMON_REG_CONFIG1	0x304
#define ML605_SYSMON_REG_CONFIG2	0x308
#define ML605_SYSMON_REG_CHAN_SEL0	0x320
#define ML605_SYSMON_REG_CHAN_SEL1	0x324

#define ML605_SYSMON_REG_P15V		ML605_SYSMON_REG_V_AUX14
#define ML605_SYSMON_REG_N15V		ML605_SYSMON_REG_V_AUX13	
#define ML605_SYSMON_REG_P12V		ML605_SYSMON_REG_V_AUX12	
#define ML605_SYSMON_REG_N12V		ML605_SYSMON_REG_V_AUX15	
#define ML605_SYSMON_REG_5_1V		ML605_SYSMON_REG_V_AUX3
#define ML605_SYSMON_REG_3_3V		ML605_SYSMON_REG_V_AUX1
#define ML605_SYSMON_REG_2_5V		ML605_SYSMON_REG_V_AUX2
#define ML605_SYSMON_REG_1_8V		ML605_SYSMON_REG_V_AUX7
#define ML605_SYSMON_REG_1_2V		ML605_SYSMON_REG_V_AUX4
#define ML605_SYSMON_REG_1_2V_A		ML605_SYSMON_REG_V_AUX0
#define ML605_SYSMON_REG_1V			ML605_SYSMON_REG_V_AUX11
#define ML605_SYSMON_REG_1V_A		ML605_SYSMON_REG_V_AUX6

#define ML605_SYSMON_WORD_SW_RES	0x0000000A	/* for SW RES */
#define ML605_SYSMON_WORD_CCSM		0x00002000	/* for CONF1 */
#define ML605_SYSMON_WORD_CLK_CONF	0x00002000	/* for CONF2 */
#define ML605_SYSMON_WORD_CHAN_SEL0	0x00000100	/* for CHAN_SEL0 */
#define ML605_SYSMON_WORD_CHAN_SEL1	0x0000FFFF	/* for CHAN_SEL1 */

#define ML605_SYSMON_SEQ_TIMEOUT		50	/* in µs */



/* 
 * GPIO 
 * ====
 */

#define ML605_GPIO_REG_DATA			0	/* channel 1 */
#define ML605_GPIO_REG_TRI			0x4	/* channel 1 */
#define ML605_GPIO_REG_DATA2		0x8	/* channel 2 */
#define ML605_GPIO_REG_TRI2			0xC	/* channel 2 */

/* GPIO 0 */

#define ML605_UNIT_OFFSET_GPIO_0	0x1000
#define ML605_UNIT_ADDR_GPIO_0		(ML605_IO_UNIT_BASE+ML605_UNIT_OFFSET_GPIO_0)

/* channel 1 of 2 */
#define ML605_GPIO_0_BIT_EPF_I			REN(0)
#define ML605_GPIO_0_BIT_LRU5_FAIL_O	REN(1)	/* to EPF */
#define ML605_GPIO_0_BIT_B_SWITCH_I		REN(2)	/* from mux D */
#define ML605_GPIO_0_BIT_B_GO_LAMP_O	REN(3)	/* to mux S1 */
#define ML605_GPIO_0_BIT_B_NO_LAMP_O	REN(4)	/* to mux S2 */
#define ML605_GPIO_0_BIT_B_LAMP_O 		REN(5)	/* to mux C */
#define ML605_GPIO_0_BIT_APICPU_O		REN(6)	/* to CPU GPIO 1 */
#define ML605_GPIO_0_BIT_APICPU_I		REN(7)	/* nc */
#define ML605_GPIO_0_BIT_SUP1_O			REN(8) 	/* to USER/API 0 */
#define ML605_GPIO_0_BIT_SUP2_O			REN(9)	/* to USER/API 1 */
#define ML605_GPIO_0_BIT_SUP3_O			REN(10)	/* to USER/API 2 */
#define ML605_GPIO_0_BIT_SUP4_O			REN(11)	/* to USER/API 3 */
#define ML605_GPIO_0_BIT_ALARM_O		REN(16)	
#define ML605_GPIO_0_BIT_THERM_SW_I		REN(17)	
#define ML605_GPIO_0_BIT_DDR_A13_O		REN(24)


#define ML605_GPIO_0_WORD_C1_TRI	(1<<ML605_GPIO_0_BIT_EPF_I | \
									1<<ML605_GPIO_0_BIT_B_SWITCH_I | \
									1<<ML605_GPIO_0_BIT_APICPU_I | \
									1<<ML605_GPIO_0_BIT_THERM_SW_I)

/* GPIO 1 */

#define ML605_UNIT_OFFSET_GPIO_1	0x2000
#define ML605_UNIT_ADDR_GPIO_1		(ML605_IO_UNIT_BASE+ML605_UNIT_OFFSET_GPIO_1)

	/* channel 1 of 2 - API/USER GPIOs
	 *
	 *  0..15(O) -> SPARE G1/C2/0..15
	 * 16..19(I) <- SUPRESSION G0/C1/8..11
	 */
#define ML605_GPIO_1_BIT_API(x) 	(x < 20 ? REN(x) : -1)
#define ML605_GPIO_1_WORD_C1_TRI	(1<<REN(16) | 1<<REN(17) | 1<<REN(18) | 1<<REN(19))


	/* channel 2 of 2
	 *  
	 *  0..15(I) <- API G1/C1/0..15 
	 * 24..31(I) <- SPARE 16..23
	 */
#define M_ML605_GPIO_1_BIT_SPARE(x) (x < 32 ? REN(x) : -1); 

#define ML605_GPIO_1_WORD_C2_TRI	0xFF00FFFF


/* TODO: GPIO API CPU (2x) */

/* 
 * LRU5/8
 * ======
 */
#define ML605_UNIT_OFFSET_LRU5_8	0x3000
#define ML605_UNIT_ADDR_LRU5_8		(ML605_IO_UNIT_BASE+ML605_UNIT_OFFSET_LRU5_8)

#define ML605_LRU5_8_REG_DAT		0
#define ML605_LRU5_8_REG_FIFO_STAT	0x4
#define ML605_LRU5_8_REG_RXTX_STAT	0x8
#define ML605_LRU5_8_REG_CTRL		0xC

#define ML605_LRU5_8_MASK_DAT		0x000000FF

#define ML605_LRU5_8_BIT_FSTAT_TX_FULL		0
#define ML605_LRU5_8_BIT_FSTAT_TX_EMPTY		1
#define ML605_LRU5_8_BIT_FSTAT_RX_FULL		2
#define ML605_LRU5_8_BIT_FSTAT_RX_EMPTY		3

#define ML605_LRU5_8_BIT_RTSTAT_RXPAR_ERR	0

#define ML605_LRU5_8_BIT_CTRL_DIR			0 /* 0=>IO0..10 TX, IO11..21 RX */
#define ML605_LRU5_8_BIT_CTRL_TX_E_IRQ_ENA	1 
#define ML605_LRU5_8_BIT_CTRL_RX_F_IRQ_ENA	2
#define ML605_LRU5_8_BIT_CTRL_TX_E_IRQ_ACT	3
#define ML605_LRU5_8_BIT_CTRL_RX_F_IRQ_ACT	4
#define ML605_LRU5_8_BIT_CTRL_CLEAR			5 /* clear fifos + control */

#define ML605_LRU5_8_FIFO_SIZE		32
#define ML605_LRU5_8_RX_TIMEOUT		50	/* in µs */


/* 
 * SCL
 * ===
 */
#define ML605_UNIT_OFFSET_SCL	0x4000
#define ML605_UNIT_ADDR_SCL		(ML605_IO_UNIT_BASE+ML605_UNIT_OFFSET_SCL)


#define ML605_SCL_REG_RX_FIFO		0x0
#define ML605_SCL_REG_TX_FIFO		0x4
#define ML605_SCL_REG_STATUS		0x8
#define ML605_SCL_REG_CTRL			0xC




#define ML605_SCL_BIT_CTRL_IRQ_ENA	REN(27)
#define ML605_SCL_BIT_CTRL_CLR_RX	REN(30)
#define ML605_SCL_BIT_CTRL_CLR_TX	REN(31)

#define ML605_SCL_BIT_STAT_PAR_ERR	REN(24)
#define ML605_SCL_BIT_STAT_FRM_ERR	REN(25)
#define ML605_SCL_BIT_STAT_OVRN_ERR	REN(26)
#define ML605_SCL_BIT_STAT_IRQ_ENA	REN(27)
#define ML605_SCL_BIT_STAT_TX_FULL	REN(28)
#define ML605_SCL_BIT_STAT_TX_EMP	REN(29)
#define ML605_SCL_BIT_STAT_RX_FULL	REN(30)
#define ML605_SCL_BIT_STAT_RX_VALID	REN(31)

#define ML605_SCL_BIT_DAT			24			/* RX and TX */

#define ML605_SCL_FIFO_SIZE			16
#define ML605_SCL_RX_TIMEOUT		500	/* in 10ms */


/* 
 * INTC
 * ====
 */

#define ML605_UNIT_OFFSET_INT_C		0x5000
#define ML605_UNIT_ADDR_INT_C		(ML605_IO_UNIT_BASE+ML605_UNIT_OFFSET_INT_C)


#define ML605_INTC_REG_STATUS		0
#define ML605_INTC_REG_PEND			0x4
#define ML605_INTC_REG_ENA			0x8
#define ML605_INTC_REG_ACK			0xC
#define ML605_INTC_REG_IRQ_ENA		0x10
#define ML605_INTC_REG_IRQ_CLR		0x14
#define ML605_INTC_REG_IRQ_VEC		0x18
#define ML605_INTC_REG_MSTR_ENA		0x1C

#define ML605_INTC_BIT_UART			REN(0)
#define ML605_INTC_BIT_LRU			REN(1)

/* 
 * =========
 * DRAM unit
 * =========
 */

#define ML605_DRAM_UNIT_BASE			0xc0000000
#define ML605_UNIT_OFFSET_DRAM		0

extern uint32_t g_ml605_verbosity;

int 	ml605_init(void);

/* error table */
enum
{
	E_OK=0,
	E_ERR_ML605_SYSMON_NOT_SEQ=1,
	E_ERR_ML605_SYSMON_VOLT_READ=2,
	E_ERR_ML605_LRU_EMPT_FIFOS=3,
	E_ERR_ML605_LRU_RX_PARITY=4,
	E_ERR_ML605_LRU_FILL_FIFOS=5,
	E_ERR_ML605_LRU_RX_TIMEOUT=6,
	E_ERR_ML605_LRU_INIT=7,
	E_ERR_ML605_LRU_RX_DATA_MISMATCH=8,
	E_ERR_ML605_SCL_EMPT_FIFOS=9,
	E_ERR_ML605_SCL_FRAMING=10,
	E_ERR_ML605_SCL_PARITY=11,
	E_ERR_ML605_SCL_OVERRUN=12,
	E_ERR_ML605_SCL_RX_TIMEOUT=13,
	E_ERR_ML605_SCL_RX_DATA_MISMATCH=14
};

/* SYSMON */
enum
{
	E_ML605_SYSMON_1V,
	E_ML605_SYSMON_1V_A,
	E_ML605_SYSMON_1_2V,
	E_ML605_SYSMON_1_2V_A,
	E_ML605_SYSMON_1_8V,
	E_ML605_SYSMON_2_5V,
	E_ML605_SYSMON_3_3V,
	E_ML605_SYSMON_5_1V,
	E_ML605_SYSMON_N12V,
	E_ML605_SYSMON_P12V,
	E_ML605_SYSMON_N15V,
	E_ML605_SYSMON_P15V
};

#define ML605_SYSMON_VOLTAGES_MAX	12
char * g_voltage_names[ML605_SYSMON_VOLTAGES_MAX] =
{
	"1V",
	"1V_A",
	"1,2V",
	"1,2V_A",
	"1,8V",
	"2,5V",
	"3,3V",
	"5,1V",
	"-12V",
	"12V",
	"-15V",
	"15V"
};

extern float g_voltages[ML605_SYSMON_VOLTAGES_MAX];
int 	ml605_sysmon_init(void);
int 	ml605_sysmon_ctemp_get(float *);
int 	ml605_sysmon_voltages_read(void);
int 	ml605_sysmon_voltages_get(float **);
int 	ml605_sysmon_voltages_print(char * p);

/* GPIOs */
enum
{
	E_ML605_GPIO_C1 = 0,
	E_ML605_GPIO_C2
};

enum
{
	E_ML605_GPIO_0 = 0,
	E_ML605_GPIO_1
};


int ml605_gpio_init(void);
int ml605_gpio_dir_set(uint32_t unit, uint32_t channel, uint32_t dir );
int ml605_gpio_dir_get(int unit, int channel, uint32_t * dir );
int ml605_gpio_dat_set(int unit, int channel, uint32_t dat );
int ml605_gpio_dat_get(int unit, int channel, uint32_t * dat );

/* LRU5/8 */
int ml605_LRU5_8_init(uint32_t);
int ml605_LRU5_8_test(void);

/* SCL */
int ml605_SCL_init(void);
int ml605_SCL_test(void);

/* UTILS */
uint32_t bit_rev32(uint32_t word);



#endif /* _ML605_H */


