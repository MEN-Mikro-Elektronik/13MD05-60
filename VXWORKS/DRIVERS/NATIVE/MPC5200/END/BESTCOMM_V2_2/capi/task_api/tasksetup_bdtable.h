#ifndef __TASK_API_TASKSETUP_BDTABLE_H
#define __TASK_API_TASKSETUP_BDTABLE_H 1

/******************************************************************************
*
*       COPYRIGHT (c) 2001-2004 MOTOROLA INC.
*       ALL RIGHTS RESERVED
*
*       The code is the property of Motorola Semiconductor Products Sector.
*
*       The copyright notice above does not evidence any
*       actual or intended publication of such source code.
*
* Filename:     $Source: /Customizations/VxWorks-6.0/mpc5200/target/config/lite5200.pkr/bestComm/capi/task_api/tasksetup_bdtable.h,v $
* Author:       $Author: pkleiner $
* Locker:       $Locker:  $
* State:        $State: Exp $
* Revision:     $Revision: 1.1 $
*
******************************************************************************/

/*
 * Table of BD rings for all BestComm tasks indexed by task ID.
 *
 *    +-----+------+--------------+    +------+-------+
 * 0: |numBD|numPtr|BDTablePtr ---|--->|status|dataPtr|
 *    +-----+------+--------------+    +------+-------+
 * 1: |numBD|numPtr|BDTablePtr    |    |status|dataPtr|
 *    +-----+------+--------------+    .      .       .
 * 2: |numBD|numPtr|BDTablePtr ---|-+  .      .       .
 *    .            .              . |  .      .       .
 *    .            .              . |  |status|dataPtr|
 *    .            .              . |  +------+-------+
 * 15:|numBD|numPtr|BDTablePtr    | |
 *    +-----+------+--------------+ |
 *                                  |
 *                                  V
 *                                  +------+--------+--------+
 *                                  |status|dataPtr0|dataPtr1|
 *                                  +------+--------+--------+
 *                                  |status|dataPtr0|dataPtr1|
 *                                  .      .        .        .
 *                                  .      .        .        .
 *                                  .      .        .        .
 *                                  |status|dataPtr0|dataPtr1|
 *                                  +------+--------+--------+
 */
typedef struct {
	uint16 numBD;		/* Size of BD ring									*/
	uint8  numPtr;		/* Number of data buffer pointers per BD			*/
	uint8  apiConfig;	/* API configuration flags							*/
	void   *BDTablePtr;	/* Pointer to BD tables, must be cast to TaskBD1_t	*/
						/*   or TaskBD2_t									*/
	volatile uint32
		   *BDStartPtr;	/* Task's current BD pointer. This pointer is
						 * used to set a task's BD pointer upon startup.
						 * It is only valid for BD tasks and only after
						 * TaskSetup() or TaskBDReset() are called. You
						 * cannot use this to track a task's BD pointer.
						 */
	uint16 currBDInUse; /* Current number of buffer descriptors assigned but*/
						/*   not released yet.                              */					
} TaskBDIdxTable_t;

typedef enum {
	API_CONFIG_NONE		= 0x00,
	API_CONFIG_BD_FLAG	= 0x01
} ApiConfig_t;

/*
 * Allocates BD table if needed and updates the BD index table.
 * Do we want to hide this from the C API since it operates on task API?
 */
void TaskSetup_BDTable(volatile uint32 *BasePtr, 
                       volatile uint32 *LastPtr,
                       volatile uint32 *StartPtr,
                       int TaskNum, uint32 NumBD, uint16 MaxBD,
                       uint8 NumPtr, ApiConfig_t ApiConfig, uint32 Status );

#endif	/* __TASK_API_TASKSETUP_BDTABLE_H */
