/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  mz25_module.h
 *
 *      \author  sv
 *        $Date: 2012/11/21 23:03:44 $
 *    $Revision: 2.8 $
 *
 *        \brief  16Z025 low-level public definitions
 *
 *                This header file consists of the public defines of the
 *                16Z025 quad UART.
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mz25_module.h,v $
 * Revision 2.8  2012/11/21 23:03:44  ts
 * R: define for Modem Status contained trailing ; which can cause build errors
 * M: removed Semicolon
 *
 * Revision 2.7  2009/05/08 18:14:55  cs
 * R: wrong ACR register description (ECHON) in ICDesignSpec_S3 of Z25/Z125
 * M: don't set MIZ25_ECHON for MZ25_MODE_RS485_FD mode
 *
 * Revision 2.6  2008/12/22 17:58:18  AWanka
 * R: If the interrupt was not from the z25, the z25 ISR handles the interrupt
 *    nevertheless as a modem status interrupt
 * M: Now the ISR checks the MIZ25_IRQN Bit to decide if the interrupt was from
 *    z25 core or not.
 *
 * Revision 2.5  2008/06/09 17:16:01  SVogel
 * R:1. each transmitted byte generated an interrupt,
 *      if fifo trigger level was set to 1
 * M:1. changed MZ25_SetFifoTriggerLevel interface (added 'rxTx' parameter)
 *
 * Revision 2.4  2008/02/26 11:12:23  cs
 * R:1. malicious detection of interrupt cause in driver
 * M:1. mask MSIEN bit in MZ25_IRQ_IDENT macro
 *
 * Revision 2.3  2007/07/06 14:53:01  SVogel
 * + cosmetics
 * + added MZ25_EnableAutoRtsCts
 *
 * Revision 2.2  2005/07/19 14:42:13  CSchuster
 * added MAC_MEM_MAPPED macro definition (needed when used in sysSerial.h)
 *
 * Revision 2.1  2005/06/23 08:59:53  SVogel
 * Initial Revision
 *
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef	_MZ25_MODULE_H
#define	_MZ25_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------+
 |  INCLUDES        |
 +------------------*/
#ifdef MAC_IO_MAPPED
#	undef MAC_IO_MAPPED
#endif
#ifndef MAC_MEM_MAPPED
#	define MAC_MEM_MAPPED
#endif

#include <MEN/dbg.h>
#include <MEN/maccess.h>
#include "../../../DRIVERS/NATIVE/Z25_UART/mz25_module_int.h"
/*------------------+
 |  DEFINES         |
 +------------------*/

/*-----------------------------+
 |  parity                     |
 +-----------------------------*/
#define MZ25_NO_PARITY			(0x00)				       /**< no parity */
#define MZ25_EVEN_PARITY		(MIZ25_PEN|MIZ25_PTYPE)  /**< even parity */
#define MZ25_ODD_PARITY		(MIZ25_PEN)	           /**< odd parity */

/*-----------------------------+
 |  stop bits                  |
 +-----------------------------*/
#define MZ25_STOPBITS_1			(0x00)		        /**< 1 stopbit */
#define MZ25_STOPBITS_2			(MIZ25_NOSTP)		/**< 2 stopbit */
/*-----------------------------+
 |  data bits                  |
 +-----------------------------*/
#define MZ25_DATABITS_5			(0x00)		                /**< 5 databits */
#define MZ25_DATABITS_6			(MIZ25_WL0)         		/**< 6 databits */
#define MZ25_DATABITS_7			(MIZ25_WL1)	            /**< 7 databits */
#define MZ25_DATABITS_8			(MIZ25_WL0|MIZ25_WL1)		/**< 8 databits */

/*-----------------------------+
 |  interrupt type             |
 +-----------------------------*/

 #define MZ25_THREIEN              MIZ25_THREIEN
 #define MZ25_MSIEN                MIZ25_MSIEN
 #define MZ25_RLSIEN               MIZ25_RLSIEN
 #define MZ25_RDAIEN               MIZ25_RDAIEN
 /*
   IIR register - interrupt code
  	-------------------------------------------------------------------
   |  bit7  |  bit6  | bit5  | bit4  |  bit3  | bit2  |  bit1  |  bit0 |
    -------------------------------------------------------------------
   | FIFOEN1| FIFOEN0|   -   |   -   |   IID2 |  IID1 |  IID0  |  IRQN |
    -------------------------------------------------------------------
*/
#define MZ25_RCV_LINE_STATUS		(MIZ25_IID0|MIZ25_IID1) /**< line status */
#define MZ25_DATA_RECEIVED		    (MIZ25_IID1)            /**< data received */
#define MZ25_CHAR_TIMEOUT			(MIZ25_IID1|MIZ25_IID2) /**< timeout */
#define MZ25_THR_REG_EMPTY		    (MIZ25_IID0)		    /**< THR empty */
#define MZ25_MODEM_STAT			(0x00)				        /**< modem status */




#define MZ25_FIFO_1_BYTE            (0x00)                  /**< 1 byte trigger */
#define MZ25_FIFO_4_BYTE            (MIZ25_RXTRLV0)         /**< 4 byte trigger */
#define MZ25_FIFO_30_BYTE           (MIZ25_RXTRLV1)         /**< 30 byte trigger */
#define MZ25_FIFO_58_BYTE           (MIZ25_RXTRLV1|MIZ25_RXTRLV0)
                                                            /* 58 byte trigger */

