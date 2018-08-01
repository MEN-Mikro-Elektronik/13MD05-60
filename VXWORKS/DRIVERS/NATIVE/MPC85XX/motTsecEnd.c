/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: motFccEnd.c
 *      Project: VxWorks for EM3 family
 *
 *       Author: WRS (modified by kp)
 *        $Date: 2010/01/22 13:34:44 $
 *    $Revision: 1.9 $
 *
 *  Description: Support routines for MPC85XX TSEC and 8540 FEC
 *
 * This driver was taken from ADS8560 BSP 1.2/3. I tried to keep
 * this driver unchanged as much as possible.
 *
 *     Required:  -
 *     Switches:  -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: motTsecEnd.c,v $
 * Revision 1.9  2010/01/22 13:34:44  cs
 * R:1. VxW 6.7 compiler is more restringent and uncovered signed/unsigned issues
 *   2. VxW 6.7: compiler warning implicit reference to ffsMsb
 * M:1. resolved signed/unsigned mismatches in comparisons and assignments
 *   2. add include of ffsLib.h for VxW >= 6.4, import ffsMsb for VxW < 6.4
 *
 * Revision 1.8  2009/11/26 18:19:20  cs
 * R: vxW 6.6 ++ caused major linker errors for ffsMsb
 * M: remove explicit import of ffsMsb, it is already imported in include files
 *
 * Revision 1.7  2008/12/18 10:56:35  cs
 * R: debug message in motTsecHandleRXFrames() was rather confusing
 * M: cosmetics in debug message
 *
 * Revision 1.6  2008/09/22 14:03:22  cs
 * R: netJobInfo not known when compiled for VxWorks 6.6 (depracated?)
 * M: use netJobQueueId instead for VxWorks >= 6.6
 *
 * Revision 1.5  2008/01/22 15:55:38  cs
 * added:
 *   - support for 4th TSEC
 *   - support for VxWorks 6.5
 *     (merged update from WR driver version 01v,05sep06)
 *
 * Revision 1.4  2007/07/20 12:11:41  cs
 * added:
 *   - support for 3rd TSEC
 *   - support for MOT_TSEC_INT_CTRL passing from sysMotTsecEnd.c
 * merged updates from WR driver version (01u,31mar06)
 *
 * Revision 1.3  2007/03/08 15:20:01  cs
 * added:
 *   - support for EP4 (TBI, more than one MII interface)
 *   - parse/address user flags passed at motTsecEndLoad()
 * fixed:
 *   - cleared undefined reference to sysUsDelay()
 *
 * Revision 1.2  2005/07/05 12:28:22  kp
 * - MII read/write funcs did not work at all
 * - allow MII functions to be used from interrupt
 * - driver can now handle line status changes from any speed to any duplex mode
 *   (on EM3 family...)
 * - Support 8540 FEC
 * Reformatted source with indent command
 *
 *---------------------------------------------------------------------------
 ****************************************************************************/

/*
 Notes: kp@men.de

 - Forget the DESCRIPTION section below. Out-of-date and
 perhaps copied from another driver.

 - The correct load string syntax is:

 "unit:immrVal:tsecNum:mac-addr:usrFlags:phyParms:tsecFuncs,tsecParms"

 usrFlags:
  MOT_TSEC_USR_MODE_xxx:	 MII mode
  MOT_TSEC_USR_PHY_xxx:		 ignored!

 phyParms:
  structure with
   - phy def mode (ignored!)
   - phy address
   - phy autoneg order table (ignored!)
   - phy delays (ignored!)

 tsecFuncs:
  structure with function callbacks

  - phyInitFunc		- currently NOP in BSP
  - phyInt			- set to motTsecPhyLSCInt. Must be called by BSP on
                      link changes
  - phyStatusFunc	- read phy status
  - enetEnable		- currently NOP in BSP
  - enetDisable		- currently NOP in BSP
  - enetAddrGet		- not used by driver
  - enetAddrSet 	- not used by driver
  - extWriteL2AllocFunc - for allocating lines in L2 cache

 initParms:
 structure with
 - memBufPtr;		not used by driver
 - memBufSize;		0 for default
 - bdBasePtr;		NULL auto
 - bdSize;			0 for default
 - rbdNum;			0 for default
 - tbdNum;			0 for default
 - unit0xxx			not used by driver
 - unit1xxx			not used by driver

 initParmsExt:	NULL for default
  - lot of params (most of them not yet used), see motTsecEnd.h



 motTsecEnd.c talks with PHY directly only in some cases,
 search for "TsecMiiPhy"

 Major changes:

 - MII read/write funcs did not work at all
 - allow MII functions to be used from interrupt
 - driver can now handle line status changes from any speed to any duplex mode
   (on EM3 family...)

 - Support 8540 FEC


*/

/* motTsecEnd.c - Motorola Three-Speed Ethernet Controller network interface.*/

/*
 * Copyright (c) 2003-2006 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */
#include "copyright_wrs.h"

/*
modification history
--------------------
01v,05sep06,kch  Removed unnecessary coreip header includes.
01u,31mar06,dlk  SPR #119803 - IPv6 forwarding under load performance.
01t,06mar06,dlk  Initialize the RXIC register.
01s,02mar06,wap  Correct RX and TX interrupt handlers (SPR #118413)
01r,01mar06,wap  Fix race condition in TX handling (SPR #118212)
01q,29aug05,wap  Add support for IFF_ALLMULTI (SPR #110201)
01p,26aug05,dlk  Add section tags.  Address SPR #111278. Minor clean-up
                 in debug code.
01o,26jun05,dlk  Replace netJobAdd() with jobQueuePost(). Remove some
                 unnecessary variables, improve show routines.
01v,22apr05,dtr  Extra synchronization for MACCFG1 writes - SPR 108400.
01u,24mar05,dtr  Changes from code review.
01t,31jan05,dtr  Decouple from interrupt controller.
                 Promiscuous mode fixed SPR 107033.
		 SPR 105491 - fix various performance issues.
01s,26oct04,rcs  Fixed SPR 102905 - multicast not working
01r,14oct04,dtr  SPR 102536 - performance enhancements.
01q,01oct04,rcs  Stream lined the receive and transmit fucnctions.
01p,24sep04,jln  fixed a memory leak for motTsecUnload
01o,09sep04,jln  fixed MIB support
01n,01sep04,rcs  Added pulled mode generic MIB support and motTsecUnload
01m,31aug04,mdo  Documentation fixes for apigen
01l,08aug04,dtr  fixed SPR 100706.
01k,21jul04,rcs  fixed SPR 99009 and SPR 96745
01j,21jun04,mil  Changed cacheArchXXX funcs to cacheXXX funcs.
01i,12may04,rcs  Set maccfg2 register to match PHY when auto-negotiation
                 establishes a half duplex connection
01h,12may04,rcs  Fixed SPR97012
01g,14apr04,rcs  Fixed SPR96253
01f,26mar04,rcs  Fixed SPR95430, SPR95433, SPR95434, and SPR95435
01e,12feb04,rcs  Added pull up code to coalesce fragments when mBlk chains
                 exceed available resources.
01d,11feb04,rcs  Added netJobAdds for motTsecHandle() when txStall condition.
01c,02feb04,rcs  Added workaround for softReset().
01b,08aug03,rcs  Modified to make operational
01a,14jan03,gjc  Motorola TSEC END Driver.
*/

/*
DESCRIPTION

This module implements a Motorola Three-Speed Ethernet Controller (TSEC)
Ethernet network interface driver.

The drivers work load is distributed between the interrupt and the net job
queue. Only one net job add is allowed per interrupt. Allocation of tuples
 happens in the code executing on the job context.

RFC 2233 and IPV6 support were added as features.

The TSEC supports several communication protocols. This driver supports the TSEC
to operate in Ethernet mode which is fully compliant with the IEEE 802.3u
10Base-T and 100Base-T specifications.

The TSEC establishes a shared memory communication system with the CPU,
which may be divided into three parts: a set of Control/Status Registers (CSR)
and TSEC-specific parameters, the buffer descriptors (BD), and the data buffers.

Both the CSRs and the internal parameters reside in the MPC8560's internal
RAM. They are used for mode control and to extract status information
of a global nature. For instance, the types of events that should
generate an interrupt, or features like the promiscuous mode or the
heartbeat control may be set programming some of the CSRs properly.
Pointers to both the Transmit Buffer Descriptors ring (TBD) and the
Receive Buffer Descriptors ring (RBD) are stored in the internal parameter
RAM. The latter also includes protocol-specific parameters, like the
individual physical address of this station or the max receive frame length.

The BDs are used to pass data buffers and related buffer information
between the hardware and the software. They may reside either on the 60x
bus, or on the CPM local bus They include local status information and a
pointer to the incoming or outgoing data buffers. These are located again
in external memory, and the user may chose whether this is on the 60x bus,
or the CPM local bus (see below).

This driver is designed to be moderately generic. Without modification, it can
operate across all the TSECs in the MPC8260, regardless of where the internal
memory base address is located. To achieve this goal, this driver must be
given several target-specific parameters, and some external support routines
must be provided.  These parameters, and the mechanisms used to communicate
them to the driver, are detailed below.

This network interface driver does not include support for trailer protocols
or data chaining.  However, buffer loaning has been implemented in an effort
to boost performance. In addition, no copy is performed of the outgoing packet
before it is sent.

BOARD LAYOUT
This device is on-board.  No jumpering diagram is necessary.

EXTERNAL INTERFACE

The driver provides the standard external interface, motTsecEnd2Load(), which
takes a string of colon-separated parameters. The parameters should be
specified in hexadecimal, optionally preceded by "0x" or a minus sign "-".

The parameter string is parsed using strtok_r() and each parameter is
converted from a string representation to binary by a call to
strtoul(parameter, NULL, 16).

The format of the parameter string is:

"<immrVal>:<tsecNum>:<bdBase>:<bdSize>:<bufBase>:<bufSize>:<fifoTxBase>:
<fifoRxBase> :<tbdNum>:<rbdNum>:<phyAddr>:<phyDefMode>:<userFlags>:
<function table>"

TARGET-SPECIFIC PARAMETERS

\is
\i <immrVal>
Indicates the address at which the host processor presents its internal
memory (also known as the internal RAM base address). With this address,
and the tsecNum (see below), the driver is able to compute the location of
the TSEC parameter RAM, and, ultimately, to program the TSEC for proper
operations.

\i <tsecNum>
This driver is written to support multiple individual device units.
This parameter is used to explicitly state which TSEC is being used (on the
vads8260 board, TSEC2 is wired to the Fast Ethernet transceiver, thus this
parameter equals "2").

\i <bdBase>
The Motorola Fast Communication Controller is a DMA-type device and typically
shares access to some region of memory with the CPU. This driver is designed
for systems that directly share memory between the CPU and the TSEC.

This parameter tells the driver that space for both the TBDs and the
RBDs needs not be allocated but should be taken from a cache-coherent
private memory space provided by the user at the given address. The user
should be aware that memory used for buffers descriptors must be 8-byte
aligned and non-cacheable. Therefore, the given memory space should allow
for all the buffer descriptors and the 8-byte alignment factor.

If this parameter is "NONE", space for buffer descriptors is obtained
by calling cacheDmaMalloc() in motTsecEndLoad().

\i <bdSize>
The memory size parameter specifies the size of the pre-allocated memory
region for the BDs. If <bdBase> is specified as NONE (-1), the driver ignores
this parameter. Otherwise, the driver checks the size of the provided memory
region is adequate with respect to the given number of Transmit Buffer
Descriptors and Receive Buffer Descriptors.

\i <bufBase>
This parameter tells the driver that space for data buffers
needs not be allocated but should be taken from a cache-coherent
private memory space provided by the user at the given address. The user
should be aware that memory used for buffers must be 32-byte
aligned and non-cacheable. The TSEC poses one more constraint in that DMA
cycles may initiate even when all the incoming data have already been
transferred to memory. This means at most 32 bytes of memory at the end of
each receive data buffer, may be overwritten during reception. The driver
pads that area out, thus consuming some additional memory.

If this parameter is "NONE", space for buffer descriptors is obtained
by calling memalign() in sbcMotTsecEndLoad().

\i <bufSize>
The memory size parameter specifies the size of the pre-allocated memory
region for data buffers. If <bufBase> is specified as NONE (-1), the driver
ignores this parameter. Otherwise, the driver checks the size of the provided
memory region is adequate with respect to the given number of Receive Buffer
Descriptors and a non-configurable number of transmit buffers
(MOT_TSEC_TX_CL_NUM).  All the above should fit in the given memory space.
This area should also include room for buffer management structures.

\i <fifoTxBase>
Indicate the base location of the transmit FIFO, in internal memory.
The user does not need to initialize this parameter, as the default
value (see MOT_TSEC_FIFO_TX_BASE) is highly optimized for best performance.
However, if the user wishes to reserve that very area in internal RAM for
other purposes, he may set this parameter to a different value.

If <fifoTxBase> is specified as NONE (-1), the driver uses the default
value.

\i <fifoRxBase>
Indicate the base location of the receive FIFO, in internal memory.
The user does not need to initialize this parameter, as the default
value (see MOT_TSEC_FIFO_TX_BASE) is highly optimized for best performance.
However, if the user wishes to reserve that very area in internal RAM for
other purposes, he may set this parameter to a different value.

If <fifoRxBase> is specified as NONE (-1), the driver uses the default
value.

\i <tbdNum>
This parameter specifies the number of transmit buffer descriptors (TBDs).
Each buffer descriptor resides in 8 bytes of the processor's external
RAM space, If this parameter is less than a minimum number specified in the
macro MOT_TSEC_TBD_MIN, or if it is "NULL", a default value of 64 (see
MOT_TSEC_TBD_DEF_NUM) is used. This number is kept deliberately high, since
each packet the driver sends may consume more than a single TBD. This
parameter should always equal a even number.

\i <rbdNum>
This parameter specifies the number of receive buffer descriptors (RBDs).
Each buffer descriptor resides in 8 bytes of the processor's external
RAM space, and each one points to a 1584-byte buffer again in external
RAM. If this parameter is less than a minimum number specified in the
macro MOT_TSEC_RBD_MIN, or if it is "NULL", a default value of 32 (see
MOT_TSEC_RBD_DEF_NUM) is used. This parameter should always equal a even number.

\i <phyAddr>
This parameter specifies the logical address of a MII-compliant physical
device (PHY) that is to be used as a physical media on the network.
Valid addresses are in the range 0-31. There may be more than one device
under the control of the same management interface. The default physical
layer initialization routine will scan the whole range of PHY devices
starting from the one in <phyAddr>. If this parameter is
"MII_PHY_NULL", the default physical layer initialization routine will find
out the PHY actual address by scanning the whole range. The one with the lowest
address will be chosen.

\i <phyDefMode>
This parameter specifies the operating mode that will be set up
by the default physical layer initialization routine in case all
the attempts made to establish a valid link failed. If that happens,
the first PHY that matches the specified abilities will be chosen to
work in that mode, and the physical link will not be tested.

\i <pAnOrderTbl>
This parameter may be set to the address of a table that specifies the
order how different subsets of technology abilities should be advertised by
the auto-negotiation process, if enabled. Unless the flag <MOT_TSEC_USR_PHY_TBL>
is set in the userFlags field of the load string, the driver ignores this
parameter.

The user does not normally need to specify this parameter, since the default
behaviour enables auto-negotiation process as described in IEEE 802.3u.

\i <userFlags>
This field enables the user to give some degree of customization to the
driver.

MOT_TSEC_USR_PHY_NO_AN: the default physical layer initialization
routine will exploit the auto-negotiation mechanism as described in
the IEEE Std 802.3u, to bring a valid link up. According to it, all
the link partners on the media will take part to the negotiation
process, and the highest priority common denominator technology ability
will be chosen. It the user wishes to prevent auto-negotiation from
occurring, he may set this bit in the user flags.

MOT_TSEC_USR_PHY_TBL: in the auto-negotiation process, PHYs
advertise all their technology abilities at the same time,
and the result is that the maximum common denominator is used. However,
this behaviour may be changed, and the user may affect the order how
each subset of PHY's abilities is negotiated. Hence, when the
MOT_TSEC_USR_PHY_TBL bit is set, the default physical layer
initialization routine will look at the motTsecAnOrderTbl[] table and
auto-negotiate a subset of abilities at a time, as suggested by the
table itself. It is worth noticing here, however, that if the
MOT_TSEC_USR_PHY_NO_AN bit is on, the above table will be ignored.

MOT_TSEC_USR_PHY_NO_FD: the PHY may be set to operate in full duplex mode,
provided it has this ability, as a result of the negotiation with other
link partners. However, in this operating mode, the TSEC will ignore the
collision detect and carrier sense signals. If the user wishes not to
negotiate full duplex mode, he should set the MOT_TSEC_USR_PHY_NO_FD bit
in the user flags.

MOT_TSEC_USR_PHY_NO_HD: the PHY may be set to operate in half duplex mode,
provided it has this ability, as a result of the negotiation with other link
partners. If the user wishes not to negotiate half duplex mode, he should
set the MOT_TSEC_USR_PHY_NO_HD bit in the user flags.

MOT_TSEC_USR_PHY_NO_100: the PHY may be set to operate at 100Mbit/s speed,
provided it has this ability, as a result of the negotiation with
other link partners. If the user wishes not to negotiate 100Mbit/s speed,
he should set the MOT_TSEC_USR_PHY_NO_100 bit in the user flags.

MOT_TSEC_USR_PHY_NO_10: the PHY may be set to operate at 10Mbit/s speed,
provided it has this ability, as a result of the negotiation with
other link partners. If the user wishes not to negotiate 10Mbit/s speed,
he should set the MOT_TSEC_USR_PHY_NO_10 bit in the user flags.

MOT_TSEC_USR_PHY_ISO: some boards may have different PHYs controlled by the
same management interface. In some cases, there may be the need of
electrically isolating some of them from the interface itself, in order
to guarantee a proper behaviour on the medium layer. If the user wishes to
electrically isolate all PHYs from the MII interface, he should set the
MOT_TSEC_USR_PHY_ISO bit. The default behaviour is to not isolate any
PHY on the board.

MOT_TSEC_USR_LOOP: when the MOT_TSEC_USR_LOOP bit is set, the driver will
configure the TSEC to work in internal loopback mode, with the TX signal
directly connected to the RX. This mode should only be used for testing.

MOT_TSEC_USR_RMON: when the MOT_TSEC_USR_RMON bit is set, the driver will
configure the TSEC to work in RMON mode, thus collecting network statistics
required for RMON support without the need to receive all packets as in
promiscuous mode.

MOT_TSEC_USR_BUF_LBUS: when the MOT_TSEC_USR_BUF_LBUS bit is set, the driver will
configure the TSEC to work as though the data buffers were located in the
CPM local bus.

MOT_TSEC_USR_BD_LBUS: when the MOT_TSEC_USR_BD_LBUS bit is set, the driver will
configure the TSEC to work as though the buffer descriptors were located in the
CPM local bus.

MOT_TSEC_USR_HBC: if the MOT_TSEC_USR_HBC bit is set, the driver will
configure the TSEC to perform heartbeat check following end of transmission
and the HB bit in the status field of the TBD will be set if the collision
input does not assert within the heartbeat window. The user does not normally
need to set this bit.

\i <Function>
This is a pointer to the structure TSEC_END_FUNCS. The structure contains mostly
FUNCPTRs that are used as a communication mechanism between the driver and the
BSP. If the pointer contains a NULL value, the driver will use system default
functions for the m82xxDpram DPRAM allocation and, obviously, the driver will
not support BSP function calls for heart beat errors, disconnect errors, and
PHY status changes that are hardware specific.

\cs
    FUNCPTR miiPhyInit; BSP Mii/Phy Init Function
    This function pointer is initialized by the BSP and call by the driver to
    initialize the mii driver. The driver sets up it's phy settings and then
    calls this routine. The BSP is responsible for setting BSP specific phy
    parameters and then calling the miiPhyInit. The BSP is responsible to set
    up any call to an interrupt. See miiPhyInt below.

\ce
\cs
    FUNCPTR miiPhyInt; Driver Function for BSP to Call on a Phy Status Change
    This function pointer is initialized by the driver and call by the BSP.
    The BSP calls this function when it handles a hardware mii specific
    interrupt. The driver initializes this to the function motTsecPhyLSCInt.
    The BSP may or may not choose to call this function. It will depend if the
    BSP supports an interrupt driven PHY. The BSP can also set up the miiLib
    driver to poll. In this case the miiPhy driver calls this function. See the
    miiLib for details.
    Note: Not calling this function when the phy duplex mode changes will
    result in a duplex mis-match. This will cause TX errors in the driver
    and a reduction in throughput.
\ce
\cs
    FUNCPTR miiPhyBitRead; MII Bit Read Function
    This function pointer is initialized by the BSP and call by the driver.
    The driver calls this function when it needs to read a bit from the mii
    interface. The mii interface is hardware specific.
\ce
\cs
    FUNCPTR miiPhyBitWrite; MII Bit Write Function
    This function pointer is initialized by the BSP and call by the driver.
    The driver calls this function when it needs to write a bit to the mii
    interface. This mii interface is hardware specific.
\ce
\cs
    FUNCPTR miiPhyDuplex; Duplex Status Call Back
    This function pointer is initialized by the BSP and call by the driver.
    The driver calls this function to obtain the status of the duplex
    setting in the phy.
\ce
\cs
    FUNCPTR miiPhySpeed; Speed Status Call Back
    This function pointer is initialized by the BSP and call by the driver.
    The driver calls this function to obtain the status of the speed
    setting in the phy. This interface is hardware specific.
\ce
\cs
    FUNCPTR hbFail; Heart Beat Fail Indicator
    This function pointer is initialized by the BSP and call by the driver.
    The driver calls this function to indicate an TSEC heart beat error.
\ce
\cs
    FUNCPTR intDisc; Disconnect Function
    This function pointer is initialized by the BSP and call by the driver.
    The driver calls this function to indicate an TSEC disconnect error.
\ce

\ie

EXTERNAL SUPPORT REQUIREMENTS
This driver requires several external support functions.
Note: Function pointers _func_xxxxxxxx were removed and replaced with
the TSEC_END_FUNCS structure located in the load string. This is a major
difference between the old motTsecEnd driver and this one.
\is
\i sysTsecEnetEnable()
\cs
    STATUS sysTsecEnetEnable (UINT32 immrVal, UINT8 tsecNum);
\ce
This routine is expected to handle any target-specific functions needed
to enable the TSEC. These functions typically include setting the Port B and C
on the MPC8260 so that the MII interface may be used. This routine is
expected to return OK on success, or ERROR. The driver calls this routine,
once per device, from the motTsecStart () routine.
\i sysTsecEnetDisable()
\cs
    STATUS sysTsecEnetDisable (UINT32 immrVal, UINT8 tsecNum);
\ce
This routine is expected to perform any target specific functions required
to disable the MII interface to the TSEC.  This involves restoring the
default values for all the Port B and C signals. This routine is expected to
return OK on success, or ERROR. The driver calls this routine from the
motTsecStop() routine each time a device is disabled.
\i sysTsecEnetAddrGet()
\cs
    STATUS sysTsecEnetAddrGet (int unit,UCHAR *address);
\ce
The driver expects this routine to provide the six-byte Ethernet hardware
address that is used by this device.  This routine must copy the six-byte
address to the space provided by <enetAddr>.  This routine is expected to
return OK on success, or ERROR.  The driver calls this routine, once per
device, from the motTsecEndLoad() routine.
\cs
    STATUS sysTsecMiiBitWr (UINT32 immrVal, UINT8 tsecNum, INT32 bitVal);
\ce
This routine is expected to perform any target specific functions required
to write a single bit value to the MII management interface of a MII-compliant
PHY device. The MII management interface is made up of two lines: management
data clock (MDC) and management data input/output (MDIO). The former provides
the timing reference for transfer of information on the MDIO signal.
The latter is used to transfer control and status information between the
PHY and the TSEC. For this transfer to be successful, the information itself
has to be encoded into a frame format, and both the MDIO and MDC signals have
to comply with certain requirements as described in the 802.3u IEEE Standard.
There is not built-in support in the TSEC for the MII management interface.
This means that the clocking on the MDC line and the framing of the information
on the MDIO signal have to be done in software. Hence, this routine is
expected to write the value in <bitVal> to the MDIO line while properly
sourcing the MDC clock to a PHY, for one bit time.
\cs
    STATUS sysTsecMiiBitRd (UINT32 immrVal, UINT8 tsecNum, INT8 * bitVal);
\ce
This routine is expected to perform any target specific functions required
to read a single bit value from the MII management interface of a MII-compliant
PHY device. The MII management interface is made up of two lines: management
data clock (MDC) and management data input/output (MDIO). The former provides
the timing reference for transfer of information on the MDIO signal.
The latter is used to transfer control and status information between the
PHY and the TSEC. For this transfer to be successful, the information itself
has to be encoded into a frame format, and both the MDIO and MDC signals have
to comply with certain requirements as described in the 802.3u IEEE Standard.
There is not built-in support in the TSEC for the MII management interface.
This means that the clocking on the MDC line and the framing of the information
on the MDIO signal have to be done in software. Hence, this routine is
expected to read the value from the MDIO line in <bitVal>, while properly
sourcing the MDC clock to a PHY, for one bit time.
\ie


SYSTEM RESOURCE USAGE
If the driver allocates the memory for the BDs to share with the TSEC,
it does so by calling the cacheDmaMalloc() routine.  For the default case
of 64 transmit buffers and 32 receive buffers, the total size requested
is 776 bytes, and this includes the 8-byte alignment requirement of the
device.  If a non-cacheable memory region is provided by the user, the
size of this region should be this amount, unless the user has specified
a different number of transmit or receive BDs.

This driver can operate only if this memory region is non-cacheable
or if the hardware implements bus snooping.  The driver cannot maintain
cache coherency for the device because the BDs are asynchronously
modified by both the driver and the device, and these fields share
the same cache line.

If the driver allocates the memory for the data buffers to share with the TSEC,
it does so by calling the memalign () routine.  The driver does not need to
use cache-safe memory for data buffers, since the host CPU and the device are
not allowed to modify buffers asynchronously. The related cache lines
are flushed or invalidated as appropriate. For the default case
of 7 transmit clusters and 32 receive clusters, the total size requested
for this memory region is 112751 bytes, and this includes the 32-byte
alignment and the 32-byte pad-out area per buffer of the device.  If a
non-cacheable memory region is provided by the user, the size of this region
should be this amount, unless the user has specified a different number
of transmit or receive BDs.

TUNING HINTS

The only adjustable parameters are the number of TBDs and RBDs that will be
created at run-time.  These parameters are given to the driver when
motTsecEndLoad() is called.  There is one RBD associated with each received
frame whereas a single transmit packet normally uses more than one TBD.  For
memory-limited applications, decreasing the number of RBDs may be
desirable.  Decreasing the number of TBDs below a certain point will
provide substantial performance degradation, and is not recommended. An
adequate number of loaning buffers are also pre-allocated to provide more
buffering before packets are dropped, but this is not configurable.

The relative priority of the netTask and of the other tasks in the system
may heavily affect performance of this driver. Usually the best performance
is achieved when the netTask priority equals that of the other
applications using the driver.

SPECIAL CONSIDERATIONS

SEE ALSO: ifLib,
\tb MPC8260 Fast Ethernet Controller (Supplement to the MPC860 User's Manual)
\tb Motorola MPC860 User's Manual ,

INCLUDE FILES:

\INTERNAL
This driver contains conditional compilation switch MOT_TSEC_DBG.
If defined, adds debug output routines.  Output is further
selectable at run-time via the motTsecEndDbg global variable.
*/

