/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  mdis_rtp.h
 *
 *      \author  uf
 *        $Date: 2006/10/18 13:36:18 $
 *    $Revision: 2.3 $
 *
 *  	 \brief  Header file for user mode ( VxWorks 6.x RTPs ) MDIS
 *
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: mdis_rtp.h,v $
 * Revision 2.3  2006/10/18 13:36:18  cs
 * fixed:
 *   - use WindRiver defines to compute Syscall IDs
 *
 * Revision 2.2  2006/06/02 09:50:15  ufranke
 * added
 *  + MDIS_RTP_SYSCALL_VX_OSS_SIG_MSGQ_INSTALL/REMOVE
 *
 * Revision 2.1  2005/12/23 11:25:20  UFRANKE
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005-2006 by MEN Mikro Elektronik GmbH, Nueremberg, Germany
 ****************************************************************************/

#ifndef _MDIS_RTP_H
#define _MDIS_RTP_H

#ifdef __cplusplus
	extern "C" {
#endif

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define MDIS_RTP_SYSCALL_GROUP 6									/**< mdis system call group - default 6 */
#define MDIS_RTP_SYSCALL_GROUP_NAME	"MDIS_Group"					/**< mdis system call group name */

#define MDIS_RTP_SYSCALL_NUM(rtn) (SYSCALL_NUMBER(MDIS_RTP_SYSCALL_GROUP,rtn))

#define MDIS_RTP_SYSCALL_M_OPEN		MDIS_RTP_SYSCALL_NUM(0)	/**< function 0 */
#define MDIS_RTP_SYSCALL_M_CLOSE	MDIS_RTP_SYSCALL_NUM(1)	/**< function 1 */
#define MDIS_RTP_SYSCALL_M_READ		MDIS_RTP_SYSCALL_NUM(2)	/**< function 2 */
#define MDIS_RTP_SYSCALL_M_WRITE	MDIS_RTP_SYSCALL_NUM(3)	/**< function 3 */
#define MDIS_RTP_SYSCALL_M_GETSTAT	MDIS_RTP_SYSCALL_NUM(4)	/**< function 4 */
#define MDIS_RTP_SYSCALL_M_SETSTAT	MDIS_RTP_SYSCALL_NUM(5)	/**< function 5 */
#define MDIS_RTP_SYSCALL_M_GETBLOCK	MDIS_RTP_SYSCALL_NUM(6)	/**< function 6 */
#define MDIS_RTP_SYSCALL_M_SETBLOCK	MDIS_RTP_SYSCALL_NUM(7)	/**< function 7 */

#define MDIS_RTP_SYSCALL_VX_OSS_SIG_MSGQ_INSTALL	MDIS_RTP_SYSCALL_NUM(10)	/**< function 10 */
#define MDIS_RTP_SYSCALL_VX_OSS_SIG_MSGQ_REMOVE		MDIS_RTP_SYSCALL_NUM(11)	/**< function 11 */


/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
extern const char *MAPI_SC_RCSid;

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
extern int MAPI_SC_InstallMdisSysCall( void ); /* called from OS2M_DrvInstall in kernel mode */


#ifdef __cplusplus
	}
#endif

#endif	/* _MDIS_RTP_H */


