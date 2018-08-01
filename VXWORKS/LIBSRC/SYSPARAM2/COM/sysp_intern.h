/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  sysp_intern.h
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2010/06/07 16:21:39 $
 *    $Revision: 1.26 $
 *
 *  	 \brief  Internal header file for sysparam library
 *
 *    \switches
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_intern.h,v $
 * Revision 1.26  2010/06/07 16:21:39  sy
 * Add declaration necessary for new EEPROD3  functions
 *
 * Revision 1.25  2010/03/11 16:37:39  sy
 * add new function SYSP_Eeprod2GrpCreateEx
 *
 * Revision 1.24  2010/02/01 17:24:40  rt
 * R: 1) Incorrect values could be stored to some of the system
 *       parameters (e.g. tdp, hdp...).
 * M: 1) Added SYSP_ValidateSD to SYSP_PDE_S16D macro.
 *
 * Revision 1.23  2010/01/22 13:31:06  RLange
 * R: Support password handling
 * M: Enhanced SYSP_PD_FLAGs
 *
 * Revision 1.22  2008/08/04 13:46:08  rt
 * R:1. Support for ESMexpress carrier boards
 * M:1. SYSP_NvsEsmxCbProbe added
 *
 * Revision 1.21  2008/06/30 13:49:48  ufranke
 * R: read only group not supported
 * M: added SYSP_MpGrpCreateStdRo()
 *
 * Revision 1.20  2008/06/11 17:22:09  rt
 * R:1. Print a note that user has to reset board after changing an ee-X
 *      parameter if needed.
 * M:1.a) Set SYSP_PD_NEEDRESET flag for some ee-X parameters.
 *     b) Add SYSP_PD_RESTART to SYSP_PD_FLAGS
 *
 * Revision 1.19  2007/07/11 14:53:35  rt
 * added:
 * - baudrate entries
 * - COM mode (RS232,RS4222,RS485) entries
 *
 * Revision 1.18  2007/06/28 13:49:39  rt
 * added:
 * - SYSP_PDE_STRUPPER
 *
 * Revision 1.17  2007/02/22 13:50:56  rla
 * changed interface for SYSP_EsmCarrierInitGlobals
 *
 * Revision 1.16  2006/12/01 14:49:17  rla
 * Adapted to support VC01, added SYSP_EsmCarrierInitGlobals()
 *
 * Revision 1.15  2006/05/16 11:07:45  ufranke
 * changed
 *  - SYSP_PDE_STWAIT from fix default 30
 *    to board dependend configuration parameter
 *
 * Revision 1.14  2006/04/19 18:52:50  rt
 * SYSP_HELP_DESC & SYSP_HELP_DESC_TBL added
 *
 * Revision 1.13  2006/01/05 14:01:14  cs
 * added
 *     + SYSP_IsEsmCB_with_EEPROD2()
 *     + SYSP_NvsHamedCompProbe()
 *
 * Revision 1.12  2005/06/23 15:54:26  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.11  2005/04/12 16:53:11  kp
 * SYSP_TusParamGet() now exported
 *
 * Revision 1.10  2005/02/22 15:00:58  kp
 * avoid OS-9 compiler warnings
 *
 * Revision 1.9  2005/02/02 16:03:11  kp
 * replaced SYSP_PDE_WDT1/2 by SYSP_PDE_WDT
 *
 * Revision 1.8  2005/01/18 15:49:28  kp
 * added size to NVS sections
 *
 * Revision 1.7  2005/01/12 14:19:57  kp
 * intermediate
 *
 * Revision 1.6  2004/12/23 09:35:36  ufranke
 * added
 *  + SYSP_NVS_ADR device size added to descriptor
 *  + SYSP_PROVIDE_KERPAR
 *  + SYSP_Pp01CarrierProbe()
 *
 * Revision 1.5  2004/12/20 08:54:12  kp
 * lots of minor changes
 *
 * Revision 1.4  2004/11/29 10:52:18  kp
 * support for MM unittest
 *
 * Revision 1.3  2004/10/20 09:20:46  kp
 * Added defs for ETH parameters
 *
 * Revision 1.2  2004/09/03 15:20:48  kp
 * added EEPROM parameter handling for 16 bit signed values
 *
 * Revision 1.1  2004/01/08 09:06:53  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef _SYSP_INTERN_H
#define _SYSP_INTERN_H

#include "sysp_sysdep.h"

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

/** initialize super's SYSP_ALLOC structure with array \a _arr */
#define SYSP_ALLOC_INIT( _super, _arr ) \
 do { \
 _super->alloc.mem = (u_int8 *)(_arr); _super->alloc.size = sizeof(_arr);\
 _super->alloc.minavail=0x7fffffff;\
} while(0);


/** alloc a number of bytes from the super objects main allocation object */
#define SYSP_ALLOC(_size, _super) \
 SYSP_Alloc( &(_super)->alloc, _size)

/** undo SYSP_ALLOC */
#define SYSP_ALLOC_UNDO( _size, _super) \
 SYSP_AllocUndo( &(_super)->alloc, _size)

/** special internal error code to restart entire sysparam */
#define SYSP_INTERNAL_RESTART 0x8000

/**** defines to fill SYSP_PAR_DESC structures ***/

/** native unsigned int decimal */
#define SYSP_PDE_NINTD( _nm, _alias, _offs, _deflt, _flg) \
 { _nm, _alias, _offs, SYSP_ToRawNatIntD, SYSP_FromRawNatIntD,SYSP_ValidateD,\
   _deflt, 0, _flg }