#include "vxWorks.h"
#include "wdLib.h"
#include "iv.h"
#include "vme.h"
#include "net/mbuf.h"
#include "sys/ioctl.h"
#include "errno.h"
#include "memLib.h"
#include "intLib.h"
#include "iosLib.h"
#include "errnoLib.h"
#include "vxLib.h"

#include "cacheLib.h"
#include "logLib.h"
#include "netLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "sysLib.h"
#include "taskLib.h"

#if (_WRS_VXWORKS_MAJOR == 6 && _WRS_VXWORKS_MINOR >=4 ) || _WRS_VXWORKS_MAJOR > 6
	#include "ffsLib.h"
#endif /* VxW >= 6.6 */

#undef ETHER_MAP_IP_MULTICAST
#include "etherMultiLib.h"
#include "end.h"

#define    END_MACROS
#include "endLib.h"
#include "miiLib.h"

#include "lstLib.h"
#include "semLib.h"
#include "sys/times.h"
#include "stdarg.h"

#include "sysEpic.h"

#ifdef WR_IPV6
#include "adv_net.h"
#endif							/* WR_IPV6 */

#include "drv/sio/m8260Cp.h"

#ifndef _WRS_FASTTEXT
#define _WRS_FASTTEXT
#endif

#include "motTsecEnd.h"



#ifdef INCLUDE_WINDVIEW
#undef INCLUDE_WINDVIEW
#endif

#undef INCLUDE_WINDVIEW

#ifdef  INCLUDE_WINDVIEW
/* WindView Event numbers */
#define WV_INT_ENTRY(b,l)            wvEvent(1000,b,l)
#define WV_INT_EXIT(b,l)             wvEvent(1001,b,l)
#define WV_INT_RXB_ENTRY(b,l)        wvEvent(1300,b,l)
#define WV_INT_RXF_ENTRY(b,l)        wvEvent(1310,b,l)
#define WV_INT_BSY_ENTRY(b,l)        wvEvent(1320,b,l)
#define WV_INT_BSY_EXIT(b,l)         wvEvent(1321,b,l)
#define WV_INT_RX_EXIT(b,l)          wvEvent(1301,b,l)
#define WV_INT_RXC_ENTRY(b,l)        wvEvent(1400,b,l)
#define WV_INT_RXC_EXIT(b,l)         wvEvent(1401,b,l)
#define WV_INT_TXC_ENTRY(b,l)        wvEvent(1500,b,l)
#define WV_INT_TXC_EXIT(b,l)         wvEvent(1501,b,l)
#define WV_INT_TXB_ENTRY(b,l)        wvEvent(1600,b,l)
#define WV_INT_TXB_EXIT(b,l)         wvEvent(1601,b,l)
#define WV_INT_TXF_ENTRY(b,l)        wvEvent(1620,b,l)
#define WV_INT_TXF_EXIT(b,l)         wvEvent(1621,b,l)
#define WV_INT_TXE_ENTRY(b,l)        wvEvent(1610,b,l)
#define WV_INT_TXE_EXIT(b,l)         wvEvent(1611,b,l)
#define WV_INT_GRA_ENTRY(b,l)        wvEvent(1700,b,l)
#define WV_INT_GRA_EXIT(b,l)         wvEvent(1701,b,l)
#define WV_INT_NETJOBADD_ENTRY(b,l)  wvEvent(1800,b,l)
#define WV_INT_NETJOBADD_EXIT(b,l)   wvEvent(1801,b,l)
#define WV_HANDLER_ENTRY(b,l)        wvEvent(2000,b,l)
#define WV_HANDLER_EXIT(b,l)         wvEvent(2001,b,l)
#define WV_MUX_TX_RESTART_ENTRY(b,l) wvEvent(2100,b,l)
#define WV_MUX_TX_RESTART_EXIT(b,l)  wvEvent(2101,b,l)
#define WV_MUX_ERROR_ENTRY(b,l)      wvEvent(2200,b,l)
#define WV_MUX_ERROR_EXIT(b,l)       wvEvent(2201,b,l)
#define WV_SEND_ENTRY(b,l)           wvEvent(5000,b,l)
#define WV_SEND_EXIT(b,l)            wvEvent(5001,b,l)
#define WV_RECV_ENTRY(b,l)           wvEvent(6000,b,l)
#define WV_RECV_EXIT(b,l)            wvEvent(6001,b,l)
#define WV_CACHE_FLUSH_ENTRY(b,l)    wvEvent(8000,b,l)
#define WV_CACHE_FLUSH_EXIT(b,l)     wvEvent(8001,b,l)
#define WV_CACHE_INVAL_ENTRY(b,l)    wvEvent(8100,b,l)
#define WV_CACHE_INVAL_EXIT(b,l)     wvEvent(8101,b,l)
#define WV_INT_BABR_ENTRY(b,l)       wvEvent(1330,b,l)
#define WV_INT_BABR_EXIT(b,l)        wvEvent(1331,b,l)
#define WV_INT_EBERR_ENTRY(b,l)      wvEvent(1340,b,l)
#define WV_INT_EBERR_EXIT(b,l)       wvEvent(1341,b,l)
#define WV_INT_MSRO_ENTRY(b,l)       wvEvent(1350,b,l)
#define WV_INT_MSRO_EXIT(b,l)        wvEvent(1351,b,l)
#define WV_INT_BABT_ENTRY(b,l)       wvEvent(1360,b,l)
#define WV_INT_BABT_EXIT(b,l)        wvEvent(1361,b,l)
#define WV_INT_LC_ENTRY(b,l)         wvEvent(1370,b,l)
#define WV_INT_LC_EXIT(b,l)          wvEvent(1371,b,l)
#define WV_INT_CRL_ENTRY(b,l)        wvEvent(1380,b,l)
#define WV_INT_CRL_EXIT(b,l)         wvEvent(1381,b,l)
#define WV_INT_XFUN_ENTRY(b,l)       wvEvent(1390,b,l)
#define WV_INT_XFUN_EXIT(b,l)        wvEvent(1391,b,l)
#define WV_INT_GRSC_ENTRY(b,l)       wvEvent(13a0,b,l)
#define WV_INT_GRSC_EXIT(b,l)        wvEvent(13a1,b,l)
#else
#define WV_INT_ENTRY(b,l)
#define WV_INT_EXIT(b,l)
#define WV_INT_RXB_ENTRY(b,l)
#define WV_INT_RXB_EXIT(b,l)
#define WV_INT_RXF_ENTRY(b,l)
#define WV_INT_BSY_ENTRY(b,l)
#define WV_INT_BSY_EXIT(b,l)
#define WV_INT_RX_EXIT(b,l)
#define WV_INT_RXC_ENTRY(b,l)
#define WV_INT_RXC_EXIT(b,l)
#define WV_INT_TXC_ENTRY(b,l)
#define WV_INT_TXC_EXIT(b,l)
#define WV_INT_TXF_ENTRY(b,l)
#define WV_INT_TXF_EXIT(b,l)
#define WV_INT_TXB_ENTRY(b,l)
#define WV_INT_TXB_EXIT(b,l)
#define WV_INT_TXE_ENTRY(b,l)
#define WV_INT_TXE_EXIT(b,l)
#define WV_INT_GRA_ENTRY(b,l)
#define WV_INT_GRA_EXIT(b,l)
#define WV_INT_BABR_ENTRY(b,l)
#define WV_INT_BABR_EXIT(b,l)
#define WV_INT_EBERR_ENTRY(b,l)
#define WV_INT_EBERR_EXIT(b,l)
#define WV_INT_MSRO_ENTRY(b,l)
#define WV_INT_MSRO_EXIT(b,l)
#define WV_INT_BABT_ENTRY(b,l)
#define WV_INT_BABT_EXIT(b,l)
#define WV_INT_LC_ENTRY(b,l)
#define WV_INT_LC_EXIT(b,l)
#define WV_INT_CRL_ENTRY(b,l)
#define WV_INT_CRL_EXIT(b,l)
#define WV_INT_XFUN_ENTRY(b,l)
#define WV_INT_XFUN_EXIT(b,l)
#define WV_INT_GRSC_ENTRY(b,l)
#define WV_INT_GRSC_EXIT(b,l)
#define WV_INT_NETJOBADD_ENTRY(b,l)
#define WV_INT_NETJOBADD_EXIT(b,l)
#define WV_HANDLER_ENTRY(b,l)
#define WV_HANDLER_EXIT(b,l)
#define WV_MUX_TX_RESTART_ENTRY(b,l)
#define WV_MUX_TX_RESTART_EXIT(b,l)
#define WV_MUX_ERROR_ENTRY(b,l)
#define WV_MUX_ERROR_EXIT(b,l)
#define WV_SEND_ENTRY(b,l)
#define WV_SEND_EXIT(b,l)
#define WV_RECV_ENTRY(b,l)
#define WV_RECV_EXIT(b,l)
#define WV_CACHE_FLUSH_ENTRY(b,l)
#define WV_CACHE_FLUSH_EXIT(b,l)
#define WV_CACHE_INVAL_ENTRY(b,l)
#define WV_CACHE_INVAL_EXIT(b,l)
#endif

/* defines */

/* general macros for reading/writing from/to specified locations */

/* Cache and virtual/physical memory related macros */


#define MOT_TSEC_CACHE_INVAL(address, len)                              \
    CACHE_DRV_INVALIDATE (&pDrvCtrl->bufCacheFuncs, (address), (len));


#define MOT_TSEC_CACHE_FLUSH(address, len)                               \
    CACHE_DRV_FLUSH (&pDrvCtrl->bufCacheFuncs, (address), (len));

#ifndef MOT_TSEC_MS_DELAY
#define MOT_TSEC_MS_DELAY(x)   \
    {                          \
    int loop;                  \
    loop = (x);                \
    while (loop--)             \
        sysDelay();            \
    }
#endif							/* MOT_TSEC_MS_DELAY */

/* Flag Macros */

#define MOT_TSEC_FLAG_CLEAR(clearBits)       \
    (pDrvCtrl->flags &= ~(clearBits))

#define MOT_TSEC_FLAG_SET(setBits)           \
    (pDrvCtrl->flags |= (setBits))

#define MOT_TSEC_FLAG_GET()                  \
    (pDrvCtrl->flags)

#define MOT_TSEC_FLAG_ISSET(setBits)         \
    (pDrvCtrl->flags & (setBits))

#define MOT_TSEC_USR_FLAG_ISSET(setBits)     \
    (pDrvCtrl->userFlags & (setBits))

#define END_FLAGS_ISSET(setBits)             \
    ((&pDrvCtrl->endObj)->flags & (setBits))

#ifdef INCLUDE_RFC_2233
#define END_HADDR(pEnd)                                                  \
        ((pEnd).pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.phyAddress)

#define END_HADDR_LEN(pEnd)                                              \
        ((pEnd).pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.addrLength)

#define END_INC_IN_UCAST(mData, mLen)  \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)      \
            {                                       \
            pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                    M2_PACKET_IN,           \
                                    mData,   \
                                    mLen );  \
            }

#define END_INC_IN_NUCAST(mData, mLen)  \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)      \
            {                                       \
            pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                    M2_PACKET_IN,           \
                                    mData,   \
                                    mLen );  \
            }

#define END_INC_IN_ERRS()  \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)  \
            {                                    \
            pDrvCtrl->endObj.pMib2Tbl->m2CtrUpdateRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                     M2_ctrId_ifInErrors, 1);  \
            }

#define END_INC_IN_DISCARDS() \
         if (pDrvCtrl->endObj.pMib2Tbl != NULL)  \
             {                                    \
             pDrvCtrl->endObj.pMib2Tbl->m2CtrUpdateRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                             M2_ctrId_ifInDiscards, 1);  \
             }

#define END_INC_IN_OCTETS(mLen)

#define END_INC_OUT_UCAST(mData, mLen) \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)  \
            {                                    \
            pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                    M2_PACKET_OUT,           \
                                    mData,   \
                                    mLen );  \
            }

#define END_INC_OUT_NUCAST(mData, mLen) \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)  \
            {                                    \
            pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                    M2_PACKET_OUT,           \
                                    mData,   \
                                    mLen );  \
            }


#define END_INC_OUT_ERRS()                         \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)      \
            {                                       \
            pDrvCtrl->endObj.pMib2Tbl->m2CtrUpdateRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                     M2_ctrId_ifOutErrors, 1);  \
            }

#define END_INC_OUT_DISCARDS()                      \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)      \
            {                                       \
            pDrvCtrl->endObj.pMib2Tbl->m2CtrUpdateRtn (pDrvCtrl->endObj.pMib2Tbl, \
	    M2_ctrId_ifOutDiscards, 1);  \
            }

#define END_INC_OUT_OCTETS(mLen)

#else

#define END_HADDR(pEnd)                                             \
        ((pEnd).mib2Tbl.ifPhysAddress.phyAddress)

#define END_HADDR_LEN(pEnd)                                         \
        ((pEnd).mib2Tbl.ifPhysAddress.addrLength)

#ifdef  INCLUDE_RFC_1213_OLD
#define END_INC_IN_ERRS()                                              \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_IN_ERRS, +1)
#define END_INC_IN_DISCARDS()                                          \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_IN_ERRS, +1)
#define END_INC_IN_UCAST(mData, mLen)                                  \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_IN_UCAST, +1)

#define END_INC_IN_NUCAST(mData, mLen)                                 \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_IN_UCAST, +1)
#define END_INC_IN_OCTETS(mLen)

#define END_INC_OUT_ERRS()                                             \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_OUT_ERRS, +1)

#define END_INC_OUT_DISCARDS()                                         \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_OUT_ERRS, +1)

#define END_INC_OUT_UCAST(mData, mLen)                                 \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1)
#define END_INC_OUT_NUCAST(mData, mLen)                                \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1)
#define END_INC_OUT_OCTETS(mLen)
#else
#define END_INC_IN_DISCARDS()           (pDrvCtrl->endObj.mib2Tbl.ifInDiscards++)
#define END_INC_IN_ERRS()               (pDrvCtrl->endObj.mib2Tbl.ifInErrors++)
#define END_INC_IN_UCAST(mData, mLen)   (pDrvCtrl->endObj.mib2Tbl.ifInUcastPkts++)
#define END_INC_IN_NUCAST(mData, mLen)  (pDrvCtrl->endObj.mib2Tbl.ifInNUcastPkts++)
#define END_INC_IN_OCTETS(mLen)         (pDrvCtrl->endObj.mib2Tbl.ifInOctets += mLen)

#define END_INC_OUT_DISCARDS()          (pDrvCtrl->endObj.mib2Tbl.ifOutDiscards++)
#define END_INC_OUT_ERRS()              (pDrvCtrl->endObj.mib2Tbl.ifOutErrors++)
#define END_INC_OUT_UCAST(mData, mLen)  (pDrvCtrl->endObj.mib2Tbl.ifOutUcastPkts++)
#define END_INC_OUT_NUCAST(mData, mLen) (pDrvCtrl->endObj.mib2Tbl.ifOutNUcastPkts++)
#define END_INC_OUT_OCTETS(mLen)        (pDrvCtrl->endObj.mib2Tbl.ifOutOctets += mLen)
#endif

#endif							/* INCLUDE_RFC_2233 */

/* locals */

/* Function declarations not in any header files */
IMPORT void   sysUsDelay (UINT32);
/* forward function declarations */

LOCAL STATUS motTsecInitParse( TSEC_DRV_CTRL * pDrvCtrl, char *initString );
LOCAL STATUS motTsecFuncInit( TSEC_DRV_CTRL * );
LOCAL STATUS motTsecParmInit( TSEC_DRV_CTRL * );
LOCAL STATUS motTsecPhyParmInit( TSEC_DRV_CTRL * pDrvCtrl );
LOCAL STATUS motTsecInitMem( TSEC_DRV_CTRL * pDrvCtrl );
LOCAL STATUS motTsecRbdInit( TSEC_DRV_CTRL * pDrvCtrl );
LOCAL STATUS motTsecTbdInit( TSEC_DRV_CTRL * pDrvCtrl );
LOCAL STATUS motTsecPhyPreInit( TSEC_DRV_CTRL * pDrvCtrl );
_WRS_FASTTEXT
LOCAL STATUS motTsecSend( TSEC_DRV_CTRL * pDrvCtrl, M_BLK * pMblk );
LOCAL void motTsecRestart( TSEC_DRV_CTRL * pDrvCtrl );
LOCAL void motTsecGracefulStop( TSEC_DRV_CTRL * pDrvCtrl );
LOCAL STATUS motTsecHashTblPopulate( TSEC_DRV_CTRL * pDrvCtrl );
_WRS_FASTTEXT
LOCAL int motTsecTbdClean( TSEC_DRV_CTRL * pDrvCtrl );
#ifdef USE_NET_JOB_ADD
_WRS_FASTTEXT
LOCAL void      motTsecHandleRXFrames( TSEC_DRV_CTRL *pDrvCtrl );
_WRS_FASTTEXT
LOCAL void	motTsecHandler ( TSEC_DRV_CTRL *pDrvCtrl );
#else
_WRS_FASTTEXT
LOCAL void      motTsecHandleRXFrames( QJOB * pRxJob );
_WRS_FASTTEXT
LOCAL void	motTsecHandler ( QJOB * pTxJob );
#endif
LOCAL STATUS motTsecMiiPhyRead(
	TSEC_DRV_CTRL * pDrvCtrl,
	UINT8 phyAddr,
	UINT8 regAddr,
	UINT16 * retVal
);
LOCAL STATUS motTsecMiiPhyWrite(
	TSEC_DRV_CTRL * pDrvCtrl,
	UINT8 phyAddr,
	UINT8 regAddr,
	UINT16 writeData
);
LOCAL STATUS motTsecAddrSet( TSEC_DRV_CTRL * pDrvCtrl, char *pAddr );
LOCAL void motTsecPhyLSCInt( TSEC_DRV_CTRL * pDrvCtrl );
LOCAL STATUS motTsecPktTransmit( TSEC_DRV_CTRL *, M_BLK *, int );

LOCAL void motTsecHandleLSCJob( TSEC_DRV_CTRL * );
_WRS_FASTTEXT
LOCAL void motTsecTxInt( TSEC_DRV_CTRL * pDrvCtrl );
_WRS_FASTTEXT
LOCAL void motTsecRxInt( TSEC_DRV_CTRL * pDrvCtrl );

/* END Specific interfaces. */

END_OBJ *motTsecEndLoad( char *initString );
STATUS motTsecStart( TSEC_DRV_CTRL * pDrvCtrl );
LOCAL STATUS motTsecUnload( TSEC_DRV_CTRL * pDrvCtrl );
LOCAL STATUS motTsecStop( TSEC_DRV_CTRL * pDrvCtrl );
LOCAL STATUS motTsecIoctl( void *pCookie, int cmd, caddr_t data );
LOCAL STATUS motTsecMCastAddrAdd( TSEC_DRV_CTRL * pDrvCtrl, UCHAR * pAddress );
LOCAL STATUS motTsecMCastAddrDel( TSEC_DRV_CTRL * pDrvCtrl, UCHAR * pAddress );
LOCAL STATUS motTsecMCastAddrGet( TSEC_DRV_CTRL * pDrvCtrl, MULTI_TABLE * pTable );

LOCAL STATUS motTsecPollSend( TSEC_DRV_CTRL * pDrvCtrl, M_BLK_ID pMblk );
LOCAL STATUS motTsecPollReceive( TSEC_DRV_CTRL * pDrvCtrl, M_BLK_ID pMblk );
LOCAL STATUS motTsecPollStart( TSEC_DRV_CTRL * pDrvCtrl );
LOCAL STATUS motTsecPollStop( TSEC_DRV_CTRL * pDrvCtrl );


LOCAL UINT32 motTsecInumToIvec(TSEC_DRV_CTRL *pDrvCtrl,UINT32 num)
    {
	if (pDrvCtrl->inumToIvec == NULL)
	    return (num);
        else
            return(pDrvCtrl->inumToIvec(num));
    };

#define MOT_TSEC_INUM_TO_IVEC(x,y) motTsecInumToIvec(x,y)


/* globals */
IMPORT POOL_FUNC *_pLinkPoolFuncTbl;
#if (_WRS_VXWORKS_MAJOR == 6 && _WRS_VXWORKS_MINOR < 4 ) || _WRS_VXWORKS_MAJOR < 6
  IMPORT FUNCPTR ffsMsb;
#endif /* VxW >= 6.6 */
/* If there are at least this many fragments in TX packet, then coalesce */

#define MOT_TSEC_FRAG_NUM_LIMIT	16


#ifdef MOT_TSEC_DBG

void motTsecMiiShow( int, int, int );
void motTsecMibShow( int );
void motTsecRegsShow( int );

#define MOT_TSEC_DBG_OFF        0x0000
#define MOT_TSEC_DBG_RX         0x0001
#define MOT_TSEC_DBG_TX         0x0002
#define MOT_TSEC_DBG_POLL       0x0004
#define MOT_TSEC_DBG_MII        0x0008
#define MOT_TSEC_DBG_LOAD       0x0010
#define MOT_TSEC_DBG_IOCTL      0x0020
#define MOT_TSEC_DBG_INT        0x0040
#define MOT_TSEC_DBG_START      0x0080
#define MOT_TSEC_DBG_INT_RX_ERR 0x0100
#define MOT_TSEC_DBG_INT_TX_ERR 0x0200
#define MOT_TSEC_DBG_RX_ERR     0x0400
#define MOT_TSEC_DBG_TX_ERR     0x0800
#define MOT_TSEC_DBG_TRACE      0x1000
#define MOT_TSEC_DBG_TRACE_RX   0x2000
#define MOT_TSEC_DBG_TRACE_TX   0x4000
#define MOT_TSEC_DBG_MONITOR    0x8000
#define MOT_TSEC_DBG_ANY        0xffff
#define MOT_TSEC_DBG_REG      0x10000


#define MOT_TSEC_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)            \
    {                                   \
    if (motTsecEndDbg & FLG)                   \
        logMsg (X0, X1, X2, X3, X4, X5, X6);                    \
    }

#define MOT_TSEC_STAT_INCR(i) (i++)

FUNCPTR _func_netJobAdd;
FUNCPTR _func_txRestart;
FUNCPTR _func_error;

/* global debug level flag */

UINT32 motTsecEndDbg =   MOT_TSEC_DBG_OFF;

TSEC_DRV_CTRL *pTsecDrvCtrlDbg[MOT_TSEC_MAX_DEVS];

#else							/* MOT_TSEC_DBG */
#define MOT_TSEC_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)
#define MOT_TSEC_STAT_INCR(i)
#endif

#ifdef MOT_TSEC_DBG
LOCAL const char *speedStr[4] = { "NA", "10", "100", "1GIG" };
LOCAL const char *duplexStr[3] = { "NA", "Half", "Full" };
LOCAL const char *linkStr[2] = { "Down", "Up" };
#endif

/* LOCAL const char *pErrorStr   = "No Clusters"; */
LOCAL const char *pIfDescrStr = "Motorola TSEC";

/*
 * Define the device function table.  This is static across all driver
 * instances.
 */

LOCAL NET_FUNCS netTsecFuncs = {
	(FUNCPTR) motTsecStart,		/* start func. */
	(FUNCPTR) motTsecStop,		/* stop func. */
	(FUNCPTR) motTsecUnload,	/* unload func. */
	(FUNCPTR) motTsecIoctl,		/* ioctl func. */
	(FUNCPTR) motTsecSend,		/* send func. */
	(FUNCPTR) motTsecMCastAddrAdd,	/* multicast add func. */
	(FUNCPTR) motTsecMCastAddrDel,	/* multicast delete func. */
	(FUNCPTR) motTsecMCastAddrGet,	/* multicast get fun. */
	(FUNCPTR) motTsecPollSend,	/* polling send func. */
	(FUNCPTR) motTsecPollReceive,	/* polling receive func. */
	endEtherAddressForm,		/* put address info into a NET_BUFFER */
	(FUNCPTR) endEtherPacketDataGet,	/* get pointer to data in NET_BUFFER */
	(FUNCPTR) endEtherPacketAddrGet	/* Get packet addresses */
};

/*******************************************************************************
*
* motTsecEndLoad - initialize the driver and device
*
* This routine initializes both driver and device to an operational state
* using device specific parameters specified by <initString>.
*
* The parameter string, <initString>, is an ordered list of parameters each
* separated by a colon. The format of <initString> is:
*
* "<CCSVal>:<ivec>:<bufBase>:<bufSize>:<fifoTxBase>:<fifoRxBase>
* :<tbdNum>:<rbdNum>:<phyAddr>:<phyDefMode>:<pAnOrderTbl>:<userFlags>
*
*
* The TSEC shares a region of memory with the driver.  The caller of this
* routine can specify the address of this memory region, or can specify that
* the driver must obtain this memory region from the system resources.
*
* A default number of transmit/receive buffer descriptors of 32 can be
* selected by passing zero in the parameters <tbdNum> and <rbdNum>.
* In other cases, the number of buffers selected should be greater than two.
*
* The <bufBase> parameter is used to inform the driver about the shared
* memory region.  If this parameter is set to the constant "NONE," then this
* routine will attempt to allocate the shared memory from the system.  Any
* other value for this parameter is interpreted by this routine as the address
* of the shared memory region to be used. The <bufSize> parameter is used
* to check that this region is large enough with respect to the provided
* values of both transmit/receive buffer descriptors.
*
* If the caller provides the shared memory region, then the driver assumes
* that this region does not require cache coherency operations, nor does it
* require conversions between virtual and physical addresses.
*
* If the caller indicates that this routine must allocate the shared memory
* region, then this routine will use cacheDmaMalloc() to obtain
* some  cache-safe memory.  The attributes of this memory will be checked,
* and if the memory is not write coherent, this routine will abort and
* return NULL.
*
* RETURNS: an END object pointer, or NULL on error.
*
* ERRNO
*
* SEE ALSO: ifLib,
* \tb MPC8260 Power QUICC II User's Manual
*/


