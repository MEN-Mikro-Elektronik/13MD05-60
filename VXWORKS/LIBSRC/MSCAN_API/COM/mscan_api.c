/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  mscan_api.c
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2010/02/25 18:05:05 $
 *    $Revision: 1.8 $
 * 
 *  	 \brief  API functions to access the MSCAN MDIS driver
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mscan_api.c,v $
 * Revision 1.8  2010/02/25 18:05:05  amorbach
 * R: driver ported to MDIS5, new MDIS_API and men_typs
 * M1: Change type of path to MDIS_PATH
 * M2: Change return value of mscan_init() to MDIS_PATH
 * M3: Cast parameter "data" at M_setstat to INT32_OR_64
 *
 * Revision 1.7  2003/07/30 16:06:34  kp
 * added dummy documentation page to import MEN images
 *
 * Revision 1.6  2003/07/11 09:25:44  kp
 * added mscan_dump_internals
 *
 * Revision 1.5  2003/04/02 08:37:11  kp
 * 1) added Boromir note for mscan_error_counters()
 * 2) added __MAPILIB to all lib functions
 *
 * Revision 1.4  2003/02/07 13:16:39  kp
 * added comments to RTR frames
 *
 * Revision 1.3  2003/02/03 10:42:55  kp
 * First alpha release to SH Winding
 *
 * Revision 1.2  2003/01/29 14:03:13  kp
 * 1) Specification changes:
 * - mscan_read_nmsg and mscan_write_nmsg changed: They are now always
 * non-blocking, since M_getblock/M_setblock can't be passed an additional
 * 	 timeout value and M_getstat/M_setstat has too much overhead.
 * - add mscan_loopback()
 * - add mscan_error_counters()
 * - Changed Tx signal behaviour: now they are sent *after* the frame has
 * been transmitted over the bus
 * - add mscan_errobj_msg()
 * - add mscan_node_status()
 * 2) Added code for everything...
 *
 * Revision 1.1  2002/11/19 17:04:42  kp
 * Checkin to produce doxygen specification from source
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2002 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
static const char _RCSid[] = "$Id: mscan_api.c,v 1.8 2010/02/25 18:05:05 amorbach Exp $";

