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
** WORKFILE::  cl1_1.c
**---------------------------------------------------------------------
** TASK::
   Application Programmers Interface (API) for PC104 access.
   Functions for initialization / handling of MVBC and
   processing data in CLASS1.1

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
        MVBCIdle() or MVBCIdle11() when in MVBM-Mode
        MVBCGetDeviceAddress()
        MVBCRetriggerWd()
**---------------------------------------------------------------------
** NOTES::     -

**=====================================================================
** HISTORY::   AUTHOR::    Baddam Tirumal Reddy
			   REVISION::  2.04.00
**---------------------------------------------------------------------  
**********************************************************************/

#include <stdio.h>
#include "config.h"
#include "host_dep.h"
#include "dpr_dep.h"
#include "cl1_ret.h"
#include "cl1_1.h"
#if defined(CLASS12) || defined(CLASS13)
#include "cl1_2u3.h"
#endif

/*---------- global variables ---------------------------------------*/

/* Traffic Store Control Block */
static LCI_TYPE_CTRL_BLK lci_ctrl_blk[LCX_TM_COUNT];

/* The control block stores the process control information.
   Each traffic store is identified by the index "ts_id" (0....LCX_TM_COUNT-1).
*/
static TYPE_LP_CB        lp_cb[LCX_TM_COUNT];

#if defined (MVB_M)
extern UNSIGNED8	gMVBMState;
extern UNSIGNED8	gbAPIInternal;
#endif

#if 0
extern unsigned int G_stop;
#endif

/*---------- global constants ---------------------------------------*/

/* Waitstates for mvbc access */
const static UNSIGNED16 lci_waitstates[LCX_TM_COUNT] = LCX_WAITSTATES;

/* Arbitration-Modes for mvbc access */
const static UNSIGNED16 lci_arb_modes[LCX_TM_COUNT] = LCX_ARB_MODE;

/* Offset constants for Service Area */
const static UNSIGNED32 tm_sa_offs[] = TM_SERVICE_OFFSETS;

#ifdef  DBG_CL11
#define DBG_FCTNNAME   fprintf( stderr, "--> %s()\n", __FUNCTION__);
#define DBG_EXIT       fprintf( stderr, "<-- %s()\n", __FUNCTION__);
#define CL11DBG(x...)  fprintf( stderr, x);
#else
#define DBG_EXIT
#define DBG_FCTNNAME  
#define CL11DBG(x...) 
#endif

/******************************************************************************
*******************************************************************************
*******************************************************************************

                                 internal functions  

*******************************************************************************
*******************************************************************************
******************************************************************************/

/******************************************************************************
*   name:         lpl_tgl_prt_page
*   function:     The pcs page bit is toggled 
*   input:        Pointer to port control and status register
*   output:       -
******************************************************************************/
static void lpl_tgl_prt_page(
    void *p_pcs         /* Pointer to port control and status register */
)
{
    TM_TYPE_PCS_WORD1 pcs1;
    VOL int i = 1;
    pcs1.w = readWordFromTS((void*)(&((TM_TYPE_PCS *) p_pcs)->word1.w));
    pcs1.w ^= TM_PCS_VP_MSK;
    writeWordToTS((void*)(&((TM_TYPE_PCS *) p_pcs)->word1.w), pcs1.w);
    i = i + 1;
    writeWordToTS((void*)(&((TM_TYPE_PCS *) p_pcs)->word1.w), pcs1.w);
    return;
} /* lpl_tgl_prt_page */


/******************************************************************************
*   name:         lpc_put_state
*   function:     This function is used during initialisation to declare a 
*                 traffic store "in use".
*   input:        Pointer to control block
*                 state to save
*   output:       -
******************************************************************************/
static void lpc_put_state (
    void *p_cb,             /* Pointer to traffic store control block */
    UNSIGNED8 state         /* state to save */
)
{
    if (state <= LPC_STATE_MAX) {
        ((TYPE_LP_CB*) p_cb)->state = state;
    } else {
        ((TYPE_LP_CB*) p_cb)->state = LPC_TS_ERROR;
    }
    return;
} /* lpc_put_state */



/******************************************************************************
*   name:         lpc_put_ts_type
*   function:     The function decodes the traffic store type and stores it as 
*                 type of individual traffic store elements pit, pcs and prt.
*                 pcs_power_of_2 is coded size of the pcs in byte (4/8 Bytes).
*   input:        Pointer to control block
*                 Type of Traffic Store
*   output:       MVB_LPS_OK
*                 MVB_LPS_ERROR
******************************************************************************/
static UNSIGNED8 lpc_put_ts_type (
    void *p_cb,                 /* Pointer to control block */
    UNSIGNED8 ts_type           /* Type of Traffic Store */
)
{
    UNSIGNED8 result = MVB_LPS_OK;

    ((TYPE_LP_CB *)p_cb)->ts_type = ts_type;

    if  (ts_type == LP_TS_TYPE_B) {
        ((TYPE_LP_CB*) p_cb)->pcs_power_of_2 = LPL_PCS_B_SIZE_IN_PWR2;
    }else{
        result = MVB_LPS_ERROR;
    }

    return (result);
} /* lpc_put_ts_type */



/******************************************************************************
*   name:         lpc_put_max
*   function:     The function stores max values for traffic store 
*                 prt_max is used for range checking and to derive size 
* 				of force table.
*                 !!! Attention, prt_max must be the most significant port of
*                     a 4 port dock set ( x x x x x x 1 1 ).
*   input:        Pointer to control block
*                 Maximum Port Address
*                 Maximum Port Index
*   output:       -
******************************************************************************/
static void lpc_put_max (
    void *p_cb,             /* Pointer to control block */
    UNSIGNED16 prt_addr_max,
    UNSIGNED16 prt_indx_max
)
{
    ((TYPE_LP_CB *) p_cb)->prt_addr_max = prt_addr_max;
    ((TYPE_LP_CB *) p_cb)->prt_indx_max = prt_indx_max;
} /* lpc_put_max */



/******************************************************************************
*   name:         lpc_open
*   function:     This function checks whether the traffic store identifier is 
*                 defined and within range.
*                 It returns the pointer to the control block of the target TS
*   input:        Traffic Store ID
*                 Pointer to pointer of control block
*   output:       MVB_LPS_OK
*                 MVB_LPS_CONFIG
*                 MVB_LPS_UNKNOWN_TS_ID
******************************************************************************/
static UNSIGNED8 lpc_open (
    UNSIGNED16 ts_id,       /* Traffic Store ID */
    void **p_cb             /* Pointer to pointer of control block */
)
{
    UNSIGNED8 result;

    *p_cb = (void *) &lp_cb[ts_id];

    if (ts_id < LCX_TM_COUNT)
    {
        if (LPC_GET_STATE (*p_cb) != LPC_TS_IN_USE)
        {
            result = MVB_LPS_CONFIG;
        }
        else
        {
            result = MVB_LPS_OK;
        }
    }
    else
    {
        result = MVB_LPS_UNKNOWN_TS_ID;
    }
    return (result);
} /* lpc_open */



/******************************************************************************
*   name:         lpl_get_prt_indx
*   function: This function reads the prt_indx stored under to the prt_addr in
*             the array of character or word port idx table (dependent of mcm).
*   input:        Memory Control Mode
*                 Pointer to port index table
*                 Port address
*   output:       port index
******************************************************************************/
static UNSIGNED16 lpl_get_prt_indx(
    UNSIGNED16 mcm,     /* Memory Control Mode */
    void      *pb_pit,  /* Pointer to port index table*/
    UNSIGNED16 prt_addr /* Port address */
)
{

	DBG_FCTNNAME;

    if ((mcm == TM_MCM_16K) || (mcm == TM_MCM_32K))
    {   /* Port odd or even */
        if ( (prt_addr) & 0x0001)  /* odd */
        {
            UNSIGNED16 pit_word;
            pit_word = readWordFromTS((UNSIGNED16*) (pb_pit) +
                                                    ((prt_addr-1) >> 1));
            return (pit_word >> 8);
        }
        else
        {
            UNSIGNED16 pit_word;
            pit_word = readWordFromTS((UNSIGNED16*) (pb_pit) +
                                                    ((prt_addr) >> 1));
            return (pit_word & 0x00FF);
        }
    }
    else
    {
        return (readWordFromTS((UNSIGNED16*) (pb_pit) + prt_addr));
    }
} /* lpl_get_prt_indx */



/******************************************************************************
*   name:         lpl_get_prt_fcode
*   function:     The pcs fcode is read
*   input:        Pointer to PCS
*   output:       fcode
******************************************************************************/
static UNSIGNED8 lpl_get_prt_fcode(
    void *p_pcs         /* Pointer to port control and status register */
)
{               
    UNSIGNED16 temp = readWordFromTS((void*)(&((TM_TYPE_PCS *) p_pcs)->word0.w));
    temp = (temp & TM_PCS_FCODE_MSK) >> TM_PCS_FCODE_OFF;
    return ((UNSIGNED8)temp);
} /* lpl_get_prt_fcode */



/******************************************************************************
*   name:         lpl_put_prt_size
*   function:     The function writes the function code of the port size to the pcs
*   input:        Pointer to PCS
*                 Size
*   output:       -
******************************************************************************/
static void lpl_put_prt_size (
    void      *p_pcs,      /* Pointer to port control and status register */
    UNSIGNED16 size
)
{
    UNSIGNED16 fcode = 0;

    size = size >> 1;
    while ((size != 1) && (size != 0))
    {
        size = size >> 1;
        fcode++;
    }
    LPL_PUT_PRT_FCODE (p_pcs, fcode);
    return;
} /* lpl_put_prt_size */



