/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!  
 *        \file  qualification_test_p.c
 *
 *      \author  MKolpak
 *        $Date: 2009/07/27 11:19:16 $
 *    $Revision: 1.1 $
 * 
 *       \brief  Test tool for F50P
 *
 *               The test runs parallel in 3 tasks. The tasks communcates 
 *               with the main task over message queues. 
 *               Task 1 makes memory accesses to F503 SDRAM, XM50 SDRAM, AD78
 *               via I2C and F503 EEPROM via I2C. Task 2 performs a hdtest
 *               on the solid state disc. Task 3 performs loopframes tests on
 *               Ethernet port1, port2, port3 and on COM2. 
 *               The main task starts the 3 test tasks and waits until all 
 *               tasks are finished except task 2. The main task cannot wait
 *               for task 2, because task 2 needs several seconds. One pass
 *               should take between 25 and 30 seconds. After every pass the main
 *               task prints the result. 
 *
 *     Switches: -
 *     Required: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: qualification_test_p.c,v $
 * Revision 1.1  2009/07/27 11:19:16  MKolpak
 * Initial Revision - Cloned from qualification_test.c
 *
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
#define TASK_MEM_FLAG   0x02
#define TASK_SSD_FLAG   0x04
#define TASK_ETH_FLAG   0x08
#define TASK_USB_FLAG	0x10

#define NBR_OF_TASKS    3
#define TASK_MEM        0
#define TASK_SSD        1
#define TASK_ETH        2
#define TASK_USB        3


#define MAX_MSGS        1
#define MAX_MSG_LEN     sizeof(ERROR_HDL)

#define MAX_NBR_TESTS_PER_RUN 11