END_OBJ *motTsecEndLoad(
	char *initString
) {
	TSEC_DRV_CTRL *pDrvCtrl = NULL;	/* pointer to TSEC_DRV_CTRL structure */

	if (initString == NULL)
		return NULL;

	if (initString[0] == 0) {
		bcopy((char *) MOT_TSEC_DEV_NAME, (void *) initString,
			  MOT_TSEC_DEV_NAME_LEN);
		return NULL;
	}

	/* allocate the device structure */

	pDrvCtrl = (TSEC_DRV_CTRL *) calloc(sizeof(TSEC_DRV_CTRL), 1);
	if (pDrvCtrl == NULL)
		return NULL;

	/* get memory for the phyInfo structure */

	pDrvCtrl->phyInfo = calloc(sizeof(PHY_INFO), 1);
	if (pDrvCtrl->phyInfo == NULL)
		return NULL;

	/* set up function pointers */
#ifdef USE_NET_JOB_ADD
	pDrvCtrl->netJobAdd = (FUNCPTR) netJobAdd;
#endif
	pDrvCtrl->muxTxRestart = (FUNCPTR) muxTxRestart;
	pDrvCtrl->muxError = (FUNCPTR) muxError;

#ifndef USE_NET_JOB_ADD
   /* Initialize job queue stuff */
  #if ( ( _WRS_VXWORKS_MAJOR == 6 ) && ( _WRS_VXWORKS_MINOR >= 6 ) )
    pDrvCtrl->jobQueueId = netJobQueueId;
  #else /* VxW >=6.6 */
    pDrvCtrl->jobQueueId = &netJobInfo;
  #endif /* VxW >= 6.6 */
    pDrvCtrl->txJob.priInfo = NET_TASK_QJOB_PRI;
    pDrvCtrl->txJob.func = (QJOB_FUNC)motTsecHandler;
    pDrvCtrl->rxJob.priInfo = NET_TASK_QJOB_PRI;
    pDrvCtrl->rxJob.func = (QJOB_FUNC)motTsecHandleRXFrames;
#endif

	/* Parse InitString */

	if (motTsecInitParse(pDrvCtrl, initString) == ERROR)
		goto errorExit;

	/* sanity check the unit number */

	if (pDrvCtrl->unit >= MOT_TSEC_MAX_DEVS)
		goto errorExit;

#ifndef MEN_EP04
	/* one MII interface for all PHYs */
	pDrvCtrl->tsecMiiPtr =
		(TSEC_REG_T *) ((UINT32) pDrvCtrl->tsecRegsPtr +
						(UINT32) MOT_TSEC_ADRS_OFFSET_1);
#endif /* MEN_EP04 */

	/* adjust the CCSBAR */
	if (pDrvCtrl->tsecNum == MOT_TSEC_DEV_1)
		pDrvCtrl->tsecRegsPtr = (TSEC_REG_T *) ((UINT32) pDrvCtrl->tsecRegsPtr +
							(UINT32) MOT_TSEC_ADRS_OFFSET_1);
	else if (pDrvCtrl->tsecNum == MOT_TSEC_DEV_2)
		pDrvCtrl->tsecRegsPtr = (TSEC_REG_T *) ((UINT32) pDrvCtrl->tsecRegsPtr +
							(UINT32) MOT_TSEC_ADRS_OFFSET_2);
	else if (pDrvCtrl->tsecNum == MOT_TSEC_DEV_3)
		pDrvCtrl->tsecRegsPtr = (TSEC_REG_T *) ((UINT32) pDrvCtrl->tsecRegsPtr +
							(UINT32) MOT_TSEC_ADRS_OFFSET_3);
	else if (pDrvCtrl->tsecNum == MOT_TSEC_DEV_4)
		pDrvCtrl->tsecRegsPtr = (TSEC_REG_T *) ((UINT32) pDrvCtrl->tsecRegsPtr +
							(UINT32) MOT_TSEC_ADRS_OFFSET_4);
	else
		return (NULL);

#ifdef MEN_EP04
	/* there are different MII interfaces for each PHY */
	pDrvCtrl->tsecMiiPtr = (TSEC_REG_T *) pDrvCtrl->tsecRegsPtr;
#endif /* MEN_EP04 */

#ifdef MOT_TSEC_DBG
	pTsecDrvCtrlDbg[pDrvCtrl->tsecNum] = pDrvCtrl;

	/* support unit test */

	_func_netJobAdd = (FUNCPTR) netJobAdd;
	_func_txRestart = (FUNCPTR) muxTxRestart;
	_func_error = (FUNCPTR) muxError;
#endif							/* MOT_TSEC_DBG */


	/* Init BSP function and driver callbacks */
	if (motTsecFuncInit(pDrvCtrl) == ERROR)
		goto errorExit;

	/* Init PHY parameters */

	if (motTsecPhyParmInit(pDrvCtrl) == ERROR)
		goto errorExit;

	/* set the MAC address in the mib interface */
	bcopy((char *) pDrvCtrl->enetAddr.ether_addr_octet,
		  (char *) END_HADDR(pDrvCtrl->endObj),
		  END_HADDR_LEN(pDrvCtrl->endObj));

	/* memory initialization */

	if (motTsecInitMem(pDrvCtrl) == ERROR)
		goto errorExit;


	/* Init TSEC hardware parameters */

	if (motTsecParmInit(pDrvCtrl) == ERROR)
		goto errorExit;

	/* endObj initializations */

	if (END_OBJ_INIT(&pDrvCtrl->endObj, (DEV_OBJ *) pDrvCtrl,
					 MOT_TSEC_DEV_NAME, pDrvCtrl->unit, &netTsecFuncs,
					 (char *) pIfDescrStr)
		== ERROR)
		goto errorExit;

#ifdef INCLUDE_RFC_2233

	/* Initialize MIB-II entries (for RFC 2233 ifXTable) */
	pDrvCtrl->endObj.pMib2Tbl =
		m2IfAlloc(M2_ifType_ethernet_csmacd,
				  (UINT8 *) pDrvCtrl->enetAddr.ether_addr_octet, 6,
				  ETHERMTU, pDrvCtrl->phyInfo->phySpeed,
				  MOT_TSEC_DEV_NAME, pDrvCtrl->unit);

	if (pDrvCtrl->endObj.pMib2Tbl == NULL) {
		logMsg("%s%d - MIB-II initializations failed\n",
			   (int) MOT_TSEC_DEV_NAME, pDrvCtrl->unit, 0, 0, 0, 0);
		goto errorExit;
	}

	/*
	 * Set the RFC2233 flag bit in the END object flags field and
	 * install the counter update routines.
	 */

	m2IfPktCountRtnInstall(pDrvCtrl->endObj.pMib2Tbl, m2If8023PacketCount);

	/*
	 * Make a copy of the data in mib2Tbl struct as well. We do this
	 * mainly for backward compatibility issues. There might be some
	 * code that might be referencing the END pointer and might
	 * possibly do lookups on the mib2Tbl, which will cause all sorts
	 * of problems.
	 */

	bcopy((char *) &pDrvCtrl->endObj.pMib2Tbl->m2Data.mibIfTbl,
		  (char *) &pDrvCtrl->endObj.mib2Tbl, sizeof(M2_INTERFACETBL));

	/* Mark the device ready */

	END_OBJ_READY(&pDrvCtrl->endObj,
				  IFF_NOTRAILERS | IFF_MULTICAST | IFF_BROADCAST |
				  END_MIB_2233);


#else

	/* Old RFC 1213 mib2 interface */

	if (END_MIB_INIT(&pDrvCtrl->endObj, M2_ifType_ethernet_csmacd,
					 (u_char *) & pDrvCtrl->enetAddr.ether_addr_octet[0], 6,
					 ETHERMTU, pDrvCtrl->phyInfo->phySpeed) == ERROR)
		goto errorExit;

	/* Mark the device ready */

	END_OBJ_READY(&pDrvCtrl->endObj,
				  IFF_NOTRAILERS | IFF_MULTICAST | IFF_BROADCAST);



#endif							/* INCLUDE_RFC_2233 */

	MOT_TSEC_LOG(MOT_TSEC_DBG_ANY, "MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
				 pDrvCtrl->enetAddr.ether_addr_octet[0],
				 pDrvCtrl->enetAddr.ether_addr_octet[1],
				 pDrvCtrl->enetAddr.ether_addr_octet[2],
				 pDrvCtrl->enetAddr.ether_addr_octet[3],
				 pDrvCtrl->enetAddr.ether_addr_octet[4],
				 pDrvCtrl->enetAddr.ether_addr_octet[5]
		);

	/* connect the interrupt handler */

	intConnect ((VOIDFUNCPTR *)((int)MOT_TSEC_INUM_TO_IVEC(pDrvCtrl,pDrvCtrl->inum_tsecTx)),  (VOIDFUNCPTR)motTsecTxInt, (int)pDrvCtrl);
	intConnect ((VOIDFUNCPTR *)((int)MOT_TSEC_INUM_TO_IVEC(pDrvCtrl,pDrvCtrl->inum_tsecRx)),  (VOIDFUNCPTR)motTsecRxInt, (int)pDrvCtrl);
	intConnect ((VOIDFUNCPTR *)((int)MOT_TSEC_INUM_TO_IVEC(pDrvCtrl,pDrvCtrl->inum_tsecErr)), (VOIDFUNCPTR)motTsecTxInt, (int)pDrvCtrl);


	pDrvCtrl->state = MOT_TSEC_STATE_LOADED;

	return (&pDrvCtrl->endObj);

  errorExit:
	motTsecUnload(pDrvCtrl);
	return NULL;
}

/*******************************************************************************
*
* motTsecUnload - unload a driver from the system
*
* This routine unloads the driver pointed to by <pDrvCtrl> from the system.
*
* RETURNS: OK, always.
*
* ERRNO
*
* SEE ALSO: motTsecLoad()
*/
LOCAL STATUS motTsecUnload(
	TSEC_DRV_CTRL * pDrvCtrl	/* pointer to TSEC_DRV_CTRL structure */
) {

	if (pDrvCtrl == NULL)
		return (ERROR);

	if ((pDrvCtrl->state & MOT_TSEC_STATE_LOADED) == MOT_TSEC_STATE_NOT_LOADED)
		return (ERROR);

	/* must stop the device before unloading it */

	if ((pDrvCtrl->state & MOT_TSEC_STATE_RUNNING) == MOT_TSEC_STATE_RUNNING)
		motTsecStop(pDrvCtrl);

	/* free allocated memory if necessary */

	if ((MOT_TSEC_FLAG_ISSET(MOT_TSEC_OWN_BUF_MEM)) &&
		(pDrvCtrl->pBufBase != NULL)) {
		free(pDrvCtrl->pBufBase);
	}

	if ((MOT_TSEC_FLAG_ISSET(MOT_TSEC_OWN_BD_MEM)) &&
		(pDrvCtrl->pBdBase != NULL)) {
		cacheDmaFree(pDrvCtrl->pBdBase);
	}

	/* free allocated memory if necessary */

	if ((pDrvCtrl->pMBlkArea) != NULL)
		free(pDrvCtrl->pMBlkArea);

#ifdef INCLUDE_RFC_2233
	/* Free MIB-II entries */

	m2IfFree(pDrvCtrl->endObj.pMib2Tbl);

	pDrvCtrl->endObj.pMib2Tbl = NULL;

#endif							/* INCLUDE_RFC_2233 */

	/* free misc resources */

	if (pDrvCtrl->pMblkList)
		free(pDrvCtrl->pMblkList);

	pDrvCtrl->pMblkList = NULL;

	if (pDrvCtrl->tBufList)
		free(pDrvCtrl->tBufList);

	pDrvCtrl->tBufList = NULL;

	END_OBJECT_UNLOAD(&pDrvCtrl->endObj);

	if ((char *) pDrvCtrl->phyInfo != NULL)
		cfree((char *) pDrvCtrl->phyInfo);

	pDrvCtrl->state = MOT_TSEC_STATE_INIT;

#ifdef MOT_TSEC_DBG
	pTsecDrvCtrlDbg[pDrvCtrl->unit] = NULL;

	free(pDrvCtrl->stats);
#endif							/* MOT_TSEC_DBG */

	/* free driver control structure */
	free(pDrvCtrl);

	return (OK);
}

/*******************************************************************************
*
* motTsecInitParse - parse parameter values from initString
*
* This routine parses parameter values from initString and stores them in
* the related fields of the driver control structure.
*
* RETURNS: OK or ERROR
*
* ERRNO
*/
LOCAL STATUS motTsecInitParse(
	TSEC_DRV_CTRL * pDrvCtrl,	/* pointer to TSEC_DRV_CTRL structure */
	char *initString			/* parameter string */
) {
	char *tok;					/* an initString token */
	char *holder = NULL;		/* points to initString fragment beyond tok */
	UINT32 i;

	/* unit number */

	tok = strtok_r(initString, ":", &holder);
	if (tok == NULL)
		return ERROR;
	pDrvCtrl->unit = (int) strtoul(tok, NULL, 16);

	/* Device Address */

	tok = strtok_r(NULL, ":", &holder);
	if (tok == NULL)
		return ERROR;
	pDrvCtrl->tsecRegsPtr = (TSEC_REG_T *) strtoul(tok, NULL, 16);

	/* tsec number */

	tok = strtok_r(NULL, ":", &holder);
	if (tok == NULL)
		return ERROR;
	pDrvCtrl->tsecNum = (int) strtoul(tok, NULL, 16);

	/* MAC Address xx-xx-xx-xx-xx-xx: */

	for (i = 0; i < sizeof(pDrvCtrl->enetAddr.ether_addr_octet) - 1; i++) {
		tok = strtok_r(NULL, "-", &holder);
		if (tok == NULL)
			return ERROR;

		pDrvCtrl->enetAddr.ether_addr_octet[i] = (int) strtoul(tok, NULL, 16);
	}

	tok = strtok_r(NULL, ":", &holder);
	if (tok == NULL)
		return ERROR;
	pDrvCtrl->enetAddr.ether_addr_octet[i] = (int) strtoul(tok, NULL, 16);

	/* USR Init FLAGS */
	tok = strtok_r(NULL, ":", &holder);
	if (tok == NULL)
		return ERROR;
	pDrvCtrl->userFlags = (UINT32) strtoul(tok, NULL, 16);

	/* Phy init parameters */
	tok = strtok_r(NULL, ":", &holder);
	if (tok == NULL)
		return ERROR;
	pDrvCtrl->phyInit = (MOT_TSEC_PHY_PARAMS *) strtoul(tok, NULL, 16);

	/* call back function table */
	tok = strtok_r(NULL, ":", &holder);
	if (tok == NULL)
		return ERROR;
	pDrvCtrl->initFuncs = (MOT_TSEC_FUNC_TABLE *) strtoul(tok, NULL, 16);

	/* Init parameters */
	tok = strtok_r(NULL, ":", &holder);
	if (tok == NULL)
		return ERROR;
	pDrvCtrl->initParms = (MOT_TSEC_PARAMS *) strtoul(tok, NULL, 16);

	tok = strtok_r(NULL, ":", &holder);
	if (tok == NULL)
		return ERROR;

	/* Extended init parameters */
	pDrvCtrl->initParmsExt = (MOT_TSEC_EXT_PARAMS *) strtoul(tok, NULL, 16);

	/* passing maxRxFrames is optional. The default is 128  */
	pDrvCtrl->maxRxFrames = pDrvCtrl->rbdNum * 10;
	tok = strtok_r(NULL, ":", &holder);
	if ((tok != NULL) && (tok != (char *) -1))
		pDrvCtrl->maxRxFrames = strtoul(tok, NULL, 16);

    /* Interrupt Controller info */
    pDrvCtrl->intCtrl = (MOT_TSEC_INT_CTRL *)NULL;
    tok = strtok_r (NULL, ":", &holder);
    if ((tok != NULL) && (tok != (char *)-1))
		pDrvCtrl->intCtrl = (MOT_TSEC_INT_CTRL *) strtoul (tok, NULL, 16);

	return (OK);
}

/*******************************************************************************
*
* motTsecFuncInit - parse functions from initString
*
* This routine parses the function pointers from initString.
*
* RETURNS: OK or ERROR
*
* ERRNO
*/
LOCAL STATUS motTsecFuncInit(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	if (pDrvCtrl->initFuncs != NULL) {
		/* set up BSP call backs passed in the initString */
		pDrvCtrl->phyInitFunc = pDrvCtrl->initFuncs->miiPhyInit;
		pDrvCtrl->phyStatusFunc = pDrvCtrl->initFuncs->miiPhyStatusGet;
		pDrvCtrl->enetEnable = pDrvCtrl->initFuncs->enetEnable;
		pDrvCtrl->enetDisable = pDrvCtrl->initFuncs->enetDisable;
		pDrvCtrl->enetAddrGet = pDrvCtrl->initFuncs->enetAddrGet;
		pDrvCtrl->enetAddrSet = pDrvCtrl->initFuncs->enetAddrSet;
		pDrvCtrl->extWriteL2AllocFunc =
			pDrvCtrl->initFuncs->extWriteL2AllocFunc;

		/* BSP call backs to driver */
		pDrvCtrl->initFuncs->miiPhyInt = (FUNCPTR) motTsecPhyLSCInt;
		pDrvCtrl->initFuncs->miiPhyRead = (FUNCPTR) motTsecMiiPhyRead;
		pDrvCtrl->initFuncs->miiPhyWrite = (FUNCPTR) motTsecMiiPhyWrite;
	}
	else {
		/* use default funcs */
		pDrvCtrl->phyInitFunc = NULL;
		pDrvCtrl->phyStatusFunc = NULL;
		pDrvCtrl->enetEnable = NULL;
		pDrvCtrl->enetDisable = NULL;
		pDrvCtrl->enetAddrGet = NULL;
		pDrvCtrl->enetAddrSet = NULL;
		pDrvCtrl->extWriteL2AllocFunc = NULL;
	}

	if (pDrvCtrl->intCtrl != NULL) {
		pDrvCtrl->inum_tsecTx = pDrvCtrl->intCtrl->inum_tsecTx;
		pDrvCtrl->inum_tsecRx = pDrvCtrl->intCtrl->inum_tsecRx;
		pDrvCtrl->inum_tsecErr = pDrvCtrl->intCtrl->inum_tsecErr;
		pDrvCtrl->inumToIvec = pDrvCtrl->intCtrl->inumToIvec;
		pDrvCtrl->ivecToInum = pDrvCtrl->intCtrl->ivecToInum;
	} else {
		/* Default to 8540 TSEC - Assumes unit 0, 1 & 2 are TSEC 1, 2 & 3
		 * respectively.
		 */
		pDrvCtrl->inumToIvec = NULL;
		pDrvCtrl->ivecToInum = NULL;
		if (pDrvCtrl->unit == 0) {
			pDrvCtrl->inum_tsecTx  = EPIC_TSEC1TX_INT_VEC;
			pDrvCtrl->inum_tsecRx  = EPIC_TSEC1RX_INT_VEC;
			pDrvCtrl->inum_tsecErr = EPIC_TSEC1ERR_INT_VEC;
		}
		else if (pDrvCtrl->unit == 1) {
			pDrvCtrl->inum_tsecTx  = EPIC_TSEC2TX_INT_VEC;
			pDrvCtrl->inum_tsecRx  = EPIC_TSEC2RX_INT_VEC;
			pDrvCtrl->inum_tsecErr = EPIC_TSEC2ERR_INT_VEC;
		}
		else if (pDrvCtrl->unit == 2) {
#ifdef MEN_EM03 /* 8540 FEC */
			pDrvCtrl->inum_tsecTx  = EPIC_FEC_INT_VEC;
			pDrvCtrl->inum_tsecRx  = EPIC_FEC_INT_VEC;
			pDrvCtrl->inum_tsecErr = EPIC_FEC_INT_VEC;
#else
			pDrvCtrl->inum_tsecTx  = EPIC_TSEC3TX_INT_VEC;
			pDrvCtrl->inum_tsecRx  = EPIC_TSEC3RX_INT_VEC;
			pDrvCtrl->inum_tsecErr = EPIC_TSEC3ERR_INT_VEC;
#endif
		}
		else if (pDrvCtrl->unit == 3) {
			pDrvCtrl->inum_tsecTx  = EPIC_TSEC4TX_INT_VEC;
			pDrvCtrl->inum_tsecRx  = EPIC_TSEC4RX_INT_VEC;
			pDrvCtrl->inum_tsecErr = EPIC_TSEC4ERR_INT_VEC;
		}
		else
			return (ERROR);
	}

	return (OK);
}

/*******************************************************************************
*
* motTsecGracefulStop - Gracefully stops the device
*
* This routine gracefully stops the device so MAC registers can be changed.
*
* RETURNS:
*
* ERRNO
*/
LOCAL void motTsecGracefulStop(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	TSEC_REG_T *tsecReg = pDrvCtrl->tsecRegsPtr;
	int count = 0;
	int intLevel;
	volatile int ieventVal, tstatVal, rstatVal;

	intLevel = intLock();

	tstatVal = MOT_TSEC_TSTAT_REG;
	MOT_TSEC_TSTAT_REG = tstatVal;

	rstatVal = MOT_TSEC_RSTAT_REG;
	MOT_TSEC_RSTAT_REG = rstatVal;

	ieventVal = MOT_TSEC_IEVENT_REG;
	MOT_TSEC_IEVENT_REG = ieventVal;

    /*
     * Mask off all interrupts while perfoming a
     * graceful stop, since we don't want to trigger
     * the ISR from here.
     */

    ieventVal = MOT_TSEC_IMASK_REG;
    MOT_TSEC_IMASK_REG = 0;

	MOT_TSEC_DMACTRL_REG |= (MOT_TSEC_DMACTRL_GTS);

	CACHE_PIPE_FLUSH();

	while (!(MOT_TSEC_IEVENT_REG & (MOT_TSEC_IEVENT_GTSC))) {
		if (count < 100000)
			count++;
		else
			break;
	}

    /* Ack the event. */

    MOT_TSEC_IEVENT_REG = MOT_TSEC_IEVENT_GTSC;

	MOT_TSEC_MACCFG1_REG &= (~(MOT_TSEC_MACCFG1_TX_EN | MOT_TSEC_MACCFG1_RX_EN));
    CACHE_PIPE_FLUSH();

	/* This delay is required by the hardware */
	MOT_TSEC_MS_DELAY(16);

	MOT_TSEC_DMACTRL_REG |= (MOT_TSEC_DMACTRL_GRS);

	count = 0;

	while (!(MOT_TSEC_IEVENT_REG & (MOT_TSEC_IEVENT_GRSC))) {
		if (count < 100000)
			count++;
		else
			break;
	}

    /* Ack the event. */

    MOT_TSEC_IEVENT_REG = MOT_TSEC_IEVENT_GRSC;

	/* This delay is required by the hardware */
	MOT_TSEC_MS_DELAY(32);

	MOT_TSEC_MACCFG1_REG |= (MOT_TSEC_MACCFG1_SOFT_RESET |
							 MOT_TSEC_MACCFG1_RESET_RX_MC |
							 MOT_TSEC_MACCFG1_RESET_TX_MC |
							 MOT_TSEC_MACCFG1_RESET_RX_FUN |
							 MOT_TSEC_MACCFG1_RESET_TX_FUN);

    CACHE_PIPE_FLUSH();
	MOT_TSEC_MS_DELAY(32);

	MOT_TSEC_MACCFG1_REG &= ~(MOT_TSEC_MACCFG1_SOFT_RESET |
							  MOT_TSEC_MACCFG1_RESET_RX_MC |
							  MOT_TSEC_MACCFG1_RESET_TX_MC |
							  MOT_TSEC_MACCFG1_RESET_RX_FUN |
							  MOT_TSEC_MACCFG1_RESET_TX_FUN);
	CACHE_PIPE_FLUSH();
	MOT_TSEC_MS_DELAY(16);

	/* Unmask interrupts. */

	MOT_TSEC_IMASK_REG = ieventVal;

	intUnlock(intLevel);

}

/*******************************************************************************
*
* motTsecRestart - Restarts device after motTsecGracefulStop
*
* This routine restarts the ethernet device after a motTsecGracefulStop event.
*
* RETURNS:
*
* ERRNO
*/
LOCAL void motTsecRestart(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	TSEC_REG_T *tsecReg = pDrvCtrl->tsecRegsPtr;
	volatile int rstatVal, tstatVal, ieventVal;
	int intLevel;

	tstatVal = MOT_TSEC_TSTAT_REG;
	MOT_TSEC_TSTAT_REG = tstatVal;

	rstatVal = MOT_TSEC_RSTAT_REG;
	MOT_TSEC_RSTAT_REG = rstatVal;

	ieventVal = MOT_TSEC_IEVENT_REG;
	MOT_TSEC_IEVENT_REG = ieventVal;
	ieventVal = MOT_TSEC_IEVENT_REG;

	MOT_TSEC_RBASE_REG = (UINT32) pDrvCtrl->pRbdBase;
	MOT_TSEC_TBASE_REG = (UINT32) pDrvCtrl->pTbdBase;

	pDrvCtrl->rbdIndex = 0;
	pDrvCtrl->tbdIndex = 0;

	/* Initialize the transmit buffer descriptors */

	if (motTsecTbdInit(pDrvCtrl) == ERROR) {
		printf("motTsecTbdInit failed in Restart \n");
		/* return ERROR; */
	}

	/* Initialize the receive buffer descriptors */

	if (motTsecRbdInit(pDrvCtrl) == ERROR) {
		printf("motTsecRbdInit failed in Restart \n");
		/* return ERROR; */
	}

	intLevel = intLock();

	/* Enable Transmit and Receive */
	MOT_TSEC_MACCFG1_REG |= (MOT_TSEC_MACCFG1_RX_EN | MOT_TSEC_MACCFG1_TX_EN);

	CACHE_PIPE_FLUSH();
	/* This delay is required by the hardware */
	MOT_TSEC_MS_DELAY(16);

	/* Ensure graceful stop rx and tx isn't set */

	MOT_TSEC_DMACTRL_REG &= (~(MOT_TSEC_DMACTRL_GTS | MOT_TSEC_DMACTRL_GRS));

	MOT_TSEC_MS_DELAY(16);
	intUnlock(intLevel);

}

