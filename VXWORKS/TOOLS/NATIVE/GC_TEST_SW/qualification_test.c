/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  qualification_test.c
 *
 *      \author  aw
 *        $Date: 2009/03/06 16:59:21 $
 *    $Revision: 1.1 $
 * 
 *  	 \brief  Can test tool for project critical software. F50 has to be 
 *               connected to F50 testboard. 
 *
 *               The test runs parallel in 5 tasks. The tasks communcates 
 *               with the main task over message queues. The task 1 tests 
 *               the 6 CANs. One CAN sends 4 frames and the other 5 CANs 
 *               receives the 4 frames. At the next state the CAN 2 send...
 *               Task2 makes memory accesses to F503 SDRAM, XM50 SDRAM, AD78
 *               via I2C and F503 EEPROM via I2C. Task 3 performs a hdtest
 *               on the solid state disc. Task 4 performs loopframes tests on
 *               Ethernet port1, port2, port3 and on COM2. Task 5 makes several
 *               tests on IDSel, ModeSel and CutOff GPIOs.
 *               The main task starts the 5 test tasks and waits until all 
 *               tasks are finished except task 3. The main task cannot wait
 *               for task 3, because task 3 needs several seconds. One pass
 *               should take between 3 and 9 seconds. After every pass the main
 *               task prints the result. 
 *
 *     Switches: -
 *     Required: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: qualification_test.c,v $
 * Revision 1.1  2009/03/06 16:59:21  AWanka
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <msgQLib.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>
#include <MEN/usr_err.h>


/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define TASK_CAN_FLAG   0x01
#define TASK_MEM_FLAG   0x02
#define TASK_SSD_FLAG   0x04
#define TASK_ETH_FLAG   0x08
#define TASK_GPIO_FLAG	0x10

#define NBR_OF_TASKS	5
#define TASK_CAN		0
#define TASK_MEM		1
#define TASK_SSD		2
#define TASK_ETH		3
#define TASK_GPIO		4

#define MAX_MSGS		1
#define MAX_MSG_LEN     sizeof(ERROR_HDL)

#define MAX_NBR_TESTS_PER_RUN 11

#define STACK_SIZE		32000	

/*--------------------------------------+
|   TYPEDEFS                            |
+--------------------------------------*/
typedef struct errorHandle{
	u_int32 tErrorsPerRun[MAX_NBR_TESTS_PER_RUN];
	u_int32 tTotalErrors;
}ERROR_HDL;

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
MSG_Q_ID msgQIdT[NBR_OF_TASKS];

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
int taskCan( MSG_Q_ID * );
int taskMemory( MSG_Q_ID * );
int taskSsd( MSG_Q_ID * );
int taskEthernetCom2( MSG_Q_ID * );
int taskGpio( MSG_Q_ID * );

int taskCan( MSG_Q_ID *msgQIdTP )
{
	int errorCnt = 0;
	u_int8 n = 0;
	ERROR_HDL errHdlTCan;

	if (msgQReceive(*msgQIdTP, (char *)&errHdlTCan, MAX_MSG_LEN, 
               NO_WAIT) == ERROR){
        return (ERROR);
	}

	for(n=0; n<6; n++){
		errHdlTCan.tErrorsPerRun[n] = 0;
	}
	taskDelay(500);

	if((errHdlTCan.tErrorsPerRun[0] = GC_CanTestA_07320(1, 500, 1, 1)) != OK){
		errorCnt++;
	}
	
	if((errHdlTCan.tErrorsPerRun[1] = GC_CanTestA_07320(2, 500, 1, 1)) != OK){
		errorCnt++;
	}
	
	if((errHdlTCan.tErrorsPerRun[2] = GC_CanTestA_07320(3, 500, 1, 1))!= OK){
		errorCnt++;
	}
	
	if((errHdlTCan.tErrorsPerRun[3] = GC_CanTestA_07320(4, 500, 1, 1)) != OK){
		errorCnt++;
	}
	
	if((errHdlTCan.tErrorsPerRun[4] = GC_CanTestA_07320(5, 500, 1, 1)) != OK){
		errorCnt++;
	}
	
	if((errHdlTCan.tErrorsPerRun[5] = GC_CanTestA_07320(6, 500, 1, 1))!= OK){
		errorCnt++;
	}
	
	if(errorCnt != 0){
		errHdlTCan.tTotalErrors += errorCnt;
	}

	if (msgQSend (*msgQIdTP, (char *)&errHdlTCan, MAX_MSG_LEN, NO_WAIT,
                  MSG_PRI_NORMAL) == ERROR){
        return (ERROR);
    }

	return OK;
}

