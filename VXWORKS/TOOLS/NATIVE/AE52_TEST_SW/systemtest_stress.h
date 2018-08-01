
#ifndef _SYSTEMTEST_STRESS_H
#define _SYSTEMTEST_STRESS_H



#define STRESS_WORD_SWAP(v)  		(((v << 8) & 0xff00) | ((v >> 8) & 0x00ff))
#define STRESS_LONG_SWAP(dword) 	((dword>>24) | (dword<<24) | \
             						((dword>>8) & 0x0000ff00) | \
									((dword<<8) & 0x00ff0000))
#define STRESS_DBG_LINE_BUFSIZE		100


#define STRESS_WIDTH_DEBUG		45
#define STRESS_HIGHT_DEBUG		11
#define STRESS_SIZE_DEBUG		(STRESS_WIDTH_DEBUG*STRESS_HIGHT_DEBUG)

/** Time structure for check_time routine */
typedef struct {
	u_int8 year;
	u_int8 mon;
	u_int8 day;
	u_int8 hour;
	u_int8 min;
	u_int8 sec;
} TIME_STRUCT;

/** Debug Handle structure */
typedef struct {
	int line;			/**< current line */
	int pos_x;			/**< x-Position of Debug Window */
	int pos_y;			/**< y-position of Debug Window */
	char dataP[STRESS_HIGHT_DEBUG][STRESS_WIDTH_DEBUG];
	char buf[STRESS_DBG_LINE_BUFSIZE];		/**< line buffer */
} STRESS_DBG_HDL;

/** Test Handle structure. */
typedef struct {
	char 		name[41];		/**< name of test */
	u_int8 		id;				/**< TestId */
	u_int8      enabled;		/**< flag test is enabled */
	u_int32		taskId;			/**< task Id of test */
	VOIDFUNCPTR	*routine;		/**< task routine of test */
	void(*init)(void*, u_int32);	/**< init routine of test */
	int			load_nom;			/**< nominal load of test */
	int			load_act;			/**< actual load of test */
	int 		prio;
	u_int32     singleRunTime;	/**< time to run single test (for TOs) */
	int			cpu;			/* < running on CPU# (only SMP) */
	u_int8     	type;			/* < type of test (rate,IO...) */
	u_int32		bytenum;
	u_int32		bytenumSave;
	u_int8      stuck;
	u_int32     startTick;
	u_int32		tickSave;
	u_int32		allExeTime;
	u_int32     loopCnt;		/**< number of loops */
	u_int32     errCnt;			/**< number of errors occurred */
	u_int32     errCntSave;		/**< old number of errors occurred */
	u_int8		verbose;		/**< verbose(debug) level 0..3 */
	u_int8      halt;			/**< lock flag */
	u_int8      focus;
	u_int8      alive;			/**< alive? */
	void        *cmdP;			/**< pointer to command data */
	u_int32     dsize;			/**< size of data buffer */
	u_int8      *dataP;			/**< pointer to data */
	u_int32     dataIdx;		/**< number of bytes in buffer */
	u_int8      isFinished;		/**< flag is finished */	
	STRESS_DBG_HDL *dbgP;		/**< pointer to debug handle */
	
} STRESS_TEST_HDL;

/** PIC Handle structure. */
typedef struct {
	u_int16 magic;		/**< magic word */
#if 0
	char name[41];		/**< name of PIC handle */
	u_int8 id;			/**< Id of PIC Hdl */
	int fd;				/**< file descriptor of opened device */
	u_int8 cmd;			/**< current command */
	u_int8 cmdCnt;		/**< number of sent commands */
	u_int32 dataCnt;	/**< number of received data */
	u_int32 errCnt;		/**< number of timeout errors */
	u_int32 errCntSave;	/**< old number of timeout errors */
	u_int8 reqCmd;		/**< external requested command */
	u_int32 timer;		/**< start timer of current command */
	u_int32 timeout;	/**< value of timeout in ticks */
	u_int32 delay;		/**< time between to send events */
	u_int8 verbose;		/**< verbose(debug) level 0..3 */
	u_int8 *cmdTblP;	/**< pointer to command table */
	int32 taskIdRx;		/**< taskId of Rx Task */
	int32 taskIdTx;		/**< taskId of Tx Task */
	u_int8 halt;		/**< flag to halt tasks */
	u_int8 stop;		/**< flag to end tasks */
	u_int8 lock;		/**< flag to lock handle */
	u_int8 data[0x200];	/**< data area for PIC data */
	STRESS_DBG_HDL *dbgP;	/**< pointer to debug handle */
#endif
} STRESS_PIC_HDL;