/*! \mainpage

  This document describes the features of the MSCAN driver API as well
  as most of the features of the underlying CAN chip (either Motorola
  Scalable CAN or MEN Boromir).


  \section FuncOv Functional Overview 

  The main features of MSCAN driver for MDIS5 is and it's API are:
  - operates on direct CAN (layer 2) frames.
  - permits access to all features of the MSCAN CAN controller. 
  - supports CAN Specification 2.0, standard and extended data and 
    remote frames 
  - programmable global acceptance filter (mask and code), standard or
    extended 	
  - 10 message objects with FIFOs (provided by driver)
  - programmable acceptance filter for each RX object

  The package consists of an MDIS5 low-level driver and a C library
  which provides an Application Programming Interface - "MSCAN_API" -
  between the application and the driver.  The driver itself does not
  implement any CAN protocol such as CANopen or DeviceNet. However it
  provides message queues for errors and for receive and transmit
  objects.

  \subsection Diffs Differences to SJA1000 API

  The MSCAN API is similar, but not identical to the API provided by
  the SJA1000 MDIS5 driver.

  The differences are:
  - CAN frames are exchanged by means of #MSCAN_FRAME structures, not
    with single parameters to the read/write routines.
  - Additional API functions to exchange more than one can frame with the
    driver FIFOs for higher performance: 
	mscan_read_nmsg() and mscan_write_nmsg().
  - Well defined priority between transmit FIFOs, see \ref Transm
  - Different setup of receive filters, see \ref ConfFilt
  - Different parameters to mscan_config_msg()
  - Additional function to control bus timing directly: mscan_set_bustiming().
  - RTR receive handling different: For MSCAN, the application must handle
    received remote transmit requests. SJA1000 implementation seems to be
	broken.
  - Error codes renamed from \c CPL_xxx to \c MSCAN_xxx.
  - sja1000_read_register, sja1000_read_counter, sja1000_read_configuration, 
    sja1000_read_alcr, sja1000_read_eccr not implemented since they are 
	SJA1000 specific.
  - Additional call mscan_error_counters() to read Rx/Tx error counters
  - Additional call mscan_node_status() to get current node status
  - Additional filtering for individual IDs (as supported by sp82c200 driver)

  \subsection FIFOs Message Objects and FIFOs 

  The driver provides up to 10 message object to the user, numbered
  from 0 to 9.  Each object has it's own FIFO. 

  Message object 0 is the error FIFO and is maintained by the driver
  to buffer error events on the CAN bus. It can be configured for
  receive only.  

  Message objects 1..9 can be configured for receive or transmit,
  but not for both.

  Each message object's FIFO buffers a configurable number of entries
  (frames).  API function #mscan_config_msg configures any message
  object.  
  
  Message objects configured to receive frames only receive those CAN
  frames that pass through the global ID filter \em and the object's
  local filter.

  Transmit message objects don't use the ID filters. Typically you will
  create only one transmit message object. However, multiple transmit
  message objects can be used to control the transmit priority of the
  frames to be sent, see \ref Transm.

  It is possible to install a signal that is sent to the
  application when a new frame arrives on a message object or when a
  frame has been transmitted. For each message object a different
  signal can be installed, but it is also possible to install the same
  signal for all message objects. You can install signals via
  #mscan_set_rcvsig or #mscan_set_xmtsig.

   \subsection InitSteps Initialisation Steps

  You should initialize the CAN driver by performing the following
  sequence of API calls:

  - Open a path to the device using #mscan_init.
  - Set the CAN bus bit rate through #mscan_set_bitrate or 
    #mscan_set_bustiming.
  - Set the global acceptance filter using #mscan_set_filter.
  - Configure the error message object (object 0) using #mscan_config_msg.
  - Configure the RX and TX message objects as required via #mscan_config_msg.
  - Install signals for message objects using #mscan_set_rcvsig or 
    #mscan_set_xmtsig.

  Now frames can be transmitted to or received from the CAN bus.


  \subsection Recv Receiving Frames

  Once bus activity has been enabled and message objects have been
  configured, the CAN controller monitors the CAN frames on the bus.
  Whenever a frame on the bus passes through the global filter and
  matches the identifier of one of the configured message objects (see
  \ref ConfFilt), the driver puts the frame into the corresponding
  receive FIFO. If multiple local filters match the receive frame,
  the message object with the lowest number will receive the frame.

  If installed, a signal is sent to the application to signify the
  arrival of a new frame.  

  \subsubsection QOvr FIFO Overrun Errors 
  
  If the buffer FIFO was already full, the new frame is discarded
  and a MSCAN_QOVERRUN error entry is put into the error
  FIFO. Additionally a signal is sent to the application if a signal
  was installed for the error object.  

  \subsubsection RecvRtr Receiving RTR Frames

  When an RTR frame is received (and it matches the global and local
  filter), the frame is written into the corresponding object’s receive
  FIFO. Application can detect the receiption of an RTR frame by looking
  at the \em flags field of the #MSCAN_FRAME structure.

  \subsubsection ReadMsg Reading Received Frames 

  The application can read received frames of a single message object
  by calling #mscan_read_msg or #mscan_read_nmsg.  These functions
  read frame(s) from the object’s FIFO and returns the received
  data.

  #mscan_read_msg can be blocking or non-blocking if no frame is
  available. The timeout parameter specifies how long to wait until a
  frame arrives. #mscan_read_nmsg is always non-blocking.

  The number of entries in the receive FIFO can be determined at any
  time by calling #mscan_queue_status. The FIFO can be cleared using
  #mscan_queue_clear.  

  It is possible that different objects are processed by different
  processes. For example, one process can wait for frames on object
  1, while a second process can wait for frames on object 2.  

  \subsubsection RxUseSigs Using Signals for Receive

  The application can use #mscan_set_rcvsig to install a signal that
  is sent whenever a new frame is put into the object’s receive
  FIFO. A signal for a specific object can be installed by a single
  process only. Each frame causes one signal to be sent to the
  application. Sending of signals can be cleared using
  #mscan_clr_rcvsig.

  \subsubsection ConfFilt Configure Filters

  MSCAN driver supports three different types of filters:
  - two global filters
  - local filter using \em mask and \em code for each Rx message object
  - additional local filtering for 11-bit IDs using an acceptance field

  The global filtering is done in hardware and can be used to reduce
  the load of the CPU. If a received frame's ID does not match the
  global filter, it is discarded.

  The local, message object related filters are implemented in
  software. They are used by the driver to determine the local object
  number to which the received frame belongs to. If the received
  frame's ID did not match any object's filter, the received frame is
  discarded.  When received frame's ID matches more than one object's
  filter, the lowest numbered object wins.

  Both global and local filters have the same format (with the
  exception of \em accField, which is used for local filters only);
  they are specified by means of the #MSCAN_FILTER structure. Each
  filter can be set to either standard \em or extended identifiers,
  but not for both (i.e.  a filter that is programmed for extended IDs
  will filter out all standard IDs).

  Since there are two global filters available, one can be set for
  standard IDs and the other for extended IDs.

  The filters can also be programmed to mask or compare the RTR bit of
  received frames.


  Each filter has an "Acceptance Code" and an "Acceptance Mask". These
  are 11-bit numbers for standard IDs and a 29-bit numbers for
  extended IDs. The filter's acceptance mask can be used to mask
  non-relevant bits of the identifier. For example, if filter
  acceptance code is set to 0x0002 and the acceptance mask is set to
  0x0001, filter passes frames with ID=2 and ID=3, because bit 0 of
  the ID is masked (ignored).

  If only one receive object is defined, the global acceptance filter
  is sufficient (provided that you don't want to mix standard and
  extended frames). The local filter can be disabled by setting the
  local acceptance mask to \c 0xffffffff (all bits to 1). In this
  case, all frames whose identifiers pass the global acceptance filter
  will be permitted for the receive object.

  \b Acceptance-Field

  For standard, 11-bit identifiers, there is an additional local
  filter, that can be used to filter out individual IDs, not only
  groups, see MSCAN_FILTER.accField. This filtering is not available
  for extended IDs.  Note that this filter is applied \em after the
  global and local filter masks.

  For example, to setup a filter that let IDs 3, 10, 22 and 99 pass
  through, use the following code:

  \code
      MSCAN_FILTER flt;

	  flt.code = 0x00000000;
	  flt.mask = 0xFFFFFFFF;
	  flt.cflags = 0;
	  flt.mflags = MSCAN_USE_ACCFIELD;
	  
	  memset( &flt.accField, 0, sizeof(flt.accField) );
	  MSCAN_ACCFIELD_SET( flt.accField, 3 );
	  MSCAN_ACCFIELD_SET( flt.accField, 10 );
	  MSCAN_ACCFIELD_SET( flt.accField, 22 );
	  MSCAN_ACCFIELD_SET( flt.accField, 99 );
  \endcode
	  
  The filter can be configured through commands #mscan_set_filter
  (global filter) and #mscan_config_msg (local filter).


  \subsection Transm Transmitting Frames

  Once message objects have been configured and bus activity has been
  enabled, the CAN controller is ready to transmit objects.  The
  application can send CAN frames to a transmit object using
  #mscan_write_msg or #mscan_write_nmsg.

  The driver puts the frame(s) into its transmit
  FIFO. If a free CAN transmit buffer is available, the driver schedules
  the the frame in front of the FIFO for transmission.

  The frames within each FIFO are sent over the CAN bus in the same
  order as they have been put into the transmit FIFO; independently of
  the CAN frame's ID. I.e. the frame that has been put first into the
  FIFO is the first frame sent over the CAN bus.

  However, you can create multiple transmit objects. These objects have 
  an implicit priority against eachother, based on their object number.
  The lower the object number, the higher the priority. For example, 
  if you create two transmit objects (let's say 3 and 4), frames in 
  transmit object #3 have priority over frames in transmit object #4.
  Frames in transmit object #4 are not sent before all frames of transmit
  object #3 have been sent.

  When the frame has been transmitted over the bus, the driver fetches
  the next frame from the lowest numbered available transmit FIFO and
  puts it into the CAN controller.

  #mscan_write_msg can be blocking or
  non-blocking if no space is available in the transmit FIFO. The
  timeout parameter specifies how long to wait until space is
  available again. #mscan_write_nmsg is always non-blocking.

  The number of \em free entries in the transmit FIFO can be
  determined at any time by calling #mscan_queue_status.

  The FIFO can be cleared using #mscan_queue_clear. The \em txabort
  parameter can be used to abort any pending transmission that has not
  been competed yet. (The txabort feature is not yet implemented!).

  \subsubsection TxUseSigs Using Signals for Transmit

  Every time the driver has successfully sent a frame over the CAN
  bus, it can inform the application by sending a signal. The
  application must call #mscan_set_xmtsig on the corresponding Tx
  message object. Note that the signal is sent for \em every
  transmitted frame. Signals can be disabled by using #mscan_clr_xmtsig.


  \subsubsection SendRtr Sending RTR Frames

  The application can force a remote CAN bus station to send a
  specific object by sending a remote transfer request (RTR)
  frame. This can be done using #mscan_rtr, #mscan_write_msg or 
  #mscan_write_nmsg.  


  \subsection ErrHandl Handling of CAN Errors

  As noted above, the driver maintains a special global FIFO that
  holds CAN errors or driver internal events. The error FIFO has the
  special object number 0. 

  Note that the error FIFO must be configured like any other message
  object before it can be used.  

  Whenever the state of the CAN controller changes or when a driver
  internal error occurs, the driver puts a new error entry into the
  error FIFO.  Each entry consists of an error code and the related
  message objects. If the error is global for all message objects, the
  "related message object" number is 0.  

  The following errors may occur:

  - \c MSCAN_BUSOFF_SET: Controller now in busoff state 
  - \c MSCAN_BUSOFF_CLR: Controller left busoff state
  - \c MSCAN_WARN_SET: Controller has entered error passive state
  - \c MSCAN_WARN_CLR: Controller has left error passive state
  - \c MSCAN_QOVERRUN: Object’s receiver FIFO overflowed 
  - \c MSCAN_DATA_OVERRUN: Data overrun interrupt occurred 
       (hardware receive FIFO full)

  The application can read from the error FIFO by calling
  #mscan_read_error. This function returns the next entry from the
  error FIFO. This function is blocking, i.e. if there are no entries
  in the error FIFO it waits until an error entry is present.  

  You can query the number of entries in the error FIFO using
  #mscan_queue_status and clear the FIFO using #mscan_queue_clear.  

  The application can install a signal that is sent whenever a new
  entry is inserted into the error FIFO using #mscan_set_rcvsig. This
  signal can be installed by a single process only.  

  \subsubsection NodeStat Reading node status

  The current node status can be queried any time by calling
  #mscan_node_status. See #MSCAN_NODE_STATUS for details.
  
  \subsubsection ReadCntr Reading controller error counters

  It is possible to read the CAN controllers error counters directly
  by calling #mscan_error_counters. On standard MSCAN implementation,
  this can be done only when bus activity is disabled (error counters
  are not accessible when controller is active on the bus). However,
  MEN's implementation "Boromir" allows to read error counters in
  either mode. For interpretation of the CAN controller error
  counters, see the MSCAN controller manual.

  \subsubsection BusOffRec Recovering from Busoff State 

  The MSCAN controller automatically recovers from bus off state.
  There is no need to call #mscan_clear_busoff. This function
  is only present for compatibility with other controllers.

*/


