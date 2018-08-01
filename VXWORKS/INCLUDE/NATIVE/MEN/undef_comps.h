/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: undef_comps.h
 *
 *       Author: thomas.schnuerer@men.de
 *       
 *       Description: VxWorks kernel configuration Items to be undefined for later
 *                    setting in a cdf file
 *
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2016 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
****************************************************************************/

#ifndef _UNDEF_COMPS_H_
#define _UNDEF_COMPS_H_

/* #undef parameters here which we want to be set by a CDF file later.
 * if these remain defined the setting in config.h always wins over any CDF 
 * file definition. See CDF developer guide! 
 */
#undef WDB_GOPHER_TAPE_LEN
#undef RTP_FD_NUM_MAX
#undef NUM_FILES
#undef WIND_JOBS_MAX
#undef DSI_NUM_SOCKETS
#undef SHELL_TASK_PRIORITY
#undef XBD_BLK_DEV_TASK_PRIORITY

#endif /* _UNDEF_COMPS_H_ */