/******************************************************************************
*   name:         lpl_put_prt_indx
*   function:     The 8 or 16 bit port index table (dependent of mcm) is loaded
*                 with the port index under port address. 
*   input:        Memory Control Mode
*                 Pointer to port index table
*                 Port address
*                 Port index
*   output:       -
******************************************************************************/
static void lpl_put_prt_indx(
    UNSIGNED16  mcm,        /* Memory Control Mode */
    UNSIGNED16 *pb_pit,     /* Pointer to port index table */
    UNSIGNED16  prt_addr,   /* Port address */
    UNSIGNED16  prt_indx    /* Port index */
)
{

	DBG_FCTNNAME;
	CL11DBG("mcm=%d prt_addr=%d prt_indx=%d\n", mcm, prt_addr, prt_indx );

    if ((mcm == TM_MCM_16K) || (mcm == TM_MCM_32K))
    {   /* Port odd or even */
        if ( (prt_addr) & 0x0001)
        {
            UNSIGNED16 pit_word;
            pit_word = readWordFromTS((UNSIGNED16*) (pb_pit) +
                                                    ((prt_addr-1) >> 1));
            pit_word &= 0x00FF;
            pit_word |= (UNSIGNED16)(prt_indx) << 8;
            writeWordToTS(((UNSIGNED16*) (pb_pit) + ((prt_addr-1) >> 1)),
                          pit_word);
        }
        else
        {
            UNSIGNED16 pit_word;
            pit_word = readWordFromTS((UNSIGNED16*) (pb_pit) +
                                      ((prt_addr) >> 1));
            pit_word &= 0xFF00;
            pit_word |= (prt_indx);
            writeWordToTS(((UNSIGNED16*) (pb_pit) + ((prt_addr) >> 1)),
                          pit_word);
        }
    }
    else /* other memory model */
    {
        writeWordToTS((UNSIGNED16*) (pb_pit) + prt_addr, prt_indx);
    }
    return;
} /* lpl_put_prt_indx */



/******************************************************************************
*   name:         lpl_put_def_values
*   function:     The function copies the default value buffer to the prt-table. 
*   input:        Pointer to control block
*                 Long with pointer to default buffer
*   output:       -
******************************************************************************/
static void lpl_put_def_values (
    void      *p_cb,    /* Pointer to control block */
    UNSIGNED32 p_def    /* Long with pointer to default buffer */
)
{
    UNSIGNED16 i;     /* loop control for port sets   */
    UNSIGNED16 j;     /* loop control for port size */
    UNSIGNED8 *p_prt;

    i     = (LPC_GET_PRT_INDX_MAX (p_cb) + 1) >> 2;
    p_prt =  LPC_GET_PB_PRT (p_cb);

    while (i > 0)
    {
		--i;
        j = sizeof(TYPE_LPL_PRT_PGE) >> 1;
        while (j > 0)
        {
			--j;
            writeWordToTS(&((TYPE_LPL_PRT_SET *) p_prt)->page_0,
                          *P_OF_ULONG(UNSIGNED16*, p_def));
            writeWordToTS(&((TYPE_LPL_PRT_SET *) p_prt)->page_1,
                          *P_OF_ULONG(UNSIGNED16*, p_def));
            p_def+=sizeof(UNSIGNED16);
        }
        p_prt += (2 * sizeof (TYPE_LPL_PRT_PGE));
    }
    return;
} /* lpl_put_def_values */


/******************************************************************************
*   name:         lpa_memset
*   function:     The function writes the parameter "value" to the traffic 
*                 store of size.
*                 !!! Attention: size must be even! 
*   input:        Pointer to destination
*                 word value to write
*                 size in bytes
*   output:       -
******************************************************************************/
static void lpa_memset (
    void        *p_dest,    /* Pointer to destination */
    TM_TYPE_WORD value,     /* word value to write */
    UNSIGNED16   size       /* size in bytes */
)
{
    UNSIGNED16    count;
    TM_TYPE_WORD *p_trgt;

    p_trgt = (TM_TYPE_WORD*)p_dest;
    count  = size >> 1;

	SRVWDT();

    while (count > 0)
    {
		--count;
        writeWordToTS(p_trgt++, value);
    }
    
	SRVWDT();

	return;
} /* lpa_memset */
/*{
    The function writes the parameter "value" to the traffic store of size.
    !!! Attention: size must be even!
}*/



/******************************************************************************
*   name:         lci_check_ts_valid
*   function:     The function checks if ts_id is within specified boundaries 
*                 and that the TS is properly configured
*   input:        Traffic Store ID
*   output:       MVB_LC_OK
*                 MVB_LC_REJECT
******************************************************************************/
static UNSIGNED8 lci_check_ts_valid (
    UNSIGNED16 ts_id         /* Traffic Store ID */
)
{
    UNSIGNED8 result = (ts_id < LCX_TM_COUNT) ? MVB_LC_OK : MVB_LC_REJECT;

    if (result == MVB_LC_OK) {
        result = (lci_ctrl_blk[ts_id].checklist != LCI_CHK_INIT) ?
                                        MVB_LC_OK : MVB_LC_REJECT;
    }
    return (result);
} /* lci_check_ts_valid */


/******************************************************************************
*   name:         lc_set_device_status_word
*   function:     The function modifies Device Status Word from TS Service Area
*   input:        Traffic Store ID
*                 Device Status Word Mask Bits
*                 New values for affected fields
*   output:       MVB_LC_OK
*                 MVB_LC_REJECT
******************************************************************************/
static UNSIGNED8 lc_set_device_status_word(
    UNSIGNED16 ts_id,   /* Traffic Store ID               */
    UNSIGNED16 mask,    /* Device Status Word Mask Bits   */
    UNSIGNED16 value    /* New values for affected fields */
)
{
    UNSIGNED8 result = MVB_LC_OK;
    /* DSW accessed to TM: Data Area of Phys. Port FC15, Page 0, Word 0 */

    /* Data Area of Physical Ports */
    TM_TYPE_DATA *p_data = (TM_TYPE_DATA*)(lci_ctrl_blk[ts_id].p_sa->pp_data);

    /* Pointer to Device Status Word */
    TM_TYPE_WORD *p_dsw = &TM_1_DATA_WD(p_data,TM_PP_FC15,TM_PAGE_0,0);

    if (lci_check_ts_valid(ts_id) == MVB_LC_OK)
    {
        writeWordToTS(p_dsw,
      (TM_TYPE_WORD)((readWordFromTS(p_dsw) & ~(mask)) | ((value) & (mask))));
    } else {
        result = MVB_LC_REJECT;
    }
    return (result);
} /* lc_set_device_status_word */


/******************************************************************************
*   name:         lc_set_laa_rld
*   function:     The function retrieves LAA and RLD from MVBC Decoder Register
*                 and updates it in the Device Status Word
*   input:        -
*   output:       MVB_LC_OK
*                 return values of lc_set_device_status_word()
******************************************************************************/
static UNSIGNED8 lc_set_laa_rld (void)
{
    UNSIGNED16    i;           /* Loop Index                        */
    TM_TYPE_WORD dr;           /* Contents of Decoder Register      */
    UNSIGNED16   laa;          /* Line A Active                     */
    UNSIGNED16   rld;          /* Redundant Line Disturbed          */
    UNSIGNED8    result = MVB_LC_OK;

    for (i = 0; (i < LCX_TM_COUNT) && (result == MVB_LC_OK); i++)
    {   /* All control blocks */
        if (lci_ctrl_blk[i].checklist != LCI_CHK_INIT)
        {
            dr = readWordFromTS((void*)(&lci_ctrl_blk[i].p_sa->int_regs.dr.w));

            laa  = (dr & TM_DR_LAA) != 0 ? LC_DSW_LAA_SET : LC_DSW_LAA_CLR;
            rld  = (dr & TM_DR_RLD) != 0 ? LC_DSW_RLD_SET : LC_DSW_RLD_CLR;

            result = lc_set_device_status_word( i, 
												LC_DSW_LAA_MSK | \
												LC_DSW_RLD_MSK, 
												(UNSIGNED16)(laa | rld) );
        }
    }
    return (result);
} /* lc_set_laa_rld */



