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
** WORKFILE::  dpr_dep.h  
**---------------------------------------------------------------------
** TASK::
   Header for Application Programmers Interface (API) for PC104 access
   in CLASS1.1. Dependecies of DPR / Traffic Store

**---------------------------------------------------------------------
** AUTHOR::    REINWALD_LU  
** CREATED::   30.04.1997
**---------------------------------------------------------------------
** CONTENTS::
      functions:
         none
**---------------------------------------------------------------------
** NOTES::     -

**=====================================================================
** HISTORY::   AUTHOR::   Baddam Tirumal Reddy
               REVISION:: 2.04.00  
**---------------------------------------------------------------------
**

**********************************************************************/

#ifndef DPR_DEP_H_
#define DPR_DEP_H_

/*---------- Locations and sizes of the different TM areas w.r.t MCM */
#define TM_PORT_COUNT       4096  /* Applies to all ports */
#define LC_TS_MAX_PORT_ADDR (TM_PORT_COUNT-1)
#define TM_OFFSET_COUNT     5

/*      Parameters for MCM = 0 (16K)  1 (32K)  2 (64K)  3 (256K)  4 (1M)  */

#define TM_LA_PCS_OFFSETS  { 0x3000L, 0x3000L, 0xC000L, 0x30000L, 0x30000L }
#define TM_DA_PCS_OFFSETS  { 0x0L,    0x7000L, 0xF000L, 0x4000L , 0x38000L }

#define TM_LA_DATA_OFFSETS { 0x1000L, 0x1000L, 0x4000L, 0x10000L, 0x10000L }
#define TM_DA_DATA_OFFSETS { 0x0L,    0x5000L, 0xE000L, 0x38000L, 0x40000L }
#define TM_LA_FRCE_OFFSETS { 0x2000L, 0x2000L, 0x8000L, 0x20000L, 0x20000L }

#define TM_LA_PIT_OFFSETS  { 0x0000L, 0x0000L, 0x0000L, 0x0000L , 0x0000L  }
#define TM_DA_PIT_OFFSETS  { 0x0L,    0x4000L, 0x2000L, 0x2000L , 0x2000L  }

#define TM_SERVICE_OFFSETS { 0x3C00L, 0x7C00L, 0xFC00L, 0xFC00L , 0xFC00L  }

#define TM_PIT_BYTE_SIZES  { 4096,    4096,    8192,    8192,     8192     }
#define TM_LA_PORT_COUNTS  { 256,     256,     1024,    4096,     4096     }
#define TM_DA_PORT_COUNTS  { 0,       256,     256,     2048,     4096     }

#define TM_MEMORY_SIZES    {16*1024l,32*1024l,64*1024l,256*1024l,1024*1024l}
/* Usage example:     UNSIGNED32 la_pcs_offsets[] = TM_LA_PCS_OFFSETS; */

/*---------- Decoder Register DR ------------------------------------*/
/* Bit   15-4    3    2    1    0  */
/*      dummy  laa  rld   ls  slm  */
typedef union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_DR;

#define TM_DR_LAA    0x0008
#define TM_DR_RLD    0x0004
#define TM_DR_LS     0x0002
#define TM_DR_SLM    0x0001

/*---------- Memory Configuration Register MCR ----------------------*/
/* Bit   15-7  6-5  4-3  2-0 */
/*      dummy   mo   qo  mcm */
typedef union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_MCR;

#define TM_MCM_16K        0
#define TM_MCM_32K        1
#define TM_MCM_64K        2
#define TM_MCM_256K       3
#define TM_MCM_1M         4

#define TM_MCR_MCM_MASK   0x0007
#define TM_MCR_MO_MASK    0x0060
#define TM_MCR_MO_OFF     5
#define TM_MCR_QO_MASK    0x0018
#define TM_MCR_QO_OFF     3

#define TM_MCR_VERS_MASK  0xF800
#define TM_MCR_VERS_S1    0x8000

#define SETMCRMCM(mcr, mcm)  \
    {(mcr.w)=(((TM_TYPE_WORD)(mcr.w) & ~(TM_TYPE_WORD)TM_MCR_MCM_MASK)  \
                                     | (TM_TYPE_WORD)(mcm));}
#define GETMCRMCM(mcr)  \
    ((TM_TYPE_WORD)(mcr.w) & (TM_TYPE_WORD)TM_MCR_MCM_MASK)

