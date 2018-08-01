/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  sysp_sysdep.h
 *
 *      \author  klaus.popp@men.de
 *        $Date: 2008/06/27 14:07:49 $
 *    $Revision: 1.15 $
 *
 *  	 \brief  SYSPARAM system dependent includes/defines
 *
 *    \switches
 *
 * - \c SYSP_CONFIG_MDIS_BUILD - Set this when compiling within MDIS build
 *      environment.
 *
 * - \c DBG - Compile with debug messages enabled (very few are there)
 * - \c SYSP_CONFIG_UNITTEST - include unittests
 *
 * If SYSP_CONFIG_MDIS_BUILD is not set, the following switches can be set:
 * - \c SYSP_CONFIG_LINUX, \c SYSP_CONFIG_MENMON, \c SYSP_CONFIG_VXWORKS,
 *   \c SYSP_CONFIG_OS9 - the operating system used.
 * - \c SYSP_CONFIG_LE, \c SYSP_CONFIG_BE little or big endian
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: sysp_sysdep.h,v $
 * Revision 1.15  2008/06/27 14:07:49  aw
 * R: kernel 2.6.19 does not use linux/config.h
 * M: use for kernel 2.6.19 linux/autoconf.h
 *
 * Revision 1.14  2006/01/20 13:00:59  UFRANKE
 * removed
 *  - define SUPPORT_EEPROD2_ON_ESM_CB
 *
 * Revision 1.13  2006/01/17 13:32:39  cs
 * esm_cb_eeprom.h now included for all OS
 *
 * Revision 1.12  2006/01/05 14:01:16  cs
 * added
 *     + define SUPPORT_EEPROD2_ON_ESM_CB
 *     + include of esm_cb_eeprom.h
 *
 * Revision 1.11  2005/06/23 15:54:34  kp
 * Copyright string changed (sbo)
 *
 * Revision 1.10  2005/05/30 12:55:11  kp
 * +SYSP_STRTOL for linux
 *
 * Revision 1.9  2005/03/03 17:28:58  CSchuster
 * added includes string.h and stdlib.h for VxWorks
 *
 * Revision 1.8  2005/02/22 15:01:07  kp
 * + OS9
 *
 * Revision 1.7  2005/02/15 12:14:42  ufranke
 * added
 *  + VxWorks
 *
 * Revision 1.6  2005/01/12 14:19:59  kp
 * intermediate
 *
 * Revision 1.5  2004/11/29 10:53:08  kp
 * MM unittest support
 *
 * Revision 1.4  2004/10/06 17:15:40  kp
 * don't include main_globs for hurricane menmon
 *
 * Revision 1.3  2004/09/03 15:20:50  kp
 * added SYSP_STRTOL
 *
 * Revision 1.2  2004/01/26 14:04:44  kp
 * added Linux kernel mode switches
 *
 * Revision 1.1  2004/01/08 09:06:58  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003-2004 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef _SYSP_SYSDEP_H
#define _SYSP_SYSDEP_H

/*
 * When building with MDIS build env, set internal switches according
 * to MDIS build switches
 */
#ifdef SYSP_CONFIG_MDIS_BUILD

# ifdef LINUX
#  define SYSP_CONFIG_LINUX
# endif

# if defined(OS9) || defined(OS9000)
#  define SYSP_CONFIG_OS9
# endif

# if defined(VXWORKS)
#  define SYSP_CONFIG_VXWORKS
# endif

# if defined _BIG_ENDIAN_
#  define SYSP_CONFIG_BE
# endif

# if defined _LITTLE_ENDIAN_
#  define SYSP_CONFIG_LE
# endif

#endif /* SYSP_CONFIG_MDIS_BUILD */

#include <MEN/men_typs.h>
#include <MEN/sysparam2.h>
#include <MEN/esm_cb_eeprom.h>

/* set default macros. can be renamed below */
#define SYSP_STRCPY 	strcpy
#define SYSP_STRNCPY 	strncpy
#define SYSP_STRCMP 	strcmp
#define SYSP_STRNCMP 	strncmp
#define SYSP_STRLEN 	strlen
#define SYSP_MEMSET 	memset
#define SYSP_MEMCMP		memcmp
#define SYSP_MEMMOVE	memmove
#define SYSP_SNPRINTF 	snprintf
#define SYSP_VSPRINTF 	vsprintf
#define SYSP_STRTOUL 	strtoul
#define SYSP_STRTOL 	strtol
#define SYSP_STRCHR		strchr