/***************************************************************************
*
* motTsecParmInit - initializes motTsec environment parameters
*
* This routine initializes the motTsec environment parameters.
*
* RETURNS: OK or ERROR
*
* ERRNO
*/
LOCAL STATUS motTsecParmInit(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	int i;
	UINT32 flags;
	UINT16 phyData;
	UINT32 maccfg1Reg;
	UINT32 maccfg2Reg;
	UINT32 ecntrlReg;
	UINT32 rctrlReg;
	UINT32 tctrlReg;
	UINT32 dmactrlReg;
	UINT32 edisReg;
	UINT32 tstatReg;
	UINT32 txicReg;
	UINT32 rstatReg;
	UINT32 mrblrReg;
	UINT32 ptvReg;
	UINT32 tbipaReg;
	UINT32 fifoTxThrReg;
	UINT32 fifoTxStarveReg;
	UINT32 fifoTxStarveShutoffReg;
	UINT32 minflrReg;
	UINT32 hafdupReg;
	UINT32 ipgifgReg;
	UINT32 maxfrmReg;
	UINT32 ifstatReg;
	UINT32 attrReg;
	UINT32 attreliReg;
	UINT32 miicfgReg;
	UINT32 miicomReg;
    UINT32   rxic;
	UINT32 macIndividualHashReg[8];
	UINT32 macGroupHashReg[8];
	TSEC_REG_T *tsecMiiReg = pDrvCtrl->tsecMiiPtr;

	MOT_TSEC_FRAME_SET(pDrvCtrl);

	/* Set and clear MACCFG1 to perform a soft reset on all modules */
	/* set up TSEC register defaults */
	maccfg1Reg = MOT_TSEC_MACCFG1_DEFAULT;

	ifstatReg = MOT_TSEC_IFSTAT_DEFAULT;
	edisReg = MOT_TSEC_EDIS_DEFAULT;
	dmactrlReg = MOT_TSEC_DMACTRL_DEFAULT;
	tctrlReg = MOT_TSEC_TCTRL_DEFAULT;
	txicReg = MOT_TSEC_TXIC_DEFAULT;
	tstatReg = MOT_TSEC_TSTAT_DEFAULT;
	rstatReg = MOT_TSEC_RSTAT_DEFAULT;
	mrblrReg = MOT_TSEC_MRBLR_DEFAULT;
	ptvReg = MOT_TSEC_PVT_DEFAULT;
	tbipaReg = MOT_TSEC_TBIPA_DEFAULT;
	minflrReg = MOT_TSEC_MINFLR_DEFAULT;
	hafdupReg = MOT_TSEC_HAFDUP_DEFAULT;
	ipgifgReg = MOT_TSEC_IPGIFG_DEFAULT;
	maxfrmReg = MOT_TSEC_MAXFRM_DEFAULT;
	miicfgReg = MOT_TSEC_MIICFG_DEFAULT;
	miicomReg = MOT_TSEC_MIICOM_DEFAULT;
    rxic       = MOT_TSEC_RXIC_DEFAULT;

	if(pDrvCtrl->extWriteL2AllocFunc==NULL) {
		attrReg = MOT_TSEC_ATTR_DEFAULT_NO_L2;
		attreliReg = 0;
	} else {
		attrReg = MOT_TSEC_ATTR_DEFAULT_L2;
		attreliReg = MOT_TSEC_ATTRELI_EL_DEFAULT;
	}

	fifoTxThrReg = MOT_TSEC_FIFO_TX_THR_DEFAULT;
	fifoTxStarveReg = MOT_TSEC_FIFO_TX_STARVE_DEFAULT;
	fifoTxStarveShutoffReg = MOT_TSEC_FIFO_TX_STARVE_OFF_DEFAULT;

	for (i = 0; i < 8; i++) {
		macIndividualHashReg[i] = 0;
		macGroupHashReg[i] = 0;
	}

	/* prepare to set the driver's initial operating mode */
	maccfg2Reg = MOT_TSEC_MACCFG2_DEFAULT;
	ecntrlReg = MOT_TSEC_ECNTRL_DEFAULT;
	rctrlReg = MOT_TSEC_RCTRL_DEFAULT;

	MOT_TSEC_LOG(MOT_TSEC_DBG_START, "TSEC MODE: %d\n",
				 pDrvCtrl->userFlags, 2, 3, 4, 5, 6);

	/* switch ((UINT32)(pDrvCtrl->userFlags)) */
	switch ((UINT32) (pDrvCtrl->userFlags & MOT_TSEC_USR_MODE_MASK)) {
		/* default to the defines in motTsecEnd.h */
	case MOT_TSEC_USR_MODE_DEFAULT:
		break;

	case MOT_TSEC_USR_MODE_TBI:
		ecntrlReg |= MOT_TSEC_ECNTRL_TBIM;
		maccfg2Reg &= ~MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_MASK);

		maccfg2Reg |= MOT_TSEC_MACCFG2_FULL_DUPLEX |
			MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_GMII_TBI);

		break;

	case MOT_TSEC_USR_MODE_RTBI:
		ecntrlReg |= MOT_TSEC_ECNTRL_TBIM | MOT_TSEC_ECNTRL_RPM;
		maccfg2Reg &= ~MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_MASK);

		maccfg2Reg |= MOT_TSEC_MACCFG2_FULL_DUPLEX |
			MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_GMII_TBI);

		break;
	case MOT_TSEC_USR_MODE_MII:
		maccfg2Reg &= ~MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_MASK);
		maccfg2Reg |= MOT_TSEC_MACCFG2_FULL_DUPLEX |
			MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_MII);

		motTsecMiiPhyRead(pDrvCtrl, pDrvCtrl->phyInfo->phyAddr, 0x0, &phyData);
		phyData |= 0x1000;
		motTsecMiiPhyWrite(pDrvCtrl, pDrvCtrl->phyInfo->phyAddr, 0x0, phyData);

		motTsecMiiPhyRead(pDrvCtrl, pDrvCtrl->phyInfo->phyAddr, 0x0, &phyData);

		break;

	case MOT_TSEC_USR_MODE_GMII:
		maccfg2Reg &= MOT_TSEC_MACCFG2_FULL_DUPLEX |
			~MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_MASK);

		maccfg2Reg |= MOT_TSEC_MACCFG2_FULL_DUPLEX |
			MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_GMII_TBI);

		break;

	case MOT_TSEC_USR_MODE_RGMII:
		ecntrlReg |= MOT_TSEC_ECNTRL_RPM;
		maccfg2Reg &= ~MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_MASK);

		maccfg2Reg |= MOT_TSEC_MACCFG2_FULL_DUPLEX |
			MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_GMII_TBI);

		break;

	case MOT_TSEC_USR_MODE_RGMII_10:
		ecntrlReg |= MOT_TSEC_ECNTRL_RPM;
		maccfg2Reg &= ~MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_MASK);

		maccfg2Reg |= MOT_TSEC_MACCFG2_FULL_DUPLEX |
			MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_GMII_TBI);

		break;

	case MOT_TSEC_USR_MODE_RGMII_100:
		ecntrlReg |= MOT_TSEC_ECNTRL_RPM | MOT_TSEC_ECNTRL_R100M;
		maccfg2Reg &= ~MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_MASK);

		maccfg2Reg |= MOT_TSEC_MACCFG2_FULL_DUPLEX |
			MOT_TSEC_MACCFG2_IF_MODE(MOT_TSEC_MACCFG2_IF_MODE_GMII_TBI);

		break;

	default:
		break;
	}
	/* initialize the extended TSEC parameters */
	if (pDrvCtrl->initParmsExt) {
		flags = pDrvCtrl->initParmsExt->usrRegFlags;
		/* Individual MAC hash table */
		if (flags & MOT_TSEC_FLAG_IADDR) {
			macIndividualHashReg[0] =
				pDrvCtrl->initParmsExt->macIndividualHash[0];

			macIndividualHashReg[1] =
				pDrvCtrl->initParmsExt->macIndividualHash[1];

			macIndividualHashReg[2] =
				pDrvCtrl->initParmsExt->macIndividualHash[2];

			macIndividualHashReg[3] =
				pDrvCtrl->initParmsExt->macIndividualHash[3];

			macIndividualHashReg[4] =
				pDrvCtrl->initParmsExt->macIndividualHash[4];

			macIndividualHashReg[5] =
				pDrvCtrl->initParmsExt->macIndividualHash[5];

			macIndividualHashReg[6] =
				pDrvCtrl->initParmsExt->macIndividualHash[6];

			macIndividualHashReg[7] =
				pDrvCtrl->initParmsExt->macIndividualHash[7];
		}

		/* Group MAC hash table */

		if (flags & MOT_TSEC_FLAG_GADDR) {
			macGroupHashReg[0] = pDrvCtrl->initParmsExt->macGroupHash[0];
			macGroupHashReg[1] = pDrvCtrl->initParmsExt->macGroupHash[1];
			macGroupHashReg[2] = pDrvCtrl->initParmsExt->macGroupHash[2];
			macGroupHashReg[3] = pDrvCtrl->initParmsExt->macGroupHash[3];
			macGroupHashReg[4] = pDrvCtrl->initParmsExt->macGroupHash[4];
			macGroupHashReg[5] = pDrvCtrl->initParmsExt->macGroupHash[5];
			macGroupHashReg[6] = pDrvCtrl->initParmsExt->macGroupHash[6];
			macGroupHashReg[7] = pDrvCtrl->initParmsExt->macGroupHash[7];
		}
	}


	/* write all configured registers */
	MOT_TSEC_MACCFG2_REG = maccfg2Reg;
    CACHE_PIPE_FLUSH();

	MOT_TSEC_ECNTRL_REG = ecntrlReg;

	/* individual MAC hash table */

	for (i = 0; i < 8; i++)
		MOT_TSEC_IADDR_REG[i] = macIndividualHashReg[i];

	/* Group MAC hash table */

	for (i = 0; i < 8; i++)
		MOT_TSEC_GADDR_REG[i] = macGroupHashReg[i];

	/* set the  ethernet address in the TSEC */
	motTsecAddrSet(pDrvCtrl, (char *) &pDrvCtrl->enetAddr);

	MOT_TSEC_RCTRL_REG = rctrlReg;

	MOT_TSEC_TBIPA_REG = MOT_TSEC_TBIPA_DEFAULT;

	pDrvCtrl->tbiAdr = MOT_TSEC_TBIPA_DEFAULT + pDrvCtrl->unit;
	MOT_TSEC_TBIPA_REG = pDrvCtrl->tbiAdr;
	MOT_TSEC_MIIMCFG_REG = miicfgReg;

	/* clear mask and any events pending */
	MOT_TSEC_IMASK_REG = 0;
	MOT_TSEC_IEVENT_REG = 0xffffffff;

	/* no error events */
	MOT_TSEC_EDIS_REG = 0xffffffff;

	MOT_TSEC_DMACTRL_REG = dmactrlReg;
	MOT_TSEC_TCTRL_REG = tctrlReg;
	MOT_TSEC_TXIC_REG = txicReg;
	MOT_TSEC_RSTAT_REG = rstatReg;
	MOT_TSEC_MRBLR_REG = mrblrReg;
	MOT_TSEC_PTV_REG = ptvReg;
	MOT_TSEC_MINFLR_REG = minflrReg;
	MOT_TSEC_HAFDUP_REG = hafdupReg;
	MOT_TSEC_IPGIFG_REG = ipgifgReg;
	MOT_TSEC_MAXFRM_REG = maxfrmReg;
	MOT_TSEC_IFSTAT_REG = ifstatReg;
	MOT_TSEC_FIFO_TX_THR_REG = fifoTxThrReg;
	MOT_TSEC_FIFO_TX_STARVE_REG = fifoTxStarveReg;
	MOT_TSEC_FIFO_TX_STARVE_SHUTOFF_REG = fifoTxStarveShutoffReg;
    MOT_TSEC_RXIC_REG = rxic;

	MOT_TSEC_ATTR_REG = attrReg;
	MOT_TSEC_ATTRELI_REG = attreliReg;

	MOT_TSEC_RBASE_REG = (UINT32) pDrvCtrl->pRbdBase;
	MOT_TSEC_TBASE_REG = (UINT32) pDrvCtrl->pTbdBase;
	MOT_TSEC_TBPTR_REG = (UINT32) pDrvCtrl->pTbdBase;

	/* set up default events */

	pDrvCtrl->intMask = (MOT_TSEC_IEVENT_BSY |
						 MOT_TSEC_IEVENT_GTSC |
						 MOT_TSEC_IEVENT_GRSC |
						 MOT_TSEC_IEVENT_TXF |
						 MOT_TSEC_IEVENT_RXF0 |
						 MOT_TSEC_IEVENT_RXC |
						 MOT_TSEC_IEVENT_XFUN);

	/* set up default error events */

	pDrvCtrl->intErrorMask = (MOT_TSEC_EDIS_BSYDIS |
							  MOT_TSEC_EDIS_EBERRDIS |
							  MOT_TSEC_EDIS_TXEDIS |
							  MOT_TSEC_EDIS_XFUNDIS |
							  MOT_TSEC_EDIS_LCDIS |
						 	  MOT_TSEC_EDIS_CRLDIS);

	/* error events that can be disabled
	   ( MOT_TSEC_EDIS_BSYDIS | MOT_TSEC_EDIS_EBERRDIS |
	   MOT_TSEC_EDIS_TXEDIS | MOT_TSEC_EDIS_LCDIS |
	   MOT_TSEC_EDIS_CRLDIS | MOT_TSEC_EDIS_XFUNDIS );
	 */

	MOT_TSEC_MACCFG1_REG = maccfg1Reg;
    CACHE_PIPE_FLUSH();

	return (OK);
}

/*******************************************************************************
* motTsecPhyParmInit - initialize PHY parameters
*
* This routine initializes all the memory needed by the driver whose control
* structure is passed in <pDrvCtrl>.
*
* RETURNS: OK or ERROR
*
* ERRNO
*/
LOCAL STATUS motTsecPhyParmInit(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	if (pDrvCtrl->phyInitFunc != NULL && pDrvCtrl->phyInit != NULL) {
		pDrvCtrl->phyFlags = pDrvCtrl->phyInit->phyDefMode;
		pDrvCtrl->phyInfo->phyFlags = pDrvCtrl->phyInit->phyDefMode;

        pDrvCtrl->phyFlags |= MII_PHY_PRE_INIT;

		pDrvCtrl->phyInfo->phyFlags = pDrvCtrl->phyFlags;

		pDrvCtrl->phyInfo->phyAnOrderTbl = pDrvCtrl->phyInit->phyAnOrderTbl;
		pDrvCtrl->phyInfo->phyAddr = pDrvCtrl->phyInit->phyAddr;
		pDrvCtrl->phyInfo->phyMaxDelay = pDrvCtrl->phyInit->phyMaxDelay;
		pDrvCtrl->phyInfo->phyDelayParm = pDrvCtrl->phyInit->phyDelayParm;
	}
	else {
		pDrvCtrl->phyFlags = MII_PHY_DEF_SET;
		pDrvCtrl->phyInfo->phyFlags = MII_PHY_DEF_SET;
		pDrvCtrl->phyInfo->phyAnOrderTbl = (MII_AN_ORDER_TBL *)NULL;
		pDrvCtrl->phyInfo->phyAddr = pDrvCtrl->unit;
		pDrvCtrl->phyInfo->phyMaxDelay = MII_PHY_DEF_DELAY;
		pDrvCtrl->phyInfo->phyDelayParm = 1;
	}

	MOT_TSEC_LOG(MOT_TSEC_DBG_START, "motTsecPhyParmInit: phyAddr = %p\n",
				 pDrvCtrl->phyInfo->phyAddr,0,0,0,0,0);
	return (OK);
}

/*******************************************************************************
* motTsecInitMem - initialize memory
*
* This routine initializes all the memory needed by the driver whose control
* structure is passed in <pDrvCtrl>.
*
* RETURNS: OK or ERROR
*
* ERRNO
*/
LOCAL STATUS motTsecInitMem(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	UINT32 bdMemSize;			/* total BD area including Alignment */
	UINT32 rbdMemSize;			/* Receive BD area size */
	UINT32 tbdMemSize;			/* Transmit BD area size */
	UINT16 clNum;				/* a buffer number holder */


/* cluster blocks configuration */
	M_CL_CONFIG mclBlkConfig = { 0, 0, NULL, 0 };

/* cluster blocks config table */
	CL_DESC clDescTbl[] = { {MOT_TSEC_MAX_CL_LEN, 0, NULL, 0} };

/* number of different clusters sizes in pool -- only 1 */
	int clDescTblNumEnt = 1;

	/* set up buffer user attributes */
	if (pDrvCtrl->initParms) {
		if ((pDrvCtrl->pBufAlloc = (char*)pDrvCtrl->initParms->memBufPtr) != NULL) {
			pDrvCtrl->initType = END_NET_POOL_INIT;
            }
        else
            {
            pDrvCtrl->initType = END_NET_POOL_CREATE;
            }
		pDrvCtrl->bufSize = pDrvCtrl->initParms->memBufSize;

		pDrvCtrl->pBdAlloc = (TSEC_BD *) pDrvCtrl->initParms->bdBasePtr;
		pDrvCtrl->bdSize = pDrvCtrl->initParms->bdSize;

		if (pDrvCtrl->initParms->tbdNum == 0)
			pDrvCtrl->tbdNum = MOT_TSEC_TBD_NUM_DEFAULT;
		else
			pDrvCtrl->tbdNum = pDrvCtrl->initParms->tbdNum;

		/* Test that the number of tbds is a power of 2 */

		if (pDrvCtrl->tbdNum & (pDrvCtrl->tbdNum - 1)) {
			/* If not a power of 2 elevate to the next power of 2 */
			pDrvCtrl->tbdNum = (0x1 << ffsMsb(pDrvCtrl->tbdNum));
		}

		pDrvCtrl->tbdMask = pDrvCtrl->tbdNum - 1;

		if (pDrvCtrl->initParms->rbdNum == 0) {
			pDrvCtrl->rbdNum = MOT_TSEC_RBD_NUM_DEFAULT;
		}
		else {
			pDrvCtrl->rbdNum = pDrvCtrl->initParms->rbdNum;
		}

		/* Test that the number of rbds is a power of 2 */

		if (pDrvCtrl->rbdNum & (pDrvCtrl->rbdNum - 1)) {
			/* If not a power of 2 elevate to the next power of 2 */
			pDrvCtrl->rbdNum = (0x1 << ffsMsb(pDrvCtrl->rbdNum));
		}
		pDrvCtrl->rbdMask = pDrvCtrl->rbdNum - 1;
	}
	else {
		/* use defaults */
		pDrvCtrl->initType = END_NET_POOL_CREATE;
		pDrvCtrl->pBufAlloc = NULL;	/* driver must allocate clusters */
		pDrvCtrl->bufSize = 0;	/* none allocated */

		pDrvCtrl->pBdAlloc = NULL;	/* driver must allocate descriptor ring */
		pDrvCtrl->bdSize = 0;
		pDrvCtrl->tbdNum = MOT_TSEC_TBD_NUM_DEFAULT;
		pDrvCtrl->rbdNum = MOT_TSEC_RBD_NUM_DEFAULT;
	}

	/* initialize the netPool */
	if ((pDrvCtrl->endObj.pNetPool = malloc(sizeof(NET_POOL))) == NULL) {
		return ERROR;
	}

	if (pDrvCtrl->pBdAlloc == NULL) {
		/*
		 * The user has not provided a BD data area
		 * This driver can't handle write incoherent caches.
		 */
		if (!CACHE_DMA_IS_WRITE_COHERENT())
			return ERROR;

		rbdMemSize = MOT_TSEC_BD_SIZE * pDrvCtrl->rbdNum;
		tbdMemSize = MOT_TSEC_BD_SIZE * pDrvCtrl->tbdNum;
		bdMemSize = ROUND_UP((rbdMemSize + tbdMemSize), tbdMemSize);

		pDrvCtrl->pBdAlloc = memalign(bdMemSize, bdMemSize);

		if (pDrvCtrl->pBdAlloc == NULL)
			return ERROR;		/* no memory available */

		pDrvCtrl->bdSize = bdMemSize;
		MOT_TSEC_FLAG_SET(MOT_TSEC_OWN_BD_MEM);
	}
	else {
		/* The user provided an area for the BDs  */
		if (pDrvCtrl->bdSize == 0)
			return ERROR;

		rbdMemSize = MOT_TSEC_BD_SIZE * pDrvCtrl->rbdNum;
		tbdMemSize = MOT_TSEC_BD_SIZE * pDrvCtrl->tbdNum;
		bdMemSize = ROUND_UP((rbdMemSize + tbdMemSize), MOT_TSEC_BD_ALIGN);

		if (pDrvCtrl->bdSize < bdMemSize)
			return ERROR;

		if ((int) pDrvCtrl->pBdAlloc & 0x00000007)
			pDrvCtrl->pBdAlloc = (TSEC_BD *) ROUND_UP(pDrvCtrl->pBdAlloc,
													  MOT_TSEC_BD_ALIGN);

		MOT_TSEC_FLAG_CLEAR(MOT_TSEC_OWN_BD_MEM);
	}

	/* zero the shared memory */
	memset(pDrvCtrl->pBdAlloc, 0, (int) pDrvCtrl->bdSize);

	/* align the shared memory */
	pDrvCtrl->pBdBase = pDrvCtrl->pBdAlloc;

	/* locate transmit ring */
	pDrvCtrl->pTbdBase = pDrvCtrl->pBdBase;

	if (pDrvCtrl->extWriteL2AllocFunc != NULL)
		pDrvCtrl->extWriteL2AllocFunc(pDrvCtrl->pTbdBase, tbdMemSize, 0);

	/*  located the receive ring after the transmit ring */
	pDrvCtrl->pRbdBase = (TSEC_BD *) ((UINT32) pDrvCtrl->pBdBase +
									  (MOT_TSEC_BD_SIZE * pDrvCtrl->tbdNum));

	MOT_TSEC_LOG(MOT_TSEC_DBG_START,
				 "\n\tBD:Alloc 0x%x Base 0x%x Total 0x%08x RxBD %d TxBD %d\n",
				 (int) pDrvCtrl->pBdAlloc, (int) pDrvCtrl->pBdBase,
				 pDrvCtrl->bdSize, pDrvCtrl->rbdNum, pDrvCtrl->tbdNum, 6);

	/*
	 * number of clusters, including loaning buffers, a min number
	 * of transmit clusters for copy-mode transmit, and one transmit
	 * cluster for polling operation.
	 */

	clNum = (MOT_TSEC_CL_MULTIPLE * pDrvCtrl->rbdNum) + MOT_TSEC_TX_POLL_NUM +
		MOT_TSEC_CL_NUM_DEFAULT;

	if (pDrvCtrl->initType == END_NET_POOL_INIT) {
		/* pool of mblks */
		if (mclBlkConfig.mBlkNum == 0) {
			mclBlkConfig.mBlkNum = clNum * 5;
		}

		/* pool of clusters, including loaning buffers */
		if (clDescTbl[0].clNum == 0) {
			clDescTbl[0].clNum = clNum;
			clDescTbl[0].clSize = MOT_TSEC_CL_SIZE;
		}


		/* there's a cluster overhead and an alignment issue */
		clDescTbl[0].memSize = (clDescTbl[0].clNum * clDescTbl[0].clSize) + 256;

		/* patched to match netBufPool.c (differnent PoolFuncTbl) */
		/* clDescTbl[0].memSize = clDescTbl[0].clNum * (clDescTbl[0].clSize + sizeof (int)); */


		if (pDrvCtrl->pBufAlloc == NULL) {
			/*
			 * The user has not provided data area for the clusters.
			 * Allocate from cachable data block.
			 */
			clDescTbl[0].memArea =
				(char *) memalign(_CACHE_ALIGN_SIZE, clDescTbl[0].memSize);

			if (clDescTbl[0].memArea == NULL)
				return ERROR;

			/* store the pointer to the clBlock area and its size */
			pDrvCtrl->pBufAlloc = clDescTbl[0].memArea;
			pDrvCtrl->bufSize = clDescTbl[0].memSize;
			MOT_TSEC_FLAG_SET(MOT_TSEC_OWN_BUF_MEM);

			/* cache functions descriptor for data buffers */
			pDrvCtrl->bufCacheFuncs = cacheUserFuncs;
		}
		else {
			/*
			 * The user provides the area for buffers. This must be from a
			 * non cacheble area.
			 */
			if (pDrvCtrl->bufSize == 0)
				return ERROR;

			/*
			 * check the user provided enough memory with reference
			 * to the given number of receive/transmit frames, if any.
			 */
			if (pDrvCtrl->bufSize < (UINT32) clDescTbl[0].memSize)
				return ERROR;

			/* Set memArea to the buffer base */

			clDescTbl[0].memArea = pDrvCtrl->pBufAlloc;
			MOT_TSEC_FLAG_CLEAR(MOT_TSEC_OWN_BUF_MEM);
			pDrvCtrl->bufCacheFuncs = cacheDmaFuncs;

		}
		/* zero and align the shared memory */
		memset(pDrvCtrl->pBufAlloc, 0, (int) pDrvCtrl->bufSize);
		pDrvCtrl->pBufBase = (char *) ROUND_UP((UINT32) pDrvCtrl->pBufAlloc,
											   MOT_TSEC_CL_ALIGNMENT);

		MOT_TSEC_LOG(MOT_TSEC_DBG_START, "\n\tCluster:Alloc 0x%x Base 0x%x"
					 " Total 0x%08x\n", (int) pDrvCtrl->pBufAlloc,
					 (int) pDrvCtrl->pBufBase, pDrvCtrl->bufSize, 4, 5, 6);

		/* pool of cluster blocks */
		if (mclBlkConfig.clBlkNum == 0) {
			mclBlkConfig.clBlkNum = clDescTbl[0].clNum;
		}

		/* get memory for mblks */

		/* RCS FIXUP MAGIC NUMBERS */

		if (mclBlkConfig.memArea == NULL) {
			/* memory size adjusted to hold the netPool pointer at the head */
			mclBlkConfig.memSize = ((((mclBlkConfig.mBlkNum + 1) * 64) +
									 ((mclBlkConfig.clBlkNum + 1) * 64)) + 0x1000);

			mclBlkConfig.memArea = (char *)
				((UINT32) (memalign(32, mclBlkConfig.memSize)) + 60);

			if (mclBlkConfig.memArea == NULL) {
				return ERROR;
			}

			/* store the pointer to the mBlock area */
			pDrvCtrl->pMBlkArea = mclBlkConfig.memArea;
			pDrvCtrl->mBlkSize = mclBlkConfig.memSize;
		}

		/* init the mem pool */
		if (netPoolInit(pDrvCtrl->endObj.pNetPool,
						&mclBlkConfig,
						&clDescTbl[0],
						clDescTblNumEnt,
						_pLinkPoolFuncTbl) == ERROR){
			return ERROR;
		}
	} else {  /* pDrvCtrl->initType == END_NET_POOL_CREATE */
		/*
		 * Allocate enough space to also hold 16 additional bytes for
		 * pDrvCtrl->pNetBufCfg->pName field
		 */

		if ((pDrvCtrl->pNetBufCfg = (NETBUF_CFG *) memalign (sizeof(long),
		                            (sizeof(NETBUF_CFG) + (END_NAME_MAX + 8))))
		                         == NULL)
			return (ERROR);

		bzero ((void*) pDrvCtrl->pNetBufCfg,sizeof(NETBUF_CFG));

		/* Initialize the pName field */

		pDrvCtrl->pNetBufCfg->pName = (char *)((int)pDrvCtrl->pNetBufCfg +
		                                       sizeof(NETBUF_CFG));

		sprintf(pDrvCtrl->pNetBufCfg->pName,"%s%d%s","fei", pDrvCtrl->unit,
				"Pool");

		/*
		 * Set the attributes to be Cached, Cache aligned, sharable, &
		 * ISR safe
		 */

		pDrvCtrl->pNetBufCfg->attributes = ATTR_AC_SH_ISR;

		/* Set pDomain to kernel, use NULL. */
		pDrvCtrl->pNetBufCfg->pDomain = NULL;

		/* Set ratio of mBlks to clusters */
		pDrvCtrl->pNetBufCfg->ctrlNumber = clNum;

		/* Set memory partition of mBlks to kernel, use NULL */
		pDrvCtrl->pNetBufCfg->ctrlPartId = NULL;

		/* Set extra memory size to zero for now */
		pDrvCtrl->pNetBufCfg->bMemExtraSize = 0;

		/* Set cluster's memory partition to kernel, use NULL */
		pDrvCtrl->pNetBufCfg->bMemPartId = NULL;

		/* Allocate memory for network cluster descriptor */
		pDrvCtrl->pNetBufCfg->pClDescTbl =
		                             (NETBUF_CL_DESC *) memalign(sizeof(long),
		                                                sizeof(NETBUF_CL_DESC));

		/* Initialize the Cluster Descriptor */
		pDrvCtrl->pNetBufCfg->pClDescTbl->clSize = MOT_TSEC_CL_SIZE;
		pDrvCtrl->pNetBufCfg->pClDescTbl->clNum = clNum;
		pDrvCtrl->pNetBufCfg->clDescTblNumEnt = 1;

		/* Call netPoolCreate() with the Link Pool Function Table */
		if ((pDrvCtrl->endObj.pNetPool =
		                netPoolCreate ((NETBUF_CFG *)pDrvCtrl->pNetBufCfg,
		                                            _pLinkPoolFuncTbl)) == NULL)

			return (ERROR);
	}

	if ((pDrvCtrl->pClPoolId = netClPoolIdGet(pDrvCtrl->endObj.pNetPool,
											  MOT_TSEC_MAX_CL_LEN,
											  FALSE)) == NULL)
		return ERROR;

	/* allocate receive buffer list */
	pDrvCtrl->pMblkList = (M_BLK_ID *) calloc(pDrvCtrl->rbdNum,
											  sizeof(M_BLK_ID));
	if (pDrvCtrl->pMblkList == NULL)
		return ERROR;

	memset(pDrvCtrl->pMblkList, 0, (pDrvCtrl->rbdNum * sizeof(M_BLK_ID)));

	/* allocate the M_BLK buffer list */

	pDrvCtrl->tBufList = (M_BLK **) calloc(pDrvCtrl->tbdNum, sizeof(M_BLK *));

	if (pDrvCtrl->tBufList == NULL)
		return ERROR;

	memset(pDrvCtrl->tBufList, 0, (pDrvCtrl->tbdNum * sizeof(M_BLK *)));

    if (pDrvCtrl->maxRxFrames == 0)
        pDrvCtrl->maxRxFrames = pDrvCtrl->rbdNum * 3 / 4;

	return OK;
}

