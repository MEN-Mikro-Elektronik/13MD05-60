/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: vxttyioctl.h
 *      Project:
 *
 *       Author: sv
 *        $Date: 2012/05/15 17:37:35 $
 *    $Revision: 2.9 $
 *
 *  Description: MEN specific VxWorks TTY ioctl definitions
 *
 *     Required:
 *     Switches:
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: vxttyioctl.h,v $
 * Revision 2.9  2012/05/15 17:37:35  channoyer
 * R: Not possible to read the UART mode
 * M: Add ioctl MEN_UART_IOCTL_MODE_GET
 *
 * Revision 2.8  2008/06/09 13:17:36  SVogel
 * R:1. each transmitted byte generated an interrupt,
 *      if fifo trigger level was set to 1
 * M:1. added additional i/o control to set fifo tx trigger level
 *
 * Revision 2.7  2008/02/26 11:12:59  cs
 * cosmetic: added newline at end of file
 *
 * Revision 2.6  2007/10/18 17:13:21  SVogel
 * cosmetics
 *
 * Revision 2.3  2007/02/27 09:26:10  svogel
 * renamed:
 * MEN_UART_XON_XOFF      ==> MEN_UART_IOCTL_XON_XOFF
 * MEN_UART_RTS_CTS       ==> MEN_UART_IOCTL_RTS_CTS
 * MEN_UART_DTR_DSR       ==> MEN_UART_IOCTL_DTR_DSR
 * MEN_UART_HANDSHAKE_OFF ==> MEN_UART_IOCTL_HANDSHAKE_OFF
 *
 * Revision 2.2  2007/02/14 17:12:18  RLange
 * added defines (SVogel)
 *
 * Revision 2.1  2005/06/23 08:59:57  SVogel
 * Initial Revision
 *
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2007 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
#ifndef __VXTTYIOCTL_H
#define __VXTTYIOCTL_H

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------+
|   INCLUDES                             |
+--------------------------------------*/

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define MEN_UART_IOCTL_DATABITS           100
#define MEN_UART_IOCTL_PARITY             101
#define MEN_UART_IOCTL_STOPBITS           102
#define MEN_UART_IOCTL_SET_RTS            113
#define MEN_UART_IOCTL_SET_DTR            114
#define MEN_UART_IOCTL_SET_OUT1           115
#define MEN_UART_IOCTL_SET_OUT2           116
#define MEN_UART_IOCTL_GET_CTS            117
#define MEN_UART_IOCTL_GET_DSR            118
#define MEN_UART_IOCTL_GET_DCD            119
#define MEN_UART_IOCTL_GET_DTR            120
#define MEN_UART_IOCTL_GET_DTS            121
#define MEN_UART_IOCTL_GET_RTS            122
#define MEN_UART_IOCTL_GET_RING           123

#define MEN_UART_IOCTL_MODE_SELECT        130
#define MEN_UART_IOCTL_AUTO_RS485         131
#define MEN_UART_IOCTL_MODE_GET           132
#define MEN_UART_IOCTL_MODEM              140
#define MEN_UART_IOCTL_SET_FIFO_BYTES     141
#define MEN_UART_IOCTL_GET_RCV_BYTES      142
#define MEN_UART_IOCTL_SET_TX_FIFO_BYTES  143
#define MEN_UART_IOCTL_LINE_STATUS        150

/* handshake variants */
#define MEN_UART_IOCTL_XON_XOFF           160
#define MEN_UART_IOCTL_RTS_CTS            161
#define MEN_UART_IOCTL_DTR_DSR            162
#define MEN_UART_IOCTL_HANDSHAKE_OFF      163

#ifdef __cplusplus
}
#endif

#endif /* __VXTTYIOCTL_H */