#define SETMCRMO(mcr, mo)  \
    {(mcr.w)=(((TM_TYPE_WORD)(mcr.w) & ~(TM_TYPE_WORD)TM_MCR_MO_MASK)  \
                                     | ((TM_TYPE_WORD)(mo)<<TM_MCR_MO_OFF));}
#define SETMCRQO(mcr, qo)  \
    {(mcr.w)=(((TM_TYPE_WORD)(mcr.w) & ~(TM_TYPE_WORD)TM_MCR_QO_MASK)  \
                                     | ((TM_TYPE_WORD)(qo)<<TM_MCR_QO_OFF));}

/*---------- Data Structure: Data Areas and Force Tables ------------*/
typedef union
{
    TM_TYPE_BYTE   b[8];
    TM_TYPE_WORD   w[4];
} TM_TYPE_DOCK;

typedef union
{
    TM_TYPE_BYTE   b[32];
    TM_TYPE_WORD   w[16];
    TM_TYPE_DOCK   dock[4];
} TM_TYPE_PAGE;  /* for 4 docks */

typedef union
{
    TM_TYPE_PAGE   page[2];
    TM_TYPE_DOCK   pgdc[2][4];
    TM_TYPE_WORD   pgwd[2][16];
} TM_TYPE_DATA;  /* for 4 docks */

#define TM_PAGE_0    0      /* Data Area: Page 0     */
#define TM_PAGE_1    1      /* Data Area: Page 1     */
#define TM_FRC_DATA  0      /* Force Table Data Page */
#define TM_FRC_MASK  1      /* Force Table Mask Page */

/*---------- Data Structure: Port Control and Status Register -------*/
/* Bit  15-12   11   10    9    8  7-5    4    3    2    1    0 */
/*     f_code  src sink twcs   wa  dti cpe1 cpe0   qa  num   fe */
typedef union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_PCS_WORD0;

/* Bit   15-8    7    6    5    4    3    2    1    0 */
/*        dec  ptd   vp  crc  sqe  alo  bni terr  sto */
typedef VOL union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_PCS_WORD1;

/* Bit   15-8  7-0 */
/*       crc1 crc0 */
typedef VOL union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_PCS_CRCS;

typedef VOL struct
{
    TM_TYPE_PCS_WORD0 word0;
    TM_TYPE_PCS_WORD1 word1;
    TM_TYPE_RWORD     tack;
    TM_TYPE_PCS_CRCS  crcs;
} TM_TYPE_PCS;

                                    /* WORD 0 */
#define TM_PCS_FCODE_MSK    0xF000
#define TM_PCS_FCODE_OFF    12

#define TM_PCS_TYPE_MSK     0x0C00
#define TM_PCS_TYPE_OFF     10
#define TM_PCS_TYPE_CLR     0
#define TM_PCS_TYPE_SNK     1
#define TM_PCS_TYPE_SRC     2

#define TM_PCS_DTI_MSK      0x00E0
#define TM_PCS_DTI_OFF      5
#define TM_PCS_DTI_6        6

#define TM_PCS_FE_MSK       0x0001
#define TM_PCS_FE_OFF       0
#define TM_PCS_FE_CLR       0
#define TM_PCS_FE_SET       1

#define TM_PCS_NUM_MSK      0x0002
#define TM_PCS_NUM_OFF      1
#define TM_PCS_NUM_CLR      0
#define TM_PCS_NUM_SET      1

                                    /* WORD 1 */
#define TM_PCS_VP_MSK       0x0040
#define TM_PCS_VP_OFF       6
#define TM_PCS_DIAG_MSK     0x0007
#define TM_PCS_DIAG_OFF     0
#define TM_PCS_DIAG_CLR     0
#define TM_PCS_DIAG_STO     1
#define TM_PCS_DIAG_TERR    2
#define TM_PCS_DIAG_RDY     3
#define TM_PCS_DIAG_BNI     4
                                    /* MR VOL */
#define TM_MR_BUSY_MSK      0x0200
#define TM_MR_BUSY_OFF      9
#define TM_MR_BUSY_CLR      0
#define TM_MR_BUSY_SET      1

                                    /* MFS VOL */
#define TM_MFS_PRT_ADDR_MSK 0x0FFF
#define TM_MFS_PRT_ADDR_OFF 0

#define TM_SCR_IL_MSK       TM_SCR_IL_MASK
#define TM_SCR_IL_OFF       0
#define TM_SCR_IL_VAL_RST   TM_SCR_IL_RESET
#define TM_SCR_IL_VAL_CFG   TM_SCR_IL_CONFIG
#define TM_SCR_IL_VAL_TST   TM_SCR_IL_TEST
#define TM_SCR_IL_VAL_RUN   TM_SCR_IL_RUNNING