#define MZ25_MODE_RS232             (MIZ25_RXEN)            /**< rs232 mode */
#define MZ25_RS485                  (MIZ25_RXEN|MIZ25_DIFF)
#define MZ25_MODE_RS485_HD          (MZ25_RS485|MIZ25_HD|MIZ25_ECHON)   /**< half duplex */
#define MZ25_MODE_RS485_FD          MZ25_RS485              /**< full duplex */


#define MZ25_U1_IRQ				  MIZ25_U1_IRQ		/**< uart 1 interrupt */
#define MZ25_U2_IRQ				  MIZ25_U2_IRQ		/**< uart 2 interrupt */
#define MZ25_U3_IRQ				  MIZ25_U3_IRQ		/**< uart 3 interrupt */
#define MZ25_U4_IRQ				  MIZ25_U4_IRQ		/**< uart 4 interrupt */

/*-----------------------------+
 |  register access            |
 +-----------------------------*/
#define MZ25_REG_READ(addr, reg)		    MREAD_D8(addr, reg)
#define MZ25_REG_WRITE(addr, reg, data)		MWRITE_D8(addr, reg, data)

#define MZ25_READ_BYTE(x)         MZ25_REG_READ (x,  MIZ25_RHR_OFFSET)
#define MZ25_WRITE_BYTE(x, byte)  MZ25_REG_WRITE(x, MIZ25_THR_OFFSET, byte)
#define MZ25_RECEIVE_STATUS(x)    (MZ25_REG_READ(x, MIZ25_LSR_OFFSET) & 0x01)
#define MZ25_LINE_STATUS(x)       MZ25_REG_READ (x,  MIZ25_LSR_OFFSET)
#define MZ25_MODEM_STATUS(x)      MZ25_REG_READ (x,  MIZ25_MSR_OFFSET)
#define MZ25_IRQ_IDENT(x)         (MZ25_REG_READ(x, MIZ25_IIR_OFFSET) & 0x0F)

#define MZ25_UARTS_EXISTING(x)    (MZ25_REG_READ(x, MIZ25_IDIRQ_OFFSET) >> 4)
#define MZ25_UART_INTERRUPT(x)    (MZ25_REG_READ(x, MIZ25_IDIRQ_OFFSET)&0x0f)



/*------------------+
 |  TYPEDEFS        |
 +------------------*/
typedef void HDL_16Z25;			/**< 16Z025 unit handle */

/** This enumeration contains the Z25 return values.
 */
typedef enum {/* MZ25_RETURN */
	MZ25_OK = OK,
	MZ25_ERROR = ERROR
} MZ25_RETURN;

/*------------------+
 |  PROTOTYPES      |
 +------------------*/
extern void MZ25_ModuleId(void);

extern STATUS MZ25_InitHandle(HDL_16Z25 **hdlP, u_int32 address);
extern STATUS MZ25_FreeHandle(HDL_16Z25 **hdlP);

extern u_int8 MZ25_EnableInterrupt(HDL_16Z25 *hdlp, u_int8 value);
extern u_int8 MZ25_DisableInterrupt(HDL_16Z25 *hdlp, u_int8 value);

extern STATUS MZ25_SetBaudrate(HDL_16Z25 *hdlP, int32 value);
extern STATUS MZ25_GetBaudrate(HDL_16Z25 *hdlP, int32 *valueP);

extern STATUS MZ25_SetSerialParameter(HDL_16Z25 *hdlP, u_int8 value);
extern STATUS MZ25_GetSerialMode(HDL_16Z25 *hdlP);

extern STATUS MZ25_SetDatabits(HDL_16Z25 *hdlP, int32 value);
extern STATUS MZ25_SetStopbits(HDL_16Z25 *hdlP, int32 value);
extern STATUS MZ25_SetParity(HDL_16Z25 *hdlP,int32 value);

extern STATUS MZ25_SetFifoTriggerLevel(HDL_16Z25 *hdlP, u_int8 rxTx, u_int8 value);
extern STATUS MZ25_SetRts(HDL_16Z25 *hdlP, BOOL status);

extern STATUS MZ25_EnableAutoRtsCts(HDL_16Z25 *hdlP, BOOL status);
extern STATUS MZ25_SetDtr(HDL_16Z25 *hdlP, BOOL status);
extern STATUS MZ25_SetOut1(HDL_16Z25 *hdlP, BOOL status);
extern STATUS MZ25_SetOut2(HDL_16Z25 *hdlP, BOOL status);

extern int32 MZ25_GetCts(HDL_16Z25 *hdlP);
extern int32 MZ25_GetDsr(HDL_16Z25 *hdlP);
extern int32 MZ25_GetDcd(HDL_16Z25 *hdlP);

extern STATUS MZ25_SetSerialMode(HDL_16Z25 *hdlP, int32 value);
extern STATUS MZ25_SetLineStatus(HDL_16Z25 *hdlP);
extern int32 MZ25_GetLineStatus(HDL_16Z25 *hdlP);

extern STATUS MZ25_SetBaseBaud(HDL_16Z25 *hdlP, u_int32 frequency);
extern STATUS MZ25_ControlModemTxInt(HDL_16Z25 *hdlP);

extern STATUS MZ25_SetModemControl(HDL_16Z25 *hdlP, BOOL onOff);
extern STATUS MZ25_GetModemControl(HDL_16Z25 *hdlP);
#ifdef __cplusplus
}
#endif

#endif/* _MZ25_MODULE_H */