/**************************************************************************
*
* motTsecStart - start the device
*
* This routine starts the TSEC device and brings it up to an operational
* state.  The driver must have already been loaded with the motTsecEndLoad()
* routine.
*
* \INTERNAL
* The speed field in the phyInfo structure is only set after the call
* to the physical layer init routine. On the other hand, the mib2
* interface is initialized in the motTsecEndLoad() routine, and the default
* value of 10Mbit assumed there is not always correct. We need to
* correct it here.
*
* RETURNS: OK, or ERROR if the device could not be started.
*
* ERRNO
*/
STATUS motTsecStart(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	int retVal;
	/*int ix;*/
	/*UINT16 value;*/


	MOT_TSEC_FRAME_SET(pDrvCtrl);

	MOT_TSEC_LOG(MOT_TSEC_DBG_START, "motTsecStart\n", 1, 2, 3, 4, 5, 6);

	/* must have been loaded */
	if ((pDrvCtrl->state & MOT_TSEC_STATE_LOADED) == MOT_TSEC_STATE_NOT_LOADED)
		return ERROR;

	/* check if already running */
	if ((pDrvCtrl->state & MOT_TSEC_STATE_RUNNING) == MOT_TSEC_STATE_RUNNING)
		return OK;

	/* call the BSP to do any other initialization (MII interface) */
	if (pDrvCtrl->enetEnable != NULL)
		pDrvCtrl->enetEnable();

	/* Initialize the Physical medium layer through the BSP */
	if (pDrvCtrl->phyInitFunc != NULL) {
		/* Initialize some fields in the PHY info structure */

		if (motTsecPhyPreInit(pDrvCtrl) != OK)
			return ERROR;

		if ((pDrvCtrl->phyInitFunc(pDrvCtrl->phyInfo)) != OK) {
			return ERROR;
		}

		/* heavily modified by kp */

		/* check current link state */
		motTsecHandleLSCJob( pDrvCtrl );
#if 0
		for (ix = 0; ix < 3; ix++) {
			motTsecMiiPhyRead(pDrvCtrl, pDrvCtrl->phyInit->phyAddr,
							  MOT_TSEC_PHY_GIG_STATUS_REG, &value);
		}

		if ((!(value & (MOT_TSEC_PHY_1000_M_LINK_FD |
						MOT_TSEC_PHY_1000_M_LINK_OK))) &&
			(pDrvCtrl->userFlags & MOT_TSEC_USR_MODE_GMII)) {
			pDrvCtrl->userFlags &= ~MOT_TSEC_USR_MODE_GMII;
			pDrvCtrl->userFlags |= MOT_TSEC_USR_MODE_MII;

			pDrvCtrl->phyInfo->phyAddr = pDrvCtrl->phyInit->phyAddr;

			if (motTsecParmInit(pDrvCtrl) != OK)
				return ERROR;

			if (motTsecPhyPreInit(pDrvCtrl) != OK)
				return ERROR;

			if ((pDrvCtrl->phyInitFunc(pDrvCtrl->phyInfo)) != OK)
				return ERROR;
		}

		if (pDrvCtrl->userFlags & MOT_TSEC_USR_MODE_MII) {

			motTsecMiiPhyRead(pDrvCtrl, pDrvCtrl->phyInit->phyAddr,
							  MOT_TSEC_PHY_LINK_STATUS, &value);

			if ((!(value & MOT_TSEC_PHY_10_M_LINK_FD)) ||
				(!(value & MOT_TSEC_PHY_100_M_LINK_FD))) {
				MOT_TSEC_MACCFG2_REG &= ~MOT_TSEC_MACCFG2_FULL_DUPLEX;
				value = MOT_TSEC_MACCFG2_REG;
			}
		}
#endif /* 0 */

#ifdef MOT_TSEC_DBG
		/*taskDelay( sysClkRateGet()/2 );  kp. let logs come out??? */
#endif /* MOT_TSEC_DBG */
	}

	/* Initialize the transmit buffer descriptors */

	if (motTsecTbdInit(pDrvCtrl) == ERROR) {
		return ERROR;
	}

	/* Initialize the receive buffer descriptors */
	if (motTsecRbdInit(pDrvCtrl) == ERROR)
		return ERROR;

	/* initialize the Two Tiered Polling fields in the DRV_CTRL structure. */
	pDrvCtrl->pollCnt = 0;
	pDrvCtrl->pollLoops = 2;
	pDrvCtrl->pollDone = FALSE;

    retVal = intEnable(pDrvCtrl->inum_tsecTx);
    retVal = intEnable(pDrvCtrl->inum_tsecRx);
    retVal = intEnable(pDrvCtrl->inum_tsecErr);

	if (retVal == ERROR)
		return ERROR;

	/* correct the speed for the mib2 stuff */
	pDrvCtrl->endObj.mib2Tbl.ifSpeed = pDrvCtrl->phyInfo->phySpeed;

	END_FLAGS_SET(&pDrvCtrl->endObj, (IFF_UP | IFF_RUNNING));

	/* clear all events */
	MOT_TSEC_IEVENT_REG = 0xffffffff;

	/* unmask events */
	MOT_TSEC_IMASK_REG = pDrvCtrl->intMask;

	/* unmask error events */
	MOT_TSEC_EDIS_REG = pDrvCtrl->intErrorMask;

	/* initialize maxRetries */
	pDrvCtrl->maxRetries = 3;

	motTsecRestart(pDrvCtrl);

	pDrvCtrl->state |= MOT_TSEC_STATE_RUNNING;

	MOT_TSEC_LOG(MOT_TSEC_DBG_START, "motTsecStart done\n", 1, 2, 3, 4, 5, 6);

	return OK;
}

/**************************************************************************
*
* motTsecStop - stop the 'mottsec' interface
*
* This routine marks the interface as inactive, disables interrupts and
* the Ethernet Controller. As a result, reception is stopped immediately,
* and transmission is stopped after a bad CRC is appended to any frame
* currently being transmitted. The reception/transmission control logic
* (FIFO pointers, buffer descriptors, etc.) is reset. To bring the
* interface back up, motTsecStart() must be called.
*
* RETURNS: OK, always.
*
* ERRNO
*/
LOCAL STATUS motTsecStop(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	int retVal;
	int intLevel;
	TSEC_BD *pRbd = NULL;		/* generic rbd pointer */
	M_BLK_ID pMblk;
	UINT16 ix;					/* a counter */
	M_BLK *pMblkFree;

	MOT_TSEC_FRAME_SET(pDrvCtrl);

	MOT_TSEC_LOG(MOT_TSEC_DBG_START, "motTsecStop\n", 1, 2, 3, 4, 5, 6);

	/* check if already stopped */
	if ((pDrvCtrl->state & MOT_TSEC_STATE_RUNNING) != MOT_TSEC_STATE_RUNNING)
		return OK;

	motTsecGracefulStop(pDrvCtrl);

	intLevel = intLock();

	/* mask chip interrupts */
	MOT_TSEC_IMASK_REG = 0;

	/* disable system interrupt */
    retVal = intDisable(pDrvCtrl->inum_tsecTx);
    retVal = intDisable(pDrvCtrl->inum_tsecRx);
    retVal = intDisable(pDrvCtrl->inum_tsecErr);

	if (pDrvCtrl->endStatsConf.ifWatchdog != NULL)
		wdDelete(pDrvCtrl->endStatsConf.ifWatchdog);

	intUnlock(intLevel);
	if (retVal == ERROR)
		return ERROR;

	/* call the BSP to disable the MII interface */

	pDrvCtrl->enetDisable();

	/* free buffer descriptors */

	pRbd = pDrvCtrl->pRbdBase;

	for (ix = 0; ix < pDrvCtrl->rbdNum; ix++) {
		/* return the cluster buffers to the pool */
		pMblk = pDrvCtrl->pMblkList[ix];

		if (pMblk != NULL) {
			/* free the tuple */
			netMblkClChainFree(pMblk);
		}
	}

	for (ix = 0; ix < pDrvCtrl->tbdNum; ix++) {
		/* return the cluster headers to the pool */
		pMblkFree = pDrvCtrl->tBufList[ix];

		if (pMblkFree != NULL) {
			netMblkClChainFree(pMblkFree);
		}
	}

	pDrvCtrl->state &= ~MOT_TSEC_STATE_RUNNING;

	return OK;
}

/*******************************************************************************
* motTsecHandler - entry point for handling job events from motTsecInt
*
* This routine is the entry point for handling job events from motTsecInt.
*
* RETURNS: N/A
*
* ERRNO
*/
LOCAL void motTsecHandler(
#ifdef USE_NET_JOB_ADD
	TSEC_DRV_CTRL * pDrvCtrl
#else
	QJOB * pTxJob
#endif
) {
#ifndef USE_NET_JOB_ADD
	TSEC_DRV_CTRL *pDrvCtrl = member_to_object (pTxJob, TSEC_DRV_CTRL, txJob);
#endif
	TSEC_REG_T *tsecReg;
    BOOL restart;

	tsecReg = pDrvCtrl->tsecRegsPtr;

	/* handle TX MUX error interrupts */

	END_TX_SEM_TAKE(&pDrvCtrl->endObj, WAIT_FOREVER);
	motTsecTbdClean(pDrvCtrl);
	restart = FALSE;
	if (pDrvCtrl->txStall &&
		pDrvCtrl->tbdFree > pDrvCtrl->txUnStallThresh)
	{
		/* turn off stall alarm */
		pDrvCtrl->txStall = FALSE;
		restart = TRUE;
	}
	END_TX_SEM_GIVE(&pDrvCtrl->endObj);

#ifdef TSEC_RESCHEDULE_TX_CLEAN

	if (restart) {
#ifdef USE_NET_JOB_ADD
		pDrvCtrl->txHandlerQued = FALSE;
#else
		QJOB_CLEAR_BUSY (&pDrvCtrl->txJob);
#endif
		/*
		 * Should we flush the cache pipe here, to make sure that
		 * the flag write is seen before the TX interrupt is reenabled?
		 */
		MOT_TSEC_IMASK_REG |= MOT_TSEC_IEVENT_TXF;

		if (MOT_TSEC_TSTAT_REG & MOT_TSEC_TSTAT_THLT) {
			MOT_TSEC_TSTAT_REG = MOT_TSEC_TSTAT_THLT;
		}

		/* call mux restart tx */
		MOT_TSEC_LOG(MOT_TSEC_DBG_INT_TX_ERR, "ERROR: MUX TX RESTART\n",
					 1, 2, 3, 4, 5, 6);

		muxTxRestart(pDrvCtrl);
	}
	else {
#ifdef USE_NET_JOB_ADD
		if ((NET_JOB_ADD((FUNCPTR) motTsecHandler, (int) pDrvCtrl,
					   0, 0, 0, 0)) != ERROR) {
			pDrvCtrl->txHandlerQued = TRUE;
		}
		else {
			logMsg("The netJobRing is full. \n", 0, 0, 0, 0, 0, 0);
		}
#else /* USE_NET_JOB_ADD */
		/* job is still busy... */
		jobQueuePost (pDrvCtrl->jobQueueId, &pDrvCtrl->txJob);
#endif /* USE_NET_JOB_ADD */
	}

#else  /* !TSEC_RESCHEDULE_TX_CLEAN */

#ifdef USE_NET_JOB_ADD
	pDrvCtrl->txHandlerQued = FALSE;
#else
	QJOB_CLEAR_BUSY (&pDrvCtrl->txJob);
#endif
	MOT_TSEC_IMASK_REG |= MOT_TSEC_IEVENT_TXF;

	/* If in Wait mode, do we need to distinguish a halt due to error
	 * from a halt due to completion?
	 */

	MOT_TSEC_TSTAT_REG = MOT_TSEC_TSTAT_THLT;

	if (restart) {
		/* call mux restart tx */
		MOT_TSEC_LOG ( MOT_TSEC_DBG_INT_TX_ERR, "ERROR: MUX TX RESTART\n",
						1,2,3,4,5,6);

		muxTxRestart (pDrvCtrl);
	}

#endif /* !TSEC_RESCHEDULE_TX_CLEAN */
}

/*******************************************************************************
* motTsecRxInt - entry point for handling receive interrupts from the TSEC
*
* The interrupting events are acknowledged to the device. The device
* will de-assert its interrupt signal.  The amount of work done here is kept
* to a minimum; the bulk of the work is deferred to the netTask. However, the
* interrupt code takes on the responsibility to clean up the RX
* BD rings.
*
* RETURNS: N/A
*
* ERRNO
*/
LOCAL void motTsecRxInt(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	UINT32 event;				/* event intr register */
	UINT32 status;				/* status word */
	TSEC_REG_T *tsecReg;

	tsecReg = pDrvCtrl->tsecRegsPtr;

	/* get pending events */
	event = MOT_TSEC_IEVENT_REG;

	/*logMsg("motTsecRxInt. event=%x\n", event, 0, 0, 0, 0, 0);*/

	/* clear pending events */
	MOT_TSEC_IEVENT_REG = event & (MOT_TSEC_IEVENT_RXF0|MOT_TSEC_IEVENT_BSY);

	/* abstract current status */
	status = event & MOT_TSEC_IMASK_REG;

	/* Process a receive frame event */

	if (status & (MOT_TSEC_IEVENT_RXF0 | MOT_TSEC_IEVENT_BSY)) {
		MOT_TSEC_IMASK_REG &= ~MOT_TSEC_IEVENT_RXF0;
#ifdef USE_NET_JOB_ADD
		if (!pDrvCtrl->rxJobQued) {
			if ((NET_JOB_ADD((FUNCPTR) motTsecHandleRXFrames, (int) pDrvCtrl,
						   0, 0, 0, 0)) != ERROR) {
				pDrvCtrl->rxJobQued = TRUE;
			}
			else {
				logMsg("The netJobRing is full. \n", 0, 0, 0, 0, 0, 0);

				MOT_TSEC_IMASK_REG &= MOT_TSEC_IEVENT_RXF0;
			}
		}
#else /* USE_NET_JOB_ADD */
	if (!QJOB_IS_BUSY (&pDrvCtrl->rxJob)) {
		QJOB_SET_BUSY (&pDrvCtrl->rxJob);
		jobQueuePost (pDrvCtrl->jobQueueId, &pDrvCtrl->rxJob);
	}
#endif /* USE_NET_JOB_ADD */
	}

}

/*******************************************************************************
* motTsecTxInt - entry point for handling TX interrupts from the TSEC
*
* The interrupting events are acknowledged to the device. The device
* will de-assert its interrupt signal.  The amount of work done here is kept
* to a minimum; the bulk of the work is deferred to the netTask. However, the
* interrupt code takes on the responsibility to clean up the TX
* BD rings.
*
* KP NOTE 8540 FEC: This routine must be intConnect'ed before RxInt to
* the single FEC interrupt
* It services and clears only the events related to Tx
*
* RETURNS: N/A
*
* ERRNO
*/

_WRS_FASTTEXT
LOCAL void motTsecTxInt(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	UINT32 event;				/* event intr register */
	UINT32 status;				/* status word */
	TSEC_REG_T *tsecReg;

	UINT32 handledEvents =
		(MOT_TSEC_IEVENT_TXF |
		 MOT_TSEC_IEVENT_TXE |
		 MOT_TSEC_IEVENT_BABT | MOT_TSEC_IEVENT_XFUN |
		 MOT_TSEC_IEVENT_GTSC | MOT_TSEC_IEVENT_GRSC );

		 tsecReg = pDrvCtrl->tsecRegsPtr;

	/* get pending events */
	event = MOT_TSEC_IEVENT_REG & handledEvents;

	/*logMsg("motTsecTxInt. event=%x\n", event, 0, 0, 0, 0, 0);*/
	/* clear pending events */
	MOT_TSEC_IEVENT_REG = event;

	/* abstract current status */
	status = event & MOT_TSEC_IMASK_REG;

	if ((status & handledEvents) != 0) {

		/* Transmit Frame Interrupt */

		if (status & MOT_TSEC_IEVENT_TXF) {
			MOT_TSEC_IMASK_REG &= ~MOT_TSEC_IEVENT_TXF;
		}

#ifdef USE_NET_JOB_ADD
		/* Restart the MUX to be able to send again */

		if (!pDrvCtrl->txHandlerQued) {
			if ((NET_JOB_ADD((FUNCPTR) motTsecHandler, (int) pDrvCtrl,
						   0, 0, 0, 0)) != ERROR) {
				pDrvCtrl->txHandlerQued = TRUE;
			}
			else {
				logMsg("The netJobRing is full. \n", 0, 0, 0, 0, 0, 0);
				MOT_TSEC_IMASK_REG |= MOT_TSEC_IEVENT_TXF;
			}
		}
#else /* USE_NET_JOB_ADD */
	if (! QJOB_IS_BUSY (&pDrvCtrl->txJob)) {
		QJOB_SET_BUSY (&pDrvCtrl->txJob);
		jobQueuePost (pDrvCtrl->jobQueueId, &pDrvCtrl->txJob);
	}
#endif /* USE_NET_JOB_ADD */
	}
}


/******************************************************************************
* motTsecDumpTxRing - Show the Transmit Ring details
*
* This routine displays the transmit ring descriptors.
*
* SEE ALSO: motTsecDumpRxRing()
*
* RETURNS: N/A
*
* ERRNO
*/

void motTsecDumpTxRing(
	int tsecNum
) {
	TSEC_BD *pTestTbd;
	TSEC_DRV_CTRL *pDrvCtrl;
	int i;
	int index;

	if ((pDrvCtrl = (TSEC_DRV_CTRL *) endFindByName("mottsec", tsecNum))
		== NULL)
		return;

	pTestTbd = pDrvCtrl->pTbdBase;

	index = (((UINT32) pDrvCtrl->pTbdNext - (UINT32) pDrvCtrl->pTbdBase) /
			 MOT_TSEC_BD_SIZE);


	printf( "motTsecDumpTxRing: Current descriptor index %d at %p status 0x%x\n",
			index, pDrvCtrl->pTbdNext, pDrvCtrl->pTbdNext->bdStat);

	for (i = 0; i < pDrvCtrl->tbdNum; i++) {
		pTestTbd = (TSEC_BD *) ((UINT32) pDrvCtrl->pTbdBase +
								(i * MOT_TSEC_BD_SIZE));

		printf("motTsecDumpTxRing: index %d pTxDescriptor %p status 0x%x \n",
			   i, pTestTbd, pTestTbd->bdStat);
	}
}

/******************************************************************************
*
* motTsecPktTransmit - transmit a packet
*
* This routine transmits the packet described by the given parameters
* over the network, without copying the mBlk to a driver buffer.
* It also updates statistics.
*
* RETURNS: OK, or ERROR if no resources were available.
*
* ERRNO
*/
LOCAL STATUS motTsecPktTransmit(
	TSEC_DRV_CTRL * pDrvCtrl,	/* pointer to TSEC_DRV_CTRL structure */
	M_BLK * pMblk,				/* pointer to the mBlk */
	int fragNum					/* number of fragments */
) {
	M_BLK *pCurr;				/* the current mBlk */
	UINT16 bdStat = 0;
	UINT32 index = pDrvCtrl->tbdIndex;
	UINT32 first;
	UINT32 last;
	UINT32 mask = pDrvCtrl->tbdMask;
	TSEC_BD *pTbdBase = pDrvCtrl->pTbdBase;
	M_BLK **tBufList = pDrvCtrl->tBufList;

	if (fragNum == 1) {
		pTbdBase[index].bdAddr = (UINT32) pMblk->mBlkHdr.mData;
		pTbdBase[index].bdLen = pMblk->mBlkHdr.mLen;

		/* store pMblk */
		tBufList[index] = pMblk;

		/* Check for wrap  */


		if (index == mask) {
			pTbdBase[index].bdStat = (MOT_TSEC_TBD_W |
									  MOT_TSEC_TBD_R |
									  MOT_TSEC_TBD_L |
									  MOT_TSEC_TBD_TC | MOT_TSEC_TBD_I);
		}
		else {
			pTbdBase[index].bdStat = (MOT_TSEC_TBD_R |
									  MOT_TSEC_TBD_L |
									  MOT_TSEC_TBD_TC | MOT_TSEC_TBD_I);
		}

		MOT_TSEC_CACHE_FLUSH(pTbdBase[index].bdAddr, pTbdBase[index].bdLen);

		index++;
		index &= mask;
		pDrvCtrl->tbdIndex = index;
		pDrvCtrl->tbdFree--;

		return OK;
	}

	pCurr = pMblk;

	/* save the first TBD index */
	first = index;

	/* calculate the last fragments index */
	last = (index + (fragNum - 1)) & mask;

	tBufList[last] = pMblk;

	/* process the number of non zero fragments */

	bdStat = MOT_TSEC_TBD_I;

	while (fragNum) {
		fragNum--;

		/* bdStat = MOT_TSEC_TBD_I; */
		/*
		 * Set the ready bit only if there is one in the chain
		 * or if this is not the first in the chain
		 */

		if (index != first) {
			bdStat |= MOT_TSEC_TBD_R;

			/* fragment length */
			pTbdBase[index].bdLen = pCurr->mBlkHdr.mLen;

			/* fragment address */
			pTbdBase[index].bdAddr = (UINT32) pCurr->mBlkHdr.mData;

			/* flush the cache, cache and tx memory must agree */
			MOT_TSEC_CACHE_FLUSH(pTbdBase[index].bdAddr,
								 pTbdBase[index].bdLen);

			/* pTbdBase[index].bdStat |= MOT_TSEC_TBD_R; */
		}
		/* Check for wrap  */

		if (index == mask) {
			pTbdBase[index].bdStat = bdStat | MOT_TSEC_TBD_W;
		}
		else {
			pTbdBase[index].bdStat = bdStat;
		}

		index++;
		index &= mask;
		pDrvCtrl->tbdFree--;

		/* get the next mBlk in the chain */
		pCurr = (M_BLK *) pCurr->mBlkHdr.mNext;
	}

	pDrvCtrl->tbdIndex = index;

	pTbdBase[first].bdAddr = (UINT32) pMblk->mBlkHdr.mData;
	pTbdBase[first].bdLen = pMblk->mBlkHdr.mLen;

	/* flush the cache, cache and tx memory must agree */
	MOT_TSEC_CACHE_FLUSH(pTbdBase[first].bdAddr, pTbdBase[first].bdLen);


	pTbdBase[last].bdStat |= (MOT_TSEC_TBD_L | MOT_TSEC_TBD_TC);

	/* Check for wrap  */

	if (first == mask) {
		pTbdBase[first].bdStat |= (MOT_TSEC_TBD_W |
								   MOT_TSEC_TBD_I | MOT_TSEC_TBD_R);
	}
	else {
		pTbdBase[first].bdStat |= (MOT_TSEC_TBD_I | MOT_TSEC_TBD_R);
	}

	return OK;
}

/*******************************************************************************
* motTsecSend - send an Ethernet packet
*
* This routine() takes a M_BLK_ID and sends off the data in the M_BLK_ID.
* The buffer must already have the addressing information properly installed
* in it. This is done by a higher layer.
*
* muxSend() calls this routine each time it wants to send a packet.
*
* RETURNS: OK, or END_ERR_BLOCK, if no resources are available, or ERROR,
* if the device is currently working in polling mode.
*
* ERRNO
*/