/* Alternative bit specification for PCS */
/* F-Codes: use W_FC0, W_FC1, etc */

#define TM_PCS0_SRC     0x0800
#define TM_PCS0_SINK    0x0400
#define TM_PCS0_TWCS    0x0200
#define TM_PCS0_WA      0x0100
#define TM_PCS0_DTI7    0x00E0
#define TM_PCS0_DTI6    0x00C0
#define TM_PCS0_DTI5    0x00A0
#define TM_PCS0_DTI4    0x0080
#define TM_PCS0_DTI3    0x0060
#define TM_PCS0_DTI2    0x0040
#define TM_PCS0_DTI1    0x0020
#define TM_PCS0_CPE1    0x0010
#define TM_PCS0_CPE0    0x0008
#define TM_PCS0_QA      0x0004
#define TM_PCS0_NUM     0x0002
#define TM_PCS0_FE      0x0001

#define TM_PCS1_DEC     0xFF00
#define TM_PCS1_PTD     0x0080
#define TM_PCS1_VP1     0x0040
#define TM_PCS1_VP0     0x0000
#define TM_PCS1_CRC     0x0020
#define TM_PCS1_SQE     0x0010
#define TM_PCS1_ALO     0x0008
#define TM_PCS1_BNI     0x0004
#define TM_PCS1_TERR    0x0002
#define TM_PCS1_STO     0x0001


/* The macro writes the value to the position in a pcs register which
   is given by the offset. */
#define TM_PUT_PCS(p_pcs, member, mask, offset, value)  \
    {  \
      writeWordToTS(  \
        (void*)(&((TM_TYPE_PCS *) (p_pcs))->member),  \
        (TM_TYPE_WORD)((readWordFromTS((void*)(&((TM_TYPE_PCS *) (p_pcs))->member)) & ~(mask)) |  \
        (((TM_TYPE_WORD)(value) << (offset)) & (mask))) ); \
    }


/* The macro returns the pcs type (source, sink).
   Return value:    pcs type */
#define LPL_GET_PRT_TYPE(p_pcs)  \
    ((readWordFromTS((void*)(&((TM_TYPE_PCS *) (p_pcs))->word0.w)) &  \
      TM_PCS_TYPE_MSK) >> TM_PCS_TYPE_OFF)


/* The macro reads the pcs tack bits.
   Return value:    TACK bits */
#define LPL_GET_PRT_TACK(p_pcs)  \
    readWordFromTS((void*)(&(((TM_TYPE_PCS*) (p_pcs))->tack)))



/* The macro reads the pcs page bit. The page bit indicates, which prt page
   (0/1) contains the valid data. Writers write to invalid page, readers
   read from valid page.
   The page bit is toggled by the writer after a valid update.
   Return value:    valid page bit */
#define LPL_GET_PRT_PAGE(p_pcs)  \
    ((readWordFromTS((void*)(&((TM_TYPE_PCS *) (p_pcs))->word1.w)) &  \
        TM_PCS_VP_MSK) >> TM_PCS_VP_OFF)


/* The macro returns the byte offset between the base of the target port
   (page 0) and the active page for reading traffic store data (page 0 or
   page 1).
   !!!Attention: Disable interrupts before using this function and reenable
                 the interrupts only after completing the data set read
                 operation.
   return vaule:    byte offset */
#define LPL_GET_PGE_OFFSET_RD(p_pcs)  \
    ((LPL_GET_PRT_PAGE(p_pcs) == 0 ) ?  \
        0 : (sizeof (TYPE_LPL_PRT_PGE)))


/* The macro returns the byte offset between the base of the target port
   (page 0) and the active page for writing traffic store data (page 0 or
   page 1).
   !!!Attention: Disable interrupts before using this function and reenable
                 the interrupts only after completing the data set write
                 operation.
   Return value:    byte offset */
#define LPL_GET_PGE_OFFSET_WT(p_pcs)  \
    ((LPL_GET_PRT_PAGE(p_pcs) == 0 ) ?  \
        (sizeof (TYPE_LPL_PRT_PGE)) : 0)


/* The macro writes the pcs type (LPL_PCS_SRCE, ...) to the port control
   and status register. */