/** native unsigned int hexadecimal */
#define SYSP_PDE_NINTX( _nm, _alias, _offs, _deflt, _flg) \
 { _nm, _alias, _offs, SYSP_ToRawNatIntX, SYSP_FromRawNatIntX,SYSP_ValidateX,\
   _deflt, 0, _flg }

/** U32 decimal */
#define SYSP_PDE_U32D( _nm, _alias, _offs, _deflt, _flg) \
 { _nm, _alias, _offs, SYSP_ToRawU32D, SYSP_FromRawU32D, SYSP_ValidateD,\
   _deflt, 0, _flg }

/** U32 hex */
#define SYSP_PDE_U32X( _nm, _alias, _offs, _deflt, _flg) \
 { _nm, _alias, _offs, SYSP_ToRawU32X, SYSP_FromRawU32X, SYSP_ValidateX,\
   _deflt, 0, _flg}

/** U16 hex */
#define SYSP_PDE_U16X( _nm, _alias, _offs, _deflt, _flg) \
 { _nm, _alias, _offs, SYSP_ToRawU16X, SYSP_FromRawU16X, SYSP_ValidateX,\
   _deflt, 0xFFFF, _flg }

/** S16 decimal */
#define SYSP_PDE_S16D( _nm, _alias, _offs, _deflt, _flg) \
 { _nm, _alias, _offs, SYSP_ToRawS16D, SYSP_FromRawS16D, SYSP_ValidateSD,\
   _deflt, 0, _flg }

/** U8 hex */
#define SYSP_PDE_U8X( _nm, _alias, _offs, _deflt, _flg) \
 { _nm, _alias, _offs, SYSP_ToRawU8X, SYSP_FromRawU8X, SYSP_ValidateX,\
   _deflt, 0xFF, _flg}

/** U8 dec */
#define SYSP_PDE_U8D( _nm, _alias, _offs, _deflt, _flg) \
 { _nm, _alias, _offs, SYSP_ToRawU8D, SYSP_FromRawU8D, SYSP_ValidateD,\
   _deflt, 0xFF, _flg }

/** ASCII string */
#define SYSP_PDE_STR( _nm, _alias, _offs, _deflt, _len, _flg ) \
 { _nm, _alias, _offs, SYSP_ToRawStr, SYSP_FromRawStr, SYSP_ValidateStr,\
   _deflt, _len, _flg}

/** ASCII string (only upper case) */
#define SYSP_PDE_STRUPPER( _nm, _alias, _offs, _deflt, _len, _flg ) \
 { _nm, _alias, _offs, SYSP_ToRawStrUpper, SYSP_FromRawStr, SYSP_ValidateStr,\
   _deflt, _len, _flg}

/** MEN HW revision */
#define SYSP_PDE_REV( _nm, _alias, _offs ) \
 { _nm, _alias, _offs, SYSP_ToRawRev, SYSP_FromRawRev, SYSP_ValidateRev,\
   NULL, 0, SYSP_PD_MMPARASTR }

/** Ethernet MAC address */
#define SYSP_PDE_ETH( _nm, _alias, _offs, _deflt ) \
 { _nm, _alias, _offs, SYSP_ToRawEth, SYSP_FromRawEth, SYSP_ValidateEth,\
   _deflt, 0, SYSP_PD_MMPARASTR }

/** Nspeed value */
#define SYSP_PDE_NSPEED( _nm, _alias, _offs, _deflt ) \
 { _nm, _alias, _offs, SYSP_ToRawNspeed, SYSP_FromRawNspeed, \
   SYSP_ValidateNspeed,\
   _deflt, 0, SYSP_PD_MMPARASTR | SYSP_PD_NEEDRESET }

/** baudrate value */
#define SYSP_PDE_BAUD( _nm, _alias, _offs, _deflt, _flg) \
 { _nm, _alias, _offs, SYSP_ToRawU32D, SYSP_FromRawU32D, SYSP_ValidateBaud,\
   _deflt, 0, _flg }

/** COM mode */
#define SYSP_PDE_COMMODE( _nm, _alias, _offs, _deflt, _flg ) \
 { _nm, _alias, _offs, SYSP_ToRawComMode, SYSP_FromRawComMode, \
   SYSP_ValidateComMode,\
   _deflt, 0, _flg }

/** Touch calibration (4 integers comma separated) */
#define SYSP_PDE_TCAL( _nm, _alias, _offs, _deflt ) \
 { _nm, _alias, _offs, SYSP_ToRawTcal, SYSP_FromRawTcal, SYSP_ValidateTcal,\
   _deflt, 0, SYSP_PD_MMPARASTR }

/** boolean value stored in bit0 of 8 bit fields */
#define SYSP_PDE_BOOL0( _nm, _alias, _offs,  _deflt, _flg ) \
 { _nm, _alias, _offs, \
   SYSP_ToRawBool0, SYSP_FromRawBool0, SYSP_ValidateBool,\
   _deflt, 0, _flg }
/** boolean value stored in bit1 of 8 bit fields */
#define SYSP_PDE_BOOL1( _nm, _alias, _offs,  _deflt, _flg ) \
 { _nm, _alias, _offs, \
   SYSP_ToRawBool1, SYSP_FromRawBool1, SYSP_ValidateBool,\
   _deflt, 0, _flg }
/** boolean value stored in bit2 of 8 bit fields */
#define SYSP_PDE_BOOL2( _nm, _alias, _offs,  _deflt, _flg ) \
 { _nm, _alias, _offs, \
   SYSP_ToRawBool2, SYSP_FromRawBool2, SYSP_ValidateBool,\
   _deflt, 0, _flg }