LOCAL STATUS motTsecSend(
	TSEC_DRV_CTRL * pDrvCtrl,
	M_BLK * pMblk
) {
	M_BLK *pCurr;
	M_BLK *pNewMblk;
	int fragNum;				/* number of fragments in this mBlk */
	UINT32 len = 0;
	UINT16 bdStat = 0;
	UINT32 index = pDrvCtrl->tbdIndex;
	UINT32 first;
	UINT32 last;
	UINT32 mask = pDrvCtrl->tbdMask;
	TSEC_BD *pTbdBase = pDrvCtrl->pTbdBase;
	M_BLK **tBufList = pDrvCtrl->tBufList;

	MOT_TSEC_LOG(MOT_TSEC_DBG_TRACE_TX, "motTsecSend <ENTER>\n", 1, 2, 3, 4, 5,
				 6);

	END_TX_SEM_TAKE(&pDrvCtrl->endObj, WAIT_FOREVER);

	/* mblk sanity check */

	if (pMblk == NULL) {
		MOT_TSEC_LOG(MOT_TSEC_DBG_TX_ERR, "Zero Mblk\n", 0, 0, 0, 0, 0, 0);
		END_TX_SEM_GIVE(&pDrvCtrl->endObj);
		errno = EINVAL;
		return (ERROR);
	}

	if ((pMblk->mBlkHdr.mNext == NULL) && (pDrvCtrl->tbdFree > 0)) {
		/* Only one fragment */
		pTbdBase[index].bdAddr = (UINT32) pMblk->mBlkHdr.mData;
		pTbdBase[index].bdLen = pMblk->mBlkHdr.mLen;

		/* store pMblk */
		tBufList[index] = pMblk;

		/* Check for wrap  */

		if (index == mask) {
			pTbdBase[index].bdStat = (MOT_TSEC_TBD_W |
									  MOT_TSEC_TBD_R |
									  MOT_TSEC_TBD_L |
									  MOT_TSEC_TBD_TC | MOT_TSEC_TBD_I);
		}
		else {
			pTbdBase[index].bdStat = (MOT_TSEC_TBD_R |
									  MOT_TSEC_TBD_L |
									  MOT_TSEC_TBD_TC | MOT_TSEC_TBD_I);
		}

		MOT_TSEC_CACHE_FLUSH(pTbdBase[index].bdAddr, pTbdBase[index].bdLen);

		index++;
		index &= mask;
		pDrvCtrl->tbdIndex = index;
		pDrvCtrl->tbdFree--;

		if (pDrvCtrl->tbdFree < pDrvCtrl->txStallThresh)
			motTsecTbdClean(pDrvCtrl);

		END_TX_SEM_GIVE(&pDrvCtrl->endObj);

		return OK;
	}

	/* get the number of valid mblks in the chain */
	fragNum = 0;
	pCurr = pMblk;

	while (pCurr) {
		if (pCurr->mBlkHdr.mLen) {
			fragNum++;
		}
		pCurr = pCurr->mBlkHdr.mNext;
	}

	pCurr = pMblk;

	/* check we have enough resources */

	if (pDrvCtrl->tbdFree >= fragNum) {
		/* save the first TBD index */
		first = index;

		/* calculate the last fragments index */
		last = (index + (fragNum - 1)) & mask;

		tBufList[last] = pMblk;

		/* process the number of non zero fragments */

		bdStat = MOT_TSEC_TBD_I;

		while (fragNum) {
			fragNum--;

			/*
			 * Set the ready bit only if there is one in the chain
			 * or if this is not the first in the chain
			 */

			if (index != first) {
				bdStat |= MOT_TSEC_TBD_R;

				/* fragment length */
				pTbdBase[index].bdLen = pCurr->mBlkHdr.mLen;

				/* fragment address */
				pTbdBase[index].bdAddr = (UINT32) pCurr->mBlkHdr.mData;

				/* flush the cache, cache and tx memory must agree */
				MOT_TSEC_CACHE_FLUSH(pTbdBase[index].bdAddr,
									 pTbdBase[index].bdLen);

				/* pTbdBase[index].bdStat |= MOT_TSEC_TBD_R; */
			}
			/* Check for wrap  */

			if (index == mask) {
				pTbdBase[index].bdStat = bdStat | MOT_TSEC_TBD_W;
			}
			else {
				pTbdBase[index].bdStat = bdStat;
			}

			index++;
			index &= mask;
			pDrvCtrl->tbdFree--;

			/* get the next mBlk in the chain */
			pCurr = (M_BLK *) pCurr->mBlkHdr.mNext;
		}

		pDrvCtrl->tbdIndex = index;

		pTbdBase[first].bdAddr = (UINT32) pMblk->mBlkHdr.mData;
		pTbdBase[first].bdLen = pMblk->mBlkHdr.mLen;

		/* flush the cache, cache and tx memory must agree */
		MOT_TSEC_CACHE_FLUSH(pTbdBase[first].bdAddr, pTbdBase[first].bdLen);

		pTbdBase[last].bdStat |= (MOT_TSEC_TBD_L | MOT_TSEC_TBD_TC);

		/* Check for wrap  */

		if (first == mask) {
			pTbdBase[first].bdStat |= (MOT_TSEC_TBD_W |
									   MOT_TSEC_TBD_I | MOT_TSEC_TBD_R);
		}
		else {
			pTbdBase[first].bdStat |= (MOT_TSEC_TBD_I | MOT_TSEC_TBD_R);
		}
	}
	else {
		if (pDrvCtrl->tbdFree == 0) {
			/* No descriptors bail out */
			END_TX_SEM_GIVE(&pDrvCtrl->endObj);

			return (END_ERR_BLOCK);
		}
		else {
			/* Coalesce packet */

			if ((pNewMblk = netTupleGet(pDrvCtrl->endObj.pNetPool,
										MOT_TSEC_CL_SIZE, M_DONTWAIT,
										MT_DATA, 0)) == NULL) {
				/* Can't get a tuple */
				MOT_TSEC_LOG(MOT_TSEC_DBG_RX_ERR,
							 "motTsecSend netTupleGet failed\n", 1, 2, 3, 4, 5,
							 6);

				END_TX_SEM_GIVE(&pDrvCtrl->endObj);
				return (END_ERR_BLOCK);
			}
			else {
				len = netMblkToBufCopy(pCurr, pNewMblk->mBlkHdr.mData, NULL);

				netMblkClChainFree(pCurr);

				if (len) {
					fragNum = 1;

					pNewMblk->mBlkPktHdr.len = pNewMblk->mBlkHdr.mLen = len;
					pNewMblk->mBlkHdr.mFlags |= M_PKTHDR;

					MOT_TSEC_CACHE_FLUSH(pNewMblk->pClBlk->clNode.pClBuf,
										 pNewMblk->mBlkHdr.mLen);

					motTsecPktTransmit(pDrvCtrl, pNewMblk, fragNum);
				}
				else {
					netMblkClChainFree(pNewMblk);

					END_TX_SEM_GIVE(&pDrvCtrl->endObj);
					return (ERROR);
				}
			}
		}
	}

	END_TX_SEM_GIVE(&pDrvCtrl->endObj);

	MOT_TSEC_LOG(MOT_TSEC_DBG_TRACE_TX, "motTsecSend <EXIT>\n", 1, 2, 3, 4, 5,
				 6);

	return (OK);
}

/******************************************************************************
*
* motTsecTbdClean - clean the transmit buffer descriptors ring
*
* This routine cleans the transmit buffer descriptors ring.
*
* RETURNS: N/A.
*
* ERRNO
*/
LOCAL int motTsecTbdClean(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	TSEC_BD *pTbd = pDrvCtrl->pTbdBase;
	M_BLK **tBufList = pDrvCtrl->tBufList;
	UINT16 tbdFree = pDrvCtrl->tbdFree;
	UINT32 index = pDrvCtrl->tbdCleanIndex;
	UINT32 mask = pDrvCtrl->tbdMask;

	MOT_TSEC_LOG(MOT_TSEC_DBG_TRACE, "motTsecTbdClean\n", 1, 2, 3, 4, 5, 6);


	while ((tbdFree < pDrvCtrl->tbdNum) &&
		   (!(pTbd[index].bdStat & MOT_TSEC_TBD_R)))
	{
		/* Free the Mblk chain */


		if (tBufList[index] != NULL) {
			netMblkClChainFree(tBufList[index]);
			tBufList[index] = NULL;
		}


		/* Inc the free count  */
		tbdFree++;
		index++;
		index &= mask;
	}

	pDrvCtrl->tbdCleanIndex = index;
	pDrvCtrl->tbdFree = tbdFree;

	return (pDrvCtrl->tbdFree);
}



/**************************************************************************
*
* motTsecTbdInit - initialize the transmit buffer ring
*
* This routine initializes the transmit buffer descriptors ring.
*
* SEE ALSO: motTsecRbdInit()
*
* RETURNS: OK, or ERROR.
*
* ERRNO
*/
LOCAL STATUS motTsecTbdInit(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	UINT16 ix;					/* a counter */
	TSEC_BD *pTbd;				/* generic TBD pointer */
	M_BLK_ID pMblk;

	/* carve up the shared-memory region */
	pTbd = (TSEC_BD *) pDrvCtrl->pBdBase;

	MOT_TSEC_LOG(MOT_TSEC_DBG_START, "motTsecTbdInit\n", 1, 2, 3, 4, 5, 6);

	pDrvCtrl->pTbdBase = pTbd;
	pDrvCtrl->pTbdNext = pDrvCtrl->pTbdBase;
	pDrvCtrl->pTbdLast = pDrvCtrl->pTbdBase;
	pDrvCtrl->tbdFree = pDrvCtrl->tbdNum;

	for (ix = 0; ix < pDrvCtrl->tbdNum; ix++) {
		if (pDrvCtrl->tBufList[ix] != NULL)
			netMblkClChainFree(pDrvCtrl->tBufList[ix]);

		pDrvCtrl->tBufList[ix] = NULL;
		pTbd->bdStat = 0;
		pTbd->bdLen = 0;
		pTbd->bdAddr = 0;
		++pTbd;
	}

	/* set the wrap bit in last BD */
	--pTbd;
	pTbd->bdStat = MOT_TSEC_TBD_W;

	/* set stall variables to default values */
	pDrvCtrl->txStall = FALSE;

	/* wait for 75% available before restart MUX */
	pDrvCtrl->txUnStallThresh = pDrvCtrl->tbdNum - (pDrvCtrl->tbdNum / 4);

	/* wait for 25% available to enabling INTS */
	pDrvCtrl->txStallThresh = pDrvCtrl->tbdNum / 4;

	/* get a cluster buffer from the pool */

	if (pDrvCtrl->pTxPollMblk == NULL) {
		if ((pMblk = netTupleGet(pDrvCtrl->endObj.pNetPool, MOT_TSEC_CL_SIZE,
								 M_DONTWAIT, MT_DATA, 0)) == NULL)
			return ERROR;

		pDrvCtrl->pTxPollMblk = pMblk;
		pDrvCtrl->pTxPollBuf = (UCHAR *)pMblk->mBlkHdr.mData;
	}

	return OK;
}

/******************************************************************************
* motTsecDumpTxRing - Show the Transmit Ring details
*
* This routine displays the transmit ring descriptors.
*
* SEE ALSO: motTsecDumpRxRing()
*
* RETURNS: N/A
*
* ERRNO
*/

void motTsecDumpRxRing(
	int tsecNum
) {
	TSEC_BD *pTestRbd;
	TSEC_DRV_CTRL *pDrvCtrl;
	unsigned int i;
	int index;

	if ((pDrvCtrl = (TSEC_DRV_CTRL *) endFindByName("mottsec", tsecNum))
		== NULL)
		return;

	pTestRbd = pDrvCtrl->pRbdBase;

	index = (((UINT32) pDrvCtrl->pRbdNext - (UINT32) pDrvCtrl->pRbdBase) /
			 MOT_TSEC_BD_SIZE);


	printf
		("motTsecDumpRxRing: Current descriptor index %d at %p status 0x%x\n",
		 index, pDrvCtrl->pRbdNext, pDrvCtrl->pRbdNext->bdStat);

	for (i = 0; i < pDrvCtrl->rbdNum; i++) {
		pTestRbd = (TSEC_BD *) ((UINT32) pDrvCtrl->pRbdBase +
								(i * MOT_TSEC_BD_SIZE));

		printf("motTsecDumpRxRing: index %d pRxDescriptor %p status 0x%x \n",
			   i, pTestRbd, pTestRbd->bdStat);
	}
}

/******************************************************************************
*
* motTsecHandleRXFrames - task-level receive frames processor
*
* This routine is run in netTask's context.
*
* RETURNS: N/A
*
* ERRNO
*/
LOCAL void motTsecHandleRXFrames(
#ifdef USE_NET_JOB_ADD
	TSEC_DRV_CTRL * pDrvCtrl
#else
	QJOB * pRxJob
#endif
) {
#ifndef USE_NET_JOB_ADD
	TSEC_DRV_CTRL *pDrvCtrl = member_to_object (pRxJob, TSEC_DRV_CTRL, rxJob);
#endif
	M_BLK_ID pMblk;				/* MBLK to send upstream */
	M_BLK_ID pNewMblk;			/* MBLK to send upstream */
	TSEC_BD *rbd = pDrvCtrl->pRbdBase;
	UINT32 mask = pDrvCtrl->rbdMask;
	UINT32 index = pDrvCtrl->rbdIndex;
	int loopCounter = pDrvCtrl->maxRxFrames;

	TSEC_REG_T *tsecReg = pDrvCtrl->tsecRegsPtr;

	MOT_TSEC_LOG(MOT_TSEC_DBG_TRACE, "motTsecHandleRXFrames <ENTER>\n",
				 0, 0, 0, 0, 0, 0);

	while ((!(rbd[index].bdStat & MOT_TSEC_RBD_E)) && (--loopCounter != 0)) {
		pMblk = pDrvCtrl->pMblkList[index];

		/* allocate new tuple  */

		if ((pNewMblk = netTupleGet(pDrvCtrl->endObj.pNetPool,
									MOT_TSEC_CL_SIZE, M_DONTWAIT, MT_DATA,
									0)) == NULL) {
			MOT_TSEC_LOG(MOT_TSEC_DBG_RX, "Rx Mblk Error\n", 0, 0, 0, 0, 0, 0);

			MOT_TSEC_LOG(MOT_TSEC_DBG_RX_ERR,
						 "motTsecHandleRXFrames ALLOC Error MBLK %x"
						 " CLBLK %x\n", (int) pMblk, 2, 3, 4, 5, 6);

			/* close the bds, pass RBD ownwership to the TSEC */
			rbd[index].bdStat |= MOT_TSEC_RBD_E;

			break;
		}
		pDrvCtrl->pMblkList[index] = pNewMblk;

		rbd[index].bdAddr = (UINT32) pNewMblk->mBlkHdr.mData;

		/* adjust length */
		pMblk->mBlkHdr.mLen = (rbd[index].bdLen - MII_CRC_LEN) & ~0xc000;
		pMblk->mBlkPktHdr.len = pMblk->mBlkHdr.mLen;

		/* set the packet header flag */
		pMblk->mBlkHdr.mFlags |= M_PKTHDR;

		/* close the bds, pass ownership to the CP */
		rbd[index].bdStat |= MOT_TSEC_RBD_E;

		MOT_TSEC_LOG(MOT_TSEC_DBG_TRACE_RX,
					 "motTsec next BD 0x%08x\n",
					 (int) pDrvCtrl->pRbdNext, 2, 3, 4, 5, 6);

		/* increment Ring Index */
		index++;
		index &= mask;

		/* Store current Ring Index */
		pDrvCtrl->rbdIndex = index;

	/* send the frame to the upper layer */
		END_RCV_RTN_CALL(&pDrvCtrl->endObj, pMblk);

	}							/* end of while loop */

	if ((pDrvCtrl->pollCnt < pDrvCtrl->pollLoops) ||
		(!(rbd[pDrvCtrl->rbdIndex].bdStat & MOT_TSEC_RBD_E)))
	{
		if (rbd[pDrvCtrl->rbdIndex].bdStat & MOT_TSEC_RBD_E)
			pDrvCtrl->pollCnt++;
		else
			pDrvCtrl->pollCnt = 0;

		pDrvCtrl->pollDone = FALSE;

		/* Put this job back on the netJobRing and leave */

#ifdef USE_NET_JOB_ADD
		if ((NET_JOB_ADD((FUNCPTR) motTsecHandleRXFrames, (int) pDrvCtrl,
					   0, 0, 0, 0)) == ERROR) {
			/* Very bad!! The stack is now probably corrupt. */

			logMsg("The netJobRing is full. \n", 0, 0, 0, 0, 0, 0);

			MOT_TSEC_IEVENT_REG = MOT_TSEC_IEVENT_RXF0;
			MOT_TSEC_IMASK_REG |= MOT_TSEC_IEVENT_RXF0;

			return;
		}
#else
		jobQueuePost (pDrvCtrl->jobQueueId, &pDrvCtrl->rxJob);
#endif
	}
	else {
		pDrvCtrl->pollCnt = 0;
		pDrvCtrl->pollDone = TRUE;
#ifdef USE_NET_JOB_ADD
		pDrvCtrl->rxJobQued = FALSE;
#else
		QJOB_CLEAR_BUSY (&pDrvCtrl->rxJob);
#endif
	}

	/* Unmask read interrupt */

	if (pDrvCtrl->pollDone) {
		/*
		 * Event register is cleared here to prevent latent interrupts
		 * for packets that have already been serviced
		 */

		MOT_TSEC_IEVENT_REG = MOT_TSEC_IEVENT_RXF0;
		MOT_TSEC_IMASK_REG |= MOT_TSEC_IEVENT_RXF0;
	}

	MOT_TSEC_LOG(MOT_TSEC_DBG_TRACE_RX, "motTsecHandleRXFrames <EXIT>\n",
				 1, 2, 3, 4, 5, 6);
}

/**************************************************************************
*
* motTsecRbdInit - initialize the receive buffer ring
*
* This routine initializes the receive buffer descriptors ring.
*
* SEE ALSO: motTsecTbdInit()
*
* RETURNS: OK, or ERROR if not enough clusters were available.
*
* ERRNO
*/
LOCAL STATUS motTsecRbdInit(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	TSEC_BD *pRbd;				/* generic rbd pointer */
	UINT16 ix;					/* a counter */

	MOT_TSEC_LOG(MOT_TSEC_DBG_START, "motTsecRbdInit\n", 1, 2, 3, 4, 5, 6);

	/* point allocation bd pointers to the first bd */
	pDrvCtrl->pRbdLast = pDrvCtrl->pRbdNext = pDrvCtrl->pRbdBase;

	/* clear out the ring */
	pRbd = pDrvCtrl->pRbdBase;

	for (ix = 0; ix < pDrvCtrl->rbdNum; ix++) {
		pRbd->bdStat = MOT_TSEC_RBD_E | MOT_TSEC_RBD_I;
		pRbd->bdLen = 0;

		if (pDrvCtrl->pMblkList[ix] != NULL)
			netMblkClChainFree(pDrvCtrl->pMblkList[ix]);

		if ((pDrvCtrl->pMblkList[ix] = netTupleGet(pDrvCtrl->endObj.pNetPool,
												   MOT_TSEC_CL_SIZE,
												   M_DONTWAIT, MT_DATA, 0))
			== NULL) {
			return (ERROR);
		}

		pRbd->bdAddr = (UINT32) pDrvCtrl->pMblkList[ix]->mBlkHdr.mData;

		/* inc pointer to the next bd */
		++pRbd;
	}

	/* set the last bd to wrap */
	--pRbd;
	pRbd->bdStat |= MOT_TSEC_RBD_W;

	/* initially replenish all */

	pDrvCtrl->rbdCnt = 0;

	/* fill the bds with valid clusters and turn them on to the CP */

	return (OK);
}

/**************************************************************************
*
* motTsecAddrSet - set an address to the MACSTNADDR register
*
* This routine writes the address pointed to by <pAddr> to some TSEC's internal
* register. It may be used to set the individual physical address, or
* an address to be added to the hash table filter. It assumes that <offset>
* is the offset in the parameter RAM to the highest-order halfword that
* will contain the address.
*
* RETURNS: OK, always.
*
* ERRNO
*/

LOCAL STATUS motTsecAddrSet(
	TSEC_DRV_CTRL * pDrvCtrl,	/* pointer to TSEC_DRV_CTRL structure */
	char *pAddr					/* address to write to register */
) {
	MOT_TSEC_FRAME_SET(pDrvCtrl);

	MOT_TSEC_MACSTNADDR1_REG =
		((int) pAddr[5] << 24) + ((int) pAddr[4] << 16) +
		((int) pAddr[3] << 8) + ((int) pAddr[2]);

	MOT_TSEC_MACSTNADDR2_REG = ((int) pAddr[1] << 24) + ((int) pAddr[0] << 16);

	return (OK);
}

/*******************************************************************************
*
* motTsecIoctl - interface ioctl procedure
*
* Process an interface ioctl request.
*
* RETURNS: OK, or ERROR.
*
* ERRNO
*/

LOCAL STATUS motTsecIoctl(
	void *pCookie,				/* pointer to TSEC_DRV_CTRL structure */
	int cmd,					/* command to process */
	caddr_t data				/* pointer to data */
) {
	TSEC_DRV_CTRL *pDrvCtrl;
	int error = OK;
	INT8 savedFlags;
	long value;
	END_OBJ *pEndObj;
	TSEC_REG_T *tsecReg;

	pDrvCtrl = (TSEC_DRV_CTRL *) pCookie;
	pEndObj = &pDrvCtrl->endObj;

	tsecReg = pDrvCtrl->tsecRegsPtr;

	MOT_TSEC_LOG(MOT_TSEC_DBG_TRACE, "motTsecIoctl IN\n", 1, 2, 3, 4, 5, 6);

	switch ((UINT32) cmd) {
	case EIOCSADDR:
		MOT_TSEC_LOG(MOT_TSEC_DBG_IOCTL, "Ioctl EIOCSADDR\n", 1, 2, 3, 4, 5, 6);

		if (data == NULL)
			error = EINVAL;
		else {
			/* Copy and install the new address */

			bcopy((char *) data,
				  (char *) END_HADDR(pDrvCtrl->endObj),
				  END_HADDR_LEN(pDrvCtrl->endObj));

			/* gracefully stop the TSEC */

			motTsecGracefulStop(pDrvCtrl);

			/* set the  ethernet address in the TSEC */
			motTsecAddrSet(pDrvCtrl, (char *) data);

			/* restart the TSEC */

			motTsecRestart(pDrvCtrl);
		}

		break;

	case EIOCGADDR:
		if (data == NULL)
			error = EINVAL;
		else
			bcopy((char *) END_HADDR(pDrvCtrl->endObj),
				  (char *) data, END_HADDR_LEN(pDrvCtrl->endObj));

		MOT_TSEC_LOG(MOT_TSEC_DBG_IOCTL,
					 "Ioctl EIOCGADDR %02x%02x%02x%02x%02x%02x\n",
					 data[0], data[1], data[2], data[3], data[4], data[5]);


		break;

	case EIOCSFLAGS:
		MOT_TSEC_LOG(MOT_TSEC_DBG_IOCTL, "Ioctl EIOCSFLAGS\n", 1, 2, 3, 4, 5, 6);
		value = (long) data;
		if (value < 0) {
			value = -value;
			value--;
			END_FLAGS_CLR(pEndObj, value);
		}
		else
			END_FLAGS_SET(pEndObj, value);


		/* handle IFF_PROMISC */

		savedFlags = MOT_TSEC_FLAG_GET();
		if (END_FLAGS_ISSET(IFF_PROMISC)) {
			MOT_TSEC_FLAG_SET(MOT_TSEC_PROM);

			if ((MOT_TSEC_FLAG_GET() != savedFlags) &&
				(END_FLAGS_GET(pEndObj) & IFF_UP)) {
				/* config down */

				END_FLAGS_CLR(pEndObj, IFF_UP | IFF_RUNNING);
				/* add: option to set mode */

				/* config up */

				END_FLAGS_SET(pEndObj, IFF_UP | IFF_RUNNING);
			}
		}
		else
			MOT_TSEC_FLAG_CLEAR(MOT_TSEC_PROM);

		/* handle IFF_ALLMULTI */

		motTsecHashTblPopulate (pDrvCtrl);

		break;

	case EIOCGFLAGS:
		MOT_TSEC_LOG(MOT_TSEC_DBG_IOCTL, "Ioctl EIOCGFLAGS\n", 1, 2, 3, 4, 5, 6);
		if (data == NULL)
			error = EINVAL;
		else
			*(long *) data = END_FLAGS_GET(pEndObj);

		break;

	case EIOCMULTIADD:
		MOT_TSEC_LOG(MOT_TSEC_DBG_IOCTL, "Ioctl EIOCMULTIADD\n", 1, 2, 3, 4, 5, 6);
		error = motTsecMCastAddrAdd(pDrvCtrl, (UCHAR *) data);
		break;

	case EIOCMULTIDEL:
		MOT_TSEC_LOG(MOT_TSEC_DBG_IOCTL, "Ioctl EIOCMULTIDEL\n", 1, 2, 3, 4, 5, 6);
		error = motTsecMCastAddrDel(pDrvCtrl, (UCHAR *) data);
		break;

	case EIOCMULTIGET:
		error = motTsecMCastAddrGet(pDrvCtrl, (MULTI_TABLE *) data);
		break;

	case EIOCPOLLSTART:
		MOT_TSEC_LOG(MOT_TSEC_DBG_IOCTL, "Ioctl EIOCPOLLSTART\n", 1, 2, 3, 4, 5, 6);
		motTsecPollStart(pDrvCtrl);
		break;

	case EIOCPOLLSTOP:
		MOT_TSEC_LOG(MOT_TSEC_DBG_IOCTL, "Ioctl EIOCPOLLSTOP\n", 1, 2, 3, 4, 5, 6);
		motTsecPollStop(pDrvCtrl);
		break;

#ifdef INCLUDE_RFC_2233

	case EIOCGMIB2233:
		MOT_TSEC_LOG(MOT_TSEC_DBG_IOCTL, "Ioctl EIOCGMIB2233\n", 1, 2, 3, 4, 5, 6);
		if ((data == NULL) || (pEndObj->pMib2Tbl == NULL))
			error = EINVAL;
		else
			*((M2_ID **) data) = pEndObj->pMib2Tbl;
		break;
#else
	case EIOCGMIB2:
		if (data == NULL)
			error = EINVAL;
		else {
			MOT_TSEC_LOG(MOT_TSEC_DBG_IOCTL, "Ioctl EIOCGMIB2\n", 1, 2, 3, 4, 5, 6);

			bcopy((char *) &pDrvCtrl->endObj.mib2Tbl, (char *) data,
				  sizeof(M2_INTERFACETBL));
		}
		break;

#endif

	default:
		MOT_TSEC_LOG(MOT_TSEC_DBG_IOCTL, "Ioctl UNKNOWN\n", 1, 2, 3, 4, 5, 6);
		error = EINVAL;
	}

	MOT_TSEC_LOG(MOT_TSEC_DBG_TRACE, "motTsecIoctl OUT\n", 1, 2, 3, 4, 5, 6);

	return (error);
}

/**************************************************************************
*
* motTsecMiiPhyRead - read the MII interface
*
* This routine reads the register specified by <phyReg> in the PHY device
* whose address is <phyAddr>. The value read is returned in the location
* pointed to by <miiData>.
*
* RETURNS: OK, or ERROR.
*
* ERRNO
*/
LOCAL STATUS motTsecMiiPhyRead(
	TSEC_DRV_CTRL * pDrvCtrl,
	UINT8 phyAddr,
	UINT8 regAddr,
	UINT16 * value
) {
	UINT16 phyValue;
	int i = 0;

	MOT_TSEC_MII_FRAME_SET(pDrvCtrl);

	CACHE_PIPE_FLUSH();
	MOT_TSEC_MIIMADD_REG = MOT_TSEC_MIIMADD_PHYADRS(phyAddr) |
		MOT_TSEC_MIIMADD_REGADRS(regAddr);

	MOT_TSEC_MIIMCOM_REG = 0;
	CACHE_PIPE_FLUSH();

	MOT_TSEC_MIIMCOM_REG = MOT_TSEC_MIIMCOM_READ_CYCLE;
	CACHE_PIPE_FLUSH();

	/*taskDelay((sysClkRateGet() / 30) + 1); */

	while (MOT_TSEC_MIIMIND_REG & (MOT_TSEC_MIIMIND_NOT_VALID |
								   MOT_TSEC_MIIMIND_BUSY)) {
		sysUsDelay(100);
		if (++i > 10000) /* terminate at 10000 iterations */
			break;
	}

	phyValue = MOT_TSEC_MIIMSTAT_PHY(MOT_TSEC_MIIMSTAT_REG);

	MOT_TSEC_LOG(MOT_TSEC_DBG_MII,
				 "motTsecMiiPhyRead PHY(%d) REG(%d)=0x%04x\n", (int) phyAddr,
				 regAddr, phyValue, 4, 5, 6);

	*value = phyValue;
	return (OK);
}