/******************************************************************************
*   name:         lp_ts_open_port
*   function:     This function completes PIT and PCS for each port in p_cfg
*   input:        Memory Control Mode
*                 Pointer to control block
*                 Pointer to ts configuration
*   output:       MVB_LPS_OK
*                 MVB_LPS_CONFIG
******************************************************************************/
static UNSIGNED8 lp_ts_open_port (
    UNSIGNED16             mcm,     /* Memory Control Mode           */
    void                  *p_cb,    /* Pointer to control block      */
    const TYPE_LP_PRT_CFG *p_cfg    /* Pointer to port configuration */
)
{
    UNSIGNED8  *p_pcs;     /*   temp pointer pcs */
    void       *pb_pit;
    UNSIGNED16  prt_addr;
    UNSIGNED16  prt_indx;
    UNSIGNED16  prt_type;
    UNSIGNED16  prt_size;
    UNSIGNED8   result = MVB_LPS_OK;

    prt_addr = p_cfg->prt_addr;
    prt_indx = p_cfg->prt_indx;
    prt_type = p_cfg->type;
    prt_size = p_cfg->size;

    pb_pit   = LPC_GET_PB_PIT (p_cb);
    p_pcs    = (((UNSIGNED8*) LPC_GET_PB_PCS (p_cb)) +
                          LPL_GEN_PCS_OFFSET (p_cb, prt_indx));

    result = !((prt_addr <= LPC_GET_PRT_ADDR_MAX (p_cb)) &&
               (prt_indx <= LPC_GET_PRT_INDX_MAX (p_cb)));
                                      /* port address / port index ??? */
    if (result != MVB_LPS_OK)
    {
        return (MVB_LPS_CONFIG);
    }

    result = ((prt_type != LPL_PCS_PASSIVE) &&
              (prt_type != LPL_PCS_SINK   ) &&
              (prt_type != LPL_PCS_SRCE   ) &&
              (prt_type != LPL_PCS_BDIR   ));
                                       /* port type error               */

    if (result == MVB_LPS_OK)
    {
        lpl_put_prt_indx (mcm, pb_pit, prt_addr, prt_indx);
        lpl_put_prt_size (p_pcs, prt_size);
        LPL_PUT_PRT_TYPE (p_pcs, prt_type);
    }
    else
    {
        result = MVB_LPS_CONFIG;
    }

    return (result);
} /* lp_ts_open_port */


/******************************************************************************
*   name:         lpl_clr_pit
*   function:     The function clears the port address table. This will have 
*                 the affect of closing all ports in this traffic store
*   input:        Pointer to control block
*   output:       -
******************************************************************************/
static void lpl_clr_pit (
    void *p_cb      /* Pointer to control block */
)
{
    void   *pb_pit;
    UNSIGNED16  pit_size;

    if ((pb_pit  = LPC_GET_PB_PIT (p_cb)) != NULL)
    {
        pit_size = LPL_GEN_PIT_OFFSET (p_cb,
                                       LPC_GET_PRT_ADDR_MAX (p_cb) + 1);
        lpa_memset (pb_pit, 0, pit_size);
    }
    return;
} /* lpl_clr_pit */


/******************************************************************************
*   name:         lp_ts_create_frc
*   function:     This function creates a zero-initialised force table array
*   input:        Pointer to control block
*                 Pointer to ts configuration
*   output:       MVB_LPS_OK
*                 MVB_LPS_CONFIG
******************************************************************************/
static UNSIGNED8 lp_ts_create_frc (
    void *p_cb,                 /* Pointer to control block    */
    const TYPE_LP_TS_CFG *p_cfg /* Pointer to ts configuration */
)
{
    UNSIGNED16  frc_size;
    void       *p_frc;
    UNSIGNED8   result = MVB_LPS_OK;

    frc_size = LPL_GEN_PRT_OFFSET (LPC_GET_PRT_INDX_MAX (p_cb) + 1);

    if ((p_frc = P_OF_ULONG(void*, p_cfg->pb_frc)) == NULL)
    {
        result = MVB_LPS_CONFIG;
    }

    LPC_PUT_PB_FRC (p_cb, p_frc);

    if (result == MVB_LPS_OK)
    {
        lpa_memset (p_frc, 0, frc_size);
    }

    return (result);
} /* lp_ts_create_frc */



/******************************************************************************
*   name:         lp_ts_create_prt
*   function:     This function creates a one-initialised prt data buffer array
*   input:        Pointer to control block
*                 Pointer to ts configuration
*   output:       MVB_LPS_OK
*                 MVB_LPS_CONFIG
******************************************************************************/
static UNSIGNED8 lp_ts_create_prt (
    void *p_cb,                 /* Pointer to control block    */
    const TYPE_LP_TS_CFG *p_cfg /* Pointer to ts configuration */
)
{
    UNSIGNED16 prt_size;
    void      *p_prt;
    UNSIGNED8  result = MVB_LPS_OK;

    prt_size = LPL_GEN_PRT_OFFSET (LPC_GET_PRT_INDX_MAX (p_cb) + 1);

    if ((p_prt = P_OF_ULONG(void*, p_cfg->pb_prt)) == NULL)
    {
        result = MVB_LPS_CONFIG;
    }

    LPC_PUT_PB_PRT (p_cb, p_prt);

    if ((p_cfg->ownership == LP_CFG_TS_OWNED) &&
        (result           == MVB_LPS_OK))
    {
        if (p_cfg->pb_def != 0 /* NULL */)
        {
            lpl_put_def_values (p_cb, p_cfg->pb_def);
        }
        else
        {
            lpa_memset (p_prt, 0xFFFF, prt_size);
        }
    }
    return (result);
} /* lp_ts_create_prt */



/******************************************************************************
*   name:         lp_ts_create_pcs
*   function:     This function creates a zero-initialised pcs array
*   input:        Pointer to control block
*                 Pointer to ts configuration
*   output:       MVB_LPS_OK
*                 MVB_LPS_CONFIG
******************************************************************************/
static UNSIGNED8 lp_ts_create_pcs (
    void *p_cb,                 /* Pointer to control block    */
    const TYPE_LP_TS_CFG *p_cfg /* Pointer to ts configuration */
)
{
    UNSIGNED16 pcs_size;
    void      *p_pcs;
    UNSIGNED8  result = MVB_LPS_OK;

    pcs_size = LPL_GEN_PCS_OFFSET (p_cb, LPC_GET_PRT_INDX_MAX (p_cb) + 1);

    if ((p_pcs = P_OF_ULONG(void*, p_cfg->pb_pcs)) == NULL)
    {
        result = MVB_LPS_CONFIG;
    }

    LPC_PUT_PB_PCS (p_cb, p_pcs);

    if ((p_cfg->ownership == LP_CFG_TS_OWNED) &&
        (result           == MVB_LPS_OK))
    {
        lpa_memset (p_pcs, 0, pcs_size);
    }

    return (result);
} /* lp_ts_create_pcs */



/******************************************************************************
*   name:         lp_ts_create_pit
*   function:     This function creates a zero-initialised port index table
*   input:        Pointer to control block
*                 Pointer to ts configuration
*   output:       MVB_LPS_OK
*                 MVB_LPS_CONFIG
******************************************************************************/
static UNSIGNED8 lp_ts_create_pit (
    void *p_cb,                 /* Pointer to control block    */
    const TYPE_LP_TS_CFG *p_cfg /* Pointer to ts configuration */
)
{
    void      *p_pit;
    UNSIGNED8  result = MVB_LPS_OK;

    if ((p_pit = P_OF_ULONG(void*, p_cfg->pb_pit)) == NULL)
    {
        result = MVB_LPS_CONFIG;
    }

    LPC_PUT_PB_PIT (p_cb, p_pit);

    if ((p_cfg->ownership == LP_CFG_TS_OWNED) &&
        (result           == MVB_LPS_OK))
    {
        lpl_clr_pit (p_cb);
    }

    return (result);
} /* lp_ts_create_pit */


/******************************************************************************
*   name:         lp_ts_create
*   function:     The function creates a traffic store
*   input:        Pointer to control block
*                 Pointer to ts configuration
*   output:       MVB_LPS_OK
*                 MVB_LPS_CONFIG
******************************************************************************/
static UNSIGNED8 lp_ts_create (
    void *p_cb,                 /* Pointer to control block    */
    const TYPE_LP_TS_CFG *p_cfg /* Pointer to ts configuration */
)
{
    UNSIGNED8 result = MVB_LPS_OK;

    result = (LPL_CHK_PRT_ADDR_MAX (p_cfg->prt_addr_max) != MVB_LPS_OK);
                                                /* port address max ??? */
    if (result != MVB_LPS_OK)
    {
        return (MVB_LPS_CONFIG);
    }
    result = (LPL_CHK_PRT_INDX_MAX (p_cfg->prt_indx_max) != MVB_LPS_OK);
                                                /* port index   max ??? */
    if (result != MVB_LPS_OK)
    {
        return (MVB_LPS_CONFIG);
    }

    lpc_put_max (p_cb, p_cfg->prt_addr_max, p_cfg->prt_indx_max);

    if (lpc_put_ts_type (p_cb, p_cfg->ts_type) != MVB_LPS_OK)
    {
        result = MVB_LPS_ERROR;
    }
    else if (lp_ts_create_pit (p_cb, p_cfg) != MVB_LPS_OK)
    {
        result = MVB_LPS_ERROR;
    }
    else if (lp_ts_create_pcs (p_cb, p_cfg) != MVB_LPS_OK)
    {
        result = MVB_LPS_ERROR;
    }
    else if (lp_ts_create_prt (p_cb, p_cfg) != MVB_LPS_OK)
    {
        result = MVB_LPS_ERROR;
    }
    else if (lp_ts_create_frc (p_cb, p_cfg) != MVB_LPS_OK)
    {
        result = MVB_LPS_ERROR;
    }
    else
    {
        LPC_PUT_TS_OWNER (p_cb, p_cfg->ownership);
    }

    return (result);
} /* lp_ts_create */



/******************************************************************************
*   name:         lpc_kill_ts
*   function:     The function clears the control block for the opened ts_id 
*   input:        Pointer to control block
*   output:       -
******************************************************************************/
static void lpc_kill_ts (
    void *p_cb      /* Pointer to control block */
)
{
    UNSIGNED16  count;
    UNSIGNED8  *p_trgt;

    p_trgt = (UNSIGNED8*) p_cb;
    count  = sizeof(TYPE_LP_CB);

    while (count > 0)
    {
		--count;
        *p_trgt++ = 0;
    }
    return;
} /* lpc_kill_ts */