/*! \page dummy

 \menimages

*/

#include <stdio.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_err.h>
#include <MEN/mdis_api.h>
#include <MEN/mscan_api.h>
#include <MEN/mscan_drv.h>

/*--------------------------------+
|  DEFINES                        |
+--------------------------------*/
#define DO_BLK_SETSTAT( obj, code ) \
   {\
       M_SG_BLOCK blk;\
       blk.size = sizeof(obj);\
       blk.data = (void *)&obj;\
       rv = M_setstat( path, code, (INT32_OR_64)&blk );\
   }

#define DO_BLK_GETSTAT( obj, code ) \
   {\
       M_SG_BLOCK blk;\
       blk.size = sizeof(obj);\
       blk.data = (void *)&obj;\
       rv = M_getstat( path, code, (int32 *)&blk );\
   }


/**********************************************************************/
/** Open path to CAN device
 * 	
 * Init and reset controller
 *
 * Actions performed:
 * - check for controller ready
 * - reset all registers
 * - set global acceptance to let all IDs pass through (standard and extended)
 * 
 * (Bus activity is left disabled by this function)
 * 
 * To enable controller activity configure all message objects, set
 * bitrate and call mscan_enable().
 *
 * \param 	device		MDIS device name
 * \return 	>=0 path number or <0= error
 *			In case of error, \em errno set to:
 *			- MSCAN_ERR_RESETACTIVE  controller did not leave reset state???
 *
 * \sa mscan_term
 */