int taskMemory( MSG_Q_ID *msgQIdTP )
{
	int errorCnt = 0;
	u_int8 n = 0;
	ERROR_HDL errHdlTMemory;

	if (msgQReceive(*msgQIdTP, (char *)&errHdlTMemory, MAX_MSG_LEN, 
                WAIT_FOREVER) == ERROR){
        return (ERROR);
	}
	
	for(n=0; n<4; n++){
		errHdlTMemory.tErrorsPerRun[n] = 0;
	}

	/* test 4 Bytes at DDR2 from F503 */
	if(GC_SdramTest( 1, 1, 1 ) != OK){
		errorCnt++;
		errHdlTMemory.tErrorsPerRun[0]++;
	}

	/* test 4 Bytes at SDRAM from XM50 */
	if(GC_SdramXM50Test( 1, 1 ) != OK){
		errorCnt++;
		errHdlTMemory.tErrorsPerRun[1]++;
	}


	/* perform memory test on AD78 */
	if(GC_I2cTest(1, 1, 1, 0) != OK){
		errorCnt++;
		errHdlTMemory.tErrorsPerRun[2]++;
	}

	/* perform memory test on EEPROM */
	if(GC_I2cTest(0, 1, 1, 1) != OK){
		errorCnt++;
		errHdlTMemory.tErrorsPerRun[3]++;
	}

	if(errorCnt != 0){
		errHdlTMemory.tTotalErrors += errorCnt;
	}
	
	if (msgQSend (*msgQIdTP, (char *)&errHdlTMemory, MAX_MSG_LEN, WAIT_FOREVER,
                  MSG_PRI_NORMAL) == ERROR){
        return (ERROR);
    }

	return OK;
}

int taskSsd( MSG_Q_ID *msgQIdTP )
{
	int errorCnt = 0;
	ERROR_HDL errHdlTSsd;

	if (msgQReceive(*msgQIdTP, (char *)&errHdlTSsd, MAX_MSG_LEN, 
             WAIT_FOREVER) == ERROR){
        return (ERROR);
	}

	errHdlTSsd.tErrorsPerRun[0] = 0;

	/* write and verify a file of 100MB on Solid State Disc */
	if(GC_SsdTest( 1, 1 ) != OK){
		errorCnt++;
		errHdlTSsd.tErrorsPerRun[0]++;
	}

	if(errorCnt != 0){
		errHdlTSsd.tTotalErrors += errorCnt;
	}

	if (msgQSend (*msgQIdTP, (char *)&errHdlTSsd, MAX_MSG_LEN, WAIT_FOREVER,
                  MSG_PRI_NORMAL) == ERROR){
        return (ERROR);
    }

	return OK;
}

int taskEthernetCom2( MSG_Q_ID *msgQIdTP )
{
	int errorCnt = 0;
	u_int8 n = 0;
	ERROR_HDL errHdlTEthCom2;

	if (msgQReceive(*msgQIdTP, (char *)&errHdlTEthCom2, MAX_MSG_LEN, 
             WAIT_FOREVER) == ERROR){
        return (ERROR);
	}

	for(n=0; n<4; n++){
		errHdlTEthCom2.tErrorsPerRun[n] = 0;
	}

	
	if(GC_EthTest( 1, 1, 2 ) != OK){
		errorCnt++;
		errHdlTEthCom2.tErrorsPerRun[0]++;
	}

	if(GC_EthTest( 2, 1, 2 ) != OK){
		errorCnt++;
		errHdlTEthCom2.tErrorsPerRun[1]++;
	}
	if(GC_EthTest( 3, 1, 2 ) != OK){
		errorCnt++;
		errHdlTEthCom2.tErrorsPerRun[2]++;
	}

	if( GC_Com2Test(1,9600) != OK){
		errorCnt++;
		errHdlTEthCom2.tErrorsPerRun[3]++;
	}

	if(errorCnt != 0){
		errHdlTEthCom2.tTotalErrors += errorCnt;
	}
	
	if (msgQSend (*msgQIdTP, (char *)&errHdlTEthCom2, MAX_MSG_LEN, 
              WAIT_FOREVER, MSG_PRI_NORMAL) == ERROR){
        return (ERROR);
    }

	return OK;
}