/** boolean value stored in bit3 of 8 bit fields */
#define SYSP_PDE_BOOL3( _nm, _alias, _offs,  _deflt, _flg ) \
 { _nm, _alias, _offs, \
   SYSP_ToRawBool3, SYSP_FromRawBool3, SYSP_ValidateBool,\
   _deflt, 0, _flg }
/** boolean value stored in bit4 of 8 bit fields */
#define SYSP_PDE_BOOL4( _nm, _alias, _offs,  _deflt, _flg ) \
 { _nm, _alias, _offs, \
   SYSP_ToRawBool4, SYSP_FromRawBool4, SYSP_ValidateBool,\
   _deflt, 0, _flg }
/** boolean value stored in bit5 of 8 bit fields */
#define SYSP_PDE_BOOL5( _nm, _alias, _offs,  _deflt, _flg ) \
 { _nm, _alias, _offs, \
   SYSP_ToRawBool5, SYSP_FromRawBool5, SYSP_ValidateBool,\
   _deflt, 0, _flg }
/** boolean value stored in bit6 of 8 bit fields */
#define SYSP_PDE_BOOL6( _nm, _alias, _offs,  _deflt, _flg ) \
 { _nm, _alias, _offs, \
   SYSP_ToRawBool6, SYSP_FromRawBool6, SYSP_ValidateBool,\
   _deflt, 0, _flg }
/** boolean value stored in bit7 of 8 bit fields */
#define SYSP_PDE_BOOL7( _nm, _alias, _offs,  _deflt, _flg ) \
 { _nm, _alias, _offs, \
   SYSP_ToRawBool7, SYSP_FromRawBool7, SYSP_ValidateBool,\
   _deflt, 0, _flg }

/** End marker */
#define SYSP_PDE_END { NULL }


/*-------- Standard parameters -----------*/

#define SYSP_PDE_CON0(_eeoff, _def) \
 SYSP_PDE_U8X(SYSP_con0, NULL, (_eeoff), (_def), \
 (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET))
#define SYSP_PDE_CON1(_eeoff, _def) \
 SYSP_PDE_U8X(SYSP_con1, NULL, (_eeoff), (_def), \
 (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET))
#define SYSP_PDE_CON2(_eeoff, _def) \
 SYSP_PDE_U8X(SYSP_con2, NULL, (_eeoff), (_def), \
 (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET))
#define SYSP_PDE_CON3(_eeoff, _def) \
 SYSP_PDE_U8X(SYSP_con3, NULL, (_eeoff), (_def), \
 (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET))
#define SYSP_PDE_GCON(_eeoff, _def) \
 SYSP_PDE_U8X(SYSP_gcon, NULL, (_eeoff), (_def), \
 (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET))

#define SYSP_PDE_VMODE(_eeoff) \
 SYSP_PDE_U16X(SYSP_vmode, NULL, (_eeoff), "101", \
 (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET))

#define SYSP_PDE_CBR(_eeoff) \
 SYSP_PDE_BAUD(SYSP_cbr, SYSP_baud, (_eeoff), "9600",\
 SYSP_PD_MMPARASTR | SYSP_PD_NEEDRESET)

#define SYSP_PDE_BSADR(_eeoff) \
 SYSP_PDE_U32X(SYSP_bsadr, SYSP_bs, (_eeoff), "0", (SYSP_PD_MMPARASTR*0))

#define SYSP_PDE_ECL(_eeoff) \
 SYSP_PDE_U8X(SYSP_ecl, NULL, (_eeoff), "ff", \
 (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET))

#define SYSP_PDE_TTO(_eeoff) \
 SYSP_PDE_U8D(SYSP_tto, NULL, (_eeoff), "0", \
 (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET))

#define SYSP_PDE_TRIES(_eeoff) \
 SYSP_PDE_U8D(SYSP_tries, NULL, (_eeoff), "20", \
 (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET))

#define SYSP_PDE_TDP(_eeoff) \
 SYSP_PDE_S16D(SYSP_tdp, NULL, (_eeoff), "-1", \
 (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET))

#define SYSP_PDE_HDP(_eeoff) \
 SYSP_PDE_S16D(SYSP_hdp, NULL, (_eeoff), "-1", \
 (SYSP_PD_MMPARASTR*0 | SYSP_PD_NEEDRESET))

#define SYSP_PDE_WDT(_eeoff, _def) \
 SYSP_PDE_S16D(SYSP_wdt, NULL, (_eeoff), (_def), (SYSP_PD_MMPARASTR*0))

#define SYSP_PDE_STWAIT(_eeoff, _def) \
 SYSP_PDE_U8D(SYSP_stwait, NULL, (_eeoff), (_def), (SYSP_PD_MMPARASTR*0))

