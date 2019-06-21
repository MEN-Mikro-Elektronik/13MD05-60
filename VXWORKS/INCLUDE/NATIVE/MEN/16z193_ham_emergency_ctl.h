/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: 16z193_ham_emergency_ctl.h
 *
 *       Author: dh
 *        $Date: 2011/12/19 14:39:02 $
 *    $Revision: 3.9 $
 *
 *  Description: Header file for Hamilton emergency ventilation controller
 *
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 2005-2019, MEN Mikro Elektronik GmbH
 ******************************************************************************/
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _Z193_EMG_CTL_H
#define _Z193_EMG_CTL_H

#ifdef __cplusplus
	extern "C" {
#endif


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

#if defined Z193_MODEL_15EM10A01
/* Z193 Address Map for 15EM10A01 */

/* define General Control registers of Z193 */
#define Z193_CTRL_OFFSET	0x000 /* Control Register */
#define Z193_STS_OFFSET		0x004 /* Status Register */

#define Z193_CRC_OFFSET		0x19C /* CRC Register, over the register set stored in RAM */

#define Z193_CRC_CALC_ADR_START     0x100 /* start address of CRC calculation */
#define Z193_CRC_CALC_ADR_END       0x198 /* end address of CRC calculation */


/* Z193_CTRL register bit definitions */
#define Z193_CTRL_CRC_ENA	0x80000000
								/**< enable CRC check */
								/**< 0: CRC check is disabled \n
								  *  1: CRC check is enabled
								  */
#define Z193_CTRL_EM_ACT	0x00010000
								/**< activate emergency mode */
								/**< 0: emergency mode is not activated \n
								  *  1: emergency mode is activated
								  */
#define Z193_CTRL_EM_ENA	0x00000001
								/**< enable emergency mode */
								/**< 0: emergency mode is disabled \n
							 	  *  1: emergency mode is enabled
							 	  */

/* Z193_STS register bit definitions */
#define Z193_STS_CRC_ERR		0x00000008
								/**< CRC check */
								/**< 0: CRC OK  \n
								  *  1: CRC error detected
								  */

#define Z193_STS_PCI_CLK_ERR	0x00000004
								/**< PCI error detected */
								/**< 0: emergency mode not activated by a pci clock error  \n
								  *  1: emergency mode activated by a pci clock error
								  */

#define Z193_STS_ADC_ERR	0x00000002
								/**< ADC error detected */
								/**< 0: emergency mode not activated by an ADC error  \n
								  *  1: emergency mode activated by an ADC error
								  */

#define Z193_STS_EM_ACT			0x00000001
								/**< emergency mode activated by software */
								/**< 0: emergency mode not activated by software  \n
								  *  1: emergency mode activated by software
								  */



/* Register set stored in RAM */
/* 16Z061 PWM */
/* see \VXWORKS\DRIVERS\MDIS_LL\Z061_PWM\DRIVER\COM\z61_int.h */
#define Z193_PWM_CH0_PERIOD_ADDR_OFFSET	0x100   /* PWM_DISP backlight */
#define Z193_PWM_CH0_PULSE_ADDR_OFFSET		0x104
#define Z193_PWM_CH1_PERIOD_ADDR_OFFSET	0x108   /* PWM_RES1 */
#define Z193_PWM_CH1_PULSE_ADDR_OFFSET		0x10C
#define Z193_PWM_CH2_PERIOD_ADDR_OFFSET	0x110   /* PWM_RES2 */
#define Z193_PWM_CH2_PULSE_ADDR_OFFSET		0x114
#define Z193_PWM_CH3_PERIOD_ADDR_OFFSET	0x118   /* PWM_LS1 alarmsound */
#define Z193_PWM_CH3_PULSE_ADDR_OFFSET		0x11C
#define Z193_PWM_CH4_PERIOD_ADDR_OFFSET	0x120   /* PWM_RES0 */
#define Z193_PWM_CH4_PULSE_ADDR_OFFSET		0x124

/* 16Z074-01 REGSPI */
#define Z193_REGSPI_CTRL_OFFSET		        0x128  /* Control Register */
#define Z193_REGSPI_CH0_PERIOD_OFFSET			0x12C  /* 02Valve PWM */
#define Z193_REGSPI_CH0_PULSE_OFFSET			0x130
#define Z193_REGSPI_CH1_PERIOD_OFFSET			0x134  /* Blower PWM */
#define Z193_REGSPI_CH1_PULSE_LOW_OFFSET	    0x138
#define Z193_REGSPI_CH1_T_LOW_OFFSET			0x13C
#define Z193_REGSPI_CH1_T_HIGH_OFFSET			0x140
#define Z193_REGSPI_CH1_PULSE_HIGH_OFFSET	    0x144
#define Z193_REGSPI_CH2_PERIOD_OFFSET			0x148  /* EValve PWM */
#define Z193_REGSPI_CH2_PULSE_OFFSET			0x14C
#define Z193_REGSPI_CH3_PERIOD_OFFSET			0x150  /* Brake PWM */
#define Z193_REGSPI_CH3_PULSE_OFFSET          0x154

/* 16Z044-01 DISP */
/* see \VXWORKS\INCLUDE\COM\MEN\16z044_disp.h */
#define Z193_DISP_FOFFS            0x158  /* Start address, where the Emergency Splash Screen is saved, 32-Bit Address, two LSBs=0b00 */


/*16QSPI- QSPI */
/* see \VXWORKS\INCLUDE\COM\MEN\qspim_drv.h */
#define Z193_QSPI_SPCR0_0             0x15C /* SPCR1 / SPCR0 */
#define Z193_QSPI_TIMER_0             0x160 /* Timer Control Register TIMH / TIML */
#define Z193_QSPI_PORTQS_DDRQS_PQSPAR 0x164 /* PIN Control Register */
#define Z193_QSPI_TIMER_1             0x168 /* Timer Control Register TIMH / TIML Note: The Timer Control Register is written twice */
#define Z193_QSPI_COMDRAM0            0x16C /* Command RAM: Alarm Lamp, Alarm Monitor 0 */
#define Z193_QSPI_COMDRAM1            0x170 /* Command RAM: Alarm Monitor 0/1  */
#define Z193_QSPI_COMDRAM2            0x174 /* Command RAM: Alarm Monitor 1 */
#define Z193_QSPI_SPCR4_0              0x178 /* SPCR5 / SPCR4*/
#define Z193_QSPI_TRANRAM0            0x17C /* Fill TX RAM  */
#define Z193_QSPI_TRANRAM1            0x180 /* Fill TX RAM  */
#define Z193_QSPI_TRANRAM2            0x184 /* Fill TX RAM  */
#define Z193_QSPI_TRANRAM3            0x188 /* Fill TX RAM  */
#define Z193_QSPI_TRANRAM4            0x18C /* Fill TX RAM  */
#define Z193_QSPI_SPCR2               0x190 /* SPCR3 / SPCR2*/
#define Z193_QSPI_SPCR4_1             0x194 /* Set pointer values  */
#define Z193_QSPI_SPCR0_1             0x198 /* SPCR1 / SPCR0 */


/* #else / * Z193_MODEL_15EM10A01 * /
# error "No valid Z193 model defined!\n" */
#endif

#ifdef __cplusplus
    }
#endif

#endif    /* _Z193_EMG_CTL_H */