/**************************************************************************
*
* motTsecMiiPhyWrite - write to the MII register
*
* This routine writes the register specified by <phyReg> in the PHY device,
* whose address is <phyAddr>, with the 16-bit value included in <miiData>.
*
* SEE ALSO: motTsecMiiPhyRead()
*
* RETURNS: OK, or ERROR.
*
* ERRNO
*/
LOCAL STATUS motTsecMiiPhyWrite(
	TSEC_DRV_CTRL * pDrvCtrl,
	UINT8 phyAddr,
	UINT8 regAddr,
	UINT16 value
) {
	int i = 0;

	MOT_TSEC_MII_FRAME_SET(pDrvCtrl);

	MOT_TSEC_LOG(MOT_TSEC_DBG_MII, "motTsecMiiWrite PHY(%d) REG(%d)=0x%04x\n",
				 phyAddr, regAddr, value, 4, 5, 6);

	MOT_TSEC_MIIMIND_REG = 0;

	MOT_TSEC_MIIMADD_REG = MOT_TSEC_MIIMADD_PHYADRS(phyAddr) |
		MOT_TSEC_MIIMADD_REGADRS(regAddr);

	CACHE_PIPE_FLUSH();
	MOT_TSEC_MIIMCON_REG = MOT_TSEC_MIIMCON_PHY_CTRL(value);
	CACHE_PIPE_FLUSH();

	/*taskDelay((sysClkRateGet() / 30) + 1); */

	while (MOT_TSEC_MIIMIND_REG & MOT_TSEC_MIIMIND_BUSY) {
		sysUsDelay(100);
		if (++i > 10000) /* terminate at 10000 iterations */
			break;
	}

	return (OK);
}

/**************************************************************************
*
* motTsecPhyPreInit - initialize some fields in the phy info structure
*
* This routine initializes some fields in the phy info structure,
* for use of the phyInit() routine.
*
* RETURNS: OK, or ERROR if could not obtain memory.
*
* ERRNO
*/

LOCAL STATUS motTsecPhyPreInit(
	TSEC_DRV_CTRL * pDrvCtrl	/* pointer to TSEC_DRV_CTRL structure */
) {
	/*TSEC_REG_T *tsecReg = pDrvCtrl->tsecRegsPtr;*/

	MOT_TSEC_LOG(MOT_TSEC_DBG_START, "motTsecPhyPreInit\n", 1, 2, 3, 4, 5, 6);

	/* set some default values */

	pDrvCtrl->phyInfo->pDrvCtrl = (void *) pDrvCtrl;

	/* in case of link failure, set a default mode for the PHY */

	pDrvCtrl->phyInfo->phyFlags = MII_PHY_DEF_SET;

	pDrvCtrl->phyInfo->phyWriteRtn = (FUNCPTR) motTsecMiiPhyWrite;
	pDrvCtrl->phyInfo->phyReadRtn = (FUNCPTR) motTsecMiiPhyRead;
	pDrvCtrl->phyInfo->phyDelayRtn = (FUNCPTR) taskDelay;
	pDrvCtrl->phyInfo->phyLinkDownRtn = (FUNCPTR) motTsecHandleLSCJob;

	/* handle some user-to-physical flags */
	if (MOT_TSEC_USR_FLAG_ISSET(MOT_TSEC_USR_PHY_TBL))
		pDrvCtrl->phyInfo->phyFlags |= MII_PHY_TBL;
	else
		pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_TBL;

	if (!(MOT_TSEC_USR_FLAG_ISSET(MOT_TSEC_USR_PHY_NO_100)))
		pDrvCtrl->phyInfo->phyFlags |= MII_PHY_100;
	else
		pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_100;

	if (!(MOT_TSEC_USR_FLAG_ISSET(MOT_TSEC_USR_PHY_NO_FD)))
		pDrvCtrl->phyInfo->phyFlags |= MII_PHY_FD;
	else
		pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_FD;

	if (!(MOT_TSEC_USR_FLAG_ISSET(MOT_TSEC_USR_PHY_NO_10)))
		pDrvCtrl->phyInfo->phyFlags |= MII_PHY_10;
	else
		pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_10;

	if (!(MOT_TSEC_USR_FLAG_ISSET(MOT_TSEC_USR_PHY_NO_HD)))
		pDrvCtrl->phyInfo->phyFlags |= MII_PHY_HD;
	else
		pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_HD;

	if (MOT_TSEC_USR_FLAG_ISSET(MOT_TSEC_USR_PHY_MON))
		pDrvCtrl->phyInfo->phyFlags |= MII_PHY_MONITOR;
	else
		pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_MONITOR;


	/* MEN board specific */
	if (MOT_TSEC_USR_FLAG_ISSET(MOT_TSEC_USR_MODE_GMII)) {
		pDrvCtrl->phyInfo->phyFlags |= MII_PHY_1000T_FD;

		/*pDrvCtrl->phyInfo->phyAddr = MOT_TSEC_TBIPA_REG; kp???*/
	}
	else if( MOT_TSEC_USR_FLAG_ISSET(MOT_TSEC_USR_MODE_MII)) {

	}
	else if( MOT_TSEC_USR_FLAG_ISSET(MOT_TSEC_USR_MODE_TBI)) {
		pDrvCtrl->phyInfo->phyFlags |= (MII_PHY_1000T_FD | MII_PHY_GMII_TYPE);
	}


	if (!(MOT_TSEC_USR_FLAG_ISSET(MOT_TSEC_USR_PHY_NO_AN))) {
		pDrvCtrl->phyInfo->phyFlags |= MII_PHY_AUTO;

		/* printf("motTsecPhyPreInit: !MOT_TSEC_USR_PHY_NO_AN phyFlags 0x%x\n",
			   pDrvCtrl->phyInfo->phyFlags); */
	}
	else {
		/* printf("motTsecPhyPreInit: MOT_TSEC_USR_PHY_NO_AN phyFlags 0x%x\n",
			   pDrvCtrl->phyInfo->phyFlags); */

		pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_AUTO;
	}

	/* mark it as pre-initialized */
	pDrvCtrl->phyInfo->phyFlags |= MII_PHY_PRE_INIT;

	return (OK);
}

/******************************************************************************
*
* motTsecConfigForSpeed - configure MACCFG2 according to speed (kp)
*
* RETURNS: N/A
*
*/
LOCAL void motTsecConfigForSpeed(
	TSEC_DRV_CTRL * pDrvCtrl,
	MOT_TSEC_PHY_STATUS *pStatus )
{
	int intLevel;				/* current intr level */
	UINT32 val, interface;

	MOT_TSEC_FRAME_SET(pDrvCtrl);

	switch( pStatus->speed ){
	case MOT_TSEC_PHY_SPEED_10:
		interface = MOT_TSEC_MACCFG2_IF_MODE_MII;
		break;
	case MOT_TSEC_PHY_SPEED_100:
		interface = MOT_TSEC_MACCFG2_IF_MODE_MII;
		break;
	case MOT_TSEC_PHY_SPEED_1000:
		interface = MOT_TSEC_MACCFG2_IF_MODE_GMII_TBI;
		break;
	default:
		interface = MOT_TSEC_MACCFG2_IF_MODE_MII;
		break;
	}


	intLevel = intLock();
	val = MOT_TSEC_MACCFG2_REG;

	val &= ~(MOT_TSEC_MACCFG2_FULL_DUPLEX |
			 MOT_TSEC_MACCFG2_IF_MODE( MOT_TSEC_MACCFG2_IF_MODE_MASK ));

	if (pStatus->duplex == MOT_TSEC_PHY_DUPLEX_FULL) {
		val |= MOT_TSEC_MACCFG2_FULL_DUPLEX;
	}

	val |= MOT_TSEC_MACCFG2_IF_MODE( interface );

	MOT_TSEC_MACCFG2_REG = val;
	intUnlock(intLevel);

	/* ECNTRL register settings for reduced interfaces */
	intLevel = intLock();
	val = MOT_TSEC_ECNTRL_REG;
	if (pStatus->speed == MOT_TSEC_PHY_SPEED_10) {
		val &= ~MOT_TSEC_ECNTRL_R100M;
		/* speed = 0; */
	} else if (pStatus->speed == MOT_TSEC_PHY_SPEED_100) {
		val |= MOT_TSEC_ECNTRL_R100M;
		/* speed = 1; */
	} else if (pStatus->speed == MOT_TSEC_PHY_SPEED_1000) {
		val |= MOT_TSEC_ECNTRL_R100M; /* don't care */
		/* speed = 2; */
	}
	MOT_TSEC_ECNTRL_REG = val;
	intUnlock(intLevel);


	MOT_TSEC_LOG(MOT_TSEC_DBG_MII, "MACCFG2 now %08x\n", MOT_TSEC_MACCFG2_REG, 2,3,4,5,6 );
}

/*******************************************************************************
*
* motTsecHandleLSCJob - task-level link status event processor
*
* This routine is run in netTask's context.
*
* RETURNS: N/A
*
* ERRNO
*/
LOCAL void motTsecHandleLSCJob(
	TSEC_DRV_CTRL * pDrvCtrl)
{
	int intLevel;				/* current intr level */
	UINT16 miiStat;
	int retVal;
	int link;
	MOT_TSEC_PHY_STATUS status;

	status.speed = 0;
	status.duplex = 0;
	link = 0;

	MOT_TSEC_LOG(MOT_TSEC_DBG_ANY, "motTsecHandleLSCJob: \n", 0, 0, 0, 0, 0, 0);

	/* read MII status register once to unlatch link status bit */

	retVal = motTsecMiiPhyRead(pDrvCtrl, pDrvCtrl->phyInfo->phyAddr, MII_STAT_REG, &miiStat);

	/* read again to know it's current value */

	retVal = motTsecMiiPhyRead(pDrvCtrl, pDrvCtrl->phyInfo->phyAddr, MII_STAT_REG, &miiStat);


	if (miiStat & MII_SR_LINK_STATUS) {	/* if link is now up */
		link = 1;
		if (pDrvCtrl->phyStatusFunc) {
			/* get duplex mode from BSP */
			retVal = pDrvCtrl->phyStatusFunc(pDrvCtrl->phyInfo, &status);

		}
		/* reconfigure MAC for new speed */
		motTsecConfigForSpeed( pDrvCtrl, &status );

	}

	MOT_TSEC_LOG(MOT_TSEC_DBG_MII,
				 "TSEC %d Unit %d Link %s @ %s BPS %s Duplex\n",
				 pDrvCtrl->tsecNum, pDrvCtrl->unit, (int) linkStr[link],
				 (int) speedStr[status.speed], (int) duplexStr[status.duplex],
				 6);

	intLevel = intLock();
	pDrvCtrl->lscHandling = FALSE;
	intUnlock(intLevel);

}

/******************************************************************************
*
* motTsecPhyLSCInt - Line Status Interrupt
*
* This interrupt handler adds a job when there is a change in the Phy status.
*
* RETURNS: None
*
* ERRNO
*/

LOCAL void motTsecPhyLSCInt(
	TSEC_DRV_CTRL * pDrvCtrl
) {
	/*
	 * There's no reason to schedule the link status change job
	 * If the BSP didn't supply a FUNC for getting duplex mode.
	 *
	 */

	MOT_TSEC_LOG(MOT_TSEC_DBG_INT, "motTsecPhyLSCInt\n", 1, 2, 3, 4, 5, 6);

	/* if no link status change job is pending */

	if (!pDrvCtrl->lscHandling) {
		/* and the BSP has supplied a duplex mode function */

		if (pDrvCtrl->phyStatusFunc) {
			if (NET_JOB_ADD( (FUNCPTR) motTsecHandleLSCJob, (int) pDrvCtrl,
							0, 0, 0, 0) == OK) {
				pDrvCtrl->lscHandling = TRUE;
			} else {
				logMsg("The netJobRing is full.\n", 0, 0, 0, 0, 0, 0);
			}
		}
	}
}

/*******************************************************************************
* motTsecMCastAddrAdd - add a multicast address for the device
*
* This routine adds a multicast address to whatever the driver
* is already listening for.
*
* SEE ALSO: motTsecMCastAddrDel() motTsecMCastAddrGet()
*
* RETURNS: OK or ERROR.
*
* ERRNO
*/
LOCAL STATUS motTsecMCastAddrAdd(
	TSEC_DRV_CTRL * pDrvCtrl,	/* pointer to TSEC_DRV_CTRL structure */
	UCHAR * pAddr				/* address to be added */
) {
	int retVal;

	retVal = etherMultiAdd(&pDrvCtrl->endObj.multiList, (char*)pAddr);

	if (retVal == ENETRESET) {
		pDrvCtrl->endObj.nMulti++;

		return (motTsecHashTblPopulate(pDrvCtrl));
	}

	return ((retVal == OK) ? OK : ERROR);
}

/*****************************************************************************
*
* motTsecMCastAddrDel - delete a multicast address for the device
*
* This routine deletes a multicast address from the current list of
* multicast addresses.
*
* SEE ALSO: motTsecMCastAddrAdd() motTsecMCastAddrGet()
*
* RETURNS: OK or ERROR.
*
* ERRNO
*/
LOCAL STATUS motTsecMCastAddrDel(
	TSEC_DRV_CTRL * pDrvCtrl,	/* pointer to TSEC_DRV_CTRL structure */
	UCHAR * pAddr				/* address to be deleted */
) {
	int retVal;

	retVal = etherMultiDel(&pDrvCtrl->endObj.multiList, (char*)pAddr);

	if (retVal == ENETRESET) {
		pDrvCtrl->endObj.nMulti--;

		/*
		 * Rewrite the hash table.
		 */

		retVal = motTsecHashTblPopulate(pDrvCtrl);
	}
	else if (retVal == ENXIO)
		retVal = ERROR;
	else
		retVal = OK;

	return (retVal);
}

/*******************************************************************************
* motTsecMCastAddrGet - get the current multicast address list
*
* This routine returns the current multicast address list in <pTable>
*
* SEE ALSO: motTsecMCastAddrAdd() motTsecMCastAddrDel()
*
* RETURNS: OK or ERROR.
*
* ERRNO
*/
LOCAL STATUS motTsecMCastAddrGet(
	TSEC_DRV_CTRL * pDrvCtrl,	/* pointer to TSEC_DRV_CTRL structure */
	MULTI_TABLE * pTable		/* table into which to copy addresses */
) {
	return (etherMultiGet(&pDrvCtrl->endObj.multiList, pTable));
}

