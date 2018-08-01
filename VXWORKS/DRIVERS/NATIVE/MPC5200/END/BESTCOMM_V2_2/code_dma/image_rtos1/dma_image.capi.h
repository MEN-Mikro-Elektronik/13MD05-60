#ifndef __DMA_IMAGE_CAPI_H
#define __DMA_IMAGE_CAPI_H 1

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
******************************************************************************/


#include "dma_image.h"

typedef enum {
	TASK_PCI_TX,
	TASK_PCI_RX,
	TASK_FEC_TX,
	TASK_FEC_RX,
	TASK_LPC,
	TASK_ATA,
	TASK_CRC16_DP_0,
	TASK_CRC16_DP_1,
	TASK_GEN_DP_0,
	TASK_GEN_DP_1,
	TASK_GEN_DP_2,
	TASK_GEN_DP_3,
	TASK_GEN_TX_BD,
	TASK_GEN_RX_BD,
	TASK_GEN_DP_BD_0,
	TASK_GEN_DP_BD_1
} TaskName_t;

TaskId TaskSetup_TASK_PCI_TX    (TASK_PCI_TX_api_t    *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_PCI_RX    (TASK_PCI_RX_api_t    *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_FEC_TX    (TASK_FEC_TX_api_t    *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_FEC_RX    (TASK_FEC_RX_api_t    *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_LPC       (TASK_LPC_api_t       *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_ATA       (TASK_ATA_api_t       *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_CRC16_DP_0(TASK_CRC16_DP_0_api_t *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_CRC16_DP_1(TASK_CRC16_DP_1_api_t *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_GEN_DP_0  (TASK_GEN_DP_0_api_t  *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_GEN_DP_1  (TASK_GEN_DP_1_api_t  *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_GEN_DP_2  (TASK_GEN_DP_2_api_t  *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_GEN_DP_3  (TASK_GEN_DP_3_api_t  *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_GEN_TX_BD (TASK_GEN_TX_BD_api_t *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_GEN_RX_BD (TASK_GEN_RX_BD_api_t *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_GEN_DP_BD_0(TASK_GEN_DP_BD_0_api_t *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);
TaskId TaskSetup_TASK_GEN_DP_BD_1(TASK_GEN_DP_BD_1_api_t *TaskAPI,
                                 TaskSetupParamSet_t  *TaskSetupParams);

#endif	/* __DMA_IMAGE_CAPI_H */

