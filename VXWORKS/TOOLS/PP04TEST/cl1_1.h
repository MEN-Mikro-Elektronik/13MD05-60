/**********************************************************************
Copyright (C) Siemens AG 1997 All rights reserved. Confidential

Permission is hereby granted, without written agreement and without
license or royalty fees, to use and modify this software and its
documentation for the use with SIEMENS PC104 cards.

IN NO EVENT SHALL SIEMENS AG BE LIABLE TO ANY PARTY FOR DIRECT,
INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF SIEMENS AG
HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

SIEMENS AG SPECIFICALLY DISCLAIMS ANY WARRANTIES,INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN
"AS IS" BASIS, AND SIEMENS AG HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
**=====================================================================
** PROJECT::   TCN
** MODULE::    PC104 CLASS1.1-1.3
** WORKFILE::  cl1_1.h
**---------------------------------------------------------------------
** TASK::
   Header for Application Programmers Interface (API) for PC104 access
   in CLASS1.1

**---------------------------------------------------------------------
** AUTHOR::    REINWALD_LU 
** CREATED::   30.04.1997
**---------------------------------------------------------------------
** CONTENTS::
      functions:
         MVBCInit()
         MVBCStart()
         MVBCStop()
         MVBCGetPort()
         MVBCPutPort()
         MVBCSetDSW()
         MVBCRetriggerWd()
	#if defined (MVB_M)
         MVBCIdle11()
    #else
         MVBCIdle()
	#endif
         MVBCGetDeviceAddress()
**---------------------------------------------------------------------
** NOTES::     -

**=====================================================================
** HISTORY::   AUTHOR::    Baddam Tirumal Reddy
               REVISION::  2.04.00
**---------------------------------------------------------------------
  
**********************************************************************/

#ifndef CL1_1_H_
#define CL1_1_H_

/*---------- structure of the link layer process control block ------*/
/* This structure exists separately for each created traffic store.  */
/* The structure is accessed via the traffic store id.               */

typedef struct  STR_LP_CB
{
    void *      pb_pit;         /* base ptr to port index table */
    void *      pb_pcs;         /* base ptr to port status & cntrl registers */
    void *      pb_prt;         /* base ptr to port data buffer */
    void *      pb_frc;         /* base ptr to force table */
    UNSIGNED16  prt_addr_max;   /* port address must not exceed this value */
    UNSIGNED16  prt_indx_max;   /* port index must not exceed this value */
    UNSIGNED8   ts_type;        /* type of traffic store */
    UNSIGNED8   hw_type;        /* hardware type */
    UNSIGNED8   ts_owner;       /* owner of traffic store */
    UNSIGNED8   pcs_power_of_2; /* size in powers of 2 */
    UNSIGNED8   irpt_sink;      /* handler for receive indication */
    UNSIGNED8   irpt_srce;      /* handler for sent indication is subscribed */
    UNSIGNED8   frc_flag;       /* forced data */
    UNSIGNED8   state;          /* init state */
} TYPE_LP_CB;

/*---------- macros and function prototypes which use TYPE_LP_CB ----*/
/* The macro returns the base pointer to the port control and status
   register. The parameter p_cb identifies the target traffic store.
   Return value:    pcs base pointer */
#define LPC_GET_PB_PCS(p_cb)  \
    (((TYPE_LP_CB*) (p_cb))->pb_pcs)


/* The macro returns the pointer to the data buffer base.
   The parameter p_cb identifies the target traffic store.
   Return value:    data base pointer */
#define LPC_GET_PB_PRT(p_cb)  \
    (((TYPE_LP_CB*) (p_cb))->pb_prt)


/* The macro returns the size of the pcs in powers of 2.
   The parameter p_cb identifies the target traffic store.
   Return value:    size of pcs in powers of 2 */
#define LPC_GET_PCS_SIZE_IN_PWR_2(p_cb)  \
    (((TYPE_LP_CB*) (p_cb))->pcs_power_of_2)


