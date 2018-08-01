

/*
 * Generated by GUI, one per configured tasks.
 */

#define TASK_BASE         TASK_FEC_TX
#define TASK_API          TASK_FEC_TX_api_t
#define TASKSETUP_NAME    TaskSetup_TASK_FEC_TX

/* Pragma settings */
#define PRECISE_INC       0
#define NO_ERROR_RST      0
#define PACK_DATA         0
#define INTEGER_MODE      0
#define SPEC_READS        1
#define WRITE_LINE_BUFFER 1
#define READ_LINE_BUFFER  1

#define MISALIGNED        1
#define MISALIGNED_START  1

#define INITIATOR_DATA    INITIATOR_FEC_TX

#define AUTO_START       -1
#define ITERATIONS       -1

#define MAX_BD          256
#define BD_FLAG           1

#define INCR_TYPE_SRC     1
#define INCR_SRC          0
#define TYPE_SRC          FLEX_T

#define INCR_TYPE_DST     0
#define INCR_DST          0
#define TYPE_DST          FLEX_T

extern int abs( int i );

#include "task_api/tasksetup_general.h"