#define STACK_SIZE      32000   

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
int taskMemory( MSG_Q_ID * );
int taskSsd( MSG_Q_ID * );
int taskEthernet( MSG_Q_ID * );
int taskUSB( MSG_Q_ID * );


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

    #if 0
    /* test 4 Bytes at DDR2 from F503 */
    if(GC_SdramTest( 1, 1, 1 ) != OK){
        errorCnt++;
        errHdlTMemory.tErrorsPerRun[0]++;
    }
    #endif

    /* test 4 Bytes at SDRAM from XM50 */
    if(GC_SdramXM50Test( 100, 1 ) != OK){
        errorCnt++;
        errHdlTMemory.tErrorsPerRun[0]++;
    }

    /* test 4 Bytes at SDRAM from XM50 */
        if(GC_PciTest( 1 , 2 ) != OK){
            errorCnt++;
            errHdlTMemory.tErrorsPerRun[1]++;
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

int taskUSB( MSG_Q_ID *msgQIdTP )
{
    int errorCnt = 0;
    ERROR_HDL errHdlTSsd;

    if (msgQReceive(*msgQIdTP, (char *)&errHdlTSsd, MAX_MSG_LEN, 
             WAIT_FOREVER) == ERROR){
        return (ERROR);
    }

    errHdlTSsd.tErrorsPerRun[0] = 0;

    /* write and verify a file of 1MB on USB */
    if(GC_UsbTest( 8, 1 ) != OK){
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


int taskEthernet( MSG_Q_ID *msgQIdTP )
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

    if(GC_EthTest( 1, 10, 2 ) != OK){
        errorCnt++;
        errHdlTEthCom2.tErrorsPerRun[0]++;
    }

    if(GC_EthTest( 2, 10, 2 ) != OK){
        errorCnt++;
        errHdlTEthCom2.tErrorsPerRun[1]++;
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


int F50P_QualiTest(void)
{
    int idTask[NBR_OF_TASKS];
    u_int8 taskState = 0;
    u_int32 pass = 0;
    u_int32 n = 0, m = 0;
    int fd, fd2;
    ERROR_HDL errHdlT[NBR_OF_TASKS];

    /* Check for running tasks, because ctrl-C stops only the main task. 
       P_QualiTest should start defined, all tasks should not exist */
    #if 0
    if( (idTask[0] = taskNameToId("tCan")) != ERROR){
        /*kill task*/
        taskDelete(idTask[0]);
    }
    #endif
    
    if( (idTask[TASK_MEM] = taskNameToId("tMemory")) != ERROR){
        /*kill task*/
        taskDelete(idTask[TASK_MEM]);
    }
    if( (idTask[TASK_SSD] = taskNameToId("tSsd")) != ERROR){
        /*kill task*/
        taskDelete(idTask[TASK_SSD]);
    }
    if( (idTask[TASK_ETH] = taskNameToId("tEthernet")) != ERROR){
        /*kill task*/
        taskDelete(idTask[TASK_ETH]);
    }
    #if 0 
    if( (idTask[TASK_USB] = taskNameToId("tUsb")) != ERROR){
        /*kill task*/
        taskDelete(idTask[TASK_USB]);
    }

    /* check if /bd0 is available for USB testing */
    cd("/null");
    fd2 = open("/bd0/test",0x202,0);
    
    if(fd2==ERROR)
    {
        printf("Error opening /bd0\n");
        return ERROR;
    }

    close(fd2);
    #endif

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
        
        /* GC_CutOffOut(3,0); */


        if ((idTask[TASK_MEM] = taskSpawn ("tMemory", 120, 0, STACK_SIZE, 
                    taskMemory, (int) &msgQIdT[TASK_MEM], 0, 0, 0, 0, 0, 0, 0, 
                    0, 0)) == ERROR){
            printf ("taskSpawn of tMemory failed\n");
            return (ERROR);
        }

        if((taskState & TASK_SSD_FLAG) == 0){
            if ((idTask[TASK_SSD] = taskSpawn ("tSsd", 110, 0, STACK_SIZE, 
                    taskSsd, (int) &msgQIdT[TASK_SSD], 0, 0, 0, 0, 0, 0, 0, 
                    0, 0)) == ERROR){
                printf ("taskSpawn of tSsd failed\n");
                return (ERROR);
            }
            else{
                taskState |= TASK_SSD_FLAG;
            }
        }
        #if 0
        if((taskState & TASK_USB_FLAG) == 0){
            if ((idTask[TASK_USB] = taskSpawn ("tUsb", 105, 0, STACK_SIZE, 
                    taskUSB, (int) &msgQIdT[TASK_USB], 0, 0, 0, 0, 0, 0, 0, 
                    0, 0)) == ERROR){
                printf ("taskSpawn of tUsb failed\n");
                return (ERROR);
            }
            else{
                taskState |= TASK_USB_FLAG;
            }
        }
        #endif

        if ((idTask[TASK_ETH] = taskSpawn ("tEthernet", 100, 0, STACK_SIZE, 
                    taskEthernet, (int) &msgQIdT[TASK_ETH], 0, 0, 0, 0, 0, 0, 0, 
                    0, 0)) == ERROR){
            printf ("taskSpawn of tEthernet failed\n");
            return (ERROR);
        }

        #if 0
        if ((idTask[TASK_GPIO] = taskSpawn ("tGpio", 100, 0, STACK_SIZE, 
                    taskGpio, (int) &msgQIdT[TASK_GPIO], 0, 0, 0, 0, 0, 0, 0, 
                    0, 0)) == ERROR){
            printf ("taskSpawn of tGpio failed\n");
            return (ERROR);
        }
        #endif

        #if 0
        taskState |= TASK_CAN_FLAG |
                     TASK_GPIO_FLAG |
                     TASK_MEM_FLAG |
                     TASK_ETH_FLAG;
        #endif

        taskState |= TASK_MEM_FLAG |
                     TASK_ETH_FLAG;

        taskDelay(1500);
        do{
            taskDelay(250);
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
            #if 0
            if(taskState & TASK_USB_FLAG){
                if(taskIdVerify(idTask[TASK_USB]) != OK){
                    /* task finished */
                    taskState &= ~TASK_USB_FLAG;        
                }
            }
            #endif

        }while((taskState != TASK_SSD_FLAG) && (taskState != 0));
       
        /*/* nur SSD Flag? */
        taskDelay(500);

        /* print result */
        for(n = 0; n < NBR_OF_TASKS; n++){
            if ((n != TASK_SSD)){
                if (msgQReceive(msgQIdT[n], (char *)&errHdlT[n], MAX_MSG_LEN, 
                        WAIT_FOREVER) == ERROR){
                    return (ERROR);
                }
            }
        }
        if(errHdlT[TASK_MEM].tErrorsPerRun[0] != 0)
            printf("Task 2: XM50 SDRAM:    FAILED\n");
        if(errHdlT[TASK_MEM].tErrorsPerRun[1] != 0)
            printf("Task 2: PCI:           FAILED\n");
        if( errHdlT[TASK_ETH].tErrorsPerRun[0] != 0 )
            printf("Task 4: Eth port 1:    FAILED\n");
        if( errHdlT[TASK_ETH].tErrorsPerRun[1] != 0 )
            printf("Task 4: Eth port 2:    FAILED\n");
        printf("\n");
        printf("Total Memory errors        : %d\n", errHdlT[TASK_MEM].tTotalErrors);
        printf("Total SSD errors           : %d\n", errHdlT[TASK_SSD].tTotalErrors);
        printf("Total Ethernet errors      : %d\n", errHdlT[TASK_ETH].tTotalErrors);
        /*
       printf("Total USB errors           : %d\n", errHdlT[TASK_USB].tTotalErrors);
       */
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