/* The macro calculates and returns the byte offset between the base of
   the pcs array and the pcs defined by the port index in bytes.
   The parameter p_cb identifies the target traffic store.
   Return value:    byte offset */
#define LPL_GEN_PCS_OFFSET(p_cb, prt_indx)  \
    ((prt_indx) << LPC_GET_PCS_SIZE_IN_PWR_2(p_cb))


/* The macro returns the base pointer to the port index table.
   The parameter p_cb identifies the target traffic store.
   Return value:    PIT base pointer */
#define LPC_GET_PB_PIT(p_cb)  \
    (((TYPE_LP_CB*) (p_cb))->pb_pit)


/* The macro returns the highest supported port (dock) index within the
   traffic store. The parameter p_cb identifies the target traffic store.
   Return value:    port index */
#define LPC_GET_PRT_INDX_MAX(p_cb)  \
    (((TYPE_LP_CB*) (p_cb))->prt_indx_max)


/* The macro returns the traffic store type (A, B, C).
   The parameter p_cb identifies the target traffic store.
   Return value:    traffic store type */
#define LPC_GET_TS_TYPE(p_cb)  \
   (((TYPE_LP_CB*) (p_cb))->ts_type)


/* The macro returns the pit type (LP_PIT_TYPE_A, _B) for the port index
   table of the target traffic store. The parameter p_cb identifies the
   target traffic store.
   !!! Attention: relies on odd constants for 8 bit and even constants for
                 16 bit pit types!
   Return value:    pit type */
#define LPC_GET_PIT_TYPE(p_cb)  \
   (LPC_GET_TS_TYPE(p_cb) & 1)


/* The macro returns the highest supported port address within the traffic
   store. The parameter p_cb identifies the target traffic store.
   Return value:    port address */
#define LPC_GET_PRT_ADDR_MAX(p_cb)  \
   (((TYPE_LP_CB*) (p_cb))->prt_addr_max)


/* The macro returns the state of the traffic store initialisation.
   The parameter p_cb identifies the target traffic store.
   Return value:    state */
#define LPC_GET_STATE(p_cb)  \
    (((TYPE_LP_CB*) (p_cb))->state)


/* The macro saves the base pointer force table to the control block.
   The parameter p_cb identifies the target traffic store. */
#define LPC_PUT_PB_FRC(p_cb, p_frc)  \
    ( ((TYPE_LP_CB*) (p_cb))->pb_frc = p_frc )


/* The macro saves the base pointer port data buffer to the control block.
   The parameter p_cb identifies the target traffic store. */
#define LPC_PUT_PB_PRT(p_cb, p_prt)  \
    ( ((TYPE_LP_CB*) (p_cb))->pb_prt = p_prt )


/* The macro saves the base pointer port control & status to the control block.
   The parameter p_cb identifies the target traffic store. */
#define LPC_PUT_PB_PCS(p_cb, p_pcs)  \
    ( ((TYPE_LP_CB*) (p_cb))->pb_pcs = p_pcs )


/* The macro saves the base pointer port index table to the control block.
   The parameter p_cb identifies the target traffic store. */
#define LPC_PUT_PB_PIT(p_cb, p_pit)  \
    ( ((TYPE_LP_CB*) (p_cb))->pb_pit = p_pit )


/* The macro sets the ownership (own or alien traffic store). An access to
   an alien traffic store is regulated by the magic word mechanism.
   The parameter p_cb identifies the target traffic store. */
#define LPC_PUT_TS_OWNER(p_cb, value)  \
    ( ((TYPE_LP_CB*) (p_cb))->ts_owner = value )


/* The macro stores the hardware type for the target traffic store control
   block. The parameter p_cb identifies the target traffic store. */
#define LPC_PUT_HW_TYPE(p_cb, my_hw_type)  \
    ( ((TYPE_LP_CB*) (p_cb))->hw_type = my_hw_type )


/* The macro "generates" and returns the byte offset between the base
   address of the array port index table and the element referenced
   by the prt_addr dependent on the PIT type.
   The parameter p_cb identifies the target traffic store.
   Return value:    byte offset */