MDIS_PATH __MAPILIB mscan_init(char *device)
{
	MDIS_PATH path;

	if( (path = M_open(device)) < 0 )
		return path;

	/*--- enable irq ---*/
	if( (M_setstat( path, M_MK_IRQ_ENABLE, TRUE ) < 0 ))
		return -1;
	return path;
}

/**********************************************************************/
/** Close path to CAN device
 *
 * \param 	path 	MDIS path number for device
 * \return 	0=success <0=error
 *			In case of error, \em errno contains an 
 *			- MSCAN_ERR_RESETACTIVE  controller in reset state???
 *			- XXX
 *
 * \sa mscan_open
 */
int32 __MAPILIB mscan_term(MDIS_PATH path)
{
	return M_close(path);
}

/**********************************************************************/
/** Set global acceptance filter
 *
 * See \ref ConfFilt for more information.
 *
 * \remark Bus activity is temporarily disabled during this call.
 *  
 * \param 	path 	MDIS path number for device
 * \param	filter1	structure that specifies the filter #1
 * \param	filter2	structure that specifies the filter #2
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADPARAMETER:	 if filter specification invalid
 */
int32 __MAPILIB mscan_set_filter(
	MDIS_PATH path,
	const MSCAN_FILTER *filter1,
	const MSCAN_FILTER *filter2)
{
	MSCAN_SETFILTER_PB pb;
	int32 rv;

	pb.filter1 = *filter1;
	pb.filter2 = *filter2;
	
	DO_BLK_SETSTAT( pb, MSCAN_SETFILTER );
	return rv;
}


/**********************************************************************/
/** Configure message object
 *
 * Used to configure one of the 10 message objects provided by the MSCAN 
 * driver. The error object (object #0) is also configured by this function.
 *
 * By default, all message objects are disabled (i.e direction is set
 * to #MSCAN_DIR_DIS).
 *
 * This function creates the message object's FIFO with specified number
 * of entries (\a qentries). It also configures the message object's local
 * \a filter (See \ref ConfFilt for more information).
 *
 * To create the special error object, object #0, call
 *
 * \code
 * 	mscan_config_msg( path, 0, MSCAN_DIR_RCV, nEntries, NULL );
 * \endcode
 *
 * If the specified message object was already configured, any frames
 * in the FIFO are lost, and the FIFO is reallocated.
 *
 * \remark This function will not touch an installed signal for that object.
 *
 * \param 	path 	MDIS path number for device
 * \param 	nr		message object number (1....) or 0 for error obj.
 * \param 	dir		object direction 
 *					- MSCAN_DIR_RCV		receive
 *					- MSCAN_DIR_XMT		transmit
 *					- MSCAN_DIR_DIS		disable
 * \param	qEntries FIFO depth (number of entries in rx or tx FIFO)
 *					must be >0
 * \param	filter	structure that specifies the filter. Used only for Rx
 *					objects. Can be NULL for Tx objects and object 0.
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADPARAMETER:    filter specification or
 *											qEntries invalid
 *			- \c MSCAN_ERR_BADMSGNUM:		illegal message object number
 *			- \c MSCAN_ERR_BADDIR:	   		bad direction
 */
int32 __MAPILIB mscan_config_msg(
	MDIS_PATH path,
	u_int32 nr,	
	MSCAN_DIR dir,
	u_int32 qEntries,
	const MSCAN_FILTER *filter )
{
	MSCAN_CONFIGMSG_PB pb;
	int32 rv;

	pb.objNr 	= nr;
	pb.dir		= dir;
	pb.qEntries	= qEntries;

	if( filter )
		pb.filter	= *filter;
	else
		memset( &pb.filter, 0, sizeof(pb.filter) );

	DO_BLK_SETSTAT( pb, MSCAN_CONFIGMSG );
	return rv;
}

/**********************************************************************/
/** Setup bustiming according to CIA DS 102 V2.0
 *
 * This function sets up the bustiming (bitrate, and timing segments)
 * according to CIA DS 102 V2.0, based on the specified \a bitrate
 * code.
 *
 * Bus activity must be disabled when calling this function
 *
 * This function computes the parameters and then calls mscan_set_bustiming().
 * 
 * 
 * \param 	path 	MDIS path number for device
 * \param	bitrate bitrate code (see #MSCAN_BITRATE)
 * \param	spl		sample mode 
 *					- 0=fast, take one sample per bit
 *					- 1=slow, take three samples per bit	
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADSPEED:      bitrate not supported
 *			- \c MSCAN_ERR_ONLINE:        controller was not disabled
 *
 * \sa mscan_set_bustiming
 */
int32 __MAPILIB mscan_set_bitrate(
	MDIS_PATH path,
	MSCAN_BITRATE bitrate,
	u_int32 spl )
{
	MSCAN_SETBITRATE_PB pb;
	int32 rv;
	
	pb.bitrate	= bitrate;
	pb.spl		= spl;

	DO_BLK_SETSTAT( pb, MSCAN_SETBITRATE );
	return rv;
}