/** Logging Handle structure */
typedef struct {
	u_int8 initDone;	/**< marker handle is init */
	char fileName[80];	/**< Logging file name */
	u_int32 time;		/**< Logging time interval in [ms] */
	TIME_STRUCT curr_time; /**< current system time (must be updated before use) */
	TIME_STRUCT start_time; /**< test start time */
#if 0	
	u_int8 year;		/**< current year */
	u_int8 month;		/**< current month */
	u_int8 day;			/**< current day */
	u_int8 hour;		/**< current hour */
	u_int8 min;			/**< current minute */
	u_int8 sec;			/**< current second */
#endif
	int8 temp1;			/**< current CPU temperature */
	int8 temp2; 		/**< current carrier temperature */
	int8 temp3;			/**< current FPGA temperature */
	u_int8 load;		/**< current system load */
	u_int8 verbose;		/**< verbose(debug) level 0..3 */
	u_int8 netLog;		/**< flag to log on network drive */
	u_int32 serial;		/**< serial number of board */
	u_int8 halt;		/**< flag to halt tasks */
	u_int8 stop;		/**< flag to end tasks */
	u_int32 errCycle;	/**< flag to initiate a error caused logging */
	STRESS_DBG_HDL *dbgP; /**< pointer to debug handle */
} STRESS_LOG_HDL;

#if 0 /* REMOVE */
typedef struct {
	u_int8  status1;
	u_int8  status2;
	u_int16 capacity;
	u_int8  version;
	u_int8  info;
	u_int8  daycounter;
	u_int16 remaining;
	u_int8  commstat;
	u_int8  fmstat;
	u_int8  fmdata;
}SC20_ACCU_INFO_STRUCT;
#endif

#define STRESS_TESTID_GPIO				1
#define STRESS_TESTID_NET				2
#define STRESS_TESTID_SYSMON			3
#define STRESS_TESTID_LRU58				4
#define STRESS_TESTID_SDRAM				5
#define STRESS_TESTID_SDRAM2			6
#define STRESS_TESTID_SCL				7
#define STRESS_TESTID_API				8
#define STRESS_TESTID_MIL				9
#define STRESS_TESTID_LOAD7				10
#define STRESS_TESTID_LOAD8				11
#define STRESS_TESTID_SATA				12
#define STRESS_TESTID_EEP				13
#define STRESS_TESTID_RS232				14
#define STRESS_TESTID_ETH               15
#define STRESS_TESTID_DUMMY				20
#define STRESS_TESTID_ENDTBL			255

#define STRESS_TEST_RUN_ONESHOOT		1
#define STRESS_TEST_RUN_ENDLESS			0
#define STRESS_TEST_RUN_2				3

#define STRESS_BUCKET_SIZE			50

#define STRESS_TICK_TOLERANCE		2

#define STRESS_TASK_PRIORITY_LOWEST		255
#define STRESS_TASK_PRIORITY_LOW		180
#define STRESS_TASK_PRIORITY_MEDIUM		100
#define STRESS_TASK_PRIORITY_HIGH		60
#define STRESS_TASK_PRIORITY_HIGHEST	1

#define STRESS_LOGGING_DEFAULT		60000	/**< 60s, default logging interval */
#define STRESS_ERR_LOG_DEFAULT		5000	/**< 5s, logging interval in case of error */

#define STRESS_DBGLEV_OFF			0x00
#define STRESS_DBGLEV_1				0x01 /* basic info an errors */
#define STRESS_DBGLEV_2				0x02 /* high level functions */
#define STRESS_DBGLEV_3				0x04 /* low level functions */
#define STRESS_DBGLEV_4				0x08 /* stress internals */

#define SECONDS_PER_HOUR     		3600

#define STRESS_LOGGING_FILE_NFS	"/nfs/XM51_stress_"
#define STRESS_LOGGING_FILE_NET	"XM51_stress_"


float calcVoltagPic( u_int8 rawVal, u_int8 voltid );
u_int8 getSystemLoad( void );
void stressTimeUpdate(TIME_STRUCT * sP );
u_int32 stressGetIdle(void);
u_int32 stressGetInt(void);
void stressLoadUpdate( u_int8 * load, STRESS_TEST_HDL *h );
u_int32 getSecondsFromTime( TIME_STRUCT * tP);
void stressDbg(STRESS_TEST_HDL *, u_int8 lvl, const char * fmt, ...);
int stressSpyInit(void);




#endif