/******************************************************************************
*   name:         lp_create
*   function:     The function creates a traffic store, announces the traffic 
*                 store to control block and starts the bus hardware
*   input:        Traffic Store ID
*                 Type of Hardware
*                 Pointer to ts configuration
*   output:       MVB_LPS_OK
*                 MVB_LPS_UNKNOWN_TS_ID
*                 return values of lp_ts_create()
*                 return values of lp_ts_open_port()
******************************************************************************/
static UNSIGNED8 lp_create (
    UNSIGNED16            ts_id,    /* Traffic Store ID            */
    UNSIGNED8             hw_type,  /* Type of Hardware            */
    const TYPE_LP_TS_CFG *p_ts_cfg  /* Pointer to ts configuration */
)
{
    const TYPE_LP_PRT_CFG *p_prt_cfg;
    void                  *p_cb;
    UNSIGNED16             prt_count;
    TM_TYPE_MCR            local_mcr;/*Local copy of Memory Config. Reg.*/
    UNSIGNED8              result = MVB_LPS_OK;

    result = lpc_open (ts_id, &p_cb);

    if (result == MVB_LPS_UNKNOWN_TS_ID)
    {                                          /* Traffic Store ID ??? */
        return (result);
    }
    lpc_kill_ts (p_cb);

    LPC_PUT_HW_TYPE (p_cb, hw_type);

    result = lp_ts_create (p_cb, p_ts_cfg);

    prt_count = p_ts_cfg->prt_count;
    p_prt_cfg = P_OF_ULONG(const TYPE_LP_PRT_CFG*, p_ts_cfg->p_prt_cfg);
    local_mcr.w = readWordFromTS((void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.mcr.w));

    while   ((prt_count > 0) && (result == MVB_LPS_OK))
    {
		--prt_count;
        result    = lp_ts_open_port((UNSIGNED16)(GETMCRMCM(local_mcr)), p_cb, p_prt_cfg);
        p_prt_cfg = p_prt_cfg + 1;
    }

    if (result == MVB_LPS_OK)
    {
        lpc_put_state (p_cb, LPC_TS_IN_USE);
    }
    else
    {
        if (p_ts_cfg->prt_count != 0)
        {
            lpl_clr_pit (p_cb);
        }

        lpc_kill_ts (p_cb);
    }

    return (result);
} /* lp_create */


/******************************************************************************
*   name:         lc_hardw_config
*   function:     This function performs additional HW Configuration:
*                    line_config      = { LC_CH_A, LC_CH_B, LC_CH_BOTH }
*                    treply_config    = { LC_TREPLY_21US, LC_TREPLY_43US,
*                                         LC_TREPLY_64US, LC_TREPLY_85US }
*                 Call is optional, not mandatory.  Default values are
*                 LC_CH_BOTH and LC_REPLY_43US, active since MVBC reset.
*                 Attention: lc_config must be called before.
*   input:        Traffic Store ID
*                 MVB line configuration
*                 MVB reply timeout configuration
*   output:       MVB_LC_OK
*                 MVB_LC_REJECT
******************************************************************************/
static UNSIGNED8 lc_hardw_config (
    UNSIGNED16 ts_id,           /* Traffic Store ID                */
    UNSIGNED16 line_config,     /* MVB line configuration          */
    UNSIGNED16 treply_config    /* MVB reply timeout configuration */
)
{
    TM_TYPE_INT_REGS *p_ir;         /* MVBC Internal Registers           */
    TM_TYPE_SCR       local_scr;    /* Local copy of SCR                 */
    TM_TYPE_SCR       testm_scr;    /* Local copy of SCR, Test Mode set  */
    TM_TYPE_RWORD    *p_dr;         /* Pointer to Decoder Register       */
                                    /* Timeout after trying to switch line*/
    UNSIGNED16        limit = LCI_LS_LIMIT;

    UNSIGNED16        exp_laa;      /* Exp. value of LAA after swtiching */
    UNSIGNED8         result = MVB_LC_OK; /* Return Value, default: OK   */

	DBG_FCTNNAME;

    if ( lci_check_ts_valid(ts_id) == MVB_LC_OK )
    {
        p_ir = &(lci_ctrl_blk[ts_id].p_sa->int_regs);
        local_scr.w = readWordFromTS((void*)(&p_ir->scr.w));
        /*
        ******************************
        * Reconfigure Reply Time     *
        ******************************
        */

        if (treply_config <= LC_TREPLY_85US)
        {
            SETSCRTMO(local_scr, treply_config);
            writeWordToTS((void*)(&p_ir->scr.w), local_scr.w);
            
            if (treply_config > LC_TREPLY_43US)
            {
                lc_set_device_status_word(ts_id,LC_DSW_ERD_MSK,LC_DSW_ERD_SET);
            }
        }
        else
        {
            result = MVB_LC_REJECT;   /* Invalid value in treply_config    */
        }

        /*
        ******************************
        * Select Line                *
        ******************************
        */
		/* 15.03.02 Check for the MVBC card type and changed SLM/LS access */

		/* Check, if MVBCS1 is available */
	    if ((p_ir->mcr.w & TM_MCR_VERS_MASK) == TM_MCR_VERS_S1)
    	{
	       lci_ctrl_blk->mvbc_type = MVBCS1;
	    }
        else
        {
           lci_ctrl_blk->mvbc_type = MVBC_01;
        }

        /* Line Switching may be influenced by following factors:            */
        /* RTI or BTI by MVBC which leads to additional line switch or RLD   */
        /* Therefore, multiple attempts to perform a successful line switch  */
        /* and then freeze the decoder input may be necessary.               */

        p_dr      = (TM_TYPE_RWORD*) &(p_ir->dr);
        testm_scr = local_scr;
        SETSCRIL(testm_scr, TM_SCR_IL_TEST);

        if (line_config == LC_CH_BOTH)
        {
        	writeWordToTS((void*)p_dr, 0);     /* Deactivate SLM                    */
        }
        else
        { /* 15.03.02 Check for the MVBC card type and changed SLM/LS access */
		  if (lci_ctrl_blk->mvbc_type == MVBCS1)
		  {
		  	if ( line_config == LC_CH_A )
		  	{
		    	writeWordToTS((void*)p_dr, (TM_DR_LAA | TM_DR_LS | TM_DR_SLM));
		    }
		    else
		    {
		    	writeWordToTS((void*)p_dr, TM_DR_LS | TM_DR_SLM);
		  	}
		  }
          else
		  {
              exp_laa   =  ( line_config == LC_CH_A ) ? TM_DR_LAA : 0;
			  writeWordToTS((void*)(&p_ir->scr.w), testm_scr.w);/* Test Mode = quiet MVB   */
            do
            {
                writeWordToTS((void*)p_dr, TM_DR_LS);    /* Line Switch, then       */
                writeWordToTS((void*)p_dr, TM_DR_SLM);   /* lock again              */
            }
            while ( ((readWordFromTS((void*)p_dr) & TM_DR_LAA) != exp_laa) &&
                    (limit-- > 0) );
			    
            writeWordToTS((void*)(&p_ir->scr.w), local_scr.w); /* Restore original SCR  */
			if (limit==0) /* Problems with line switching      */
			{
				result = MVB_LC_REJECT; /* Unsuccessful switchover           */
			}
          } /* end of else if it is not a mvbc_type(MVBCS1) */

		} /* end of else if it not LC_CH_BOTH */
		result = lc_set_laa_rld();  /* Update changes in DSW             */

    } /* if (lci_check_ts_valid ... */
    else
    {
        result = MVB_LC_REJECT;
    }

    return (result);
} /* lc_hardw_config */


/******************************************************************************
*   name:         lci_port_init
*   function:     This function configures the Device Status Source Port (FC15)
*   input:        Pointer to service area
*   output:       -
******************************************************************************/
static void lci_port_init (
    TM_TYPE_SERVICE_AREA *p_srv_area    /* Pointer to service area */
)
{
    /*
    ******************************
    * Initialize Physical Ports  *
    ******************************
    */

    lpa_memset ((void*)(p_srv_area->pp_pcs), 0, sizeof(p_srv_area->pp_pcs));
    lpa_memset (p_srv_area->pp_data, 0, sizeof(p_srv_area->pp_data));

    /* All physical ports are passive now */

    /*
    ******************************
    * Device Status Port    FC15 *
    ******************************
    */

    writeWordToTS (
        (void*)(&p_srv_area->pp_pcs[TM_PP_FC15].word0.w),
        W_FC15      |           /* F-Code 15; Mandatory                    */
        TM_PCS0_SRC |           /* Active Source                           */
        TM_PCS0_NUM);           /* Data is numeric                         */

    writeWordToTS (
        (void*)(&p_srv_area->pp_pcs[TM_PP_FC15].word1.w),
        TM_PCS1_VP0);           /* Addresses Data Area page 0 only         */

    writeWordToTS (                 /* Device Status Report: Initial Value */
        &TM_1_DATA_WD(p_srv_area->pp_data,TM_PP_FC15,TM_PAGE_0,0), /*Word 0*/
        LC_DSW_TYPE_SPECIAL_CLR   | /* No special device                   */
        LC_DSW_TYPE_BUS_ADMIN_CLR | /* No bus administrator yet attached   */
        LC_DSW_TYPE_BRIDGE_CLR    | /* No bridge/gateway SW yet attached   */
        LC_DSW_TYPE_CLASS_2_3_CLR | /* No Link-Layer Message yet attached  */
        LC_DSW_LAA_SET            | /* Line A Active                       */
        LC_DSW_DNR_SET);            /* Device not (yet) ready              */
    return;
} /* lci_port_init */