#define SYSP_PDE_UXX(_eeoff) \
	SYSP_PDE_U16X("u00", 		NULL, (_eeoff)+0x0, 	"0", 0),\
	SYSP_PDE_U16X("u01", 		NULL, (_eeoff)+0x2, 	"0", 0),\
	SYSP_PDE_U16X("u02", 		NULL, (_eeoff)+0x4,		"0", 0),\
	SYSP_PDE_U16X("u03", 		NULL, (_eeoff)+0x6, 	"0", 0),\
	SYSP_PDE_U16X("u04", 		NULL, (_eeoff)+0x8, 	"0", 0),\
	SYSP_PDE_U16X("u05", 		NULL, (_eeoff)+0xa, 	"0", 0),\
	SYSP_PDE_U16X("u06", 		NULL, (_eeoff)+0xc, 	"0", 0),\
	SYSP_PDE_U16X("u07", 		NULL, (_eeoff)+0xe, 	"0", 0),\
	SYSP_PDE_U16X("u08", 		NULL, (_eeoff)+0x10, 	"0", 0),\
	SYSP_PDE_U16X("u09", 		NULL, (_eeoff)+0x12, 	"0", 0),\
	SYSP_PDE_U16X("u10", 		NULL, (_eeoff)+0x14, 	"0", 0),\
	SYSP_PDE_U16X("u11", 		NULL, (_eeoff)+0x16, 	"0", 0),\
	SYSP_PDE_U16X("u12", 		NULL, (_eeoff)+0x18, 	"0", 0),\
	SYSP_PDE_U16X("u13", 		NULL, (_eeoff)+0x1a, 	"0", 0),\
	SYSP_PDE_U16X("u14", 		NULL, (_eeoff)+0x1c, 	"0", 0),\
	SYSP_PDE_U16X("u15", 		NULL, (_eeoff)+0x1e, 	"0", 0)

/** test if \a ch is a valid printable ASCII char */
#define SYSP_PRINTABLE( ch )			IN_RANGE( ch, 0x20, 0x7e )




/* debug */
#define DBH	(SYSP_super->dbh)
#define DBG_MYLEVEL DBG_ALL

#ifdef SYSP_CONFIG_UNITTEST

#ifndef SYSP_UNITTEST
# define SYSP_UNITTEST(_name_,_func_)	/* as nothing by default */
#endif