#define LPL_GEN_PIT_OFFSET(p_cb, prt_addr)  \
    (LPC_GET_PIT_TYPE(p_cb) == LP_PIT_TYPE_A ? (prt_addr) : ((prt_addr) << 1))


/*---------- traffic store control block ----------------------------*/
/* Describes location and size of supported Traffic Store            */
/* The structure is accessed via the traffic store id.               */

typedef struct
{
    void                 *p_tm;         /* Pointer to Traffic Memory */
    TM_TYPE_SERVICE_AREA *p_sa;         /* Pointer to Service Area   */
    UNSIGNED16            checklist;    /* Init state for checking   */
	UNSIGNED8			  mvbc_type;    /* MVBC Controller type
										   MVBC01-0
										   MVBCS1-1					 */
} LCI_TYPE_CTRL_BLK;


/* values for checklist */
#define LCI_CHK_INIT  0x00 /* Initialized with lc_init               */
#define LCI_CHK_LC    0x01 /* Link Layer Common checked in           */


/*---------- port configuration list --------------------------------*/
/* The parameters required to open a traffic store port are          */
/* configured in the port configuration list.                        */

typedef struct STR_LP_PRT_CFG
{
    UNSIGNED16  prt_addr; /* The port index table is an array of port_nr.
                             This is the virtual connection between the
                             port address and the port number           */
    UNSIGNED16  prt_indx; /* The port index defines the port to use     */
    UNSIGNED16  size;     /* Defines the port size in bytes             */
    UNSIGNED16  type;     /* Defines the port type (sink / source)      */
} TYPE_LP_PRT_CFG;

/*---------- traffic store configuration ----------------------------*/
/* The parameters required to open a traffic store port are          */
/* configured in this type of list.                                  */

typedef struct      STR_LP_TS_CFG
{
	/* port index table */
    UNSIGNED32      pb_pit;     /* External representation of a pointer to
                                   the base address of the port index table */
	/* port control & status */
    UNSIGNED32      pb_pcs;     /* External representation of a pointer to the
                                   port control & status field base address */

    UNSIGNED32      pb_prt;     /* External representation of a pointer to
                                   the port base address */

    UNSIGNED32      pb_frc;     /* External representation of a pointer to
                                   the force table base address */
	
    UNSIGNED32      pb_def;     /* External representation of a pointer to
								   buffer with port default values.
								   NULL: Initialise all port bits to '1' */
    UNSIGNED8       ownership;  	/* LP_CFG_TS_OWNED */
    UNSIGNED8       ts_type;    	/* traffic store type */
    UNSIGNED16      dev_addr;   	/* MVB Address */
    UNSIGNED32      tm_start;   	/* Traffic store base address */

    UNSIGNED16      mcm;        	/* Memory configuration mode */
    UNSIGNED16      msq_offset; 	/* Message queue offset */
    UNSIGNED16      mft_offset; 	/* Master frame table offset */
    UNSIGNED16      line_cfg;   	/* Line configuration */
    UNSIGNED16      reply_to;   	/* Reply timeout coefficients */
    UNSIGNED16      prt_addr_max; 	/* Port index table range is
									   0 .... prt_addr_max */
    UNSIGNED16      prt_indx_max; 	/* Port range is 0 .... prt_indx_max */
    UNSIGNED16      prt_count;  	/* The structure is terminated after
									   prt_count elements */
    UNSIGNED32      p_prt_cfg;  /* External representation of a pointer the
								  port configuration (struct STR_LP_PRT_CFG)*/
} TYPE_LP_TS_CFG;




/* The macro calculates and returns the offset in bytes of the data port
   within the data port buffer.
   This is done using following algorithm:
   16 * (prt_indx - (prt_indx % 4)) + 8 * (prt_indx % 4)
   Return value:    byte offset */
#define LPL_GEN_PRT_OFFSET(prt_indx)  \
    ((((UNSIGNED16)(prt_indx) & 0xfffc) << 4) + (((prt_indx) & 0x3) << 3))


/* The macro checks whether port index max is an allowed value
   (count in fours)
   Return values:   MVB_LPS_OK
                    MVB_LPS_CONFIG if error */
