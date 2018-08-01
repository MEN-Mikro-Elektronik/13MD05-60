/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: oss_os.h
 *
 *      $Author: sma $
 *        $Date: 2012/09/17 15:47:09 $
 *    $Revision: 1.28 $
 *
 *  Description: VxWorks specific data types and defines
 *               headers
 *
 *     Switches: _OSS_COMPILE
 *               INCLUDE_MIPIOS_VX_TARGET for MIPIOS targets 
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oss_os.h,v $
 * Revision 1.28  2012/09/17 15:47:09  sma
 * R: spinLockLib.h not available before vxWorks 6.4
 * M: made include of spinLockLib.h depending on VxWorks version
 *
 * Revision 1.27  2012/08/09 21:33:19  ts
 * R: Macro to determine VxWorks version can fail if minor or maintenance > 10
 * M: reworked macro in unixoid OS style: Major,minor,maintenance are 3 bytes
 *
 * Revision 1.26  2012/07/10 15:49:10  ts
 * R: APIs funcs changed in 6.8.3 (usrAtaDevCreate...) menata2xbd.c build failed
 * M: added define VX_KERNEL_VERSION to compile code depending from it
 *
 * Revision 1.25  2012/03/07 17:29:26  channoyer
 * R: Page Fault during OSS_SpinLockCreate
 * M: Replace previous defined empty type OSS_SPINL_HANDLE with spinlockIsr_t
 *
 * Revision 1.24  2010/12/02 15:46:49  UFranke
 * R: can'nt compile with oss.h Revision 1.53
 * M: temporary workaround: empty type OSS_SPINL_HANDLE added
 *
 * Revision 1.23  2010/05/11 16:33:48  ufranke
 * R: DIAB linker error SDA VxWorks 6.7 PPC due to access to global variable
 * M: OSS_IrqNum0 is static now
 *
 * Revision 1.22  2010/01/28 15:54:59  ufranke
 * R: compiler error in uos_key.c
 * M: readded stdio.h always
 *
 * Revision 1.21  2010/01/25 16:44:04  cs
 * R:1. variable argument list definitions where moved in WR headers
 *   2. missing include caused "undefined reference .." error with VxW 6.7
 * M:1. include stdarg.h instead of stdio.h for VxWorks >= 6.7
 *   2. added include file semLib.h
 *
 * Revision 1.20  2009/02/19 10:07:25  ufranke
 * prepared
 *  + for MIPIOS
 *
 * Revision 1.19  2008/09/26 13:50:26  ufranke
 * R: VxWorks 6.6 SMP
 * M: internal changes
 *
 * Revision 1.18  2008/04/28 18:02:22  CKauntz
 * Cosmetics
 *
 * Revision 1.17  2006/06/08 14:08:55  ufranke
 * added
 *  + OSS_VxMikroDelayReInit()
 *
 * Revision 1.16  2006/06/02 09:51:43  ufranke
 * added
 * + MSG_Q_ID to OSS_SIG_HANDLE
 * + OSS_VxKernelBridgeInit(), OSS_VXKB_GetMsgQIdScKernel()
 *   OSS_VXKB_msgQOpen(), OSS_VXKB_semB/CCreate(),
 *
 * Revision 1.15  2005/09/08 14:56:52  UFranke
 * added
 *  + OSS_OS_HANDLE used in shareable code
 *    NAND ATA - MM NIOSII and VxWorks
 *
 * Revision 1.14  2005/06/20 11:23:29  SVogel
 * Added function OSS_GetIrqNum0.
 *
 * Revision 1.13  2004/07/27 16:04:43  ufranke
 * added
 *  + export of OSS_MemCheck...() functions
 *
 * Revision 1.12  2004/05/14 09:38:48  UFranke
 * added
 *  + OSS_IrqMaskR()/OSS_IrqRestore()
 *
 * Revision 1.11  2002/02/08 17:54:04  Franke
 * added nested IrqMask/UnMask support and Interface change -> OSS_IRQ_HANDLE!!!
 *       the data elements of the IRQ_HANDLE are now known internaly only
 * added OSS_IrqHdlCreate/OSS_IrqHdlRemove
 *
 * Revision 1.10  2000/05/11 14:45:15  loesel
 * changed prototype of OSS_SetIntNum0 to OSS_SetIrqNum0
 *
 * Revision 1.9  2000/03/16 16:05:16  kp
 * removed fields from OSS_IRQ_HANDLE that are no longer required
 *
 * Revision 1.8  2000/03/09 09:12:38  kp
 * added prototype for OSS_SetIntNum0
 *
 * Revision 1.7  1999/12/08 17:11:15  Franke
 * added function OSS_PciAddrTranslationInit() e.g. for MPC106 Address Map A
 *
 * Revision 1.6  1999/08/30 11:03:11  Franke
 * changed OSS_CALLBACK_HANDLE
 * added   _BIG_ENDIAN_, _LITTLE_ENDIAN_ defines
 *
 * Revision 1.5  1999/05/05 11:11:02  Franke
 * added OSS_CALLBACK_HANDLE, OSS_SHMEM_HANDLE, OSS_CALLBACK_QUEUETYPE
 *
 * Revision 1.4  1998/06/27 14:23:41  Franke
 * changed documentation  OSS_AlarmTaskPriority -  default = 1
 *
 * Revision 1.3  1998/06/23 17:31:47  Franke
 * added	errnoLib.h
 *
 * Revision 1.2  1998/06/17 14:27:02  Franke
 * added    OSS_ALARM_HANDLE
 * OSS_DBG_DEFAULT
 * removed	OSS_DbgWrite macro, logMsg() prototype
 *
 * Revision 1.1  1998/03/10 12:07:58  franke
 * Added by mcvs
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1997..2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
#ifndef _INC_OOS_
#define _INC_OOS_