#define LPL_PUT_PRT_TYPE(p_pcs, value)  \
    {  \
      UNSIGNED16 temp = (value) == LPL_PCS_BDIR ? LPL_PCS_SRCE : (value);  \
      TM_PUT_PCS (p_pcs, word0.w, TM_PCS_TYPE_MSK, TM_PCS_TYPE_OFF, temp); \
    }


/* The macro writes the fcode to port control and status register. */
#define LPL_PUT_PRT_FCODE(p_pcs, value)  \
    TM_PUT_PCS (p_pcs, word0.w, TM_PCS_FCODE_MSK, TM_PCS_FCODE_OFF, value)



/*---------- Data Structure: Bitmaps of Internal Registers (MVBC) ---*/

/*---------- Status Control Register SCR ----------------------------*/

/* Bit     15    14  13   12   11-10 9-8  7-6    5    4    3    2  1-0 */
/*         im quiet mbc  dummy  tmo   ws  arb  uts  utq  mas rcev   il */
typedef union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_SCR;

#define TM_SCRX_WS_0     0
#define TM_SCRX_WS_1     1
#define TM_SCRX_WS_2     2
#define TM_SCRX_WS_3     3

#define TM_SCRX_ARB_0    0
#define TM_SCRX_ARB_1    1
#define TM_SCRX_ARB_2    2
#define TM_SCRX_ARB_3    3

#define TM_SCRX_TMO_21US 0
#define TM_SCRX_TMO_43US 1
#define TM_SCRX_TMO_64US 2
#define TM_SCRX_TMO_83US 3

/* Alternative: 16-bit Mask Values */

#define TM_SCR_IM       0x8000
#define TM_SCR_QUIET    0x4000
#define TM_SCR_MBC      0x2000

#define TM_SCR_TMO_MASK 0x0C00
#define TM_SCR_TMO_OFF  10
#define TM_SCR_TMO_83US 0x0C00
#define TM_SCR_TMO_64US 0x0800
#define TM_SCR_TMO_43US 0x0400
#define TM_SCR_TMO_21US 0x0000

#define TM_SCR_WS_MASK  0x0300
#define TM_SCR_WS_3     0x0300
#define TM_SCR_WS_2     0x0200
#define TM_SCR_WS_1     0x0100
#define TM_SCR_WS_0     0x0000

#define TM_SCR_ARB_MASK 0x00C0
#define TM_SCR_ARB_3    0x00C0
#define TM_SCR_ARB_2    0x0080
#define TM_SCR_ARB_1    0x0040
#define TM_SCR_ARB_0    0x0000

#define TM_SCR_UTS      0x0020
#define TM_SCR_UTQ      0x0010
#define TM_SCR_MAS      0x0008
#define TM_SCR_RCEV     0x0004

#define TM_SCR_IL_MASK    0x0003
#define TM_SCR_IL_RUNNING 0x0003
#define TM_SCR_IL_TEST    0x0002
#define TM_SCR_IL_CONFIG  0x0001
#define TM_SCR_IL_RESET   0x0000

#define TM_SCR_MAS_SET  0x0001
#define TM_SCR_MAS_CLR  0x0000
#define TM_SCR_MAS_MSK  0x0008
#define TM_SCR_MAS_OFF  3

#define SETSCRIL(scr, il)  \
    {(scr.w)=(((scr.w) & ~(TM_TYPE_WORD)TM_SCR_IL_MASK)  \
                       | (TM_TYPE_WORD)(il));}
#define GETSCRIL(scr)  \
    ((TM_TYPE_WORD)(scr.w) & (TM_TYPE_WORD)TM_SCR_IL_MASK)

#define SETSCRTMO(scr, tmo)  \
    {(scr.w)=(((scr.w) & ~(TM_TYPE_WORD)TM_SCR_TMO_MASK)  \
                       | ((TM_TYPE_WORD)(tmo)<<TM_SCR_TMO_OFF));}

/*---------- Sink-Time Supervision Register STSR --------------------*/

/* Bit  15-12  11-0  */
/*     interv range  */
typedef union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_STSR;

#define TM_STSR_DOCKS_MASK 0x0FFF

#define TM_STSR_SI_MASK    0xF000    /* rei, instead of 0xc000 */
#define TM_STSR_SI_OFF         12
#define TM_STSR_INTERV_OFF      0
#define TM_STSR_INTERV_1MS      1
#define TM_STSR_INTERV_2MS      2
#define TM_STSR_INTERV_4MS      3
#define TM_STSR_INTERV_8MS      4
#define TM_STSR_INTERV_16MS     5
#define TM_STSR_INTERV_32MS     6
#define TM_STSR_INTERV_64MS     7
#define TM_STSR_INTERV_128MS    8
#define TM_STSR_INTERV_256MS    9