/******************************************************************************
*   name:         lci_mvbc_init
*   function:     This function performs hard MVBC Configuration and Initialization
*                 Correct reset of MVBC is tested with loopback test
*   input:        Traffic Store ID
*   output:       MVB_LC_OK
*                 MVB_ERROR_NO_MVBC
*                 MVB_ERROR_MVBC_RESET
******************************************************************************/
static UNSIGNED8 lci_mvbc_init (
    UNSIGNED16 ts_id         /* Traffic Store ID */
)
{
    TM_TYPE_INT_REGS *p_ir = NULL;      /* Internal Registers            */
    TM_TYPE_SCR       local_scr;        /* Local copy of Status Control Reg. */
    short             i;                /* Loop Index                        */

    /*
    ******************************
    * Reset MVBC                 *
    ******************************
    */
	DBG_FCTNNAME;

    local_scr.w =                 /* Reset MVBC                      */
        TM_SCR_IL_RESET |         /* Reset Mode                      */
        lci_arb_modes[ts_id]     |/* Arbitration Strategy            */
        TM_SCR_TMO_43US  |        /* Default Timeout: 42.7 us        */
        lci_waitstates[ts_id];    /* Waitstate Specification         */

    /* Assume MVBC is in unknown MCM, but within 64K */
	CL11DBG("1. MCM unknown, guessing from TM_MCM_64K to TM_MCM_16K:\n" );
    for (i = TM_MCM_64K; i >= TM_MCM_16K; i--)
    {
		CL11DBG("   i = %d:\n", i );
        lci_ctrl_blk[ts_id].p_sa = LCX_PTR_ADD_SEG( lci_ctrl_blk[ts_id].p_tm,
                                                    tm_sa_offs[i] );
        p_ir = &(lci_ctrl_blk[ts_id].p_sa->int_regs);
        writeWordToTS((void*)(&p_ir->scr.w), local_scr.w);
    }

    /*
    ******************************
    * Status Control Register    *
    ******************************
    */
    local_scr.w =                 /* Configure MVBC                          */
        TM_SCR_IL_CONFIG |        /* Configuration Mode                      */
        lci_arb_modes[ts_id]     |/* Arbitration Strategy                    */
        ((LCX_INTEL_MODE == 0) ? 0 : TM_SCR_IM ) | 
                                  /* Intel or Motorola Mode                  */
        TM_SCR_RCEV      |        /* Event Polling always permitted          */
        TM_SCR_TMO_43US  |        /* Default Timeout: 42.7 us                */
        lci_waitstates[ts_id];    /* Waitstate Specification                 */

	CL11DBG("2. writing SCR:\n" );
    writeWordToTS((void*)(&p_ir->scr.w), local_scr.w);

    /*
    ******************************
    * Test if MVBC+SCR are OK    *
    ******************************
    */

	/* Write access to a MVBC-characteristic   */
	CL11DBG("3. writing to a MVBC-characteristic for test :\n" );
    writeWordToTS((void*)(&p_ir->dpr), LCI_ALL_1);
	Wait2MS();
	/* register to check that MVBC and not a   */
	/* memory is accessed. Use 14-bit reg. DPR */

#ifndef MVBCEMU
	CL11DBG("4. 2 reads to test if a MVBC is there :\n" );
    if ((readWordFromTS((void*)(&p_ir->scr.w)) != local_scr.w) ||
        (readWordFromTS((void*)(&p_ir->dpr)) != TM_DPR_MASK))
    {

        return (MVB_ERROR_NO_MVBC);      /* MVBC not found */
    }
#if 0
	if (G_stop){
		printf(" >>>>>>>>>>>  intended return:<<<<<<<<<<<<\n" );
		return (MVB_ERROR_NO_MVBC);      /* MVBC not found */
	}
#endif
    /*
    ******************************
    * MVBC Interrupt Logic       *
    ******************************
    */

    /* The interrupt controller of the MVBC is already initialized by        */
    /* setting SCR to zero.  See code above                                  */

    /*****************************
    * MVBC loopback test         *
    * See, if MVBC is resetted   *
    * properly                   *
    ******************************
    */
   /* Configure test ports */
    {
        TM_TYPE_SERVICE_AREA *p_sa;

        p_sa = (TM_TYPE_SERVICE_AREA*)
               (LCX_PTR_ADD_SEG(lci_ctrl_blk[ts_id].p_tm,tm_sa_offs[0]));

        writeWordToTS((void*)(&p_sa->pp_pcs[TM_PP_TSRC].word0.w),
                      (TM_TYPE_WORD)W_FC1|TM_PCS0_SRC|TM_PCS0_NUM);
        writeWordToTS((void*)(&p_sa->pp_pcs[TM_PP_TSNK].word0.w),
                      (TM_TYPE_WORD)W_FC1|TM_PCS0_SINK|TM_PCS0_NUM);
        writeWordToTS((void*)(&p_sa->pp_pcs[TM_PP_TSRC].word1.w),
                      (TM_TYPE_WORD)TM_PCS1_VP0);
        writeWordToTS((void*)(&p_sa->pp_pcs[TM_PP_TSNK].word1.w),
                      (TM_TYPE_WORD)TM_PCS1_VP0);

        writeWordToTS(&TM_1_DATA_WD(p_sa->pp_data,TM_PP_TSRC,TM_PAGE_0,0),
                      (TM_TYPE_WORD)0xa55a);
        writeWordToTS(&TM_1_DATA_WD(p_sa->pp_data,TM_PP_TSNK,TM_PAGE_1,0),
                      (TM_TYPE_WORD)0x0000);

        local_scr.w =               /* Configure MVBC                    */
            TM_SCR_IL_TEST  |       /* Configuration Mode                */
            lci_arb_modes[ts_id] |  /* Arbitration Strategy              */
            ((LCX_INTEL_MODE == 0) ? 0 : TM_SCR_IM ) |
                                    /* Intel or Motorola Mode            */
            TM_SCR_RCEV     |       /* Event Polling always permitted    */
            TM_SCR_TMO_43US |       /* Default Timeout: 42.7 us          */
            TM_SCR_MAS      |       /* Self test sends MF */
            TM_SCR_UTQ      |
            TM_SCR_UTS      |
            lci_waitstates[ts_id];  /* Waitstate Specification           */

        /* loopback test mode on */
        writeWordToTS((void*)(&p_ir->scr.w), local_scr.w);
        Wait2MS();

        /* send test master frame manually */
        writeWordToTS(&p_sa->mfs, (TM_TYPE_WORD)0x1001);
        writeWordToTS((void*)(&p_ir->mr.w), (TM_TYPE_WORD)TM_MR_SMFM);

        /* wait for response or timeout */
        do {
        	Wait100US();
        } while (readWordFromTS((void*)(&p_ir->mr.w)) & TM_MR_BUSY);

        if (readWordFromTS(&TM_1_DATA_WD(p_sa->pp_data,TM_PP_TSNK,TM_PAGE_1,0))
            != 0xa55a)
        {
            return (MVB_ERROR_MVBC_RESET);
        }            
        /* additional wait loop til BTI occured */
        Wait2MS();
    }
#endif

    local_scr.w =               /* Configure MVBC                          */
        TM_SCR_IL_CONFIG |      /* Configuration Mode                      */
        lci_arb_modes[ts_id] |  /* Arbitration Strategy                    */
        ((LCX_INTEL_MODE == 0) ? 0 : TM_SCR_IM ) |
                                /* Intel or Motorola Mode                  */
        TM_SCR_RCEV      |      /* Event Polling always permitted          */
        TM_SCR_TMO_43US  |      /* Default Timeout: 42.7 us                */
        lci_waitstates[ts_id];  /* Waitstate Specification                 */

    /* loopback teset mode off */
    writeWordToTS((void*)(&p_ir->scr.w), local_scr.w);

    return (MVB_LC_OK);
} /* lci_mvbc_init */