#include <vxWorks.h>
#include <taskLib.h>
#include <version.h>
#include <semLib.h>
#include <errnoLib.h>
#include <msgQLib.h>
#include <stdio.h>
#include <vxBusLib.h>


#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR >= 6)
    #include <spinLockLib.h>
	#include <stdarg.h>
#endif

#ifdef __cplusplus
   extern "C" {
#endif

#define OSS_HAS_IRQMASKR
#define OSS_IRQ_STATE		int  /* lock out key */

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/
#ifdef _OSS_COMPILE
	typedef struct
	{
    	int32   ownMemSize;
    	u_int32 magic;        /* to check if the new structure  is used */
    	int     vector;       /* vector number (0=none)      */
    	int     level;        /* saved irq level             */
	} OSS_IRQ_HANDLE;
#else
	typedef void *OSS_IRQ_HANDLE;
#endif /*_OSS_COMPILE*/

typedef struct
{
    int32   ownMemSize;
    SEM_ID  semId;
    int     semType;
} OSS_SEM_HANDLE;

typedef struct
{                              /* signal data: */
    int32   ownMemSize;
    u_int32 taskId;            /* process-id   */
    u_int32 sigNbr;            /* signal       */
    MSG_Q_ID msgQueueId;	   /* message queue id */
  #ifdef INCLUDE_MIPIOS_VX_TARGET
    void     *mipiosSigHdl;    /* for MIPIOS target test only */
  #endif /*INCLUDE_MIPIOS_VX_TARGET*/
} OSS_SIG_HANDLE;

typedef struct
{
    int32   ownMemSize;
} OSS_ALARM_HANDLE;

#if (_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR >= 6)
typedef spinlockIsr_t  OSS_SPINL_HANDLE;
#else
typedef void* OSS_SPINL_HANDLE;
#endif

/*------------+
| CALLBACK    |
+------------*/
typedef struct
{
    void 	(*funct)(void *,void *);
    void    *arg0;
    void    *arg1;
}OSS_CB_QUEUE_ENTRY;


typedef struct {				/*--- CALLBACK queue data ---*/
    u_int32 callNr;				/* callback function index (0..n) */
    void 	(*funct)(			/* callback function pointer */
				void *appArg,
				void *drvArg);
    void    *appArg;			/* function argument from application */
    void    *drvArg;			/* function argument from driver */
} OSS_CALLBACK_QUEUEENTRY;

#ifdef COMPILE_OSS_CALLBACK
 typedef struct OSS_CALLBACK_HANDLE OSS_CALLBACK_HANDLE;
#else
 typedef void OSS_CALLBACK_HANDLE;
#endif

/*--- CALLBACK Setstat data ---*/
typedef struct
{
	u_int32	action;				/* action code */
	union
	{
		/*--- OSS_CB_INSTALL ---*/
		struct
		{
			u_int32 callNr;		/* call number */
			void 	(*funct)(	/* callback function pointer */
				void *appArg,
				void *drvArg);
			void    *appArg;	/* function argument from application */
			u_int32 mode;       /* callback mode indirect/direct call */
		} install;
		/*--- OSS_CB_DEINSTALL ---*/
		struct
		{
			u_int32 callNr;		/* call number */
		} deinstall;
	} p;
} OSS_CALLBACK_SETSTAT;

/*--- CALLBACK Getstat data ---*/
typedef struct
{
	u_int32	action;				/* action code */
	union
	{
		/*--- OSS_CB_MODE    ---*/
		u_int32 mode;           /* callback mode indirect/direct call */
	} p;
} OSS_CALLBACK_GETSTAT;

/* action arguments in OSS_CALLBACK_SET/GETSTAT
   UOS/MDIS internal use only */
#define OSS_CB_MODE			0
#define OSS_CB_INSTALL		1
#define OSS_CB_DEINSTALL	2
#define OSS_CB_MASK			3
#define OSS_CB_UNMASK		4

/* callback modes */
#define OSS_CALLBACK_MODE_INDIRECT	1
#define OSS_CALLBACK_MODE_DIRECT	2

/* Macros to determine VxWorks version (from defines in target/h/version.h) */
#define VXW_VERSION_CODE 			((_WRS_VXWORKS_MAJOR << 16 ) + (_WRS_VXWORKS_MINOR << 8 ) + (_WRS_VXWORKS_MAINT)) 
#define VXW_VERSION(x,y,z) 			((x)<<16)+((y)<<8)+((z)) 

/*------------+
| SHMEM       |
+------------*/
typedef struct oss_shmem_tbl {	/*--- SHMEM table entry (per area) ---*/
    u_int32 memAlloc;			/* mem allocated for the entry itself */
	struct oss_shmem_tbl *last; /* ptr to last */
	struct oss_shmem_tbl *next; /* ptr to next */
    u_int32    smNr;			/* shared memory area index (0..n) */
	u_int32    size;			/* shared memory size [bytes] */
	void       *drvAddr;		/* shared memory address (driver) */
    u_int32    linkCnt;			/* nr of links */
} OSS_SHMEM_TBL;

typedef struct {				/*--- SHMEM handle (per device) ---*/
    u_int32 memAlloc;			/* mem allocated for the handle itself */
	OSS_SHMEM_TBL smTbl;		/* installed areas (root of linked list) */
} OSS_SHMEM_HANDLE;


typedef void OSS_HANDLE;

/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
#define OSS_IRQ_EXCLUSIVE   0
#define OSS_IRQ_SHARED      1

#define OSS_VXWORKS_OS_HDL  NULL
#define OSS_OS_HANDLE  		OSS_VXWORKS_OS_HDL
#define OSS_DBG_DEFAULT     0xc0008000 /* set to task and interrupt level at error */
#define OSS_VXWORKS_IRQ_HDL_MAGIC  0xaf12fe34

/* the VxWorks architecture specific define _BYTE_ORDER
   controls the MDIS defines of big/little endian */
#if _BYTE_ORDER == _BIG_ENDIAN
#   define _BIG_ENDIAN_
#else
#   define _LITTLE_ENDIAN_
#endif


/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
extern u_int32 OSS_PciSlot1DevNbr;	   /* default = 16  at a Compact PCI backplane */
extern u_int32 OSS_PciSlot2DevNbr;	   /* default = 15  at a Compact PCI backplane */
extern u_int32 OSS_AlarmTaskPriority;  /* default = 1 */
extern u_int32 OSS_AlarmTaskStackSize; /* default = 0x1000 */
extern MSG_Q_ID OSS_CbMsgQid;
/*extern int32    OSS_IrqNum0;*/       /* default -1 = uninitialized, must be set to a
					                      architecture depend value e.g. 0x20 for PC's */

/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/
#ifdef INCLUDE_MIPIOS_VX_TARGET
	#ifndef OSS_MIPIOS_VX_INTERN
		#define OSS_MIPIOS_VX_SIG	void
	#endif

	extern int32 OSS_MIPIOS_VX_SigCreate
	(
	    OSS_HANDLE     			*osHdl,
	    int32           		value,
	    OSS_MIPIOS_VX_SIG  		**sigHandleP
	);
	extern int32 OSS_MIPIOS_VX_SigRemove
	(
	    OSS_HANDLE     *osHdl,
	    OSS_MIPIOS_VX_SIG **sigHandleP
	);
	extern int32 OSS_MIPIOS_VX_SigSend
	(
	    OSS_HANDLE *osHdl,
	    OSS_MIPIOS_VX_SIG *sigHandle
	);
	extern int32 OSS_MIPIOS_VX_SigCheckInstalled
	(
	    OSS_MIPIOS_VX_SIG  		**sigHandleP,
	    int32           		*sigNoP
	);
	extern int32 OSS_MIPIOS_VX_SigCheckRemoved
	(
	    OSS_MIPIOS_VX_SIG  		**sigHandleP,
	    int32           		*sigNoP
	);
	extern int32 OSS_MIPIOS_VX_SigCheck
	(
	    OSS_MIPIOS_VX_SIG *sigHandle,
		u_int32 *sigNbrP
	);

#endif /*INCLUDE_MIPIOS_VX_TARGET*/

extern int32  OSS_Init( void );
extern int32  OSS_Exit( void );

extern void   OSS_PciAddrTranslationInit( u_int32 pciIoBase, u_int32 pciMemBase );
extern void   OSS_SetIrqNum0( int intNumIrq0 );
extern int    OSS_GetIrqNum0( void );
extern int    OSS_MemCheck( int verbose );
extern void	  OSS_MemCheckTerm( void );

extern int32  OSS_IrqHdlCreate( int32 intNbr, int32 level, OSS_IRQ_HANDLE **irqHdlP );
extern int32  OSS_IrqHdlRemove( OSS_IRQ_HANDLE	**irqHdlP );
extern int32  OSS_IrqHdlGetLevel( OSS_IRQ_HANDLE *irqHdl );
extern int32  OSS_IrqHdlGetVector( OSS_IRQ_HANDLE *irqHdl );

extern int 		OSS_VxKernelBridgeInit( void );
extern MSG_Q_ID OSS_VXKB_GetMsgQIdScKernel( char *msgQName );
extern MSG_Q_ID OSS_VXKB_msgQOpen(	const char * name, int maxMsgs, int maxMsgLength, int options, int mode, void *context );
extern SEM_ID	OSS_VXKB_semBCreate( int options, SEM_B_STATE initialState );
extern SEM_ID	OSS_VXKB_semCCreate( int options, SEM_B_STATE initialState );
extern int32    OSS_VxMikroDelayReInit( OSS_HANDLE *osHdl ); /* call this if mikrodelay was adjusted in sysHwInit2() (disabled caches) */


#ifdef __cplusplus
   }
#endif
#endif /*_INC_OOS_*/