/**********************************************************************/
/** Setup bustiming directly
 *
 * This function allows to setup the bus timing parameters directly,
 * in case the predefined CIA DS 102 2.0 parameters provided by 
 * mscan_set_bitrate() are not sufficient.
 *
 * Bus activity must be disabled when calling this function
 *
 * \param 	path 	MDIS path number for device
 * \param	brp		baud rate prescaler (1..64)
 * \param	sjw		Syncronisation jump width (1..4)
 * \param 	tseg1	timing segment 1 length (1..16)
 * \param 	tseg2	timing segment 2 length (1..8)
 * \param	spl		sample mode 
 *					- 0=fast, take one sample per bit
 *					- 1=slow, take three samples per bit	
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADTMDETAILS:  wrong parameter
 *			- \c MSCAN_ERR_ONLINE:        controller was not disabled
 *
 * \sa mscan_set_bitrate
 */
int32 __MAPILIB mscan_set_bustiming(
	MDIS_PATH path,
	u_int8 brp, 
	u_int8 sjw,
	u_int8 tseg1,
	u_int8 tseg2,
	u_int8 spl )
{
	MSCAN_SETBUSTIMING_PB pb;
	int32 rv;
	
	pb.brp		= brp;
	pb.sjw		= sjw;
	pb.tseg1	= tseg1;
	pb.tseg2	= tseg2;
	pb.spl		= spl;

	DO_BLK_SETSTAT( pb, MSCAN_SETBUSTIMING );
	return rv;
}

/**********************************************************************/
/** Read single frame from CAN object's receive FIFO
 *
 *  Reads the next frame from the object's receive FIFO and copies the
 *  frame identifier, length and data portion to the user's buffer 
 *  pointed to by \a msg.
 *
 *  The \a timeout parameter specifies how to wait for frames:
 *  - If \a timeout is -1 and no frame is present in the FIFO, the function
 *    returns -1 and errno is set to MSCAN_ERR_NOMESSAGE.
 *  - If \a timeout is >=0 and no frame is present in the FIFO, the function
 *    waits until a frame arrives or a timeout occurs. 
 *  - A value of 0 causes this function to wait forever. 
 *
 * Depending on the operating system, this wait may be aborted by a 
 * deadly signal.
 *
 * \param 	path 	MDIS path number for device
 * \param	nr		message object number (1....)
 * \param	timeout	flags if this call waits until frame available
 *					(-1=don't wait, 0=wait forever, >0=tout in ms)
 * \param 	msg 	user buffer where received frame will be stored.
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_NOMESSAGE:	no frame in receive buffer
 *			- \c MSCAN_ERR_BADMSGNUM:	illegal message object number
 *			- \c MSCAN_ERR_BADDIR:	   	object configured for transmit
 *			- \c ERR_OSS_TIMEOUT:	   	timeout occurred	
 *
 * \sa \ref Recv, mscan_read_nmsg, mscan_set_rcvsig
 */
int32 __MAPILIB mscan_read_msg(
	MDIS_PATH path,
	u_int32 nr,
	int32 timeout,
	MSCAN_FRAME *msg )
{
	MSCAN_READWRITEMSG_PB pb;
	int32 rv;

	pb.objNr		= nr;
	pb.timeout		= timeout;
	
	DO_BLK_GETSTAT( pb, MSCAN_READMSG );

	if( rv == 0 )
		*msg = pb.msg;

	return rv;
}

/**********************************************************************/
/** Read multiple frames from CAN object's receive FIFO
 *
 *  Reads the next \a nFrames frames from the object's receive FIFO and
 *  copies the frames to the user's buffer pointed to by \a msg.
 *
 *  This is essentially the same as mscan_read_msg(), but shows better
 *  performance when transferring large amounts of frames between 
 *  the driver and application, since less context switches are 
 *  required. 
 *
 *  However, if user intends to read only a single frame, mscan_read_msg()
 *  is faster.
 *
 *  Unlike mscan_read_msg(), this function is always non blocking. If
 *  not enough frames in fifo, it copies the number of entries
 *  currently present in the fifo.
 *
 * \param 	path 	MDIS path number for device
 * \param	nr		message object number (1....)
 * \param 	nFrames	maximum number of frames to read
 * \param 	msg 	user buffer where received frames will be stored.
 *
 * \return 	number of successfully copied CAN frames, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADMSGNUM:	illegal message object number
 *			- \c MSCAN_ERR_BADDIR:	   	object configured for transmit
 *
 * \remarks modifies current MDIS channel number
 * \sa \ref Recv, mscan_read_msg, mscan_set_rcvsig 
 */
int32 __MAPILIB mscan_read_nmsg(
	MDIS_PATH path,
	u_int32 nr,
	int32 nFrames,
	MSCAN_FRAME *msg )
{
	int32 rv;

	/* set the current channel to the objHdl */
	if( M_setstat( path, M_MK_CH_CURRENT, nr ))
		return -1;

	/* read frames */
	rv = M_getblock( path, (u_int8 *)msg, nFrames * sizeof(*msg));

	if( rv < 0 )
		return -1;

	return rv / sizeof(*msg);	
}