#define SETSTSRSI(stsr, si)  \
    {(stsr.w)=(((stsr.w) & ~(TM_TYPE_WORD)TM_STSR_SI_MASK)  \
                         | ((TM_TYPE_WORD)(si)<<TM_STSR_SI_OFF));}

#define SETSTSRDOCKS(stsr, docks)  \
    {(stsr.w)=(((stsr.w) & ~(TM_TYPE_WORD)TM_STSR_DOCKS_MASK)  \
                         | (TM_TYPE_WORD)(docks));}

/*---------- Master Register MR -------------------------------------*/

/* Bit   15   14   13   12   11   10    9    8  7-6    5  4-0 */
/*     par1 par0  ea1  ea0  ec1  ec0 busy csmf  smf smfm size */
typedef union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_MR;

/* Alternative definition: integer masks */

#define TM_MR_SMFM   0x0020
#define TM_MR_SMFA   0x0040
#define TM_MR_SMFT   0x0080
#define TM_MR_SMFE   0x00C0
#define TM_MR_CSMF   0x0100
#define TM_MR_BUSY   0x0200
#define TM_MR_EC0    0x0400
#define TM_MR_EC1    0x0800
#define TM_MR_EA0    0x1000
#define TM_MR_EA1    0x2000
#define TM_MR_PAR0   0x4000
#define TM_MR_PAR1   0x8000

#define TM_MR_CSMF_SET   0x0001
#define TM_MR_CSMF_MSK   0x0100
#define TM_MR_CSMF_OFF   8

#define TM_MR_SMFM_SET   0x0001
#define TM_MR_SMFM_MSK   0x0020
#define TM_MR_SMFM_OFF   5

#define TM_MR_SMFX_MSK   0x00DF
#define TM_MR_SMFX_OFF   0

#define TM_MR_SMFE_SET   0x00C0

/* Bit field 'smf' only: Valid values, for both MR and MR2 */

#define TM_MRX_MONE    0;
#define TM_MRX_SMFA    1;
#define TM_MRX_SMFT    2;
#define TM_MRX_SMFE    3;


/*---------- Secondary Master Register MR2 --------------------------*/
/* !!! Use TM_TYPE_MR, but following bits are ignored: */
/*     par1, par0, ea1, ea0, ec1, ec0, busy, csmf      */

/*---------- Interrupt 0 Registers: IMR0, ISR0, IVR0 ----------------*/

/* Bit   15   14   13   12   11   10    9    8    7 */
/*      emf  esf  dmf  dsf amfx  mfc  sfc  rti  bti */
/* Bit    6    5    4    3    2    1    0           */
/*     dti7 dti6 dti5 dti4 dti3 dti2 dti1           */
typedef union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_I0;

/* Alternative definition: integer masks */

#define TM_I0_EMF    0x8000
#define TM_I0_ESF    0x4000
#define TM_I0_DMF    0x2000
#define TM_I0_DSF    0x1000
#define TM_I0_AMFX   0x0800
#define TM_I0_MFC    0x0400
#define TM_I0_SFC    0x0200
#define TM_I0_RTI    0x0100
#define TM_I0_BTI    0x0080

#define TM_I0_DTI    0x007F

#define TM_I0_DTI7   0x0040
#define TM_I0_DTI6   0x0020
#define TM_I0_DTI5   0x0010
#define TM_I0_DTI4   0x0008
#define TM_I0_DTI3   0x0004
#define TM_I0_DTI2   0x0002
#define TM_I0_DTI1   0x0001

/*---------- Interrupt 1 Registers: IMR1, ISR1, IVR1 ----------------*/

/* Bit   15   14   13   12   11   10    9    8    7   6-3   2    1    0 */
/*      ti2  xi3  xi2  xqe  rqe xq1c xq0c  rqc  fev dummy ti1  xi1  xi0 */
typedef union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_I1;

/* Alternative definition: integer masks */

#define TM_I1_TI2    0x8000
#define TM_I1_XI3    0x4000
#define TM_I1_XI2    0x2000
#define TM_I1_XQE    0x1000
#define TM_I1_RQE    0x0800
#define TM_I1_XQ1C   0x0400
#define TM_I1_XQ0C   0x0200
#define TM_I1_RQC    0x0100
#define TM_I1_FEV    0x0080
#define TM_I1_TI1    0x0004
#define TM_I1_XI1    0x0002
#define TM_I1_XI0    0x0001