#define LPL_CHK_PRT_INDX_MAX(prt_indx)  \
   ((((prt_indx) & LP_PRT_INDX_MAX_MASK) ==  \
     LP_PRT_INDX_MAX_MASK) ? MVB_LPS_OK : MVB_LPS_CONFIG)


/* The macro checks whether port address max is an allowed value
   (Count in twos)
   Return values:   MVB_LPS_OK
                    MVB_LPS_CONFIG if error */
#define LPL_CHK_PRT_ADDR_MAX(prt_addr)  \
   ((((prt_addr) & LP_PRT_ADDR_MAX_MASK) ==  \
     LP_PRT_ADDR_MAX_MASK) ? MVB_LPS_OK: MVB_LPS_CONFIG)


/*---------- values for access to control block ---------------------*/
#define LPC_TS_FREE    0
#define LPC_TS_IN_USE  1
#define LPC_TS_ERROR   2
#define LPC_STATE_MAX  3

/*---------- constants for PCS size and PCS type --------------------*/
#define LP_CFG_TS_ALIEN                  0
#define LP_CFG_TS_OWNED                  1
#define LP_CFG_SINK                      1
#define LP_CFG_SRCE                      2
#define LP_CFG_BIDIRECTIONAL             3
#define LP_CFG_02_BYTES                  2
#define LP_CFG_04_BYTES                  4
#define LP_CFG_08_BYTES                  8
#define LP_CFG_16_BYTES                 16
#define LP_CFG_32_BYTES                 32

/*---------- constants for PCS configuration and state info ---------*/
#define LPL_PCS_PASSIVE                 0
#define LPL_PCS_SINK                    LP_CFG_SINK
#define LPL_PCS_SRCE                    LP_CFG_SRCE
#define LPL_PCS_BDIR                    LP_CFG_BIDIRECTIONAL

/*---------- size of PCS for LP_TS_TYPE_B ---------------------------*/
#define LPL_PCS_B_SIZE_IN_PWR2          3

/*---------- constants to identify the TS structure types -----------*/
#define         LP_TS_TYPE_B            0x0001   /* MVB, MVBC        */
#define         LP_PIT_TYPE_A           0x0000   /*  8 Bit Port Indx */
#define         LP_PIT_TYPE_B           0x0001   /* 16 Bit Port Indx */

/*---------- masks for check of port address and index --------------*/
#define         LP_PRT_ADDR_MAX_MASK    1  /* for check of max. addr */
#define         LP_PRT_INDX_MAX_MASK    3  /* for check of max. index */

/*---------- constant to identify the bus (controller) --------------*/
#define         LP_HW_TYPE_MVBC         0x0001  /* MVB new Bus Controller */

/*---------- constants to identify the MVBC Type -----------------------*/
#define MVBCS1				   1	/* MVBC type is S1 - MVBCS1 */
#define MVBC_01                0	/* MVBC type is 01 - MVBC01 */


/*---------- function prototypes for internal use -------------------*/
UNSIGNED8 lc_tmo_config (
    UNSIGNED16   ts_id,     /* ID of traffic store                     */
    TM_TYPE_WORD stsr,      /* value for Sinktime Supervision Register */
    TM_TYPE_WORD docks      /* number of docks to be supervised        */
);

/*---------- function prototypes ------------------------------------*/
UNSIGNED8 MVBCInit (
    TYPE_LP_TS_CFG *Configuration,  /* Pointer to configuration data */
    UNSIGNED16      ts_id           /* ID of traffic store           */
);
/*{
    The function initializes MVBC and traffic store.
    Return values:  MVB_NO_ERROR
                    MVB_UNKNOWN_TRAFFICSTORE : wrong TS ID
                    MVB_ERROR_PARA           : inconsistent or wrong params
                    MVB_ERROR_NO_MVBC        : MVBC not found
                    MVB_ERROR_MVBC_RESET     : MVBC not correct resetted
}*/