/**********************************************************************/
/** Put single frame into CAN object's transmit FIFO
 *
 *  Puts the specified \a msg into the object's FIFO. This
 *  frame will be scheduled for transmit once it is at the head of
 *  the transmit FIFO. 
 *
 *  The \a timeout parameter specifies how to wait for free FIFO space:
 *	- If \a timeout is -1 and no space left in the FIFO, the function
 *    returns -1 and errno is set to MSCAN_ERR_QFULL.
 *	- If \a timeout is >=0 and no space left in the FIFO, the function
 *    waits until space is again available or a timeout occurs. 
 *	- A value of 0 causes this function to wait forever. 
 * 
 * \remark To send RTR frames, set the #MSCAN_RTR bit in msg.flags. Set
 * msg.dataLen to the value of the DLC field of the RTR frame. However,
 * even if a data length != 0 is specified, no data bytes are included in
 * the frame.
 *
 * Depending on the operating system, this wait may be aborted by a
 * deadly signal.
 *
 * \param 	path 	MDIS path number for device
 * \param	nr		message object number (1....)
 * \param	timeout	flags if this call waits until FIFO space available
 *					(-1=don't wait, 0=wait forever, >0=tout in ms)
 * \param 	msg 	CAN message id and data to send 
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADMSGNUM:	illegal message object number
 *			- \c MSCAN_ERR_QFULL: 	   	no space in FIFO (if wait=0)
 *			- \c MSCAN_ERR_BADDIR:	   	object configured for receive
 *			- \c ERR_OSS_TIMEOUT:	   	timeout occurred	
 *			- \c ERR_OSS_SIG_OCCURED	a deadly signal occurred while waiting
 *			- \c MSCAN_ERR_NOTINIT		CAN not online
 *
 * \sa \ref Transm, mscan_write_nmsg, mscan_set_xmtsig
 */
int32 __MAPILIB mscan_write_msg(
	MDIS_PATH path,
	u_int32 nr,
	int32 timeout,
	const MSCAN_FRAME *msg)
{
	MSCAN_READWRITEMSG_PB pb;
	int32 rv;

	pb.objNr		= nr;
	pb.timeout		= timeout;
	pb.msg			= *msg;

	DO_BLK_SETSTAT( pb, MSCAN_WRITEMSG );
	return rv;
}

/**********************************************************************/
/** Put multiple frames into CAN object's transmit FIFO
 *
 *  Puts the specified CAN frames pointed to by \a msg into the
 *  object's FIFO.
 *
 *  This is essentially the same as mscan_write_msg(), but shows better
 *  performance when transferring large amounts of frames between 
 *  the driver and application, since less context switches are 
 *  required.
 *
 *  However, if user intends to write only a single frame, mscan_write_msg()
 *  is faster.
 *
 *  Unlike mscan_write_msg(), this function is always non blocking. If
 *  not enough space left in FIFO it places only those entries into
 *  the fifo that fit into the fifo.
 *
 * \param 	path 	MDIS path number for device
 * \param	nr		message object number (1....)
 * \param 	nFrames	maximum number of frames to send
 * \param 	msg 	CAN message id and data to send 
 *
 * \return 	number of CAN frames put into FIFO, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADMSGNUM:	illegal message object number
 *			- \c MSCAN_ERR_BADDIR:	   	object configured for receive
 *
 * \remarks modifies current MDIS channel number
 * \sa \ref Transm, mscan_write_msg, mscan_set_xmtsig 
 */
int32 __MAPILIB mscan_write_nmsg(
	MDIS_PATH path,
	u_int32 nr,
	int32 nFrames,
	const MSCAN_FRAME *msg)
{
	int32 rv;

	/* set the current channel to the objHdl */
	if( M_setstat( path, M_MK_CH_CURRENT, nr ))
		return -1;

	/* read frames */
	rv = M_setblock( path, (u_int8 *)msg, nFrames * sizeof(*msg));

	if( rv < 0 )
		return -1;

	return rv / sizeof(*msg);	
}

/**********************************************************************/
/** Read error entry from driver's global error FIFO
 *	
 * For each error detected by the CAN controller one entry is inserted
 * into the CAN's global error FIFO.
 *
 * This function is used to retrieve these error entries. This function
 * blocks until there is at least one entry in the error FIFO. You can
 * check for error entries using mscan_queue_status().
 *
 *  Each entry contains:
 *	- errcode (placed into \a *errCodeP): See #MSCAN_ERRENTRY_CODE for codes.
 *	- nr (placed into \a *nrP): contains the related message object number. 
 *	  For global errors, \em nr is always 0, 
 *	  for \c MSCAN_QOVERRUN, \em nr contains the related
 *	  message object number (1....).
 *
 * \remark FIFO overrun errors (#MSCAN_QOVERRUN) are generated only
 * once until the user calls mscan_read_msg() or mscan_read_nmsg.
 *
 * \param 	path 		MDIS path number for device
 * \param	errCodeP	Pointer to variable where error code will be stored.
 *						(see #MSCAN_ERRENTRY_CODE)
 * \param	nrP			Pointer to variable where object number will be stored.
 *
 * \return 	0 on success, or -1 on error.
 * \sa mscan_queue_status */
int32 __MAPILIB mscan_read_error(
	MDIS_PATH path,
	u_int32 *errCodeP,
	u_int32 *nrP)
{
	MSCAN_READERROR_PB pb;
	int32 rv;

	DO_BLK_GETSTAT( pb, MSCAN_READERROR );

	if( rv == 0 ){
		*errCodeP 	= pb.errCode;
		*nrP		= pb.objNr;
	}
	return rv;
}

/**********************************************************************/
/** Install signal for frame receiption
 *	
 *  Installs a signal for the specified message object number. This signal
 *  is sent to the calling process whenever a new frame is written into 
 *  the object's receive FIFO.
 *
 *  This function is also used to install a signal for the special error
 *  object (nr=0). 
 *
 *  It is possible to install different signals for different message objects.
 *  For each message object, this function must be called once.
 *  
 *  Only one process can install a signal for a specific message object.
 *
 * \param 	path 	MDIS path number for device
 * \param	nr		message object number (1....)or 0 for error object
 * \param	signal	signal code to be installed
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADMSGNUM:  illegal message object number
 *			- \c MSCAN_ERR_BADDIR:	   object configured for transmit
 *			- \c MSCAN_ERR_SIGBUSY:	   signal already installed
 *
 * \sa mscan_clr_rcvsig
 */