/*---------- Interrupt Vector Register IVR0/1 -----------------------*/

/* Bit 15-8  7-0 */
/*      iav  vec */
typedef union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_IVR;

/* Alternative definition: integer masks */

#define TM_IVR_IAV   0x0100
#define TM_IVR_MASK  0x00FF


/*---------- Timer Control Register: TCR ----------------------------*/

/* Bit  15-6   5    4    3    2    1    0 */
/*     dummy rs2  ta2 dummy xsyn rs1  ta1 */
typedef union
{
    TM_TYPE_BYTE   b[2];
    TM_TYPE_WORD   w;
} TM_TYPE_TCR;

/* Alternative definition: integer masks */

#define TM_TCR_RS2   0x0020
#define TM_TCR_TA2   0x0010
#define TM_TCR_XSYN  0x0004
#define TM_TCR_RS1   0x0002
#define TM_TCR_TA1   0x0001


/*---------- Data Structure: Queue Descriptor Table -----------------*/
typedef VOL struct
{
    TM_TYPE_WORD   xmit_q[2];
    TM_TYPE_WORD   rcve_q;
} TM_TYPE_QDT;

/*---------- Data Structure: Internal Registers (MVBC) --------------*/
typedef struct
{
    VOL TM_TYPE_SCR     scr;   /* + 0x00 */
    VOL TM_TYPE_WORD    dmy__02;
    VOL TM_TYPE_MCR     mcr;   /* + 0x04 */
    VOL TM_TYPE_WORD    dmy__06;
    VOL TM_TYPE_DR      dr;    /* + 0x08 */
    VOL TM_TYPE_WORD    dmy__0A;
    VOL TM_TYPE_STSR    stsr;  /* + 0x0C */
    VOL TM_TYPE_WORD    dmy__0E;

    VOL TM_TYPE_WORD    fc;    /* + 0x10 */
    VOL TM_TYPE_WORD    dmy__12;
    VOL TM_TYPE_WORD    ec;    /* + 0x14 */
    VOL TM_TYPE_WORD    dmy__16;
    VOL TM_TYPE_WORD    mfr;   /* + 0x18 */
    VOL TM_TYPE_WORD    dmy__1A;
    VOL TM_TYPE_WORD    mfre;  /* + 0x1C */
    VOL TM_TYPE_WORD    dmy__1E;

    VOL TM_TYPE_MR      mr;    /* + 0x20 */
    VOL TM_TYPE_WORD    dmy__22;
    VOL TM_TYPE_MR      mr2;   /* + 0x24 */
    VOL TM_TYPE_WORD    dmy__26;
    VOL TM_TYPE_WORD    dpr;   /* + 0x28 */
    VOL TM_TYPE_WORD    dmy__2A;
    VOL TM_TYPE_WORD    dpr2;  /* + 0x2C */
    VOL TM_TYPE_WORD    dmy__2E;

    VOL TM_TYPE_I0      ipr0;  /* + 0x30 */
    VOL TM_TYPE_WORD    dmy__32;
    VOL TM_TYPE_I1      ipr1;  /* + 0x34 */
    VOL TM_TYPE_WORD    dmy__36;
    VOL TM_TYPE_I0      imr0;  /* + 0x38 */
    VOL TM_TYPE_WORD    dmy__3A;
    VOL TM_TYPE_I1      imr1;  /* + 0x3C */
    VOL TM_TYPE_WORD    dmy__3E;

    VOL TM_TYPE_I0      isr0;  /* + 0x40 */
    VOL TM_TYPE_WORD    dmy__42;
    VOL TM_TYPE_I1      isr1;  /* + 0x44 */
    VOL TM_TYPE_WORD    dmy__46;
    VOL TM_TYPE_IVR     ivr0;  /* + 0x48 */
    VOL TM_TYPE_WORD    dmy__4A;
    VOL TM_TYPE_IVR     ivr1;  /* + 0x4C */
    VOL TM_TYPE_WORD    dmy__4E;

    VOL TM_TYPE_WORD    conf;  /* + 0x50 */
    VOL TM_TYPE_WORD    dmy__52;
    VOL TM_TYPE_WORD    wdc;   /* + 0x54 */	 /* Only available with MVBCS1 */
    VOL TM_TYPE_WORD    dmy__56;

    VOL TM_TYPE_WORD    daor;  /* + 0x58 */

    VOL TM_TYPE_WORD    dmy__5A;
    VOL TM_TYPE_WORD    daok;  /* + 0x5C */
    VOL TM_TYPE_WORD    dmy__5E;
    VOL TM_TYPE_TCR     tcr;   /* + 0x60 */
    VOL TM_TYPE_WORD    dmy__62;
	VOL TM_TYPE_WORD    ec_a;  /* + 0x64 */
	VOL TM_TYPE_WORD	dmy__66;
	VOL	TM_TYPE_WORD	ec_b;  /* + 0x68 */
	VOL TM_TYPE_WORD	dmy__6A;
	VOL TM_TYPE_WORD	liv;   /* + 0x6C */
	VOL TM_TYPE_WORD	dmy__6E;

    VOL TM_TYPE_WORD    tr1;   /* + 0x70 */
    VOL TM_TYPE_WORD    dmy__72;
    VOL TM_TYPE_WORD    tr2;   /* + 0x74 */
    VOL TM_TYPE_WORD    dmy__76;
    VOL TM_TYPE_WORD    tc1;   /* + 0x78 */
    VOL TM_TYPE_WORD    dmy__7A;
    VOL TM_TYPE_WORD    tc2;   /* + 0x7C */
    VOL TM_TYPE_WORD    dmy__7E;
} TM_TYPE_INT_REGS;