UNSIGNED8 MVBCStart(
    UNSIGNED16 ts_id     /* ID of traffic store */
);
/*{
    The function gives the last kick to MVBC.
    Return values:  MVB_NO_ERROR
                    MVB_UNKNOWN_TRAFFICSTORE : wrong TS ID
                    MVB_ERROR_MVBC_INIT      : MVBC not initialized
}*/


UNSIGNED8 MVBCStop (
    UNSIGNED16 ts_id         /* ID of traffic store */
);
/*{
    The function stops MVBC operation.
    Return values:  MVB_NO_ERROR
                    MVB_UNKNOWN_TRAFFICSTORE : wrong TS ID
}*/


UNSIGNED8 MVBCGetPort(
    UNSIGNED16  Port,        /* Port number            */
    UNSIGNED16 *PortBuffer,  /* Pointer to port buffer */
    UNSIGNED16  Tack,        /* Limit for age of data  */
    UNSIGNED16 *Age,         /* Real age of data       */
    UNSIGNED16  ts_id        /* ID of traffic store    */
);
/*{
    The function reads data from MVBC port into the port buffer.
    Return values:  MVB_NO_ERROR
                    MVB_UNKNOWN_TRAFFICSTORE : wrong TS ID
                    MVB_ERROR_MVBC_STOP      : MVBC is not running
                    MVB_ERROR_PORT_UNDEF     : port not found in TS or port
                                               number/index not ok
                    MVB_WARNING_NO_SINK      : not a sink port
                    MVB_WARNING_OLD_DATA     : read data old
}*/


UNSIGNED8 MVBCPutPort(
    UNSIGNED16  Port,        /* Port number            */
    UNSIGNED16 *PortBuffer,  /* Pointer to port buffer */
    UNSIGNED16  ts_id        /* Traffic Store ID       */
);
/*{
    The function writes data from the port buffer into MVBC port.
    Return values:  MVB_NO_ERROR
                    MVB_UNKNOWN_TRAFFICSTORE : wrong TS ID
                    MVB_ERROR_PORT_UNDEF     : port not found in TS or port
                                               number/index not ok
                    MVB_ERROR_NO_SRC         : not a source port
}*/


UNSIGNED8 MVBCSetDSW(
    UNSIGNED16 ts_id,   /* ID of Traffic Store            */
    UNSIGNED16 mask,    /* Device Status Word Mask Bits   */
    UNSIGNED16 value    /* New values for affected fields */
);                     
/*{
    The function sets the Device-Status-Word.
    Only the bits 1, 4 and 5 may be affected.
    The other bits shall be left.
    Return values:  MVB_NO_ERROR
                    MVB_UNKNOWN_TRAFFICSTORE : wrong TS ID
}*/


#if defined (MVB_M)
UNSIGNED8 MVBCIdle11(
#else
UNSIGNED8 MVBCIdle(
#endif
    UNSIGNED16  ts_id       /* Traffic Store ID */
);
/*{
    The function shall be called cyclicaly.
    It set the DSW-Bits LAA, RLD.
    Return values:  MVB_NO_ERROR
                    MVB_UNKNOWN_TRAFFICSTORE : wrong TS ID
}*/


UNSIGNED8 MVBCGetDeviceAddress(
    UNSIGNED16  *devAddress,
    UNSIGNED16  ts_id        /* Traffic Store ID       */
);
/*{
    The function resolves the current device-address.
 
    Return values:  MVB_NO_ERROR
                    MVB_UNKNOWN_TRAFFICSTORE : wrong TS ID
}*/

UNSIGNED8 MVBCRetriggerWd(
    UNSIGNED16 ts_id, 
    UNSIGNED16 trig_val
);
/*{
   The function retriggers the MVBCS1 watchdog. Details s. MVBCS1 datasheet
   
   Return values:  MVB_NO_ERROR
                   MVB_UNKNOWN_TRAFFICSTORE : wrong TS ID
				   MVB_ERROR_MVBC_INIT
				   MVB_WATCHDOG_NOT_AVAIL   : If mvbcs1 is not available then no watchdog function
											  and returns with this error 

}*/

#endif