int32 __MAPILIB mscan_set_rcvsig(
	MDIS_PATH path,
	u_int32 nr,
	int32 signal)
{
	MSCAN_SIGNAL_PB pb;
	int32 rv;

	pb.objNr 	= nr;
	pb.signal	= signal;

	DO_BLK_SETSTAT( pb, MSCAN_SETRCVSIG );
	return rv;
}

/**********************************************************************/
/** Install signal for frame transmission
 *	
 *  Installs a signal for the specified message object number. This signal
 *  is sent to the calling process whenever a CAN frame has been passed to
 *  the CAN controller for transmission. This does not mean that the frame
 *  is already transmitted! 
 *  
 *  It is possible to install different signals for different message objects.
 *  For each message object, this function must be called once.
 *  
 *  Only one process can install a signal for a specific message object.
 *
 * \param 	path 	MDIS path number for device
 * \param	nr		message object number (1....)or 0 for error object
 * \param	signal	signal code to be installed
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADMSGNUM:  illegal message object number
 *			- \c MSCAN_ERR_BADDIR:	   object configured for receive
 *			- \c MSCAN_ERR_SIGBUSY:	   signal already installed
 *
 * \sa mscan_clr_xmtsig
 */
int32 __MAPILIB mscan_set_xmtsig(
	MDIS_PATH path,
	u_int32 nr,
	int32 signal)
{
	MSCAN_SIGNAL_PB pb;
	int32 rv;

	pb.objNr 	= nr;
	pb.signal	= signal;

	DO_BLK_SETSTAT( pb, MSCAN_SETXMTSIG );
	return rv;
}

/**********************************************************************/
/** Deactivate signal installed by mscan_set_rcvsig()
 *
 * \param 	path 	MDIS path number for device
 * \param	nr		message object number (1....)or 0 for error object
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADMSGNUM:  illegal message object number
 *			- \c MSCAN_ERR_SIGBUSY:	   signal \b not installed
 */
int32 __MAPILIB mscan_clr_rcvsig(
	MDIS_PATH path,
	u_int32 nr)
{
	MSCAN_SIGNAL_PB pb;
	int32 rv;

	pb.objNr 	= nr;
	pb.signal	= 0;

	DO_BLK_SETSTAT( pb, MSCAN_CLRRCVSIG );
	return rv;
}

/**********************************************************************/
/** Deactivate signal installed by mscan_set_xmtsig()
 *
 * \param 	path 	MDIS path number for device
 * \param	nr		message object number (1....)or 0 for error object
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADMSGNUM:  illegal message object number
 *			- \c MSCAN_ERR_SIGBUSY:	   signal \b not installed
 */
int32 __MAPILIB mscan_clr_xmtsig(
	MDIS_PATH path,
	u_int32 nr)
{
	MSCAN_SIGNAL_PB pb;
	int32 rv;

	pb.objNr 	= nr;
	pb.signal	= 0;

	DO_BLK_SETSTAT( pb, MSCAN_CLRXMTSIG );
	return rv;
}

/**********************************************************************/
/** Get number of entries in an object's receive/transmit FIFO
 *			   
 * For receive message objects reports the number of received frames in
 * the FIFO.
 *
 * For transmit message objects reports the number of \em free entries
 * (frames) in the FIFO.
 * 
 * For the special error object (nr=0) reports the number of error entries
 * in the error FIFO.
 *
 * This function reports also the direction for which the object has been	
 * configured.
 *
 * \param 	path 	MDIS path number for device
 * \param	nr		message object number (1....)or 0 for error object
 * \param	entriesP pointer to variable where number of entries will be stored
 * \param	directionP pointer to variable where direction will be stored
 *
 * Both \a entriesP and \a directionP can be NULL; in this case the parameter
 * is ignored
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADMSGNUM:  illegal message object number
 */
int32 __MAPILIB mscan_queue_status(
	MDIS_PATH path,
	u_int32 nr,
	u_int32 *entriesP,
	MSCAN_DIR *directionP)
{
	MSCAN_QUEUESTATUS_PB pb;
	int32 rv;

	pb.objNr 	= nr;

	DO_BLK_GETSTAT( pb, MSCAN_QUEUESTATUS );

	if( rv == 0 ){
		if( entriesP )
			*entriesP 	= pb.entries;
		if( directionP )
			*directionP	= pb.direction;
	}

	return rv;	
}

/**********************************************************************/
/** Clear object FIFO
 *			   
 * Clears all entries in a message FIFO (nr>0) or error FIFO (nr=0).
 *
 * For transmit message FIFOs, if \a txabort is not 0, any pending
 * transmit request related to that transmit object that has not been
 * completed should also aborted.
 *
 * \remark In the current implementation, \a txabort is ignored!
 *
 * \param 	path 	MDIS path number for device
 * \param	nr		message object number (1....)or 0 for error object
 * \param	txabort	Tx FIFOs: if !=0 abort pending transmission
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_BADMSGNUM:  illegal message object number
 */
int32 __MAPILIB mscan_queue_clear(
	MDIS_PATH path,
	u_int32 nr,
	int txabort)
{
	MSCAN_QUEUECLEAR_PB pb;
	int32 rv;

	pb.objNr 	= nr;
	pb.txabort	= txabort;

	DO_BLK_SETSTAT( pb, MSCAN_QUEUECLEAR );
	return rv;
}

/**********************************************************************/
/** Reset controller from busoff state
 *			   
 * \remark this function is a no-op for MSCAN
 *
 * \param 	path 	MDIS path number for device
 *
 * \return 	0 on success, or -1 on error.
 */