/*******************************************************************************
* motTsecHashTblPopulate - populate the hash table
*
* This routine populates the hash table with the entries found in the
* multicast table. We have to reset the hash registers first, and
* populate them again, as more than one address may be mapped to
* the same bit.
*
* RETURNS: OK, or ERROR.
*
* ERRNO
*/
LOCAL STATUS motTsecHashTblPopulate(
	TSEC_DRV_CTRL * pDrvCtrl	/* pointer to TSEC_DRV_CTRL structure */
) {
	ETHER_MULTI *mCastNode = NULL;

#define ETHER_POLY (0x04C11DB7)
	UINT8 byte;
	UINT8 msb;
	UINT32 crc;
	int count;
	UINT32 hash[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int i;

	MOT_TSEC_FRAME_SET(pDrvCtrl);


	/*
	 * If we're in ALLMULTI mode, just set all the bits in the hash
	 * table. This insures we'll receive multicasts on all groups.
	 */

	if (END_FLAGS_ISSET (IFF_ALLMULTI)) {
		for (i = 0; i < 8; i++)
			MOT_TSEC_GADDR_REG[i] = 0xFFFFFFFF;
		return (OK);
	}

	/* clear the hash table */
	for (i = 0; i < 8; i++)
		MOT_TSEC_GADDR_REG[i] = 0;

	/* scan the multicast address list to re-populate */

	for (mCastNode = (ETHER_MULTI *) lstFirst(&pDrvCtrl->endObj.multiList);
		 mCastNode != NULL;
		 mCastNode = (ETHER_MULTI *) lstNext(&mCastNode->node)) {
		/* add this entry */
		crc = 0xffffffff;
		for (i = 0; i < 6; i++) {
			byte = mCastNode->addr[i];
			for (count = 0; count < 8; count++) {
				if (crc & 0x80000000)
					msb = 1;
				else
					msb = 0;
				crc <<= 1;
				if (msb ^ (byte & 1))
					crc ^= ETHER_POLY;
				byte >>= 1;
			}
		}

		/* The 8 most significant bits determine the hash code. */

		crc = ((~crc) >> 24) & 0x000000ff;

		hash[((crc >> 5))] |= (1 << (crc & 0x1f));
	}

	/*
	 * Note: the group address registers are in reverse order
	 * with respect to our temporary hash array. (Reg 0 == MSB,
	 * reg 7 == LSB)
	 */

	for (i = 0; i < 8; i++)
		MOT_TSEC_GADDR_REG[i] = hash[7 - i];

	return (OK);
}

/*****************************************************************************
*
* motTsecPollSend - transmit a packet in polled mode
*
* This routine is called by a user to try and send a packet on the
* device. It sends a packet directly on the network, without having to
* go through the normal process of queuing a packet on an output queue
* and then waiting for the device to decide to transmit it.
*
* These routine should not call any kernel functions.
*
* SEE ALSO: motTsecPollReceive()
*
* RETURNS: OK or EAGAIN.
*
* ERRNO
*/

LOCAL STATUS motTsecPollSend(
	TSEC_DRV_CTRL * pDrvCtrl,
	M_BLK_ID pMblk
) {
	int retVal = OK;			/* holder for return value */
	int len = 0, tmpLen = 0;	/* length of data to be sent */
	int i = 0;
	UINT32 mask = pDrvCtrl->tbdMask;
	TSEC_BD *pTbd = pDrvCtrl->pTbdBase;


	if (!MOT_TSEC_FLAG_ISSET(MOT_TSEC_POLLING))
		return (EAGAIN);

	MOT_TSEC_LOG(MOT_TSEC_DBG_POLL,
				 "motTsecPollSend <ENTER>\n", 1, 2, 3, 4, 5, 6);

	if (!pMblk)
		return EAGAIN;

	if (pDrvCtrl->tbdFree) {
		/* copy data but do not free the Mblk */

		tmpLen = netMblkToBufCopy(pMblk, (char *) pDrvCtrl->pTxPollBuf, NULL);
		len = max(MOT_TSEC_MIN_TX_PKT_SZ, tmpLen);

		/* flush the cache, if necessary */
		MOT_TSEC_CACHE_FLUSH(pDrvCtrl->pTxPollBuf, len);

		pDrvCtrl->tBufList[pDrvCtrl->tbdIndex] = (M_BLK *) NULL;

		pTbd[pDrvCtrl->tbdIndex].bdAddr = (unsigned long) pDrvCtrl->pTxPollBuf;
		pTbd[pDrvCtrl->tbdIndex].bdLen = len;

		if (pDrvCtrl->tbdIndex == mask) {
			pTbd[pDrvCtrl->tbdIndex].bdStat = (MOT_TSEC_TBD_W |
											   MOT_TSEC_TBD_R |
											   MOT_TSEC_TBD_L |
											   MOT_TSEC_TBD_TC |
											   MOT_TSEC_TBD_PADCRC);
		}
		else {
			pTbd[pDrvCtrl->tbdIndex].bdStat = (MOT_TSEC_TBD_R |
											   MOT_TSEC_TBD_L |
											   MOT_TSEC_TBD_TC |
											   MOT_TSEC_TBD_PADCRC);
		}

		/* Flush the write pipe */
		CACHE_PIPE_FLUSH();

		while (pTbd[pDrvCtrl->tbdIndex].bdStat & MOT_TSEC_TBD_R) {
			if (i++ > 10000)
				break;
		}

		pDrvCtrl->tbdIndex++;
		pDrvCtrl->tbdIndex &= mask;
		pDrvCtrl->tbdFree--;
	}

	motTsecTbdClean(pDrvCtrl);

	MOT_TSEC_LOG(MOT_TSEC_DBG_POLL,
				 "motTsecPollSend <EXIT>\n", 1, 2, 3, 4, 5, 6);

	return retVal;
}

/*******************************************************************************
* motTsecPollReceive - receive a packet in polled mode
*
* This routine is called by a user to try and get a packet from the
* device. It returns EAGAIN if no packet is available. The caller must
* supply a M_BLK_ID with enough space to contain the received packet. If
* enough buffer is not available then EAGAIN is returned.
*
* These routine should not call any kernel functions.
* TSEC events and/or ints must be masked and/or disabled before calling
* this function.
*
* SEE ALSO: motTsecPollSend(), motTsecPollStart(), motTsecPollStop().
*
* RETURNS: OK or EAGAIN.
*
* ERRNO
*/

LOCAL STATUS motTsecPollReceive(
	TSEC_DRV_CTRL * pDrvCtrl,
	M_BLK * pMblk
) {
	int retVal = OK;			/* holder for return value */
	UINT16 rbdStatus;			/* holder for the status of this RBD */
	UINT16 rbdLen;				/* number of bytes received */
	char *pData = NULL;			/* a rx data pointer */
	TSEC_BD *rbd = pDrvCtrl->pRbdBase;
	UINT32 mask = pDrvCtrl->rbdMask;

	if (!MOT_TSEC_FLAG_ISSET(MOT_TSEC_POLLING))
		return (EAGAIN);

	if ((pMblk->mBlkHdr.mFlags & M_EXT) != M_EXT)
		return EAGAIN;

	/* if it's not ready, don't touch it! */

	if ((rbd[pDrvCtrl->rbdIndex].bdStat & MOT_TSEC_RBD_E))
		return EAGAIN;

	MOT_TSEC_LOG(MOT_TSEC_DBG_POLL,
				 "motTsecPollReceive <ENTER>\n", 1, 2, 3, 4, 5, 6);

	rbdStatus = rbd[pDrvCtrl->rbdIndex].bdStat;	/* read the RBD status word */
	rbdLen = rbd[pDrvCtrl->rbdIndex].bdLen;	/* get receive length */


	/* pass the packet up only if reception was Ok &  buffer is large enough */

	if ((rbdStatus & MOT_TSEC_RBD_ERR) || pMblk->mBlkHdr.mLen < rbdLen) {
		retVal = EAGAIN;
	}
	else {
		pData = (char *) rbd[pDrvCtrl->rbdIndex].bdAddr;

		/* set up the mBlk properly */
		pMblk->mBlkHdr.mFlags |= M_PKTHDR;
		pMblk->mBlkHdr.mLen = (rbdLen - MII_CRC_LEN) & ~0xc000;
		pMblk->mBlkPktHdr.len = pMblk->mBlkHdr.mLen;

		/* Make cache consistent with memory */
		MOT_TSEC_CACHE_INVAL((char *) pData, pMblk->mBlkHdr.mLen);
		bcopy((char *) pData, (char *) pMblk->mBlkHdr.mData,
			  ((rbdLen - MII_CRC_LEN) & ~0xc000));
		retVal = OK;
	}

	/* close the bds, pass ownership to the CP */
	rbd[pDrvCtrl->rbdIndex].bdStat |= MOT_TSEC_RBD_E;

	/* increment Ring Index */
	pDrvCtrl->rbdIndex++;
	pDrvCtrl->rbdIndex &= mask;

	MOT_TSEC_LOG(MOT_TSEC_DBG_POLL,
				 "motTsecPollReceive <EXIT>\n", 1, 2, 3, 4, 5, 6);
	return retVal;
}

/*******************************************************************************
* motTsecPollStart - start polling mode
*
* This routine starts polling mode by disabling ethernet interrupts and
* setting the polling flag in the END_CTRL structure.
*
* SEE ALSO: motTsecPollStop()
*
* RETURNS: OK, or ERROR if polling mode could not be started.
*
* ERRNO
*/
LOCAL STATUS motTsecPollStart(
	TSEC_DRV_CTRL * pDrvCtrl	/* pointer to TSEC_DRV_CTRL structure */
) {
	int intLevel;				/* current intr level */

	MOT_TSEC_FRAME_SET(pDrvCtrl);

	END_TX_SEM_TAKE(&pDrvCtrl->endObj, WAIT_FOREVER);

	MOT_TSEC_LOG(MOT_TSEC_DBG_POLL,
				 "motTsecPollStart <ENTER>\n", 1, 2, 3, 4, 5, 6);

	if (MOT_TSEC_FLAG_ISSET(MOT_TSEC_POLLING))
		return ERROR;

	intLevel = intLock();

	pDrvCtrl->intMask = MOT_TSEC_IMASK_REG;

	/* mask tsec(tsecNum) interrupts   */

	MOT_TSEC_IMASK_REG = 0;

	pDrvCtrl->txStall = FALSE;

	MOT_TSEC_FLAG_SET(MOT_TSEC_POLLING);

	intUnlock(intLevel);

	MOT_TSEC_LOG(MOT_TSEC_DBG_POLL,
				 "motTsecPollStart <EXIT>\n", 1, 2, 3, 4, 5, 6);

	END_TX_SEM_GIVE(&pDrvCtrl->endObj);

	return (OK);
}

/*******************************************************************************
* motTsecPollStop - stop polling mode
*
* This routine stops polling mode by enabling ethernet interrupts and
* resetting the polling flag in the END_CTRL structure.
*
* SEE ALSO: motTsecPollStart()
*
* RETURNS: OK, or ERROR if polling mode could not be stopped.
*
* ERRNO
*/

LOCAL STATUS motTsecPollStop(
	TSEC_DRV_CTRL * pDrvCtrl	/* pointer to TSEC_DRV_CTRL structure */
) {
	int intLevel;

	MOT_TSEC_FRAME_SET(pDrvCtrl);

	MOT_TSEC_LOG(MOT_TSEC_DBG_POLL,
				 "motTsecPollStop <ENTER>\n", 1, 2, 3, 4, 5, 6);

	intLevel = intLock();

	/* enable tsec(tsecNum) interrupts */

	MOT_TSEC_IMASK_REG = pDrvCtrl->intMask;

	MOT_TSEC_IEVENT_REG = 0xffffffff;

	pDrvCtrl->txStall = FALSE;

	/* clear poll flag */

	MOT_TSEC_FLAG_CLEAR(MOT_TSEC_POLLING);

	intUnlock(intLevel);

	MOT_TSEC_LOG(MOT_TSEC_DBG_POLL,
				 "motTsecPollStop <EXIT>\n", 1, 2, 3, 4, 5, 6);

	return (OK);
}

#ifdef MOT_TSEC_DBG


void motTsecPhyShow( int tsecNum )
{
	TSEC_DRV_CTRL *pDrvCtrl;
	int i;
	UINT16 value;
	int addr;

	printf( "tsecNum = %d; tsecMaxDevs = %d;\n" );
	if (tsecNum >= MOT_TSEC_MAX_DEVS)
		return;

	if ((pDrvCtrl = (TSEC_DRV_CTRL *) endFindByName("mottsec", tsecNum))
		== NULL)
		return;

	addr = pDrvCtrl->phyInfo->phyAddr;
	{
		MOT_TSEC_MII_FRAME_SET(pDrvCtrl);
		CACHE_PIPE_FLUSH();
		printf("motTsecPhyShow mottsec%d   phyAddr 0x%x   phyAddrReg=%p\n", tsecNum, addr, MOT_TSEC_MIIMADD_REG);
	}

	for( i=0; i<31; i++ ){
		motTsecMiiPhyRead(pDrvCtrl, addr, i, &value);
		printf("%d: 0x%04x\n", i, value );
	}


}

int motTsecPhySet( int tsecNum, int reg, int value )
{
	TSEC_DRV_CTRL *pDrvCtrl;
	int addr;

	if (tsecNum >= MOT_TSEC_MAX_DEVS)
		return -1;

	if ((pDrvCtrl = (TSEC_DRV_CTRL *) endFindByName("mottsec", tsecNum))
		== NULL)
		return -1;

	addr = pDrvCtrl->phyInfo->phyAddr;

	return motTsecMiiPhyWrite(pDrvCtrl, addr, reg, value);
}

/****************************************************************************
*
* motTsecMiiShow - Debug Function to show the Mii settings in the Phy Info
*                  structure.
*
* This function is only available when MOT_TSEC_DBG is defined. It should be
* used for debugging purposes only.
*
* RETURNS: N/A
*/
void motTsecMiiShow
    (
    int tsecNum,
    int addr,
    int num
    )
    {
    TSEC_DRV_CTRL * pDrvCtrl;
    UCHAR speed [20];
    int i;
    UINT16 value;
    TSEC_REG_T * tsecMiiReg;

    if (tsecNum >= MOT_TSEC_MAX_DEVS)
        return;

    if ((pDrvCtrl = (TSEC_DRV_CTRL *)endFindByName ("mottsec", tsecNum))
                     == NULL)
        return;

    tsecMiiReg = pDrvCtrl->tsecMiiPtr;

	switch (pDrvCtrl->phyInfo->phySpeed){
	case 2:
		strcpy ((char *) speed, "1Gbit/s");
		break;
	case 1:
		strcpy ((char *) speed, "100Mbit/s");
		break;
	default:
		strcpy ((char *) speed, "10Mbit/s");
		break;
	}

    printf("\n\tphySpeed=%s\n\tphyMode=%s\n\tphyAddr=0x%x"
           "\n\tphyFlags=0x%x\n\tphyDefMode=0x%x\n",
           (int)&speed[0],
           (int)pDrvCtrl->phyInfo->phyMode,
           pDrvCtrl->phyInfo->phyAddr,
           pDrvCtrl->phyInfo->phyFlags,
           pDrvCtrl->phyInfo->phyDefMode, 6);

    printf("\n\tphyStatus=0x%x\n\tphyCtrl=0x%x\n\tphyAds=0x%x"
           "\n\tphyPrtn=0x%x phyExp=0x%x\n\tphyNext=0x%x\n",
           pDrvCtrl->phyInfo->miiRegs.phyStatus,
           pDrvCtrl->phyInfo->miiRegs.phyCtrl,
           pDrvCtrl->phyInfo->miiRegs.phyAds,
           pDrvCtrl->phyInfo->miiRegs.phyPrtn,
           pDrvCtrl->phyInfo->miiRegs.phyExp,
           pDrvCtrl->phyInfo->miiRegs.phyNext );

    motTsecMiiPhyRead (pDrvCtrl, addr, MOT_TSEC_MIIMCFG_REG, &value);
    printf("miicfg %x\n",&value);
    motTsecMiiPhyRead (pDrvCtrl, addr, MOT_TSEC_MIIMCOM_REG, &value);
    printf("miicom %x\n",&value);
    motTsecMiiPhyRead (pDrvCtrl, addr, MOT_TSEC_MIIMADD_REG, &value);
    printf("miimadd %x\n",&value);
    motTsecMiiPhyRead (pDrvCtrl, addr, MOT_TSEC_MIIMCON_REG, &value);
    printf("miimcon %x\n",&value);
    motTsecMiiPhyRead (pDrvCtrl, addr, MOT_TSEC_MIIMSTAT_REG, &value);
    printf("miimstat %x\n",&value);
    motTsecMiiPhyRead (pDrvCtrl, addr, MOT_TSEC_MIIMIND_REG, &value);
    printf("miimind %x\n",&value);


#if 0
    MOT_TSEC_LOG ( MOT_TSEC_DBG_MII,
        ("\n\tphySpeed=%s\n\tphyMode=%s\n\tphyAddr=0x%x"
         "\n\tphyFlags=0x%x\n\tphyDefMode=0x%x\n"),
        (int)&speed[0],
        (int)pDrvCtrl->phyInfo->phyMode,
        pDrvCtrl->phyInfo->phyAddr,
        pDrvCtrl->phyInfo->phyFlags,
        pDrvCtrl->phyInfo->phyDefMode, 6);


   MOT_TSEC_LOG ( MOT_TSEC_DBG_MII,
       ("\n\tphyStatus=0x%x\n\tphyCtrl=0x%x\n\tphyAds=0x%x"
        "\n\tphyPrtn=0x%x phyExp=0x%x\n\tphyNext=0x%x\n"),
       pDrvCtrl->phyInfo->miiRegs.phyStatus,
       pDrvCtrl->phyInfo->miiRegs.phyCtrl,
       pDrvCtrl->phyInfo->miiRegs.phyAds,
       pDrvCtrl->phyInfo->miiRegs.phyPrtn,
       pDrvCtrl->phyInfo->miiRegs.phyExp,
       pDrvCtrl->phyInfo->miiRegs.phyNext );
#endif
   for (i = 0; i < num; i++)
       {
       motTsecMiiPhyRead (pDrvCtrl, addr, i, &value);
       printf("Phy Reg %d Value %x\n",i,value);
       }
    }

/******************************************************************************
*
* motTsecMibShow - Debug Function to show MIB statistics.
*
* This function is only available when MOT_TSEC_DBG is defined. It should be
* used for debugging purposes only.
*
* RETURNS: N/A
*
* ERRNO
*/
void motTsecMibShow(
	int tsecNum
) {
	TSEC_DRV_CTRL *pDrvCtrl;

	if (tsecNum >= MOT_TSEC_MAX_DEVS)
		return;

	if ((pDrvCtrl = (TSEC_DRV_CTRL *) endFindByName("mottsec", tsecNum))
		== NULL)
		return;

	MOT_TSEC_LOG(MOT_TSEC_DBG_MII,
				 "\n\tifOutNUcastPkts=%d\n\tifOutUcastPkts=%d\n\tifInNUcastPkts=%d\n\tifInUcastPkts=%d\n\tifOutErrors=%d\n",
				 pDrvCtrl->endObj.mib2Tbl.ifOutNUcastPkts,
				 pDrvCtrl->endObj.mib2Tbl.ifOutUcastPkts,
				 pDrvCtrl->endObj.mib2Tbl.ifInNUcastPkts,
				 pDrvCtrl->endObj.mib2Tbl.ifInUcastPkts,
				 pDrvCtrl->endObj.mib2Tbl.ifOutErrors, 6);

}

/******************************************************************************
*
* motTsecRegsShow - Debug Function to show driver specific control data.
*
* This function is only available when MOT_TSEC_DBG is defined.
* It should be used for debugging purposes only.
*
* RETURNS: N/A
*
* ERRNO
*/
void motTsecRegsShow(
	int tsecNum
) {
	TSEC_REG_T *tsecReg, *tsecMiiReg;
	TSEC_DRV_CTRL *pDrvCtrl;

	if (tsecNum >= MOT_TSEC_MAX_DEVS)
		return;

	if ((pDrvCtrl = (TSEC_DRV_CTRL *) endFindByName("mottsec", tsecNum))
		== NULL)
		return;

	tsecReg = pDrvCtrl->tsecRegsPtr;
	tsecMiiReg = pDrvCtrl->tsecMiiPtr;

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MACCFG1\t0x%08x\n",
				 (int) &MOT_TSEC_MACCFG1_REG, MOT_TSEC_MACCFG1_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "TX_EN",
				 (int) ((MOT_TSEC_MACCFG1_REG & MOT_TSEC_MACCFG1_TX_EN) ? "ON" : "OFF"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "RX_EN",
				 (int) ((MOT_TSEC_MACCFG1_REG & MOT_TSEC_MACCFG1_RX_EN) ? "ON" : "OFF"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "LOOPBACK",
				 (int) ((MOT_TSEC_MACCFG1_REG & MOT_TSEC_MACCFG1_LOOPBACK) ? "ON" : "OFF"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "RX_FLOW",
				 (int) ((MOT_TSEC_MACCFG1_REG & MOT_TSEC_MACCFG1_RX_FLOW) ? "ON" : "OFF"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "TX_FLOW",
				 (int) ((MOT_TSEC_MACCFG1_REG & MOT_TSEC_MACCFG1_TX_FLOW) ? "ON" : "OFF"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "SYNCD_RX_EN(RO)",
				 (int) ((MOT_TSEC_MACCFG1_REG & MOT_TSEC_MACCFG1_SYNCD_RX_EN) ? "ON" : "OFF"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "SYNCD_TX_EN(RO)",
				 (int) ((MOT_TSEC_MACCFG1_REG & MOT_TSEC_MACCFG1_SYNCD_TX_EN) ? "ON" : "OFF"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MACCFG2\t0x%08x\n",
				 (int) &MOT_TSEC_MACCFG2_REG, MOT_TSEC_MACCFG2_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%d\n", (int) "PRE LENGTH",
				 MOT_TSEC_MACCFG2_PRE_LEN_GET(MOT_TSEC_MACCFG2_REG), 3, 4, 5, 6);

	switch (MOT_TSEC_MACCFG2_IF_MODE_GET(MOT_TSEC_MACCFG2_REG)) {
	case 0:
		MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "IF MODE 0",
					 (int) "Reserved", 3, 4, 5, 6);
		break;
	case 1:
		MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "IF MODE 1",
					 (int) "MII", 3, 4, 5, 6);
		break;
	case 2:
		MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "IF MODE 2",
					 (int) "GMII,RGMII,TBI or RTBI", 3, 4, 5, 6);
		break;
	case 3:
		MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "IF MODE 3",
					 (int) "Reserved", 3, 4, 5, 6);
		break;
	}
	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "HUGE_FRAME",
				 (int) ((MOT_TSEC_MACCFG2_REG & MOT_TSEC_MACCFG2_HUGE_FRAME) ? "ALLOW" : "NO"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "LENGTH CHECK",
				 (int) ((MOT_TSEC_MACCFG2_REG & MOT_TSEC_MACCFG2_LENGTH_CHECK) ? "YES" : "NO"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "PAD-CRC",
				 (int) ((MOT_TSEC_MACCFG2_REG & MOT_TSEC_MACCFG2_PADCRC) ? "YES" : "NO"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "CRC ENABLE",
				 (int) ((MOT_TSEC_MACCFG2_REG & MOT_TSEC_MACCFG2_CRC_EN) ? "YES" : "NO"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "DUPLEX",
				 (int) ((MOT_TSEC_MACCFG2_REG & MOT_TSEC_MACCFG2_FULL_DUPLEX) ? "FULL" : "HALF"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "MACSTNADDR\t%08x%08x\n",
				 MOT_TSEC_MACSTNADDR2_REG, MOT_TSEC_MACSTNADDR1_REG, 3, 4, 5, 6);


	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)DMACTRL\t0x%08x\n",
				 (int) &MOT_TSEC_DMACTRL_REG, MOT_TSEC_DMACTRL_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "TX SNOOP",
				 (int) ((MOT_TSEC_DMACTRL_REG & MOT_TSEC_DMACTRL_TDSEN) ? "ENABLE" : "DISABLE"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "TXBD SNOOP",
				 (int) ((MOT_TSEC_DMACTRL_REG & MOT_TSEC_DMACTRL_TBDSEN) ? "ENABLE" : "DISABLE"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "GRA STOP",
				 (int) ((MOT_TSEC_DMACTRL_REG & MOT_TSEC_DMACTRL_GRS) ? "STOP" : "CONTINUE"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "TOD",
				 (int) ((MOT_TSEC_DMACTRL_REG & MOT_TSEC_DMACTRL_TOD) ? "BEGIN" : "CONTINUE"),
						3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "WWR",
				 (int) ((MOT_TSEC_DMACTRL_REG & MOT_TSEC_DMACTRL_WWR) ? "WAIT" : "NO"), 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "WOP",
				 (int) ((MOT_TSEC_DMACTRL_REG & MOT_TSEC_DMACTRL_WOP) ? "WAIT" : "POLL"),
						3, 4, 5, 6);


	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)ECNTRL\t0x%08x\n",
				 (int) &MOT_TSEC_ECNTRL_REG, MOT_TSEC_ECNTRL_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "CLEAR STATS",
				 (int) ((MOT_TSEC_ECNTRL_REG & MOT_TSEC_ECNTRL_CLRCNT) ? "YES" : "NO"), 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "AUTOZ ENABLE",
				 (int) ((MOT_TSEC_ECNTRL_REG & MOT_TSEC_ECNTRL_AUTOZ) ? "YES" : "NO"), 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "STATS ENABLE",
				 (int) ((MOT_TSEC_ECNTRL_REG & MOT_TSEC_ECNTRL_STEN) ? "YES" : "NO"), 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "TEN BIT MODE",
				 (int) ((MOT_TSEC_ECNTRL_REG & MOT_TSEC_ECNTRL_TBIM) ? "YES" : "NO"), 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "REDUCED PIN",
				 (int) ((MOT_TSEC_ECNTRL_REG & MOT_TSEC_ECNTRL_RPM) ? "YES" : "NO"), 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "%15s:%s\n", (int) "RGMII",
				 (int) ((MOT_TSEC_ECNTRL_REG & MOT_TSEC_ECNTRL_R100M) ? "100" : "10"), 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)IMASK\t0x%08x\n",
				 (int) &MOT_TSEC_IMASK_REG, MOT_TSEC_IMASK_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)IEVENT\t0x%08x\n",
				 (int) &MOT_TSEC_IEVENT_REG, MOT_TSEC_IEVENT_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)DMACTRL\t0x%08x\n",
				 (int) &MOT_TSEC_DMACTRL_REG, MOT_TSEC_DMACTRL_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)TBIPA\t0x%08x\n",
				 (int) &MOT_TSEC_TBIPA_REG, MOT_TSEC_TBIPA_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)RSTAT\t0x%08x\n",
				 (int) &MOT_TSEC_RSTAT_REG, MOT_TSEC_RSTAT_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)RBPTR\t0x%08x\n",
				 (int) &MOT_TSEC_RBPTR_REG, MOT_TSEC_RBPTR_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)RBASE\t0x%08x\n",
				 (int) &MOT_TSEC_RBASE_REG, MOT_TSEC_RBASE_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MACCFG1\t0x%08x\n",
				 (int) &MOT_TSEC_MACCFG1_REG, MOT_TSEC_MACCFG1_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MACCFG2\t0x%08x\n",
				 (int) &MOT_TSEC_MACCFG2_REG, MOT_TSEC_MACCFG2_REG, 3, 4, 5,6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)RCTRL\t0x%08x\n",
				 (int) &MOT_TSEC_RCTRL_REG, MOT_TSEC_RCTRL_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)CRBPTR\t0x%08x\n",
				 (int) &MOT_TSEC_CRBPTR_REG, MOT_TSEC_CRBPTR_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)RBDLEN\t0x%08x\n",
				 (int) &MOT_TSEC_RBDLEN_REG, MOT_TSEC_RBDLEN_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)TCTRL\t0x%08x\n",
				 (int) &MOT_TSEC_TCTRL_REG, MOT_TSEC_TCTRL_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)TBASE\t0x%08x\n",
				 (int) &MOT_TSEC_TBASE_REG, MOT_TSEC_TBASE_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)TSTAT\t0x%08x\n",
				 (int) &MOT_TSEC_TSTAT_REG, MOT_TSEC_TSTAT_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)CTBPTR\t0x%08x\n",
				 (int) &MOT_TSEC_CTBPTR_REG, MOT_TSEC_CTBPTR_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)TBDLEN\t0x%08x\n",
				 (int) &MOT_TSEC_TBDLEN_REG, MOT_TSEC_TBDLEN_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)TBPTR\t0x%08x\n",
				 (int) &MOT_TSEC_TBPTR_REG, MOT_TSEC_TBPTR_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MRBLR\t0x%08x\n",
				 (int) &MOT_TSEC_MRBLR_REG, MOT_TSEC_MRBLR_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MINFLR\t0x%08x\n",
				 (int) &MOT_TSEC_MINFLR_REG, MOT_TSEC_MINFLR_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MAXFRM\t0x%08x\n",
				 (int) &MOT_TSEC_MAXFRM_REG, MOT_TSEC_MAXFRM_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)EDIS\t0x%08x\n",
				 (int) &MOT_TSEC_EDIS_REG, MOT_TSEC_EDIS_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)IPGIFG\t0x%08x\n",
				 (int) &MOT_TSEC_IPGIFG_REG, MOT_TSEC_IPGIFG_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)IFSTAT\t0x%08x\n",
				 (int) &MOT_TSEC_IFSTAT_REG, MOT_TSEC_IFSTAT_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)TBIPA\t0x%08x\n",
				 (int) &MOT_TSEC_TBIPA_REG, MOT_TSEC_TBIPA_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)PTV\t0x%08x\n",
				 (int) &MOT_TSEC_PTV_REG, MOT_TSEC_PTV_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)HAFDUP\t0x%08x\n",
				 (int) &MOT_TSEC_HAFDUP_REG, MOT_TSEC_HAFDUP_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)ATTR\t0x%08x\n",
				 (int) &MOT_TSEC_ATTR_REG, MOT_TSEC_ATTR_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)ATTRELI\t0x%08x\n",
				 (int) &MOT_TSEC_ATTRELI_REG, MOT_TSEC_ATTRELI_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)OSTBD\t0x%08x\n",
				 (int) &MOT_TSEC_OSTBD_REG, MOT_TSEC_OSTBD_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)OSTBDP\t0x%08x\n",
				 (int) &MOT_TSEC_OSTBDP_REG, MOT_TSEC_OSTBDP_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MIIMCFG\t0x%08x\n",
				 (int) &MOT_TSEC_MIIMCFG_REG, MOT_TSEC_MIIMCFG_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MIIMIND\t0x%08x\n",
				 (int) &MOT_TSEC_MIIMIND_REG, MOT_TSEC_MIIMIND_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MIIMADD\t0x%08x\n",
				 (int) &MOT_TSEC_MIIMADD_REG, MOT_TSEC_MIIMADD_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MIIMCON\t0x%08x\n",
				 (int) &MOT_TSEC_MIIMCON_REG, MOT_TSEC_MIIMCON_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MIIMCOM\t0x%08x\n",
				 (int) &MOT_TSEC_MIIMCOM_REG, MOT_TSEC_MIIMCOM_REG, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MIIMSTAT\t0x%08x\n",
				 (int) &MOT_TSEC_MIIMSTAT_REG, MOT_TSEC_MIIMSTAT_REG, 3, 4, 5,6);


	MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "(0x%08x)MOT_TSEC_FIFO_TX_THR_REG:0x%08x\n",
				 (int) &MOT_TSEC_FIFO_TX_THR_REG, MOT_TSEC_FIFO_TX_THR_REG, 3, 4,
				 5, 6);
	MOT_TSEC_LOG(MOT_TSEC_DBG_REG,
				 "(0x%08x)MOT_TSEC_FIFO_TX_STARVE_REG:0x%08x\n",
				 (int) &MOT_TSEC_FIFO_TX_STARVE_REG, MOT_TSEC_FIFO_TX_STARVE_REG,
				 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_REG,
				 "(0x%08x)MOT_TSEC_FIFO_TX_STARVE_SHUTOFF_REG:0x%08x\n",
				 (int) &MOT_TSEC_FIFO_TX_STARVE_SHUTOFF_REG,
				 MOT_TSEC_FIFO_TX_STARVE_SHUTOFF_REG, 3, 4, 5, 6);
#if 0
	for (i = 0; i < 8; i++)
		MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "MOT_TSEC_IADDR%d_REG:0x%08x\n", i,
					 MOT_TSEC_IADDR_REG[i], 3, 4, 5, 6);

	for (i = 0; i < 8; i++)
		MOT_TSEC_LOG(MOT_TSEC_DBG_REG, "MOT_TSEC_GADDR%d_REG:0x%08x\n", i,
					 MOT_TSEC_GADDR_REG[i], 3, 4, 5, 6);
#endif

	taskDelay(2 * sysClkRateGet());
}

/****************************************************************************
*
* motTsecShow - Debug Function to show driver specific control data.
*
* This function is only available when MOT_TSEC_DBG is defined.
* It should be used for debugging purposes only.
*
* RETURNS: N/A
*
* ERRNO
*/
void motTsecShow(
	int tsecNum
) {
	int i;
	TSEC_BD *bdPtr;
	TSEC_REG_T *tsecReg;
	TSEC_DRV_CTRL *pDrvCtrl;

	if (tsecNum >= MOT_TSEC_MAX_DEVS)
		return;

	if ((pDrvCtrl = (TSEC_DRV_CTRL *) endFindByName("mottsec", tsecNum))
		== NULL)
		return;

	tsecReg = pDrvCtrl->tsecRegsPtr;

	MOT_TSEC_LOG(MOT_TSEC_DBG_TX,
				 "\n\tTxBds:Base 0x%x Total %d Free %d Next %d Last %d\n",
				 (int) pDrvCtrl->pTbdBase,
				 pDrvCtrl->tbdNum,
				 pDrvCtrl->tbdFree,
				 (pDrvCtrl->pTbdNext - pDrvCtrl->pTbdBase),
				 (pDrvCtrl->pTbdLast - pDrvCtrl->pTbdBase), 6);

	bdPtr = pDrvCtrl->pTbdBase;
	for (i = 0; i < pDrvCtrl->tbdNum; i++) {
		MOT_TSEC_LOG(MOT_TSEC_DBG_TX,
					 ("TXBD:%3d Status 0x%04x Length 0x%04x Address 0x%08x Alloc 0x%08x\n"),
					 i,
					 bdPtr->bdStat,
					 bdPtr->bdLen,
					 bdPtr->bdAddr,
					 (int) pDrvCtrl->tBufList[i], 6);

		bdPtr++;
	}

	MOT_TSEC_LOG(MOT_TSEC_DBG_RX,
				 ("\n\tRxBds:Base 0x%x Total %d Full %d Next %d Last %d\n"),
				 (int) pDrvCtrl->pRbdBase,
				 pDrvCtrl->rbdNum,
				 pDrvCtrl->rbdCnt,
				 (pDrvCtrl->pRbdNext - pDrvCtrl->pRbdBase),
				 (pDrvCtrl->pRbdLast - pDrvCtrl->pRbdBase), 6);

	bdPtr = pDrvCtrl->pRbdBase;
	for (i = 0; i < pDrvCtrl->rbdNum; i++) {
		MOT_TSEC_LOG(MOT_TSEC_DBG_RX,
					 ("RXBD:%3d Status 0x%04x Length 0x%04x Address 0x%08x Alloc 0x%08x\n"),
					 i,
					 bdPtr->bdStat,
					 bdPtr->bdLen,
					 bdPtr->bdAddr,
					 (int) pDrvCtrl->pMblkList[i], 6);

		bdPtr++;
	}

	MOT_TSEC_LOG(MOT_TSEC_DBG_INT,
				 ("\n\tEvent 0x%04x --- Mask  0x%04x\n"
				  "\n\tnumInts:%d"
				  "\n\tTXCInts:%d"
				  "\n\tRXBInts:%d\n"),
				 MOT_TSEC_IEVENT_REG,
				 MOT_TSEC_IMASK_REG,
				 pDrvCtrl->stats->numInts,
				 pDrvCtrl->stats->numTXCInts,
				 pDrvCtrl->stats->numRXBInts, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_INT,
				 ("\n\tTxErrInts:%d"
				  "\n\tRxBsyInts:%d"
				  "\n\tTXBInts:%d"
				  "\n\tRXFInts:%d"
				  "\n\tGRAInts:%d"
				  "\n\tRXCInts:%d\n"),
				 pDrvCtrl->stats->txErr,
				 pDrvCtrl->stats->rxBsyErr,
				 pDrvCtrl->stats->numTXBInts,
				 pDrvCtrl->stats->numRXFInts,
				 pDrvCtrl->stats->numGRAInts,
				 pDrvCtrl->stats->numRXCInts);


	MOT_TSEC_LOG(MOT_TSEC_DBG_INT_TX_ERR,
				 ("\n\tTxHbFailErr:%d"
				  "\n\tTxLcErr:%d"
				  "\n\tTxUrErr:%d"
				  "\n\tTxCslErr:%d"
				  "\n\tTxRlErr:%d"
				  "\n\tTxDefErr:%d\n"),
				 pDrvCtrl->stats->HbFailErr,
				 pDrvCtrl->stats->txLcErr,
				 pDrvCtrl->stats->txUrErr,
				 pDrvCtrl->stats->txCslErr,
				 pDrvCtrl->stats->txRlErr,
				 pDrvCtrl->stats->txDefErr);

	MOT_TSEC_LOG(MOT_TSEC_DBG_INT_RX_ERR,
				 ("\n\tRxHandler:%d"
				  "\n\tFramesLong:%d\n"),
				 pDrvCtrl->stats->numRXFHandlerEntries,
				 pDrvCtrl->stats->numRXFHandlerFramesLong, 3, 4, 5, 6);

	MOT_TSEC_LOG(MOT_TSEC_DBG_INT_RX_ERR,
				 "\n\tCollisions:%d"
				 "\n\tCrcErrors:%d"
				 "\n\tRejected:%d"
				 "\n\tNetBufAllocErrors:%d"
				 "\n\tNetCblkAllocErrors:%d"
				 "\n\tNetMblkAllocErrors:%d\n",
				 pDrvCtrl->stats->numRXFHandlerFramesCollisions,
				 pDrvCtrl->stats->numRXFHandlerFramesCrcErrors,
				 pDrvCtrl->stats->numRXFHandlerFramesRejected,
				 pDrvCtrl->stats->numRXFHandlerNetBufAllocErrors,
				 pDrvCtrl->stats->numRXFHandlerNetCblkAllocErrors,
				 pDrvCtrl->stats->numRXFHandlerNetMblkAllocErrors);

	taskDelay(500);

	return;
}
#if 0
void motTsecLog(
	int flag,
	char *fmt,
	...
) {
	char sbuf[256];

	va_list argPtr;

	va_start(argPtr, fmt);
	if (motTsecEndDbg & flag)
		vsprintf(sbuf, fmt, argPtr);
	va_end(argPtr);
	logMsg(sbuf, 1, 2, 3, 4, 5, 6);
}
#endif
#endif /* MOT_TSEC_DBG */