int taskGpio( MSG_Q_ID *msgQIdTP )
{
	int errorCnt = 0;
	u_int8 n = 0;
	ERROR_HDL errHdlTGpio;

	if (msgQReceive(*msgQIdTP, (char *)&errHdlTGpio, MAX_MSG_LEN, 
               WAIT_FOREVER) == ERROR){
        return (ERROR);
	}


	for(n=0; n<11; n++){
		errHdlTGpio.tErrorsPerRun[n] = 0;
	}

	taskDelay(500);

	/* state 1 */
	if(GC_IdSel() != 0){
		errorCnt++;
		errHdlTGpio.tErrorsPerRun[0]++;
	}
	if(GC_ModeSel() != 0){
		errorCnt++;
		errHdlTGpio.tErrorsPerRun[1]++;
	}
	
	/* state 2 */
	if(GC_CutOffOut(2,0) != OK){
		errorCnt++;
		errHdlTGpio.tErrorsPerRun[2]++;
	}
	if(GC_CutOffOut(1,1) != OK){
		errorCnt++;
		errHdlTGpio.tErrorsPerRun[3]++;
	}
	
	taskDelay(500);

	/* state 3 */
	if(GC_IdSel() != 1){
		errorCnt++;
		errHdlTGpio.tErrorsPerRun[4]++;
	}
	if(GC_ModeSel() != 1){
		errorCnt++;
		errHdlTGpio.tErrorsPerRun[5]++;
	}

	/* state 4 */
	if(GC_CutOffOut(3,0) != OK){
		errorCnt++;
		errHdlTGpio.tErrorsPerRun[6]++;
	}
	if(GC_CutOffOut(2,1) != OK){
		errorCnt++;
		errHdlTGpio.tErrorsPerRun[7]++;
	}
	if(GC_CutOffOut(1,0) != OK){
		errorCnt++;
		errHdlTGpio.tErrorsPerRun[8]++;
	}

	taskDelay(500);

	/* state 5 */
	if(GC_IdSel() != 2){
		errorCnt++;
		errHdlTGpio.tErrorsPerRun[9]++;
	}
	if(GC_ModeSel() != 2){
		errorCnt++;
		errHdlTGpio.tErrorsPerRun[10]++;
	}
	if(errorCnt != 0){
		errHdlTGpio.tTotalErrors += errorCnt;
	}

	if (msgQSend (*msgQIdTP, (char *)&errHdlTGpio, MAX_MSG_LEN, WAIT_FOREVER,
                  MSG_PRI_NORMAL) == ERROR){
        return (ERROR);
    }

	return OK;
}