int32 __MAPILIB mscan_clear_busoff(
	MDIS_PATH path)
{
	return M_setstat( path, MSCAN_CLEARBUSOFF, 0 );
}		


/**********************************************************************/
/** Enable/Disable controller's bus activity
 *			   
 *  While bus activity is disabled, no frames can be transmitted or received.
 *  Transmitter line is then in recessive state.
 *			   
 *  Before enabling bus activity, message objects and bitrate must have been
 *	already configured.
 *
 * \param 	path 	MDIS path number for device
 * \param	enable	enable/disable bus activity (0=disable, 1=enable)
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_NOTINIT:  bit-timing not initialized
 *			- \c ERR_LL_DEV_NOTRDY:  controller handshake problem
 */
int32 __MAPILIB mscan_enable(
	MDIS_PATH path,
	u_int32 enable )
{
	return M_setstat( path, MSCAN_ENABLE, enable );
}

/**********************************************************************/
/** Send a remote frame request
 *			   
 * This is a convienience function for mscan_write_msg.
 *
 * It is the same as:
 * \code
 *   MSCAN_FRAME frm;
 *
 *   frm.id = id;
 *   frm.flags = MSCAN_RTR;
 *   frm.dataLen = 0;
 *
 *   mscan_write_msg( path, nr, 1000, &frm );
 * \endcode
 *
 *  Requests a remote frame to be sent on the specified object. Once the
 *  frame has been received it is put into the object's receive FIFO and
 *  can be read using mscan_read_msg().
 *  
 *  The message object must be configured for transmit mode. The remote
 *	request contains no data byte and the data length is set to zero. 
 *
 * \param 	path 	MDIS path number for device
 * \param	nr		message object number (1....)
 * \param	id		ID that is put into the RTR frame (standard ID always!)
 *
 * \return 	0 on success, or -1 on error.
 *
 * \remark Using this function, the RTR frame has always a data length code
 * of 0. This may cause problems in some implementations. 
 * Use #mscan_write_msg() instead
 *
 * \sa mscan_write_msg()
 */
int32 __MAPILIB mscan_rtr(
	MDIS_PATH path,
	u_int32 nr,
	u_int32 id)
{
	MSCAN_FRAME frm;

	frm.id = id;
	frm.flags = MSCAN_RTR;
	frm.dataLen = 0;

	return mscan_write_msg( path, nr, 1000, &frm );
}

/**********************************************************************/
/** Setup loopback mode
 *
 * For testing purpose, the CAN controller can be set into loopback mode
 * In this mode, all frames transmitted by the controller are looped
 * back to the receiver
 *
 * Bus activity must be disabled when calling this function
 *
 * \param 	path 	MDIS path number for device
 * \param	enable	0=disable loopback 1=enable loopback
 *
 * \return 	0 on success, or -1 on error.
 *
 */
int32 __MAPILIB mscan_set_loopback(
	MDIS_PATH path,
	int enable)
{
	return M_setstat( path, MSCAN_LOOPBACK, enable );
}

/**********************************************************************/
/** Read CAN node status
 *
 * \param 	path 	MDIS path number for device
 * \param	statusP pointer to variable where node status will be stored
 *
 * \return 	0 on success, or -1 on error.
 *
 * \sa mscan_error_counters
 */
int32 __MAPILIB mscan_node_status(
	MDIS_PATH path,
	MSCAN_NODE_STATUS *statusP )
{
	int32 ns, rv;

	rv = M_getstat( path, MSCAN_NODESTATUS, &ns );

	if( rv == 0 )
		*statusP = (MSCAN_NODE_STATUS)ns;

	return rv;		
}

/**********************************************************************/
/** Read CAN controller error counters
 *
 * Reads the CAN controller error counters directly.
 *
 * \remark For standard MSCAN implementations, bus activity must be
 * disabled when calling this function. MENs "Boromir" allows to 
 * read error counters always.
 *
 * \param 	path 	MDIS path number for device
 * \param	txErrCntP pointer to variable where number of Tx errors will
 *					be stored
 * \param	rxErrCntP pointer to variable where number of Rx errors will
 *					be stored
 *
 * Both \a txErrCntP and \a rxErrCntP can be NULL; in this case the parameter
 * is ignored
 *
 * \return 	0 on success, or -1 on error.
 *			In case of error, \em errno set to:
 *			- \c MSCAN_ERR_ONLINE:        controller was not disabled
 *
 * \sa mscan_node_status
 */
int32 __MAPILIB mscan_error_counters(
	MDIS_PATH path,
	u_int8 *txErrCntP,
	u_int8 *rxErrCntP)
{
	MSCAN_ERRORCOUNTERS_PB pb;
	int32 rv;

	DO_BLK_GETSTAT( pb, MSCAN_ERRORCOUNTERS );

	if( rv == 0 ){
		if( txErrCntP )
			*txErrCntP = pb.txErrCnt;
		if( rxErrCntP )
			*rxErrCntP = pb.rxErrCnt;
	}

	return rv;		
}



/**********************************************************************/
/** Tell driver to dump internal state as a string
 *
 * Driver will generate an ASCII string containing most important
 * parameters of device. String will not be longer than \a maxLen
 */
int32 __MAPILIB mscan_dump_internals(
	MDIS_PATH path,
	char *buffer,
	int maxLen )
{
	M_SG_BLOCK blk;

	blk.size = maxLen;
	blk.data = (void *)buffer;

	return M_getstat( path, MSCAN_DUMPINTERNALS, (int32 *)&blk );
}