/******************************************************************************
*   name:         lc_config
*   function:     This function initializes the LC Basic Structure
*   input:        Traffic Store ID
*                 Pointer to start of traffic store
*                 Memory Configuration Mode of MVBC
*                 Queue Offset Value
*                 Master-Frame Offset Value
*   output:       MVB_LC_OK
*                 MVB_LC_REJECT
******************************************************************************/
static UNSIGNED8 lc_config (
    UNSIGNED16 ts_id,           /* Traffic Store ID                  */
    void      *p_tm_start_addr, /* Pointer to start of traffic store */
    UNSIGNED16 mcm,             /* Memory Configuration Mode of MVBC */
    UNSIGNED16 qo,              /* Queue Offset Value                */
    UNSIGNED16 mo               /* Master-Frame Offset Value         */
)
{
    UNSIGNED8 result = MVB_LC_REJECT;   /* Return Value, default: Reject     */

    TM_TYPE_WORD   *p_la_pit;           /* LA Port Index Table               */
    TM_TYPE_WORD   *p_da_pit;           /* DA Port Index Table               */
    TM_TYPE_WORD   *p_pcs;              /* Port Control + Status Register    */
    TM_TYPE_SERVICE_AREA  *p_srv_area;  /* Local pointer to Service Area  */
    TM_TYPE_MCR            local_mcr;   /* Local copy of Memory Config. Reg. */

    /* Offset values, specific to Traffic Memory structure */

    static const UNSIGNED32  la_pcs_addr[] = TM_LA_PCS_OFFSETS;
    static const UNSIGNED32  da_pcs_addr[] = TM_DA_PCS_OFFSETS;
    static const UNSIGNED32  la_pit_addr[] = TM_LA_PIT_OFFSETS;
    static const UNSIGNED32  da_pit_addr[] = TM_DA_PIT_OFFSETS;
    static const UNSIGNED16  pit_size   [] = TM_PIT_BYTE_SIZES;
    
    int mvbcinit;  /* counter for several resets */

    DBG_FCTNNAME;

    if (ts_id < LCX_TM_COUNT)
    {

        /* Initialize the Control Block */

        lci_ctrl_blk[ts_id].p_tm      = p_tm_start_addr;
        lci_ctrl_blk[ts_id].checklist = LCI_CHK_LC;

        /*******************************
		 * Configure MVBC              *
		 *******************************/

        /* Try to init MVBC for some times if not fully reset */   
        mvbcinit = 8;
		CL11DBG("%s: 1.Try init MVBC for %d times\n", __FUNCTION__,mvbcinit );
        do
        {
            result = lci_mvbc_init(ts_id); /* Hard Initialization */
        } while (result != MVB_LC_OK && --mvbcinit);

        /* Check if MCM is valid according to maximum available TM size */
        if ( ((UNSIGNED16) mcm) > TM_MCM_64K)
        {
            result = MVB_LC_REJECT;
        }

        if (result == MVB_LC_OK)
        {
			CL11DBG("%s 2. ok. \n", __FUNCTION__ );
            local_mcr.w = 0;
            SETMCRMO(local_mcr, mo);
            SETMCRQO(local_mcr, qo);
            SETMCRMCM(local_mcr, mcm);
			
            writeWordToTS((void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.mcr.w),
                          local_mcr.w);

            /*
            ******************************
            * Compute Serv. Area Address *
            ******************************
            */

            p_srv_area = LCX_PTR_ADD_SEG( p_tm_start_addr, tm_sa_offs[mcm] );
            lci_ctrl_blk[ts_id].p_sa = p_srv_area;

            /*
            ******************************
            * Clear Port Index Tables    *
            ******************************
            */

            p_la_pit = LCX_PTR_ADD_SEG(p_tm_start_addr,la_pit_addr[mcm]);
            p_da_pit = LCX_PTR_ADD_SEG(p_tm_start_addr,da_pit_addr[mcm]);

            lpa_memset (p_la_pit, 0, pit_size[mcm]);
                /* Harmless action if MCM=0: p_la_pit = p_da_pit */
            lpa_memset (p_da_pit, 0, pit_size[mcm]);

            /*
            ******************************
            * Make default ports inactive*
            ******************************
            */

            /* Port Index 0 in the logical address space */

            p_pcs = LCX_PTR_ADD_SEG(p_tm_start_addr, la_pcs_addr[mcm]);
            writeWordToTS(p_pcs, 0); /* delete LA PCS(Port 0), Word 0 */
            p_pcs++;
            writeWordToTS(p_pcs, 0); /* delete LA PCS(Port 0), Word 1 */

            /* Port Index 0 in the device  address space  */
            /* Harmless action if MCM=0: p_pcs = p_la_pit */

            p_pcs = LCX_PTR_ADD_SEG(p_tm_start_addr, da_pcs_addr[mcm]);
            writeWordToTS(p_pcs, 0); /* delete DA PCS(Port 0), Word 0 */
            p_pcs++;
            writeWordToTS(p_pcs, 0); /* delete DA PCS(Port 0), Word 1 */

            /*
            ******************************
            * Initialize Physical Ports  *
            ******************************
            */

            lci_port_init( p_srv_area );

        } /* if (result... */
    }     /* if (ts_id...   */
	
	DBG_EXIT;
    return (result);
} /* lc_config */


/******************************************************************************
*   name:         lc_init
*   function:     This function initializes the LC Basic Structure
*              Clears entire control block for traffic store. MVBCs and Traffic
*                 Stores are not yet accessible
*   input:        Traffic Store ID
*   output:       -
******************************************************************************/
static void lc_init (
    UNSIGNED16 ts_id     /* Traffic Store ID */
)
{
    UNSIGNED16  count;
    UNSIGNED8  *p_trgt;

    p_trgt = (UNSIGNED8*) &lci_ctrl_blk[ts_id];
    count  = sizeof(LCI_TYPE_CTRL_BLK);

    while (count > 0)
    {
		--count;
        *p_trgt++ = 0;
    }
    return;
} /* lc_init */



/******************************************************************************
*   name:         lc_tmo_config
*   function:     The function sets Sinktime Supervision on MVBC
*   input:        Traffic Store ID
*                 value for Sinktime Supervision Register
*                 number of docks to be supervised
*   output:       MVB_LC_OK
*                 return values of lci_check_ts_valid()
******************************************************************************/
UNSIGNED8 lc_tmo_config (
    UNSIGNED16   ts_id,     /* Traffic Store ID                        */
    TM_TYPE_WORD stsr,      /* value for Sinktime Supervision Register */
    TM_TYPE_WORD docks      /* number of docks to be supervised        */
)
{
    TM_TYPE_STSR local_stsr;
    UNSIGNED8    result = lci_check_ts_valid(ts_id);

    if (result == MVB_LC_OK)
    {
        local_stsr.w = \
			readWordFromTS((void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.stsr));
        SETSTSRDOCKS(local_stsr, docks);
        SETSTSRSI(local_stsr, stsr);
        writeWordToTS((void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.stsr), 
					  local_stsr.w);
    } /* Ende if MVB_LC_OK */

    return(result);
} /* Ende lc_tmo_config */



/******************************************************************************
*   name:         lpc_init
*   function:     The function clears the control block for the TS ID.
*                 All information is lost.
*   input:        Traffic Store ID
*   output:       -
******************************************************************************/
static void lpc_init (
    UNSIGNED16 ts_id     /* Traffic Store ID */
)
{
    UNSIGNED16  count;
    UNSIGNED8  *p_trgt;

    p_trgt = (UNSIGNED8*) &lp_cb[ts_id];
    count  = sizeof(TYPE_LP_CB);

    while (count > 0) {
		--count;
        *p_trgt++ = 0;
    }
    return;
} /* lpc_init */


/******************************************************************************
*   name:         lc_go
*   function:     The function starts MVBC operation
*   input:        Traffic Store ID
*   output:       MVB_LC_OK
*                 return values of lci_check_ts_valid()
******************************************************************************/
static UNSIGNED8 lc_go (
    UNSIGNED16 ts_id         /* Traffic Store ID */
)
{
    TM_TYPE_SCR local_scr;              /* Local copy of SCR                 */
    UNSIGNED8   result  = lci_check_ts_valid(ts_id);
                                        /* Return Value, check for valid ts  */

    if (result == MVB_LC_OK)            /* MVB Communication Kickoff !       */
    {
        lc_set_laa_rld( );              /* Valid LAA and RLD                 */

        local_scr.w =\
			readWordFromTS((void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.scr.w));

        SETSCRIL(local_scr, TM_SCR_IL_RUNNING);

        writeWordToTS((void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.scr.w), 
					  local_scr.w);
    }
    return (result);
} /* lc_go */


/******************************************************************************
*   name:         lc_set_device_address
*   function:     The function applies new Device Address to MVBC
*   input:        MVBC device address
*                 Traffic Store ID
*   output:       MVB_LC_OK
*                 MVB_LC_REJECT
******************************************************************************/
static UNSIGNED8 lc_set_device_address (
    UNSIGNED16 ts_id,               /* Traffic Store ID    */
    UNSIGNED16 dev_address          /* MVBC device address */
)
{
    TM_TYPE_INT_REGS *p_ir;             /* MVBC Internal Registers        */
    UNSIGNED8  result = MVB_LC_REJECT; 	/* Return Value, default: Reject  */
	UNSIGNED16 val=0;

	DBG_FCTNNAME;
	
	CL11DBG(" %s: dev_address = 0x%04x\n", __FUNCTION__, dev_address );
    if ( lci_check_ts_valid(ts_id) == MVB_LC_OK )
    {
        if ((dev_address & LCI_DA_MASK) == dev_address)
        {
            p_ir = &lci_ctrl_blk[ts_id].p_sa->int_regs;
			CL11DBG(" 1. -- wr dev_address 0x%04x\n", dev_address );
            writeWordToTS((void*)(&p_ir->daor), dev_address);

            writeWordToTS((void*)(&p_ir->daok), TM_DAOK_ENABLE);
			CL11DBG(" 2. -- rd back dev_address: " );
			val =  readWordFromTS( (void*)(&p_ir->daor) );
            if ( val == dev_address)
            {   /* Check if successful */
                result = MVB_LC_OK;
				CL11DBG("OK  val = 0x%04x\n", val);
            } else
				CL11DBG("failed : val = 0x%04x\n", val);
        }
    }
	DBG_EXIT;
    return (result);
} /* lc_set_device_address */