int GC_QualiTest(void)
{
	int idTask[NBR_OF_TASKS];
	u_int8 taskState = 0;
	u_int32 pass = 0;
	u_int32 n = 0, m = 0;
	int fd;
	ERROR_HDL errHdlT[NBR_OF_TASKS];

	/* Check for still running tasks, because ctrl-C stops only the main task. 
       GC_QualiTest should start defined, all tasks should not exist */
	if( (idTask[0] = taskNameToId("tCan")) != ERROR){
		/*kill task*/
		taskDelete(idTask[0]);
	}
	if( (idTask[1] = taskNameToId("tMemory")) != ERROR){
		/*kill task*/
		taskDelete(idTask[1]);
	}
	if( (idTask[2] = taskNameToId("tSsd")) != ERROR){
		/*kill task*/
		taskDelete(idTask[2]);
	}
	if( (idTask[3] = taskNameToId("tEthernetCom2")) != ERROR){
		/*kill task*/
		taskDelete(idTask[3]);
	}
	if( (idTask[4] = taskNameToId("tGpio")) != ERROR){
		/*kill task*/
		taskDelete(idTask[4]);
	}

	/* hdtest prints error messages to avoid this redirect the outputs to 
       the unused serial port */
	fd = open("/tyCo/0", 0,0);
	if(fd == ERROR){
		return (ERROR);
	}
	UOS_KeyStdIoFd = fd;

	for(n = 0; n < NBR_OF_TASKS; n++){
		errHdlT[n].tTotalErrors = 0;
		idTask[n] = 0;

		/* create task message queues */
    	if ((msgQIdT[n] = msgQCreate (MAX_MSGS, MAX_MSG_LEN, MSG_Q_PRIORITY)) 
    	    == NULL){
    	    return (ERROR);
		}
		if (msgQSend (msgQIdT[n], (char *)&errHdlT[n], MAX_MSG_LEN, 
                 WAIT_FOREVER, MSG_PRI_NORMAL) == ERROR){
            return (ERROR);
        }
	}
	

	while(1){
		printf("*************************\n");
		printf(" Pass: %d\n", pass);
		printf("*************************\n");
		GC_CutOffOut(3,0);

		/* create Tasks */
		if ((idTask[TASK_CAN] = taskSpawn ("tCan", 100, 0, STACK_SIZE, taskCan, 
                   	(int) &msgQIdT[TASK_CAN], 0, 0, 0, 0, 0, 0, 0, 0, 0)) 
					== ERROR){
			printf ("taskSpawn of tCan failed\n");
			return (ERROR);
		}

		if ((idTask[TASK_MEM] = taskSpawn ("tMemory", 100, 0, STACK_SIZE, 
                   	taskMemory, (int) &msgQIdT[TASK_MEM], 0, 0, 0, 0, 0, 0, 0, 
					0, 0)) == ERROR){
			printf ("taskSpawn of tMemory failed\n");
			return (ERROR);
		}

		if((taskState & TASK_SSD_FLAG) == 0){
			if ((idTask[TASK_SSD] = taskSpawn ("tSsd", 100, 0, STACK_SIZE, 
					taskSsd, (int) &msgQIdT[TASK_SSD], 0, 0, 0, 0, 0, 0, 0, 
					0, 0)) == ERROR){
				printf ("taskSpawn of tSsd failed\n");
				return (ERROR);
			}
			else{
				taskState |= TASK_SSD_FLAG;
			}
		}

		if ((idTask[TASK_ETH] = taskSpawn ("tEthernetCom2", 100, 0, 
					STACK_SIZE, taskEthernetCom2, (int) &msgQIdT[TASK_ETH], 0, 
					0, 0, 0, 0, 0, 0, 0, 0)) == ERROR){
			printf ("taskSpawn of tEthernetCom2 failed\n");
			return (ERROR);
		}

		if ((idTask[TASK_GPIO] = taskSpawn ("tGpio", 100, 0, STACK_SIZE, 
					taskGpio, (int) &msgQIdT[TASK_GPIO], 0, 0, 0, 0, 0, 0, 0, 
					0, 0)) == ERROR){
			printf ("taskSpawn of tGpio failed\n");
			return (ERROR);
		}

		taskState |= TASK_CAN_FLAG |
					 TASK_GPIO_FLAG |
					 TASK_MEM_FLAG |
					 TASK_ETH_FLAG;

		taskDelay(1500);
		do{
			taskDelay(250);
			if(taskState & TASK_CAN_FLAG){
				if(taskIdVerify(idTask[TASK_CAN]) != OK){
					/* task finished */
					taskState &= ~TASK_CAN_FLAG;
				}
			}

			if(taskState & TASK_MEM_FLAG){
				if(taskIdVerify(idTask[TASK_MEM]) != OK){
					/* task finished */
					taskState &= ~TASK_MEM_FLAG;
					
				}
			}

			if(taskState & TASK_SSD_FLAG){
				if(taskIdVerify(idTask[TASK_SSD]) != OK){
					/* task finished */
					taskState &= ~TASK_SSD_FLAG;
					if (msgQReceive(msgQIdT[TASK_SSD], 
							(char *)&errHdlT[TASK_SSD], MAX_MSG_LEN, 
                            WAIT_FOREVER) == ERROR){
						return (ERROR);
					}
					if(errHdlT[TASK_SSD].tErrorsPerRun[0] != 0)
						printf("Task 3: F503 SSD: \t\t\tFAILED\n");
					if (msgQSend (msgQIdT[TASK_SSD], 
							(char *)&errHdlT[TASK_SSD], MAX_MSG_LEN, 
							WAIT_FOREVER, MSG_PRI_NORMAL) == ERROR){
			            return (ERROR);
			        }
				}
			}

			if(taskState & TASK_ETH_FLAG){
				if(taskIdVerify(idTask[TASK_ETH]) != OK){
					/* task finished */
					taskState &= ~TASK_ETH_FLAG;		
				}
			}

			if(taskState & TASK_GPIO_FLAG){
				if(taskIdVerify(idTask[TASK_GPIO]) != OK){
					/* task finished */
					taskState &= ~TASK_GPIO_FLAG;
					
				}   
			}
		}while((taskState != TASK_SSD_FLAG) && (taskState != 0));

		GC_CutOffOut(3,1);
		taskDelay(500);

		if(GC_CanTestA_07320(1,500,1,1) == OK){
			printf("Disabled Can test: \t\tFAILED\n");
			errHdlT[TASK_CAN].tTotalErrors++;
		}

		/* print result */
		for(n = 0; n < NBR_OF_TASKS; n++){
			if ((n != TASK_SSD)){
				if (msgQReceive(msgQIdT[n], (char *)&errHdlT[n], MAX_MSG_LEN, 
                     	WAIT_FOREVER) == ERROR){
					return (ERROR);
				}
			}
		}

		for(n=0; n<6; n++){
			if(errHdlT[TASK_CAN].tErrorsPerRun[n] != 0){
				if((errHdlT[TASK_CAN].tErrorsPerRun[n] & 0xFF) != 0){
					printf("Task 1: CAN%d send: \t\t\tFAILED\n", n+1);
				}
			
				for(m=0; m<6; m++){
					if((errHdlT[TASK_CAN].tErrorsPerRun[n]>>(m+8)) & 0x1){
						printf("Task 1: CAN%d send, CAN%d received: "
                            "\tFAILED\n", n+1, m+1);
					}
				}
			}
		}
		if(errHdlT[TASK_MEM].tErrorsPerRun[0] != 0)
			printf("Task 2: F503 SDRAM: \t\t\tFAILED\n");
		if(errHdlT[TASK_MEM].tErrorsPerRun[1] != 0)
			printf("Task 2: XM50 SDRAM: \t\t\tFAILED\n");
		if(errHdlT[TASK_MEM].tErrorsPerRun[2] != 0)
			printf("Task 2: I2C AD78: \t\t\tFAILED\n");
		if(errHdlT[TASK_MEM].tErrorsPerRun[3] != 0)
			printf("Task 2: I2C EEPROM: \t\t\tFAILED\n");
		if( errHdlT[TASK_ETH].tErrorsPerRun[0] != 0 )
			printf("Task 4: Eth port 1: \t\t\tFAILED\n");
		if( errHdlT[TASK_ETH].tErrorsPerRun[1] != 0 )
			printf("Task 4: Eth port 2: \t\t\tFAILED\n");
		if( errHdlT[TASK_ETH].tErrorsPerRun[2] != 0 )
			printf("Task 4: Eth port 3: \t\t\tFAILED\n");
		if( errHdlT[TASK_ETH].tErrorsPerRun[3] != 0 )
			printf("Task 4: Com2: \t\t\t\tFAILED\n");
		if( errHdlT[TASK_GPIO].tErrorsPerRun[0] != 0 )
			printf("Task 5: State 1 IdSel: \t\t\tFAILED\n");
		if( errHdlT[TASK_GPIO].tErrorsPerRun[1] != 0 )
			printf("Task 5: State 1 ModeSel: \t\tFAILED\n");
		if( errHdlT[TASK_GPIO].tErrorsPerRun[2] != 0 )
			printf("Task 5: State 2 CutOff-: \t\tFAILED\n");
		if( errHdlT[TASK_GPIO].tErrorsPerRun[3] != 0 )
			printf("Task 5: State 2 CutOff+: \t\tFAILED\n");
		if( errHdlT[TASK_GPIO].tErrorsPerRun[4] != 0 )
			printf("Task 5: State 3 IdSel: \t\t\tFAILED\n");
		if( errHdlT[TASK_GPIO].tErrorsPerRun[5] != 0 )
			printf("Task 5: State 3 ModeSel: \t\tFAILED\n");
		if( errHdlT[TASK_GPIO].tErrorsPerRun[6] != 0 )
			printf("Task 5: State 4 CutOff+/-: \t\tFAILED\n");
		if( errHdlT[TASK_GPIO].tErrorsPerRun[7] != 0 )
			printf("Task 5: State 4 CutOff-: \t\tFAILED\n");
		if( errHdlT[TASK_GPIO].tErrorsPerRun[8] != 0 )
			printf("Task 5: State 4 CutOff+: \t\tFAILED\n");
		if( errHdlT[TASK_GPIO].tErrorsPerRun[9] != 0 )
			printf("Task 5: State 5 IdSel: \t\t\tFAILED\n");
		if( errHdlT[TASK_GPIO].tErrorsPerRun[10] != 0 )
			printf("Task 5: State 5 ModeSel: \t\tFAILED\n");
		printf("\n");
		printf("Total Can errors           : %d\n", errHdlT[TASK_CAN].tTotalErrors);
		printf("Total Memory errors        : %d\n", errHdlT[TASK_MEM].tTotalErrors);
		printf("Total SSD errors           : %d\n", errHdlT[TASK_SSD].tTotalErrors);
		printf("Total Ethernet Com2 errors : %d\n", errHdlT[TASK_ETH].tTotalErrors);
		printf("Total GPIO errors          : %d\n", errHdlT[TASK_GPIO].tTotalErrors);

		for(n = 0; n < NBR_OF_TASKS; n++){
			if(n != TASK_SSD ){
				if (msgQSend (msgQIdT[n], (char *)&errHdlT[n], MAX_MSG_LEN, 
	                     WAIT_FOREVER, MSG_PRI_NORMAL) == ERROR){
		            return (ERROR);
		        }
			}
		}

		pass++;
	}

	close(fd);

	return OK;
}