/*---------- mask for word register ---------------------------------*/
#define LCI_ALL_1 ((TM_TYPE_WORD) 0xFFFF) 

/*---------- mask for device address (daor register) ----------------*/
#define LCI_DA_MASK  0x0FFF     /* Max. Dev Addr is 4095 for class 1 */

/*---------- dispatch pointer register (dpr) ------------------------*/
#define TM_DPR_MASK  0xFFFC   /* Lower 2 bits are zero */


/*---------- Device Address Override Key (daok register) ------------*/
#define    TM_DAOK_ENABLE   0x94    /* Enable override  (Wr)             */
#define    TM_DAOK_DISABLE  0x49    /* Disable override (Wr)             */
#define    TM_DAOK_ENABLED  0xFF    /* Enabled Status   (Rd)             */
#define    TM_DAOK_DISALBED 0x00    /* Disabled Status  (Rd)             */

/*---------- Data Structure: Entire Service Area --------------------*/
typedef struct
{
    TM_TYPE_DATA     pp_data[ 8];  /* !!! Only 1st  4 of them are used */
    TM_TYPE_PCS      pp_pcs [32];  /* !!! Only 1st 16 of them are used */
    TM_TYPE_WORD     mfs;          /* Master Frame Slot                */
    TM_TYPE_WORD     dmy__1[7];    /* Vacancy between MFS and QDT      */
    TM_TYPE_QDT      qdt;          /* Queue Descriptor Table           */
    TM_TYPE_WORD     dmy__2[53];   /* Vacancy between QDT and 1st reg. */
    TM_TYPE_INT_REGS int_regs;     /* !!! Located inside MVBC          */
} TM_TYPE_SERVICE_AREA;


/* The macro references a selected port in the data area. This expression
   can be used on both left and right side of the assignment statements.
   input: tm_da      Start address to Traffic Store Data Area
          pidx       Port Index (0,1,2,...4095)
          pg         Page = { TM_PAGE_0, TM_PAGE_1 }
          wd         Word index, allowed values:
                     0..15: If pidx = {0,4,8,...4092}
                     0.. 7: If pidx = {2,6,10,..4094}
                     0.. 3: If pidx = {1,3,5,...4095}
   Return value:    Reference to data */
#define TM_1_DATA_WD(tm_da,pidx,pg,wd)  \
    ((tm_da)[(pidx)>>2].page[pg].dock[(pidx)&0x3].w[wd])


/* Location of physical ports, needed for pp_pcs and pp_data */

#define TM_PP_FC8     0x0   /* Mastership Offer Source Port             */
#define TM_PP_EFS     0x1   /* Event Frame Sink Port                    */
#define TM_PP_EF0     0x4   /* Event Frame Source Port for Ev. Type = 0 */
#define TM_PP_EF1     0x5   /* Event Frame Source Port for Ev. Type = 1 */

#define TM_PP_MOS     0x6   /* Mastership Offser Sink  Port             */
#define TM_PP_FC15    0x7   /* Device Status Report Source Port         */

#define TM_PP_MSRC    0x8   /* Message Source Port                      */
#define TM_PP_TSRC    0x8   /* Test    Source Port                      */
#define TM_PP_MSNK    0xC   /* Message Sink   Port                      */
#define TM_PP_TSNK    0xC   /* Test    Sink   Port                      */