/******************************************************************************
*******************************************************************************
*******************************************************************************

                                 external functions  

*******************************************************************************
*******************************************************************************
******************************************************************************/

/******************************************************************************
*   name:         MVBCInit
*   function:     The function initializes MVBC and traffic store
*   input:        Pointer to port configuration
*                 Traffic Store ID
*   output:       MVB_NO_ERROR
*                 MVB_ERROR_PARA
*                 MVB_ERROR_NO_MVBC
******************************************************************************/
UNSIGNED8 MVBCInit (TYPE_LP_TS_CFG *Configuration, UNSIGNED16 ts_id)
{
    UNSIGNED8 result;
	
	DBG_FCTNNAME;
    if (ts_id >= LCX_TM_COUNT){
        return (MVB_ERROR_PARA);
    }
	
    lpc_init(ts_id); /* makes "memset 0" */
    lc_init(ts_id);  /* makes "memset 0" */

    result = lc_config(ts_id, 
					   P_OF_ULONG(void*, Configuration->tm_start),
					   Configuration->mcm,
                       Configuration->msq_offset,
                       Configuration->mft_offset);

    if (result != MVB_LC_OK){
		CL11DBG(" result != OK\n");
        if ((result != MVB_ERROR_NO_MVBC) && (result != MVB_ERROR_MVBC_RESET)){
            result = MVB_ERROR_PARA;
        }

        return (result);
    }

    lc_hardw_config(ts_id,
                    Configuration->line_cfg,
                    Configuration->reply_to);

    if (lc_set_device_address(ts_id, Configuration->dev_addr) != MVB_LC_OK){
        return (MVB_ERROR_NO_MVBC);
    }

    result = lp_create (ts_id, LP_HW_TYPE_MVBC, Configuration);
	/* result = lp_create (ts_id, Configuration->ts_type, Configuration); */
	/* RG280198: Hardwaretype fixed to MVBC-Type. No other one 
	   will work on PC/104 */
    if (result != MVB_LPS_OK){
        return (MVB_ERROR_PARA);
    }

    result = lc_tmo_config (ts_id, TM_STSR_INTERV, TM_STSR_DOCKS);
    if (result != MVB_LC_OK) {
        return  (MVB_ERROR_PARA);
    }
	
    return(MVB_NO_ERROR);
} /* MVBCInit */



/******************************************************************************
*   name:         MVBCStart
*   function:     The function starts MVBC operation
*   input:        Traffic Store ID
*   output:       MVB_NO_ERROR
*                 MVB_UNKNOWN_TRAFFICSTORE
*                 MVB_ERROR_MVBC_INIT
******************************************************************************/
UNSIGNED8 MVBCStart(UNSIGNED16 ts_id)
{
    TM_TYPE_SCR local_scr;       /* Local copy of SCR                 */

    UNSIGNED8   result = lci_check_ts_valid(ts_id);
                                 /* Return Value, check for valid ts  */
	DBG_FCTNNAME;

    if (result == MVB_LC_OK)    /* MVB Communication Kickoff !       */
    {
        local_scr.w = readWordFromTS(
                       (void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.scr.w));
        /* MVBC is initialized ? */
        if (GETSCRIL(local_scr) != TM_SCR_IL_RESET)
        {
            lc_set_device_status_word(ts_id, LC_DSW_DNR_MSK, LC_DSW_DNR_CLR);
            lc_go(ts_id);
            result = MVB_NO_ERROR;
#if defined(MVB_M)
			if (gMVBMState == MVBM_CONFIG_UNDEF)
			{
				result = MVBM_CONFIG_UNDEF;
			}
#endif
        }
        else
        {
            result = MVB_ERROR_MVBC_INIT;
        }
    }
    else
    {
        result = MVB_UNKNOWN_TRAFFICSTORE;
    }

    return(result);
} /* MVBCStart */


/******************************************************************************
*   name:         MVBCStop
*   function:     The function stops MVBC operation
*   input:        Traffic Store ID
*   output:       MVB_NO_ERROR
*                 MVB_UNKNOWN_TRAFFICSTORE
******************************************************************************/
UNSIGNED8 MVBCStop (UNSIGNED16 ts_id)
{
    TM_TYPE_SCR  local_scr;      /* Local copy of SCR                 */
    UNSIGNED8    result = lci_check_ts_valid(ts_id);
                                 /* Return Value, check for valid ts  */
	DBG_FCTNNAME;

    if (result == MVB_LC_OK)        /* MVB Communication Kickoff !       */
    {
		local_scr.w 	= \

		readWordFromTS( (void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.scr.w));

        local_scr.w 	&= ~TM_SCR_IL_TEST;

        writeWordToTS(  (void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.scr.w), 
						local_scr.w);
        result = MVB_NO_ERROR;
    }
    else
    {
        result = MVB_UNKNOWN_TRAFFICSTORE;
    }
    return (result);
} /* MVBCStop */



/******************************************************************************
*   name:         MVBCGetPort
*   function:     The function reads data from MVBC port into the port buffer
*   input:        Port number
*                 ptr to port-buffer
*                 Limit for age of data
*                 ptr to Real age of data
*                 Traffic Store ID
*   output:       MVB-error-codes
******************************************************************************/
UNSIGNED8 MVBCGetPort(UNSIGNED16  Port,        /* Port number            */
                      UNSIGNED16 *PortBuffer,  /* Pointer to port buffer */
                      UNSIGNED16  Tack,        /* Limit for age of data  */
                      UNSIGNED16 *Age,         /* Real age of data       */
                      UNSIGNED16  ts_id)       /* Traffic Store ID       */
{
    void       *pb_pit;     /* Address  Port Index Table (PIT) 		*/
    UNSIGNED16  prt_indx;   /* Contents PIT byte 					*/
    TYPE_LP_CB *p_cb = &lp_cb[ts_id];
    TM_TYPE_SCR local_scr;  /* Local copy of SCR                 	*/
    TM_TYPE_MCR local_mcr;  /* Local copy of Memory Config. Reg. 	*/
    UNSIGNED8   result = MVB_NO_ERROR;  /* Return value 			*/
	
	CL11DBG("%s: Port = %d \n ", __FUNCTION__, Port );
	
#if defined(MVB_M)
    if (!gbAPIInternal && (gMVBMState == MVBM_CONFIG_UNDEF)) {
		return(MVBM_CONFIG_UNDEF);
	}
#endif

    if ( lci_check_ts_valid(ts_id) != MVB_LC_OK ){
        return (MVB_UNKNOWN_TRAFFICSTORE);
    }

#ifndef MVBCEMU
    local_scr.w = \
		readWordFromTS((void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.scr.w));
    /* MVBC is running? */
    if (GETSCRIL(local_scr) != TM_SCR_IL_RUNNING){
        return MVB_ERROR_MVBC_STOP;
    }
#endif

    if (!(Port <= LPC_GET_PRT_ADDR_MAX(p_cb)) != MVB_LPS_OK){
        return (MVB_ERROR_PORT_UNDEF);
    }

    pb_pit = LPC_GET_PB_PIT(p_cb);
    local_mcr.w = \
		readWordFromTS((void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.mcr.w));
    prt_indx = lpl_get_prt_indx((UNSIGNED16)(GETMCRMCM(local_mcr)), 
								pb_pit, 
								Port);

    if (!(prt_indx <= LPC_GET_PRT_INDX_MAX (p_cb)) != MVB_LPS_OK) {
        return (MVB_ERROR_PORT_UNDEF);
    }

    /* Check existence of port */
    if (prt_indx != 0) {
        UNSIGNED8 *p_pcs;   /* Address Port Ctrl and Status Register (PCS) */
        UNSIGNED8  f_code;      /* Function code */
        UNSIGNED8  frame_size;  /* Port size in word */
        UNSIGNED16 *addr_prt;
        UNSIGNED16 *p_sha;

        p_pcs = (((UNSIGNED8*) LPC_GET_PB_PCS (p_cb)) +
                           LPL_GEN_PCS_OFFSET (p_cb, prt_indx));

        /* Get data with warning if port is not sink (value 01) */
        if (LPL_GET_PRT_TYPE(p_pcs) != TM_PCS_TYPE_SNK)
        {
            result = MVB_WARNING_NO_SINK;
        }

        /* Function code  to calculate the port size in words */
        f_code = lpl_get_prt_fcode(p_pcs);

        frame_size = (UNSIGNED8)1 << f_code; /* Port size in words */

        MVB_INT_DISABLE();
        p_sha = (UNSIGNED16*)PortBuffer;

        *Age = LPL_GET_PRT_TACK(p_pcs);
        if (*Age < Tack) {
            /* Keep in mind: TACK check result */
            *(p_sha++) = MVB_WARNING_OLD_DATA;
            if (result == MVB_NO_ERROR) {   
				/* no overwrite of error */
                result = MVB_WARNING_OLD_DATA;
            }
        } else {
            /* Keep in mind: TACK check result */
            *(p_sha++) = MVB_NO_ERROR;
        }

        /* Address  port (data area) */
        addr_prt = (UNSIGNED16*)((UNSIGNED8*)LPC_GET_PB_PRT(p_cb) +
                                 LPL_GEN_PRT_OFFSET(prt_indx) +
                                 LPL_GET_PGE_OFFSET_RD(p_pcs));
        /* Copy all data */
        while (frame_size > 0)
        {
            *(p_sha++) = readWordFromTS(addr_prt++);
            frame_size--;
        }
        MVB_INT_ENABLE();
    }
    else
    {  /* Error: port not defined */
        result = MVB_ERROR_PORT_UNDEF;
    }
    return (result);
} /* MVBCGetPort */