#ifndef UTASSERT
# define UTASSERT(_expr) \
 if( !(_expr) ){\
   DBGWRT_ERR((DBH,"*** UNIT-TEST assertion failed %s line %d: %s\n", \
   __FILE__, __LINE__, #_expr ));\
   goto ABORT;\
 }
#endif /* UTASSERT */

#endif /* SYSP_CONFIG_UNITTEST */

#define SYSP_DONT_PROVIDE_KERPAR		0
#define SYSP_PROVIDE_KERPAR				1

/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/

/** Descriptor for SYSPARAM parameter descriptions.
 *
 *
 */
typedef struct {
	const char *parName;
	const char *aliasName;
	const char *shortDesc;
	const char *longDescFirstLine;
	const char *longDescContinuation;
	const char *allowedChars;
} SYSP_HELP_DESC;

/** Descriptor for parameter description table.
 *
 *
 */
typedef struct SYSP_HELP_DESC_TBL {
	struct SYSP_HELP_DESC_TBL* next;      /* next table (NULL if last) */
	const SYSP_HELP_DESC* desc;           /* start of table */
} SYSP_HELP_DESC_TBL;

/** Descriptor for NV storage address.
 *
 *
 */
typedef struct {
	enum {
		SYSP_NVSADR_SMB,		/**< address is smb address  */
		SYSP_NVSADR_MEM			/**< address is memory address */
	} type;						/**< how union below is to be interpreted */

	union {
		struct {
			u_int8 bus;			/**< SMB bus number */
			u_int8 dev;			/**< device address on SMB bus */
		} smb;					/**< SMB address (if type == SYSP_NVSADR_SMB)*/
		void *mem;				/**< mem address (if type == SYSP_NVSADR_MEM)*/
	} addr;

} SYSP_NVS_ADR;

/** Parameter group object
 *
 * Contains the methods and variables to handle an instance of
 * a parameter group, e.g. a section in an EEPROM or parameter group
 * for VxBline or menmon parameter string
 */
typedef struct SYSP_PAR_GRP {

	/** pointer to next parameter group (must be first elem!) */
	struct SYSP_PAR_GRP *next;

	/** name of this parameter group (e.g. "prod") */
	const char *grpName;

	/** the NV storage object that contains this parameter group
	 *  (null if none)
	 */
	struct SYSP_NV_STORAGE *nvs;

	/** starting byte offset of this group within NV storage */
	int nvsOffset;

	/** get the number of parameters handled by that group */
	int (*numParams)( struct SYSP_PAR_GRP *parGrp );

	/** function to enumerate all parameters of that group
	 *
	 * Writes info about parameter specified by \a idx to \a info
	 *
	 * \param parGrp 		\IN  this param group object
	 * \param idx			\IN  the nth parameter to query
	 * \param alias			\IN  if TRUE, return alias name
	 * \param info			\OUT receives parameter name (without prefix)
	 *							 and parameter's access attributes
	 *
	 * \return SYSPARAM error code\n
	 *	- SYSPARAM_INVAL_VALUE if \a idx out of range
	 */
	int (*enumParams)(
		struct SYSP_PAR_GRP *parGrp,
		int idx,
		int alias,
		SYSPARAM_PARAM_INFO *info);

	/** function to set a parameter of that group
	 *
	 * parameter names do not include prefix.
	 *
	 * parameter is written only to internal shadow copy.
	 * Not saved until save() method is called
	 *
	 * \param parGrp 	\IN  this param group object
	 * \param parName	\IN  the parameter name to set (without prefix)
	 * \param parVal	\IN  new parameter value
	 * \param passwd	\IN  password for password protected values
	 *						 (may be NULL)
	 * \param attr		\IN  0 or #SYSPARAM_SA_FROM_FIRMWARE
	 *
	 * \return SYSPARAM error code
	 */
	int (*setParam)(
		struct SYSP_PAR_GRP *parGrp,
		const char *parName,
		const char *parVal,
		const char *passwd,
		int attr);

	/** function to get a parameter of that group
	 *
	 * \param parGrp 	\IN  this param group object
	 * \param parName   \IN  the parameter name to get (without prefix)
	 * \param parVal	\OUT receives parameter value as a string
	 * \param parValMaxLen \IN max length of parVal (including terminating 0)
	 *
	 * \return SYSPARAM error code
	 */
	int (*getParam)(
		struct SYSP_PAR_GRP *parGrp,
		const char *parName,
		char *parVal,
		int parValMaxLen );

	/** function to get a parameter's default of that group
	 *
	 * This pointer can be NULL
	 */
	int (*getParamDefault)(
		struct SYSP_PAR_GRP *parGrp,
		const char *parName,
		char *parVal,
		int parValMaxLen );

	/** set default values of all parameters of that group
	 *
	 * parameters are written only to internal shadow copy.
	 *
	 * This method can be NULL
	 *
	 * \param parGrp 	\IN  this param group object
	 * \param force		\IN  if TRUE, apply defaults, even if this group
	 *						 is a inventory group
	 *
	 * \return SYSPARAM error code
	 */
	int (*setDefaults)(
		struct SYSP_PAR_GRP *parGrp,
		int force);

	/** save parameter group to NV storage
	 * This method can be NULL
	 * \param parGrp 	\IN  this param group object
	 * \return SYSPARAM error code
	 */
	int (*save)(
		struct SYSP_PAR_GRP *parGrp);


} SYSP_PAR_GRP;

/** Descriptor for a single parameter, used by SYSP_PGRP_MAGPAR
 *
 */
typedef struct {
	const char *parName;		/**< parameter name  */
	const char *aliasName;		/**< alias name (may be NULL)  */
	int offset;					/**< raw data offset within group  */

	/** write new value to raw data.
	 * \param dest 		\OUT receives raw data
	 * \param src		\IN new value as string
	 * \return sysparam error code
	 */
	int (*toRaw)( void *dest, const char *src );

	/** convert raw data to string
	 * \param dest		\OUT receives string
	 * \param destLen	\IN max length of dest, incl. '\\0'
	 * \param src		\IN raw data
	 * \return sysparam error code
	 */
	int (*fromRaw) ( char *dest, int destLen, void *src );

	/** check if value \a str is valid for parameter.
	 * this method can be NULL.
	 * \param str 		\IN pointer to raw data
	 * \param vParam	\IN method specific validation parameter
	 * \return sysparam error code
	 */
	int (*validate) ( const char *str, int vParam );
	const char *defaultVal;		/**< parameter default (as string)  */

	/** vParam is a genereric parameter passed to validate().
	 * used to specify limits.
	 */
	int vParam;

	int pdFlags;				/**< see SYSP_PD_FLAGS */

} SYSP_PAR_DESC;

typedef	enum {
		SYSP_PD_NONE=0,
		SYSP_PD_MMPARASTR=0x1,	/**< include it in MENMON parameter string  */
		SYSP_PD_RESTART=0x2,	/**< return SYSP_INTERNAL_RESTART when set */
		SYSP_PD_NEEDRESET=0x4,	/**< parameter needs restart to take effect */
		SYSP_PD_ENCRYPT=0x8		/**< parameter output encrypted */
} SYSP_PD_FLAGS;				/**< param desc flags  */


/** Parameter group for EEPROM structures with NV magic/parity
 *
 */
typedef struct {
	SYSP_PAR_GRP parGrp;		/**< common parameter group object  */
	const SYSP_PAR_DESC *parList; /**< list of parameters  */
	int size;					/**< size of section (incl. magic/parity)  */
	u_int8 *rawData;			/**< raw data  */
	u_int8 magic;				/**< magic ID of section */
	u_int8 _pad;
	enum {
		SYSP_MPGRP_NONE=0,
		SYSP_MPGRP_PROD=0x1 	/**< group contains production data */
	} mpFlags;					/**< group flags */
	int numParams;				/**< number of parameters handled  */
} SYSP_PGRP_MAGPAR;


/** NV storage object
 *
 * Contains the methods and variables to handle an instance of
 * a sysparam NV storage (EEPROM).
 */
typedef struct SYSP_NV_STORAGE {

	/** pointer to next NVS  (must be first elem!) */
	struct SYSP_NV_STORAGE *next;

	/** name of this NVS (e.g. "em04cpu") */
	const char *nvsName;

	/** sysparam super object */
	struct SYSP_SUPER *super;

	/** parameter name prefix of that group */
	char prefix[SYSP_PREFIX_LEN];

	/** phys. address of NV storage */
	SYSP_NVS_ADR nvsAdr;

	/** size of phys. section in bytes */
	u_int32 nvsSize;

	/** linked list of parameter group objects */
	SYSP_PAR_GRP *parGrpLst;

	/** function to read from NV storage
	 *
	 * \param nvs			\IN this NV storage object
	 * \param offset		\IN starting byte offset within NVS
	 * \param size			\IN number of bytes to read from NVS
	 * \param data			\OUT receives read data
	 * \return SYSPARAM error code
	 */
	int (*readNvs)(
		struct SYSP_NV_STORAGE *nvs,
		int offset,
		int size,
		u_int8 *data);

	/** function to write to NV storage
	 *
	 * \param nvs			\IN this NV storage object
	 * \param offset		\IN starting byte offset within NVS
	 * \param size			\IN number of bytes to write to NVS
	 * \param data			\IN data to write to NVS
	 * \return SYSPARAM error code
	 */
	int (*writeNvs)(
		struct SYSP_NV_STORAGE *nvs,
		int offset,
		int size,
		const u_int8 *data);

	/** get the number of parameters handled by that group */
	int (*numParams)( struct SYSP_NV_STORAGE *nvs );

	/** function to enumerate all parameters of that NVS
	 *
	 * Writes info about parameter specified by \a idx to \a info.
	 *
	 * \param nvs			\IN  this NV storage object
	 * \param idx			\IN  the nth parameter to query
	 * \param alias			\IN  if TRUE, return alias name
	 * \param info			\OUT receives the parameter name
	 *							 and the parameter's access attributes
	 *
	 * \return SYSPARAM error code\n
	 *	- SYSPARAM_INVAL_VALUE if \a idx out of range
	 */
	int (*enumParams)(
		struct SYSP_NV_STORAGE *nvs,
		int idx,
		int alias,
		SYSPARAM_PARAM_INFO *info);

	/** function to set a parameter of that NV storage object
	 *
	 * parameter names must include possible prefix.
	 *
	 * New parameter value is written only to internal shadow copy and
	 * is not saved until save() method is called.
	 *
	 * \param nvs	 	\IN  this NV storage object
	 * \param parName	\IN  the parameter name to set (with prefix)
	 * \param parVal	\IN  new parameter value
	 * \param passwd	\IN  password for password protected values
	 *						 (may be NULL)
	 * \param attr		\IN  0 or #SYSPARAM_SA_FROM_FIRMWARE
	 *
	 * \return SYSPARAM error code
	 */
	int (*setParam)(
		struct SYSP_NV_STORAGE *nvs,
		const char *parName,
		const char *parVal,
		const char *passwd,
		int attr);

	/** function to get a parameter of that NV storage object
	 *
	 * \param nvs	 	\IN  this NV storage object
	 * \param parName   \IN  the parameter name to get (with prefix)
	 * \param parVal	\OUT receives parameter value as a string
	 * \param parValMaxLen \IN max length of parVal (including terminating 0)
	 *
	 * \return SYSPARAM error code
	 */
	int (*getParam)(
		struct SYSP_NV_STORAGE *nvs,
		const char *parName,
		char *parVal,
		int parValMaxLen );

	/** function to get a parameter's default of that NV storage object
	 */
	int (*getParamDefault)(
		struct SYSP_NV_STORAGE *nvs,
		const char *parName,
		char *parVal,
		int parValMaxLen );

	int (*setDefaults)(
		struct SYSP_NV_STORAGE *nvs,
		int force);

	/** save all parameters of that NVS object to physical NV storage
	 *
	 * \param nvs	 	\IN  this NV storage object
	 * \return SYSPARAM error code
	 */
	int (*save)(
		struct SYSP_NV_STORAGE *nvs);

} SYSP_NV_STORAGE;

/** structure to maintain allocated objects
 *
 */
typedef struct SYSP_ALLOC {

	/** byte array */
	u_int8 *mem;

	/** number of bytes available at mem */
	int size;

	int minavail;
} SYSP_ALLOC;


/** Sysparam super object
 *
 */
typedef struct SYSP_SUPER {

   	/** function to read from NV storage
	 *
	 * \param super			\IN this object
	 * \param nvsAdr		\IN NV storage address
	 * \param offset		\IN starting byte offset within NVS
	 * \param size			\IN number of bytes to read from NVS
	 * \param data			\OUT receives read data
	 * \return SYSPARAM error code
	 */
	int (*readNvs)(
		struct SYSP_SUPER *super,
		SYSP_NVS_ADR nvsAdr,
		int offset,
		int size,
		u_int8 *data);

	/** function to write to NV storage
	 *
	 * \param super			\IN this object
	 * \param nvsAdr		\IN NV storage address
	 * \param offset		\IN starting byte offset within NVS
	 * \param size			\IN number of bytes to write to NVS
	 * \param data			\IN data to write to NVS
	 * \return SYSPARAM error code
	 */
	int (*writeNvs)(
		struct SYSP_SUPER *super,
		SYSP_NVS_ADR nvsAdr,
		int offset,
		int size,
		const u_int8 *data);

	/** function to restart entrire sysparam lib. */
	int (*reInit)( struct SYSP_SUPER *super );

	/** function to do any required post processing after a SysParamSet().
	 * can be used to update MENMON parameter string.
	 *
	 * \param super			\IN this object
	 * \param parName		\IN parameter name including possible prefix.
	 *							NULL if called from SysParamSetDefaults
	 * \param attr			\IN attribute flags (see #SYSPARAM_SET_ATTR)
	 *
	 */
	int (*postProcessSetParam)( struct SYSP_SUPER *super,
								const char *parName,
								int attr);


	/** second function to do any required post processing
 	 * after a SysParamSet(). Installed by SYSP_PostProcessingHookInstall()
	 *
	 * \param parName		\IN parameter name including possible prefix.
	 *							NULL if called from SysParamSetDefaults
	 * \param attr			\IN attribute flags (see #SYSPARAM_SET_ATTR)
	 * \return SYSPARAM error code
	 */
	/** users' hook.  */
	SYSP_POST_PROCESS_CB postProcessHook;

	/** linked list of NV storage objects */
	SYSP_NV_STORAGE *nvsLst;

	/** main allocation object */
	SYSP_ALLOC alloc;

	/** sysparam initialized */
	int initialized;

	DBG_HANDLE *dbh;			/**< debug handle  */

} SYSP_SUPER;

/** Sysparam super object extended for MENMONs CPUs with SMB NV storage
 *
 */
typedef struct {
	SYSP_SUPER s;
	SYSPARAM_MMSMB_INIT initBlk;
} SYSP_MMSMB_SUPER;


/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
extern SYSP_SUPER *SYSP_super;

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/

/* sysp_core.c */
void SYSP_SuperMmsmbInit( SYSP_MMSMB_SUPER *xSuper );
int SYSP_RunPostProcessHooks(
	SYSP_SUPER *super,
	const char *parName,
	int attr,
	int errCode);
SYSP_NV_STORAGE *SYSP_NvsCreate(
	int objSize,
	const char *nvsName,
	SYSP_SUPER *super,
	const char *prefix,
	SYSP_NVS_ADR nvsAdr,
	u_int32 nvsSize);
void SYSP_NvsAdd( SYSP_SUPER *super, SYSP_NV_STORAGE *nvs );
SYSP_NV_STORAGE *SYSP_NvsFind( SYSP_SUPER *super, const char *nvsName);
void SYSP_ParGrpAdd( SYSP_NV_STORAGE *nvs, SYSP_PAR_GRP *parGrp );
void *SYSP_Alloc( SYSP_ALLOC *alloc, int size );
void SYSP_AllocUndo( SYSP_ALLOC *alloc, int size );
const char *SYSP_Prefix( const char *parName, char *prefix );
SYSP_PAR_GRP *SYSP_ParNameToGroup(
	SYSP_SUPER *super,
	const char *parName,
	const char **pureNameP);
char *SYSP_FullParName(
	SYSP_NV_STORAGE *nvs,
	const char *parName,
	char *buf );
int SYSP_ContainsBlanks( const char *s );
const SYSP_PAR_DESC *SYSP_FindParDesc(
	const SYSP_PAR_DESC *parList,
	const char *parName );
int SYSP_ParDescArrayLen( const SYSP_PAR_DESC *parList );

int SYSP_FromRawNatIntD( char *dest, int destLen, void *src );
int SYSP_ToRawNatIntD( void *dest, const char *src );
int SYSP_FromRawNatIntX( char *dest, int destLen, void *src );
int SYSP_ToRawNatIntX( void *dest, const char *src );
int SYSP_FromRawU32D( char *dest, int destLen, void *src );
int SYSP_ToRawU32D( void *dest, const char *src );
int SYSP_FromRawU32X( char *dest, int destLen, void *src );
int SYSP_ToRawU32X( void *dest, const char *src );
int SYSP_FromRawU16X( char *dest, int destLen, void *src );
int SYSP_ToRawU16X( void *dest, const char *src );
int SYSP_FromRawS16D( char *dest, int destLen, void *src );
int SYSP_ToRawS16D( void *dest, const char *src );
int SYSP_FromRawU16D( char *dest, int destLen, void *src );
int SYSP_ToRawU16D( void *dest, const char *src );
int SYSP_FromRawU8X( char *dest, int destLen, void *src );
int SYSP_ToRawU8X( void *dest, const char *src );
int SYSP_FromRawU8D( char *dest, int destLen, void *src );
int SYSP_ToRawU8D( void *dest, const char *src );
int SYSP_FromRawStr( char *dest, int destLen, void *src );
int SYSP_ToRawStr( void *dest, const char *src );
int SYSP_FromRawRev( char *dest, int destLen, void *src );
int SYSP_ToRawStrUpper( void *dest, const char *src );
int SYSP_ToRawRev( void *dest, const char *src );
int SYSP_FromRawTcal( char *dest, int destLen, void *src );
int SYSP_ToRawTcal( void *dest, const char *src );
int SYSP_FromRawNspeed( char *dest, int destLen, void *src );
int SYSP_ToRawNspeed( void *dest, const char *src );
int SYSP_FromRawComMode( char *dest, int destLen, void *src );
int SYSP_ToRawComMode( void *dest, const char *src );


int SYSP_ToRawBool7( void *dest, const char *src );
int SYSP_ToRawBool6( void *dest, const char *src );
int SYSP_ToRawBool5( void *dest, const char *src );
int SYSP_ToRawBool4( void *dest, const char *src );
int SYSP_ToRawBool3( void *dest, const char *src );
int SYSP_ToRawBool2( void *dest, const char *src );
int SYSP_ToRawBool1( void *dest, const char *src );
int SYSP_ToRawBool0( void *dest, const char *src );

int SYSP_FromRawBool7( char *dest, int destLen, void *src );
int SYSP_FromRawBool6( char *dest, int destLen, void *src );
int SYSP_FromRawBool5( char *dest, int destLen, void *src );
int SYSP_FromRawBool4( char *dest, int destLen, void *src );
int SYSP_FromRawBool3( char *dest, int destLen, void *src );
int SYSP_FromRawBool2( char *dest, int destLen, void *src );
int SYSP_FromRawBool1( char *dest, int destLen, void *src );
int SYSP_FromRawBool0( char *dest, int destLen, void *src );

int SYSP_ValidateBool( const char *src, int max );

int SYSP_ValidateD( const char *src, int max );
int SYSP_ValidateSD( const char *src, int max );
int SYSP_ValidateX( const char *src, int max );
int SYSP_ValidateStr( const char *src, int max );
int SYSP_ValidateRev( const char *src, int max );
int SYSP_ValidateTcal( const char *src, int max );
int SYSP_ValidateEth( const char *src, int max );
int SYSP_ValidateNspeed( const char *src, int max );
int SYSP_ValidateComMode( const char *src, int max );
int SYSP_ValidateBaud( const char *src, int max );

/* sysp_mpgrp.c */
int SYSP_MpGrpCreateStd(
	SYSP_NV_STORAGE *nvs,
	const char *name,
	int nvsOffset,
	int nvsSize,
	const SYSP_PAR_DESC *parList,
	u_int8 magic,
	SYSP_PGRP_MAGPAR **mpGrpP);

int SYSP_MpGrpCreateStdRo(
	SYSP_NV_STORAGE *nvs,
	const char *name,
	int nvsOffset,
	int nvsSize,
	const SYSP_PAR_DESC *parList,
	u_int8 magic,
	SYSP_PGRP_MAGPAR **mpGrpP);

int SYSP_MpGrpCreateProd(
	SYSP_NV_STORAGE *nvs,
	const char *name,
	int nvsOffset,
	int nvsSize,
	const SYSP_PAR_DESC *parList,
	u_int8 magic,
	SYSP_PGRP_MAGPAR **mpGrpP);

/* sysp_eeprod.c */
int SYSP_EeprodGrpCreate(
	SYSP_NV_STORAGE *nvs,
	const char *expHwNames[],
	int ignoreNvsError);

/* sysp_eeprod2.c */
int SYSP_Eeprod2GrpCreate(
	SYSP_NV_STORAGE *nvs,
	const char *expHwNames[],
	int ignoreNvsError);

int SYSP_Eeprod2GrpCreateEx(
	SYSP_NV_STORAGE *nvs,
	const char *expHwNames[],
	u_int32 offset,
	int ignoreNvsError);	
	
/* sysp_eeprod3.c */
int SYSP_Eeprod3GrpCreate(
	SYSP_NV_STORAGE *nvs,
	const char *expHwNames[],
	int ignoreNvsError);

int SYSP_Eeprod3GrpCreateEx(
	SYSP_NV_STORAGE *nvs,
	const char *expHwNames[],
	u_int32 offset,
	int ignoreNvsError);		

/* sysp_mmpara.c */


int SYSP_TusParamSet(
	char *tus,
	int tusMaxLen,
	const char *parName,
	const char *parVal);

int SYSP_MmgrpCreate(
	SYSP_NV_STORAGE *nvs,
	const char *name,
	const SYSP_CONST_STRING *parList,
	int bufSize,
	const char *mmParaStr,
	int mmParaStrMaxLen);

int SYSP_MmParaStringBuild(
	char *mmParaStr,
	int mmParaStrMaxLen,
	SYSP_SUPER *super);

/* sysp_vxbline.c */
int SYSP_VxGrpCreate(
	SYSP_NV_STORAGE *nvs,
	int nvsOffset,
	int nvsSize,
	char *vxBline,
	const char *vxDefBline,
	int provideKerpar);

int SYSP_FromRawVxblIp( char *dest, int destLen, void *src );
int SYSP_FromRawVxblSm( char *dest, int destLen, void *src );
int SYSP_ToRawVxblIp( void *dest, const char *src );
int SYSP_ToRawVxblSm( void *dest, const char *src );


/* sysp_strgrp.c */
int SYSP_StrGrpCreate(
	SYSP_NV_STORAGE *nvs,
	const char *name,
	const char *alias,
	int nvsOffset,
	int nvsSize,
	const char *defVal);

/* sysp_esm_cb.c */
extern void SYSP_EsmCarrierInitGlobals( int addr, int size, char prefix );
extern int SYSP_EsmCarrierProbe( SYSP_SUPER *super, int smbBusNo);
extern int SYSP_NvsAd65EtcProbe( SYSP_NV_STORAGE *nvs );
extern int SYSP_Pp01CarrierProbe
(
	SYSP_SUPER  *super,
	char		*sectionName,
	char 		*prefix,
	int 		smbBusNo,
	u_int32		devAddr,
	u_int32		devSize
);

extern int SYSP_Vc01CarrierProbe
(
	SYSP_SUPER  *super,
	char		*sectionName,
	char 		*prefix,
	int 		smbBusNo,
	u_int32		devAddr,
	u_int32		devSize
);

extern int SYSP_IsEsmCB_with_EEPROD2( const char *brdName );
extern int SYSP_NvsHamedCompProbe( SYSP_NV_STORAGE *nvs );
extern int SYSP_NvsSaurerProbe( SYSP_NV_STORAGE *nvs );
extern int SYSP_NvsEsmxCbProbe( SYSP_NV_STORAGE *nvs );

/* sysp_mm_netif.c */
extern void SYSP_MmNetIfParamsCreate( SYSP_SUPER *super );

/* sysp_paramhelp.c */
int SYSP_AddParamTable( SYSP_HELP_DESC_TBL* newTable );

#ifdef SYSP_CONFIG_UNITTEST
/* this must be provided by unittest main */
void SYSP_UtSuperInit(void);

int SYSP_UtCore( void );
int SYSP_UtMpGrp( void );
int SYSP_UtMmGrp( void );
int SYSP_UtTus(void);
int SYSP_UtMmParaStringBuild( void );
int SYSP_UtVxBl( void );
int SYSP_UtStrGrp( void );
int SYSP_UtProd2Date( void );
int SYSP_UtHelp( void );
int SYSP_UtStringTbl( void );

#endif /* SYSP_CONFIG_UNITTEST */

#endif /* _SYSP_INTERN_H */