/* MVBC Hardware Configuration Parameters */
#define LC_CH_A                  1        /* Channel A */
#define LC_CH_B                  0        /* Channel B */
#define LC_CH_BOTH               2        /* Both Channels */

#define LC_TREPLY_21US           0        /* Reply Timeout Coefficients */
#define LC_TREPLY_43US           1
#define LC_TREPLY_64US           2
#define LC_TREPLY_85US           3

/* Device Status Report */
#define LC_DSW_DNR_MSK              0x0002   /* Device Not Ready */
#define LC_DSW_DNR_SET              0x0002
#define LC_DSW_DNR_CLR              0x0000

#define LC_DSW_ERD_MSK              0x0008   /* Extended Reply Delay */
#define LC_DSW_ERD_SET              0x0008
#define LC_DSW_ERD_CLR              0x0000

#define LC_DSW_SDD_MSK              0x0010   /* Some Device Disturbance */
#define LC_DSW_SDD_SET              0x0010
#define LC_DSW_SDD_CLR              0x0000

#define LC_DSW_SSD_MSK              0x0020   /* Some System Disturbance */
#define LC_DSW_SSD_SET              0x0020
#define LC_DSW_SSD_CLR              0x0000

#define LC_DSW_LAA_MSK              0x0080   /* Line A Active (same as LAT) */
#define LC_DSW_LAA_SET              0x0080
#define LC_DSW_LAA_CLR              0x0000

#define LC_DSW_RLD_MSK              0x0040   /* Redundant Line Disturbed (same as OLD) */
#define LC_DSW_RLD_SET              0x0040
#define LC_DSW_RLD_CLR              0x0000

#define LC_DSW_TYPE_BRIDGE_MSK      0x2000
#define LC_DSW_TYPE_BRIDGE_SET      0x2000
#define LC_DSW_TYPE_BRIDGE_CLR      0x0000

#define LC_DSW_TYPE_BUS_ADMIN_MSK   0x4000
#define LC_DSW_TYPE_BUS_ADMIN_SET   0x4000
#define LC_DSW_TYPE_BUS_ADMIN_CLR   0x0000

#define LC_DSW_TYPE_CLASS_2_3_MSK   0x1000
#define LC_DSW_TYPE_CLASS_2_3_SET   0x1000
#define LC_DSW_TYPE_CLASS_2_3_CLR   0x0000

#define LC_DSW_TYPE_SPECIAL_MSK     0x8000
#define LC_DSW_TYPE_SPECIAL_SET     0x8000
#define LC_DSW_TYPE_SPECIAL_CLR     0x0000

/*---------- Type definition for a port block -----------------------*/
/* This type definition will be used, when the size of the port is   */
/* not evaluated. Copying with this structure will be typically      */
/* controlled by a variable "size".                                  */

typedef struct
{
    TM_TYPE_BYTE data[32];
} TYPE_LPL_PRT_PGE;


/*---------- Structure of port data store ---------------------------*/
/* The word "set" indicates, that ports come in "sets" of 4.         */
typedef struct
{
    TYPE_LPL_PRT_PGE    page_0;
    TYPE_LPL_PRT_PGE    page_1;

} TYPE_LPL_PRT_SET;

/*---------- Universal Constants ------------------------------------*/
/* Function Code, applies to both PCS Word 0 and Master Frames */
#define    W_FC0        0x0000    /* F-Code 0                 */
#define    W_FC1        0x1000    /* F-Code 1                          */
#define    W_FC2        0x2000    /* F-Code 2                          */
#define    W_FC3        0x3000    /* F-Code 3                          */
#define    W_FC4        0x4000    /* F-Code 4                          */
#define    W_FC5        0x5000    /* F-Code 5                          */
#define    W_FC6        0x6000    /* F-Code 6                          */
#define    W_FC7        0x7000    /* F-Code 7                          */
#define    W_FC8        0x8000    /* F-Code 8                          */
#define    W_FC9        0x9000    /* F-Code 9                          */
#define    W_FC10       0xa000    /* F-Code 10                         */
#define    W_FC11       0xb000    /* F-Code 11                         */
#define    W_FC12       0xc000    /* F-Code 12                         */
#define    W_FC13       0xd000    /* F-Code 13                         */
#define    W_FC14       0xe000    /* F-Code 14                         */
#define    W_FC15       0xf000    /* F-Code 15                         */

#endif