/******************************************************************************
*   name:         MVBCPutPort
*   function:     The function writes data from the port buffer into MVBC port
*   input:        Port number
*                 ptr to port-buffer
*                 Traffic Store ID
*   output:       MVB-error-codes
******************************************************************************/
UNSIGNED8 MVBCPutPort(UNSIGNED16  Port,        /* Port number */
                      UNSIGNED16 *PortBuffer,  /* Pointer to port buffer  */
                      UNSIGNED16  ts_id)       /* Traffic Store ID */
{
    void       *pb_pit;     /* Address  Port Index Table (PIT) */
    UNSIGNED16  prt_indx;   /* Contents PIT byte */
    TYPE_LP_CB *p_cb 		= &lp_cb[ts_id];
    TM_TYPE_MCR local_mcr;  /* Local copy of Memory Config. Reg. */
    UNSIGNED8   result 		= MVB_NO_ERROR;  /* Return value */

	CL11DBG("%s   Port = %d PortBuffer = %p\n ",
			__FUNCTION__, Port,	PortBuffer);

    if ( lci_check_ts_valid(ts_id) != MVB_LC_OK ){
        return (MVB_UNKNOWN_TRAFFICSTORE);
    }

    if (!(Port <= LPC_GET_PRT_ADDR_MAX(p_cb)) != MVB_LPS_OK){
        return (MVB_ERROR_PORT_UNDEF);
    }

	CL11DBG("     1. read MCR to gather MCM mode:\n ");
    pb_pit = LPC_GET_PB_PIT(p_cb);

    local_mcr.w = \
		readWordFromTS((void*)(&lci_ctrl_blk[ts_id].p_sa->int_regs.mcr.w));

    prt_indx = lpl_get_prt_indx((UNSIGNED16)(GETMCRMCM(local_mcr)), 
								pb_pit, Port);

	CL11DBG("     2. got port index %d:\n ", prt_indx );
    if (!(prt_indx <= LPC_GET_PRT_INDX_MAX (p_cb)) != MVB_LPS_OK){
        return (MVB_ERROR_PORT_UNDEF);
    }

    /* Check existence of port */
    if (prt_indx != 0) {
        UNSIGNED8 *p_pcs;   /* Address Port Ctrl and Status Register (PCS) */
        UNSIGNED8  f_code;      /* Function code */
        UNSIGNED8  frame_size;  /* Port size in word */
        UNSIGNED16 *addr_prt;
        UNSIGNED16 *p_sha;

        p_pcs = (((UNSIGNED8*) LPC_GET_PB_PCS (p_cb)) +
                               LPL_GEN_PCS_OFFSET (p_cb, prt_indx));

        /* Get data only if port is sink (value 01) */
        if (LPL_GET_PRT_TYPE(p_pcs) != TM_PCS_TYPE_SRC) {
            return (MVB_ERROR_NO_SRC);
        }
		CL11DBG("     3. checked that port is sink: ok.\n " );
        /* Function code  to calculate the port size in words */
        f_code = lpl_get_prt_fcode(p_pcs);
		
        frame_size = (UNSIGNED8)1 << f_code; /* Port size in words */
		
		CL11DBG("     4. frame size = %d \n ", frame_size );
        MVB_INT_DISABLE();
        /* Address  port (data area) */
        addr_prt = (UNSIGNED16*)((UNSIGNED8*)LPC_GET_PB_PRT(p_cb) +
                                 LPL_GEN_PRT_OFFSET(prt_indx) +
                                 LPL_GET_PGE_OFFSET_WT(p_pcs));
        p_sha = (UNSIGNED16*)PortBuffer;
		
        /* Copy all data */
		CL11DBG("     2. Copy all data:\n ");
			
        p_sha++; /* skip TACK check result */
        while (frame_size > 0) {
            writeWordToTS(addr_prt++, *(p_sha++));
            frame_size--;
        }
        /* Make port valid: toggle page */
        lpl_tgl_prt_page(p_pcs);
        MVB_INT_ENABLE();

#if defined(MVB_M)
	    if (gMVBMState == MVBM_CONFIG_UNDEF){
			return(MVBM_CONFIG_UNDEF);
		}
#endif
    }
    else { 
		/* Error: port not defined */
        result = MVB_ERROR_PORT_UNDEF;
    }
    return (result);
} /* MVBCPutPort */


/******************************************************************************
*   name:         MVBCSetDSW
*   function:     The function sets the bits in the Device Status Word which
*                 shall be set by the user.
*                 i.e: Device Not Ready
*                      Some Device Disturbed
*                      Some System Disturbed
*   input:        Traffic Store ID
*                 mask to seperate the desired bits
*                 value of the bits
*   output:       MVB_NO_ERROR
*                 MVB_UNKNOWN_TRAFFICSTORE 
******************************************************************************/
UNSIGNED8 MVBCSetDSW(UNSIGNED16 ts_id,   /* Traffic Store ID               */
                     UNSIGNED16 mask,    /* Device Status Word Mask Bits   */
                     UNSIGNED16 value)   /* New values for affected fields */
{
    UNSIGNED8   result = MVB_NO_ERROR;  /* Return value */
    
    UNSIGNED16  lMask = mask & (LC_DSW_DNR_MSK |
                                LC_DSW_SDD_MSK |
                                LC_DSW_SSD_MSK);  
 /* Only these Bits shall be modified */

    if ( lc_set_device_status_word(ts_id,
                                   lMask,
                                   value) != MVB_LC_OK )
    {
        result = MVB_UNKNOWN_TRAFFICSTORE;
    }

    return (result);
} /* MVBCSetDSW */


/******************************************************************************
*   name:         MVBCIdle  (in MVBM: MVBCIdle11)
*   function:     The function retrieves LAA and RLD from MVBC Decoder Register
*                 and updates it in the Device Status Word.
*   input:        Traffic Store ID
*   output:       MVB_NO_ERROR
*                 MVB_UNKNOWN_TRAFFICSTORE 
******************************************************************************/
#if defined (MVB_M)
UNSIGNED8 MVBCIdle11(UNSIGNED16  ts_id)       /* Traffic Store ID */
#else
UNSIGNED8 MVBCIdle(UNSIGNED16  ts_id)       /* Traffic Store ID */
#endif
{
    UNSIGNED8   result = MVB_UNKNOWN_TRAFFICSTORE;  /* Return value */
    if ( lci_check_ts_valid(ts_id) == MVB_LC_OK )
    {
	    lc_set_laa_rld();
        result = MVB_NO_ERROR;
    }

    return (result);
} /* MVBCIdle */


/******************************************************************************
*   name:         MVBCGetDeviceAddress
*   function:     reads the current deviveaddress.
*   input:        Traffic Store ID
*                 ptr where the address shall be written to
*   output:       MVB_NO_ERROR
*                 MVB_UNKNOWN_TRAFFICSTORE 
******************************************************************************/
UNSIGNED8 MVBCGetDeviceAddress(
    UNSIGNED16  *devAddress,
    UNSIGNED16  ts_id)        /* Traffic Store ID       */
{
    UNSIGNED8   result = MVB_UNKNOWN_TRAFFICSTORE;  /* Return value */

    if ( lci_check_ts_valid(ts_id) == MVB_LC_OK )
    {
        *devAddress = lci_ctrl_blk[ts_id].p_sa->int_regs.daor;
		result = MVB_NO_ERROR;
    }

    return (result);
} /* MVBCGetDeviceAddress */


/****************************************************************************
**  name         : MVBCRetriggerWd
**===========================================================================
**  parameters   : in     ts_id    Traffic Store  Identification
                          trig_val  value to be written into the watchdog register

**  return value : status MVB_NO_ERROR or MVB_UNKNOWN_TRAFFICSTORE

**  used globals : lci_ctrl_blk (read)

**  description  : retriggers MVBCS1 watchdog 

**  caveats      : -
*****************************************************************************/
UNSIGNED8 MVBCRetriggerWd(UNSIGNED16 ts_id, UNSIGNED16 trig_val)
{

  	if(lci_ctrl_blk->mvbc_type == MVBCS1)
	{
		if (MVB_LC_OK != lci_check_ts_valid(ts_id)) 
		{
			return MVB_UNKNOWN_TRAFFICSTORE;
		}
		lci_ctrl_blk[ts_id].p_sa->int_regs.wdc = trig_val;

        /* 26.03.02; If MVBCS1 watchdog is activated and the watchdog 
		   counter register reaches zero,the initialization
           level changes to configuration mode.Check the SCR-Initialization 
		   level and return the error if it is config mode */

		if ((lci_ctrl_blk[ts_id].p_sa->int_regs.scr.w & TM_SCR_IL_MASK) == TM_SCR_IL_RUNNING) 
		{
			return MVB_NO_ERROR;
		}
		else
		{ 
			if ((lci_ctrl_blk[ts_id].p_sa->int_regs.scr.w & TM_SCR_IL_MASK) == TM_SCR_IL_CONFIG)
			{
				return MVB_ERROR_MVBC_INIT;
			}
		}  
		return MVB_NO_ERROR;
	}
	else
	{
		return MVB_WATCHDOG_NOT_AVAIL;
	}
} /* End of	MVBCRetriggerWd() */