/*------------------+
|  Linux user land  |
+------------------*/
#if defined(SYSP_CONFIG_LINUX) && !defined(__KERNEL__)

# include <stdio.h>
# include <stddef.h>
# include <string.h>
# include <stdlib.h>
# include <stdarg.h>

# define DBG_WRITE_DEFINED_BY_DBG_OS_H
# define DBG_Write(dbh,fmt,args...) printf( fmt, ## args )

# include <MEN/dbg.h>
#endif /* LINUX && !_KERNEL__ */

/*------------------+
| Linux kernel mode |
+------------------*/
#if defined(SYSP_CONFIG_LINUX) && defined(__KERNEL__)

# include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
# include <linux/config.h>
#else
# include <linux/autoconf.h>
#endif
# include <linux/module.h>

# include <linux/kernel.h>
# include <linux/string.h>
# include <asm/byteorder.h>
# include <MEN/dbg.h>

# undef  SYSP_STRTOUL
# define SYSP_STRTOUL 	simple_strtoul
# undef  SYSP_STRTOL
# define SYSP_STRTOL 	simple_strtol

#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN)
#if __BYTE_ORDER == __BIG_ENDIAN || \
    (defined(__BIG_ENDIAN) && !defined(__LITTLE_ENDIAN))
#  define SYSP_CONFIG_BE
# else
#  define SYSP_CONFIG_LE
# endif
#endif /* defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) */

#endif /* LINUX && !_KERNEL__ */

/*------------------+
| VxWorks           |
+------------------*/
#if defined(SYSP_CONFIG_VXWORKS)

# include <MEN/dbg.h>
# include <stdio.h>
# include <string.h>
# include <stdlib.h>

#endif /*SYSP_CONFIG_VXWORKS*/


/*------------------+
|  MENMON           |
+------------------*/
#if defined(SYSP_CONFIG_MENMON)

# include <mmglob.h>
# ifndef HURRICANE
#  include <main_globs.h>
# endif
# define offsetof( type, part )	((int)((char*)&((type*)0)->part-(char*)0))

# define DBG_WRITE_DEFINED_BY_DBG_OS_H
# define DBG_Write(dbh,fmt,args...) mm_usr_printf( fmt, ## args )

# include <MEN/dbg.h>

# if defined(MM_DRY_UNITTEST_ENABLED)
#  define SYSP_CONFIG_UNITTEST
#  define SYSP_UNITTEST(_name_,_func_) MM_UNITTEST(_name_,_func_)
#  define UTASSERT MM_UTASSERT
# endif

#endif /* SYSP_CONFIG_MENMON */

/*------------------+
|  OS9              |
+------------------*/
#if defined(SYSP_CONFIG_OS9)

# include <stdarg.h>
# include <stddef.h>
# include <MEN/dbg.h>
# include <MEN/drvsupp.h>


#endif /* SYSP_CONFIG_OS9 */




/* endian issues */
#if defined(SYSP_CONFIG_LE) && defined(SYSP_CONFIG_BE)
# error "Don't define SYSP_CONFIG_LE and SYSP_CONFIG_BE together"
#elif !defined(SYSP_CONFIG_LE) && !defined(SYSP_CONFIG_BE)
# error "Please define either SYSP_CONFIG_LE or SYSP_CONFIG_BE"
#endif

#ifdef SYSP_CONFIG_LE
# define SYSP_TOBE16(word)	( (((word)>>8) & 0xff) | (((word)<<8)&0xff00) )

# define SYSP_TOBE32(dword)	( ((dword)>>24) | ((dword)<<24) | \
							  (((dword)>>8) & 0x0000ff00)   | \
							  (((dword)<<8) & 0x00ff0000)     )
#else
# define SYSP_TOBE16(word) (word)
# define SYSP_TOBE32(dword) (dword)
#endif /* SYSP_CONFIG_LE */

#define SYSP_FROMBE16 SYSP_TOBE16
#define SYSP_FROMBE32 SYSP_TOBE32



#endif /* _SYSP_SYSDEP_H */



