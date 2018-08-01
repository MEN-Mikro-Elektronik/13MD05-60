
/********************* P r o g r a m - M o d u l e ***********************/
/*!
* \file systemtest_AE52.c
*
* \author maximilian.kolpak@men.de
* $Date: 2012/09/06 15:32:06 $
* $Revision: 1.2 $
*
* \brief This file contains the lowlevel testfunctions for the AE52 
*        carrier board.
*
*
* Switches: -
*/
/*-------------------------------[ History ]---------------------------------
*
* $Log: systemtest_AE52.c,v $
* Revision 1.2  2012/09/06 15:32:06  MKolpak
* M: Release version
*
* Revision 1.1  2012/04/13 09:37:08  MKolpak
* Initial Revision
*
*
*---------------------------------------------------------------------------
* (c) Copyright 2011 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <vxWorks.h>
#include <vxBusLib.h>
#include <memLib.h>
#include <memPartLib.h>
#include "sysLib.h"
#include <pingLib.h>



#include <MEN/men_typs.h>
#include <MEN/oss.h>
#include <MEN/eeprod.h>
#include <MEN/xm51_cfg.h>
#include <MEN/smb2.h>


#include "ml605.h"

/* #include "MsgTest.h" */

/* ### error reporting and debugging ### */

#define DBG_AE52    g_AE52_verbosity
#include <MEN/dbg.h>        /* debug module */

#undef DEBUG_LOCAL
#undef LRU_FAIL_DEBUG


#ifdef DEBUG_LOCAL
#define dbg_AE52(fmt,args...) printf("%s - ",__FUNCTION__);printf (fmt ,##args)
#define locate_AE52() printf("%s: line %u\n",__FILE__,__LINE__);OSS_Delay(NULL,1)   
#define err_AE52(fmt,args...) printf("### %s - line %u: ",__FUNCTION__,__LINE__); \
                            printf (fmt ,##args);OSS_Delay(NULL,1)
#else
#define dbg_AE52(fmt,args...) if(DBG_AE52)DBG_Write(DBH,"%s - "fmt,__FUNCTION__,##args)
#define locate_AE52()
#define err_AE52(fmt,args...) DBG_Write(DBH,"### %s - line %u: "fmt,__FUNCTION__,__LINE__,##args);
#endif

#define DBH sysDbgHdlP
u_int32 g_AE52_verbosity = 0;


#define ST_AE52_PRINT(fmt,args...) if(!quiet)printf("%u: "fmt,tickGet(),##args);
#define ST_AE52_ERR(fmt,args...)   if(!quiet)printf("%u: ### "fmt,tickGet(),##args);

#define MIN(x,y) (x<y?x:y)

/* ### module specific ### */
enum
{
E_MODE_SINGLE,
E_MODE_CONTIUEOUS
};

#define AE52_SILENT_TICKS   sysClkRateGet()
#define AE52_MEM_TEST_SIZE  0x10000000
#define AE52_FPGA_MEM_SIZE  0x10000000 /* physical 512MB but we can only map 256MB */
#define AE52_FPGA_MEM_ADDR  ((u_int8 *) 0xd0000000)
#define AE52_MIL_MEM_ADDR   ((u_int8 *) 0xa4400000)
#define AE52_MIL_MEM_SIZE   0xfffff
#define AE52_P598_MEM_ADDR   ((u_int8 *) 0xa0000000)
#define AE52_P598_MEM_SIZE   0x1fffff0



#define AE52_ETH_LOOP_DTSEC 2
#define AE52_NOM_DELAY()    OSS_Delay(NULL,1)


#define AE52_XM51_GPIO_PIN_FPGA_DONE    26
#define AE52_XM51_GPIO_PIN_OUT          25

/* globals */

u_int32 g_AE52_slowtest = 0;
u_int32 g_AE52_MIL4 = 0;
u_int32 g_AE52_BitLampMode = 0;
enum
{
    AE51_BITLAMP_ON = 0,
    AE51_BITLAMP_FAST,
    AE51_BITLAMP_OFF
};


extern DBG_HANDLE * sysDbgHdlP;
extern int mtest(char *);
extern VXB_DEVICE_ID G_vxbGpioDev;


static u_int32 AE52GPIOInitDone = 0;
static u_int32 AE52LRUInitDone = 0;
static u_int32 AE52SCLInitDone = 0;
static u_int32 AE52InitDone = 0;
static u_int32 AE52SysmonInitDone = 0;
static u_int32 AE52SSDReadRate = 0;


static int          MessageMode;
static int          MinorFrameCount;
static int          MajorFrameCount;

/* 
The low level test functions must:
- run in single mode (manual tests)
- run in continuous mode (env./burn-in tests)
- have configurable verbosity

Under VxWorks the functions get zero as parameters when called from CLI.
Mode and verbosity were selected respectively.

Single mode:
============
Print errors to console, stop after errors.

Continuous mode:
================
Print errors to log, continue after errors (don't flood!)

*/

static int AE52GPIOInit(int quiet)
{
    int ret = OK;

    OSS_MikroDelayInit(NULL);
    
    if(AE52GPIOInitDone)
        return OK;
    
    if((ret = gpioInputSelect(G_vxbGpioDev,AE52_XM51_GPIO_PIN_FPGA_DONE)) != OK)
    {
        ST_AE52_ERR("gpioInputPinSelect(AE52_XM51_GPIO_PIN_FPGA_DONE) (%u)\n",ret);
    }

    if((ret = gpioInputSelect(G_vxbGpioDev,AE52_XM51_GPIO_PIN_OUT)) != OK)
    {
        ST_AE52_ERR("gpioInputPinSelect(AE52_XM51_GPIO_PIN_OUT) (%u)\n",ret);
    }
    
    if((ret = ml605_gpio_init()) != OK)
    {
        ST_AE52_ERR("ml605_gpio_init (%u)\n",ret);
    }
    
    AE52GPIOInitDone = 1;
    return ret;
}


void gpioTestISR(UINT8 num)
{
	puts("ISR GPIO\n");
}


int AE52IoApiCpuIntTest(void)
{
    return gpioIntConnect(
        G_vxbGpioDev,
        AE52_XM51_GPIO_PIN_OUT,
        gpioTestISR,
        G_vxbGpioDev);
}


 /**********************************************************************/
 /** Routine to perform sainity checks on EEPROD structure
  *
  *  This routine performs a few sainity checks on a given EEPROD structure.
  *  An error is only reported if parity is wrong or serial number causes 
  *  a MAC adress rollover.
  *
  *  \param      pEEPROD -   pointer to EEPROD structure
  *
  *  \return     OK/ERROR
  */
UINT32 AE52EeprodCheck(EEPROD2 * pEEPROD)
{
    UINT8 chksum = 0, parity = 0;
    int error = 0;
    
    /* check ID */ 
    if( (( pEEPROD->pd_id >> 4 ) != EEID_PD2) )
    {
        err_AE52("EPROD ID wrong (act: 0x%x, should:  0x%x )!\n",
                        (pEEPROD->pd_id >> 4) ,EEID_PD2);
        error = ERROR; /* ID wrong */
    }
    
    /* check parity */
    chksum = pEEPROD->pd_id & 0x0f; 
    parity = sysEeprodCalcParity((char*)pEEPROD + 1, sizeof(EEPROD2) - 1);
    if( parity != chksum )
    {
        err_AE52("EEPROD parity mismatch (0x%X != 0x%x)!\n",
            parity,chksum);
        error = ERROR; /* Checksum wrong */
    } else
        dbg_AE52("EEPROD parity OK (0x%X)\n",chksum);
    
    return error;
}


int AE52IoTempSwitchLED(int cnt)
{
    int ret = 0;
    const int quiet = 0;
    u_int32 ok = 0;
    u_int32 err = 0; 
    u_int32 out = 0;
    u_int32 in = 0; 
    char key = 0;
    
    struct tst
    {
        uint8_t lamp;
        uint8_t alarm;
        uint8_t thermsw;
        uint8_t led;
    };
    
    struct tst tab[] = 
    {
            {0,0,0,1},
            {0,1,0,0},
            {1,1,1,1},
            {0,1,0,0}
    };

    /* init GPIOs */
    ret = AE52GPIOInit(quiet);
    if(ret)
        return ret; 


    /* write */
    out = 
        (tab[cnt].lamp<<ML605_GPIO_0_BIT_B_LAMP_O) |
        (tab[cnt].alarm<<ML605_GPIO_0_BIT_ALARM_O);
    ml605_gpio_dat_set(
        E_ML605_GPIO_0,
        E_ML605_GPIO_C1,
        out
        );
    AE52_NOM_DELAY();
    /* read */
    ml605_gpio_dat_get(
        E_ML605_GPIO_0,
        E_ML605_GPIO_C1,
        &in
        );
    /* check */
    if(!!(in & (1<<ML605_GPIO_0_BIT_THERM_SW_I)) != tab[cnt].thermsw)
    {
        err_AE52("Temp switch mismatch (l%u a%u t%u o:0x%x i:0x%x)\n",
            tab[cnt].lamp,
            tab[cnt].alarm,
            tab[cnt].thermsw,
            out,
            in);
        err++;
     }else
    {
        dbg_AE52("Temp switch ok (l%u a%u t%u o:0x%x i:0x%x)\n",
            tab[cnt].lamp,
            tab[cnt].alarm,
            tab[cnt].thermsw,
            out,
            in);
        ok++;
    }
    
    printf("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
        
    return (int) !!err;

}

#if 0
int ll_AE52EthAgeTest(int mode, int quiet)
{
    char *  name = "dtsec";
    u_int8  idx = 2;

    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;    
    u_int32 outtick = tickGet();
    float t = 0;
    int print = 0;

    do{
        ret = Loopframes(name,idx,1,!quiet,0,0,NULL);
        if(ret)
            err++;
        else
            ok++;
        if(!quiet)
        {
            /* print only once per tick every AE52_SILENT_TICKS */
            if(tickGet() > (outtick + AE52_SILENT_TICKS))
            {
                print = 1; 
                outtick = tickGet();
            }else
                print = 0; 
            /* in single mode output immediately */
            if(!mode)
                print = 1; 
        }       

        /* have a break */
        OSS_Delay(NULL,1);

        if(print)
        {
            ST_AE52_PRINT("%s OK: %u ERR: %u\n",__FUNCTION__,ok,err);
            print = 0;
        }
    }while(mode);


    return (int) !!err;
}
#endif

#if 1
#define FASTTEST_P598
#ifndef FASTTEST_P598
#define AE52_P598_MEM_CHUNKSIZE 0x100000
#else
#define AE52_P598_MEM_CHUNKSIZE 0x1000000
#endif
int ll_AE52P598Test(int mode, int quiet)
{
int ret = 0;
u_int32 ok = 0;
u_int32 err = 0;    
u_int32 outtick = tickGet();
int print = 0;

char params[50];
u_int8 * const physStartAddr = AE52_P598_MEM_ADDR;
u_int8 * const physEndAddr = AE52_P598_MEM_ADDR + AE52_P598_MEM_SIZE;
u_int8 * startAddr = physStartAddr;
#ifndef FASTTEST_P598
u_int8 * endAddr = physStartAddr + AE52_P598_MEM_CHUNKSIZE;
#else
u_int8 * endAddr = physStartAddr + 0x80000;
#endif
u_int8 passes = 1;

do{
    /* set para string */
    sprintf(params, "-n=%d 0x%x 0x%x -t=bwlBWL -o=%d", 
                passes, 
                startAddr, 
                endAddr, 
                quiet ? 1 : 2);

    dbg_AE52("mtest %s\n",params);
    
    /* do mtest */
    ret = mtest(params);

    /* check */
    if(ret)
        err++;
    else
        ok++;
    if(!quiet)
    {
        /* print only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
            print = 1; 
            outtick = tickGet();
        }else
            print = 0; 
        /* in single mode output immediately */
        if(!mode)
            print = 1; 
    }       

    /* have a break */
    OSS_Delay(NULL,1);
    
    if(print)
    {
        ST_AE52_PRINT("%s start: 0x%08X end: 0x%08X\n",
                __FUNCTION__,
                startAddr,
                endAddr);
        ST_AE52_PRINT("%s OK: %u ERR: %u\n",__FUNCTION__,ok,err);
        print = 0;
    }

    if((startAddr + AE52_P598_MEM_CHUNKSIZE) < physEndAddr)
        startAddr += AE52_P598_MEM_CHUNKSIZE;
    else
        startAddr = physStartAddr; /* wrap around */
#ifndef FASTTEST_P598
    endAddr += AE52_P598_MEM_CHUNKSIZE;
    endAddr %= startAddr + AE52_P598_MEM_CHUNKSIZE;
#else
    endAddr = MIN(startAddr + 0x80000, physEndAddr); /* only 4k for fast results */
#endif
}while(mode);

return (int) !!err;

}
#endif


/* 
g_AE52_slowtest = 1: try to alloc biggest block
g_AE52_slowtest = 0: alloc AE52_MEM_TEST_SIZE
TODO: chunkanize like ll_AE52FpgaSdramTest
*/
int ll_AE52SdramTest(int mode, int quiet)
{
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;    
    u_int32 outtick = tickGet();
    int print = 0;

    char params[50];
    u_int32 * startAddr = NULL;
    u_int32 * endAddr= NULL;    
    u_int32 passes = 1;
    u_int32 freeMem = 0;
    MEM_PART_STATS  partStats;

    /* alloc mem */
    freeMem = 0;
    if(g_AE52_slowtest)
    {
        memPartVerifiedLock(memSysPartId);
        if(memPartInfoGet (memSysPartId, &partStats) == OK)
        {
            freeMem = partStats.maxBlockSizeFree; /* biggest free block*/
            freeMem -= freeMem%0x1000000ul; /* 16MB align */
        }
        semMGive (&(memSysPartId)->sem);
    }
    if(freeMem == 0)
    {
        startAddr = malloc(AE52_MEM_TEST_SIZE);
        endAddr = (u_int32 *)((u_int8 *)startAddr + AE52_MEM_TEST_SIZE);
        dbg_AE52("default size (0x%X)\n",AE52_MEM_TEST_SIZE);
    }
    else
    {
        startAddr = malloc(freeMem);
        endAddr = (u_int32 *)((u_int8 *)startAddr + freeMem);
        dbg_AE52("biggest block (0x%X)\n",freeMem);
    }

    if(startAddr == NULL)
    {
        ST_AE52_ERR("malloc (size:%u)\n",AE52_MEM_TEST_SIZE);
        return ret;
    }
    endAddr = (u_int32 *)((u_int8 *)startAddr + AE52_MEM_TEST_SIZE);

    /* set para string */
    sprintf(params, "-n=%d 0x%x 0x%x -t=bwlBWL -o=%d", 
                passes, 
                startAddr, 
                endAddr, 
                quiet ? 1 : 2);

    dbg_AE52("mtest %s\n",params);

    /* do mtest */
    do{
        ret = mtest(params);
               
        if(ret)
            err++;
        else
            ok++;
        if(!quiet)
        {
            /* print only once per tick every AE52_SILENT_TICKS */
            if(tickGet() > (outtick + AE52_SILENT_TICKS))
            {
                print = 1; 
                outtick = tickGet();
            }else
                print = 0; 
            /* in single mode output immediately */
            if(!mode)
                print = 1; 
        }       

        /* have a break */
        OSS_Delay(NULL,1);
        
        if(print)
        {
            ST_AE52_PRINT("%s start: 0x%08X end: 0x%08X\n",
                    __FUNCTION__,
                    startAddr,
                    endAddr);
            ST_AE52_PRINT("%s OK: %u ERR: %u\n",__FUNCTION__,ok,err);
            print = 0;
        }
    }while(mode);

    free(startAddr);

return (int) !!err;
}



int ll_AE52FastSdramTest(int mode, int quiet)
{
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;    
    u_int32 outtick = tickGet();
    int print = 0;

    char params[50];
    u_int32 * startAddr = NULL;
    u_int32 * endAddr= NULL;    
    u_int32 passes = 1;
    u_int32 freeMem = 0;
    MEM_PART_STATS  partStats;

    /* alloc mem */
    freeMem = 0;
    if(g_AE52_slowtest)
    {
        memPartVerifiedLock(memSysPartId);
        if(memPartInfoGet (memSysPartId, &partStats) == OK)
        {
            freeMem = partStats.maxBlockSizeFree; /* biggest free block*/
            freeMem -= freeMem%0x1000000ul; /* 16MB align */
        }
        semMGive (&(memSysPartId)->sem);
    }
    if(freeMem == 0)
    {
        startAddr = malloc(AE52_MEM_TEST_SIZE);
        endAddr = (u_int32 *)((u_int8 *)startAddr + AE52_MEM_TEST_SIZE);
        dbg_AE52("default size (0x%X)\n",AE52_MEM_TEST_SIZE);
    }
    else
    {
        startAddr = malloc(freeMem);
        endAddr = (u_int32 *)((u_int8 *)startAddr + freeMem);
        dbg_AE52("biggest block (0x%X)\n",freeMem);
    }

    if(startAddr == NULL)
    {
        ST_AE52_ERR("malloc (size:%u)\n",AE52_MEM_TEST_SIZE);
        return ret;
    }
    endAddr = (u_int32 *)((u_int8 *)startAddr + AE52_MEM_TEST_SIZE);

    /* set para string */
    sprintf(params, "-n=%d 0x%x 0x%x -t=l -o=%d", 
                passes, 
                startAddr, 
                endAddr, 
                quiet ? 1 : 2);

    dbg_AE52("mtest %s\n",params);

    /* do mtest */
    do{
        ret = mtest(params);
               
        if(ret)
            err++;
        else
            ok++;
        if(!quiet)
        {
            /* print only once per tick every AE52_SILENT_TICKS */
            if(tickGet() > (outtick + AE52_SILENT_TICKS))
            {
                print = 1; 
                outtick = tickGet();
            }else
                print = 0; 
            /* in single mode output immediately */
            if(!mode)
                print = 1; 
        }       

        /* have a break */
        OSS_Delay(NULL,1);
        
        if(print)
        {
            ST_AE52_PRINT("%s start: 0x%08X end: 0x%08X\n",
                    __FUNCTION__,
                    startAddr,
                    endAddr);
            ST_AE52_PRINT("%s OK: %u ERR: %u\n",__FUNCTION__,ok,err);
            print = 0;
        }
    }while(mode);

    free(startAddr);

return (int) !!err;
}



/* ########## FPGA ########### */

/* 
g_AE52_slowtest = 1: test takes 1,5h!
g_AE52_slowtest = 0: test takes almost 3min! 
*/
#define AE52_FPGA_MEM_CHUNKSIZE 0x1000000
/* this test is not thread-save! (GPIOs are not saved/restored) */
int ll_AE52FpgaSdramTest(int mode, int quiet)
{
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;    
    u_int32 outtick = tickGet();
    int print = 0;

    char params[50];
    volatile u_int8 * const physStartAddr = AE52_FPGA_MEM_ADDR;
    volatile u_int8 * const physEndAddr = AE52_FPGA_MEM_ADDR + AE52_FPGA_MEM_SIZE;
    volatile u_int8 * startAddr = physStartAddr;
    volatile u_int8 * endAddr;

    u_int8 passes = 1;
    u_int8 area = 0;

    /* init GPIOs */
    ret = AE52GPIOInit(quiet);
    if(ret)
    {
        return ret;

    }


    if(g_AE52_slowtest)
        endAddr = physStartAddr + AE52_FPGA_MEM_CHUNKSIZE;
    else
        endAddr = physStartAddr + 0x80000;

    do{
        /* check switching lower/upper area (256MB) */

        /* write to lower */
        ml605_gpio_dat_set(
                E_ML605_GPIO_0,
                E_ML605_GPIO_C1,
                0   /* lower area */
                );
        /* have a break */
        OSS_Delay(NULL,1);      
        strcpy(physStartAddr,"lower");
        /* write to upper */
        ml605_gpio_dat_set(
                E_ML605_GPIO_0,
                E_ML605_GPIO_C1,
                (1<<ML605_GPIO_0_BIT_DDR_A13_O) /* upper area */
                );
        /* have a break */
        OSS_Delay(NULL,1);      
        strcpy(physStartAddr,"upper");
        /* check lower*/
        ml605_gpio_dat_set(
                E_ML605_GPIO_0,
                E_ML605_GPIO_C1,
                0   /* lower area */
                );
        /* have a break */
        OSS_Delay(NULL,1);
        if(strcmp(physStartAddr,"lower"))
        {
            ST_AE52_ERR("reading lower (\"%s\")\n",physStartAddr);
            err++;
        }
        else
        {
            ST_AE52_PRINT("reading area OK (\"%s\")\n",physStartAddr);
            ok++;
        }
        /* check upper*/
        ml605_gpio_dat_set(
                E_ML605_GPIO_0,
                E_ML605_GPIO_C1,
                (1<<ML605_GPIO_0_BIT_DDR_A13_O) /* upper area */
                );
        /* have a break */
        OSS_Delay(NULL,1);
        if(strcmp(physStartAddr,"upper"))
        {
            ST_AE52_ERR("reading upper (\"%s\")\n",physStartAddr);
            err++;
        }
        else
        {
            ST_AE52_PRINT("reading area OK (\"%s\")\n",physStartAddr);
            ok++;
        }
        
        /* do mtest of both areas*/
        startAddr = physStartAddr;
        if(g_AE52_slowtest)
            endAddr = physStartAddr + AE52_FPGA_MEM_CHUNKSIZE;
        else
            endAddr = physStartAddr + 0x80000;
        
        do{
                /* test both areas */
            for(area = 0; area < 2; area++)
            {
                /* switch lower/upper area (256MB) */
                ml605_gpio_dat_set(
                    E_ML605_GPIO_0,
                    E_ML605_GPIO_C1,
                    ((!!area)<<ML605_GPIO_0_BIT_DDR_A13_O)
                    );          
                /* set para string */
                sprintf(params, "-n=%d 0x%x 0x%x -t=bwlBWL -o=%d", 
                            passes, 
                            startAddr, 
                            endAddr, 
                            quiet ? 1 : 2);
                
                /* do mtest */
                ret = mtest(params);

                /* check */
                if(ret)
                {
                    ST_AE52_ERR("%s area 0x%x 0x%x\n",
                        area ? "upper" : "lower",
                        startAddr,
                        endAddr);
                    err++;
                }
                else
                {
                    dbg_AE52("%s area 0x%x 0x%x OK\n",
                        area ? "upper" : "lower",
                        startAddr,
                        endAddr);
                    ok++;
                }
                
                if(!quiet)
                {
                    /* print only once per tick every AE52_SILENT_TICKS */
                    if(tickGet() > (outtick + AE52_SILENT_TICKS))
                    {
                        print = 1; 
                        outtick = tickGet();
                    }else
                        print = 0; 
                    /* in single mode output immediately */
                    if(!mode)
                        print = 1; 
                }       

                /* have a break */
                OSS_Delay(NULL,1);
                
                if(print)
                {
                    ST_AE52_PRINT("%s OK: %u ERR: %u (%c 0x%08X-0x%08X)\n",
                            __FUNCTION__,
                            ok,
                            err,
                            area ? 'u' : 'l',
                            startAddr,
                            endAddr);
                    print = 0;
                }
            }
            startAddr += AE52_FPGA_MEM_CHUNKSIZE;
            if(g_AE52_slowtest)
                endAddr += AE52_FPGA_MEM_CHUNKSIZE;
            else
                endAddr = MIN(startAddr + 0x80000, physEndAddr); /* only 4k for fast results */
        }while(startAddr < physEndAddr);
    }while(mode);
    return (int) !!err;

}

/* MIL-BUS module */
int ll_AE52MilRamTest(int mode, int quiet)
{
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;    
    u_int32 outtick = tickGet();
    int print = 0;

    char params[50];
    u_int32 * startAddr = NULL;
    u_int32 * endAddr= NULL;    
    u_int32 passes = 1;
    u_int32 freeMem = 0;

    /* alloc mem */
    startAddr = AE52_MIL_MEM_ADDR;
    endAddr = (u_int32 *)((u_int8 *)startAddr + AE52_MIL_MEM_SIZE);

    /* set para string */
    sprintf(params, "-n=%d 0x%x 0x%x -t=bwl -o=%d", 
                passes, 
                startAddr, 
                endAddr, 
                quiet ? 1 : 2);

    dbg_AE52("mtest %s\n",params);

    /* do mtest */
    do{
        ret = mtest(params);
               
        if(ret)
            err++;
        else
            ok++;
        if(!quiet)
        {
            /* print only once per tick every AE52_SILENT_TICKS */
            if(tickGet() > (outtick + AE52_SILENT_TICKS))
            {
                print = 1; 
                outtick = tickGet();
            }else
                print = 0; 
            /* in single mode output immediately */
            if(!mode)
                print = 1; 
        }       

        /* have a break */
        OSS_Delay(NULL,1);
        
        if(print)
        {
            ST_AE52_PRINT("%s start: 0x%08X end: 0x%08X\n",
                    __FUNCTION__,
                    startAddr,
                    endAddr);
            ST_AE52_PRINT("%s OK: %u ERR: %u\n",__FUNCTION__,ok,err);
            print = 0;
        }
    }while(mode);

return (int) !!err;
}


int ll_AE52SysMonTest(int mode, int quiet)
{

int ret = 0;
u_int32 ok = 0;
u_int32 err = 0;    
u_int32 outtick = tickGet();
float t = 0;
int print = 0;


/* init unit */
if(!AE52SysmonInitDone)
{
    if((ret = ml605_sysmon_init()))
    {
        ST_AE52_ERR("init (%u)\n",ret);
        return ret;
    }
    AE52SysmonInitDone = 1;
}

do{
    ret = ml605_sysmon_ctemp_get(&t);
    ret = ml605_sysmon_voltages_read(); /* TODO - check thresholds */
    if(ret)
        err++;
    else
        ok++;
    if(!quiet)
    {
        /* print only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
            print = 1; 
            outtick = tickGet();
        }else
            print = 0; 
        /* in single mode output immediately */
        if(!mode)
            print = 1; 
    }       

    /* have a break */
    OSS_Delay(NULL,1);
    
    if(print)
    {
        ST_AE52_PRINT("System monitor:\n\n");
        printf("On-die temperature:\n"
                "     %.2f°C\n"
                "Voltages:\n",t);
        ml605_sysmon_voltages_print(NULL); /* print values */
        ST_AE52_PRINT("%s OK: %u ERR: %u\n",__FUNCTION__,ok,err);
        print = 0;
    }
}while(mode);

    
return (int) !!err;
}

int ll_AE52IoSclTest(int mode, int quiet)
{
int ret = 0;
u_int32 ok = 0;
u_int32 err = 0;    
u_int32 outtick = tickGet();
int print = 0;

/* init unit */
if(!AE52SCLInitDone)
{
    if((ret = ml605_SCL_init()))
    {
        ST_AE52_ERR("init (%u)\n",ret);
        return ret;
    }
    AE52SCLInitDone = 1;
}

do{
    ret = ml605_SCL_test();
    if(ret)
        err++;
    else
        ok++;
    if(!quiet)
    {
        /* output only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
            print = 1; 
            outtick = tickGet();
        }else
            print = 0; 
        /* in single mode output immediately */
        if(!mode)
            print = 1; 
    }
    if(print)
    {
        ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
        print = 0;
    }   
}while(mode);



return (int) !!err;
}


int ll_AE52IoLRU58Test(int mode, int quiet)
{
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;    
    u_int32 outtick = tickGet();
    int print = 0;
    static u_int8 dir = 0;

    /* init unit */
    if(!AE52LRUInitDone)
    {
        if((ret = ml605_LRU5_8_init(0)))
        {
            ST_AE52_ERR("init (%u)\n",ret);
            return ret;
        }
        AE52LRUInitDone = 1;
    }

    do{
        dir = !dir;
        ret = ml605_LRU5_8_test();
        if(ret)
            err++;
        else
            ok++;
        if(!quiet)
        {
            /* output only once per tick every AE52_SILENT_TICKS */
            if(tickGet() > (outtick + AE52_SILENT_TICKS))
            {
                print = 1; 
                outtick = tickGet();
            }else
                print = 0; 
            /* in single mode output immediately */
            if(!mode)
                print = 1; 
        }
        
        if(print)
        {
            ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
            print = 0;
        }   
    }while(mode);
        
    return (int) mode ? !!err : ret;
}


/* "walking one" test */
#define AE52_SUPR_PINS 4
#define AE52_SUPR_MASK 0xf
#define AE52_SUPR_API_OFFS 16

/* this test is not thread-save! (GPIOs are not saved/restored) */
int ll_AE52IoSupressionTest(int mode, int quiet)
{
int ret = 0;
u_int32 ok = 0;
u_int32 err = 0;
u_int32 outtick = tickGet();
u_int32 in = 0;
u_int32 out = 0;
uint8_t cnt = 0;
uint8_t pin = 0;
uint8_t miss = 0;
int print = 0;


/* init GPIOs */
ret = AE52GPIOInit(quiet);
if(ret)
{
    return ret;

}

do{
    cnt = 0;
    miss = 0;
    while(cnt < AE52_SUPR_PINS)
    {
        out = 1<<cnt;
        ml605_gpio_dat_set(
            E_ML605_GPIO_0,
            E_ML605_GPIO_C1,
            out<<ML605_GPIO_0_BIT_SUP1_O
            );
        AE52_NOM_DELAY();
        ml605_gpio_dat_get(
            E_ML605_GPIO_1,
            E_ML605_GPIO_C1,
            &in
            );

        /* shift and mask */
        in >>= AE52_SUPR_API_OFFS;
        in &= AE52_SUPR_MASK;

        /* check all 4 pins */
        
        for(pin = 0; pin < AE52_SUPR_PINS; pin++)
        {
            if((in & (1<<pin)) != (out & (1<<pin)))
            {
                err_AE52("SUPRESSION/API %u mismatch (0x%x!=0x%x)\n",
                    pin+1,
                    out,
                    in);
                miss = 1;
            }else
            {
                dbg_AE52("SUPRESSION/API %u ok (0x%x=0x%x)\n",
                    pin+1,
                    out,
                    in);
            }
        }
            
        cnt++;
    }
    
    if(miss)
        err++;
    else
        ok++;
    
    /* output only once per tick every AE52_SILENT_TICKS */
    if(tickGet() > (outtick + AE52_SILENT_TICKS))
    {
        print = 1; 
        outtick = tickGet();
    }else
        print = 0;
    /* in single mode output immediately */
    if(!mode)
        print = 1; 
    if(print)
    {
        ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
        print = 0;
    }   
}while(mode);
    
return (int) !!err;
}

/* this test is not thread-save! (GPIOs are not saved/restored) */
int ll_AE52IoBitLampTest(int mode, int quiet)
    {
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;
    u_int32 outtick = tickGet();
    u_int32 out = 0;
    u_int32 in = 0;
    uint8_t miss = 0;
    uint8_t cnt = 0;
    int print = 0;
    struct tst
    {
        uint8_t go;
        uint8_t no;
        uint8_t lamp;
        uint8_t sw;
    };
    struct tst tab[] = 
    {
            {1,0,0,0},
            {0,0,0,1},
            {0,1,1,0},
            {0,0,1,1}
    };

#if 0
    /* test */
    ml605_gpio_dir_set(
        0,
        0,
        ~((u_int32)1<<ML605_GPIO_0_BIT_B_SWITCH_I));
    while(cnt<6)
    {
        out = !out;
        ml605_gpio_dat_set(
            E_ML605_GPIO_0,
            E_ML605_GPIO_C1,
            (out<<ML605_GPIO_0_BIT_B_SWITCH_I)
            );
        AE52_NOM_DELAY();
        cnt++;
    };
#endif

    /* init GPIOs */
    ret = AE52GPIOInit(quiet);
    if(ret)
    {
        return ret;

    }

    do{
        
        cnt = 0;
        miss = 0;
        while(cnt < 4)
        {
            /* write */
            out = 
                (tab[cnt].go<<ML605_GPIO_0_BIT_B_GO_LAMP_O) |
                (tab[cnt].no<<ML605_GPIO_0_BIT_B_NO_LAMP_O) |
                (tab[cnt].lamp<<ML605_GPIO_0_BIT_B_LAMP_O);
            ml605_gpio_dat_set(
                E_ML605_GPIO_0,
                E_ML605_GPIO_C1,
                out
                );
            AE52_NOM_DELAY();
            /* read */
            ml605_gpio_dat_get(
                E_ML605_GPIO_0,
                E_ML605_GPIO_C1,
                &in
                );
            /* check */
            if(!!(in & (1<<ML605_GPIO_0_BIT_B_SWITCH_I)) != tab[cnt].sw)
            {
                err_AE52("BIT LAMP test %u mismatch (g%u n%u l%u s%u o:0x%x i:0x%x)\n",
                    cnt,
                    tab[cnt].go,
                    tab[cnt].no,
                    tab[cnt].lamp,
                    tab[cnt].sw,
                    out,
                    in);
                miss = 1;
            }else
            {
                dbg_AE52("BIT LAMP test %u ok (g%u n%u l%u s%u o:0x%x i:0x%x)\n",
                    cnt,
                    tab[cnt].go,
                    tab[cnt].no,
                    tab[cnt].lamp,
                    tab[cnt].sw,
                    out,
                    in);
            }
            cnt++;
        }
        
        if(miss)
            err++;
        else
            ok++;
                
        /* output only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
            print = 1; 
            outtick = tickGet();
        }else
            print = 0;
        /* in single mode output immediately */
        if(!mode)
            print = 1; 

        if(print)
        {
            ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
            print = 0;
        }   
    }while(mode);
        
    return (int) !!err;
}




/* this test is not thread-save! (GPIOs are not saved/restored) */
int ll_AE52IoLRUFailTest(int mode, int quiet)
    {
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;
    u_int32 outtick = tickGet();
    u_int32 out = 0;
    u_int32 in = 0;
    uint8_t cnt = 0;
    uint8_t miss = 0;
    uint8_t fail = 0;
    int print = 0;

    /* init GPIOs */
    ret = AE52GPIOInit(quiet);
    if(ret)
    {
        return ret;

    }

    do{
        cnt = 0;
        miss = 0;
        while(cnt < 4)
        {
            fail = !fail;
            out = fail<<ML605_GPIO_0_BIT_LRU5_FAIL_O;
            ml605_gpio_dat_set(
                E_ML605_GPIO_0,
                E_ML605_GPIO_C1,
                out
                );
            AE52_NOM_DELAY();
            /* read */
            ml605_gpio_dat_get(
                E_ML605_GPIO_0,
                E_ML605_GPIO_C1,
                &in
                );
            /* check EPF*/
            if((in&(1<<ML605_GPIO_0_BIT_EPF_I)) != fail)
            {
                err_AE52("mismatch (o:0x%x i:0x%x)\n",
                    out,
                    in);
                miss = 1;
            }else
            {
                dbg_AE52("EPF ok (o:0x%x i:0x%x)\n",
                    out,
                    in);
            }
            /* check thermal switch */
            if(in&(1<<ML605_GPIO_0_BIT_THERM_SW_I))
            {
                err_AE52("thermal switch (i:0x%x)\n",
                    in);
                miss = 1;
            }else
            {
                dbg_AE52("thermal switch ok (i:0x%x)\n",
                    in);
            }
            
            cnt++;
        }

        if(miss)
            err++;
        else
            ok++;
                
        /* output only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
            print = 1; 
            outtick = tickGet();
        }else
            print = 0;
        /* in single mode output immediately */
        if(!mode)
            print = 1; 

        if(print)
        {
            ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
            print = 0;
        }   
    }while(mode);
        
    return (int) !!err;
}


/* "walking one" test */
#define AE52_API_PINS 16
#define AE52_API_MASK 0xffff
#define AE52_SPARE_OFFS_OUT 16
#define AE52_SPARE_OFFS_IN 24
#define AE52_DEBUG_TRIGGER_ADDR 0xc0005000
/* this test is not thread-save! (GPIOs are not saved/restored) */
int ll_AE52IoApiTest(int mode, int quiet)
    {
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;
    u_int32 outtick = tickGet();
    u_int32 in = 0;
    u_int32 out = 0;
    uint8_t cnt = 0;
    uint8_t pin = 0;
    uint8_t miss = 0;
    uint8_t try = 0;
    int print = 0;


    /* init GPIOs */
    ret = AE52GPIOInit(quiet);
    if(ret)
    {
        return ret;

    }

    do{
        /* API 0..15 -> SPARE 0..15 */
        cnt = 0;
        miss = 0;
        while(cnt < AE52_API_PINS)
        {
            out = 1<<cnt;
            ml605_gpio_dat_set(
                E_ML605_GPIO_1,
                E_ML605_GPIO_C1,
                out
                );
            WRS_ASM("sync;isync;eieio");
            AE52_NOM_DELAY();
            ml605_gpio_dat_get(
                E_ML605_GPIO_1,
                E_ML605_GPIO_C2,
                &in
                );

            /* mask */
            in &= ML605_GPIO_1_WORD_C2_TRI;
            
            /* check all pins */
            for(pin = 0; pin < AE52_API_PINS; pin++)
            {
                /* circuitous but better for error printing */
                if((in & (1<<pin)) != (out & (1<<pin)))
                {
                    AE52_NOM_DELAY();
                    /* try to remove sporadic API errors by reading again */
                    WRS_ASM("sync;isync;eieio");
                    ml605_gpio_dat_get(
                        E_ML605_GPIO_1,
                        E_ML605_GPIO_C2,
                        &in
                        );
                    if((in & (1<<pin)) != (out & (1<<pin)))
                    {
                        miss++;
                        err_AE52("API %u -> SPARE %u mismatch (o:0x%x!=i:0x%x)\n",
                            pin,
                            pin,
                            out,
                            in);
                    }else
                    {
                        err_AE52("API %u -> SPARE %u ok after 2nd read (o:0x%x=i:0x%x)\n",
                            pin,
                            pin,
                            out,
                            in);
                    }
                }else
                    dbg_AE52("API %u -> SPARE %u ok (o:0x%x=i:0x%x)\n",
                        pin,
                        pin,
                        out,
                        in);
            }
            cnt++;
        }
        /* reset */
        ml605_gpio_dat_set(
                E_ML605_GPIO_1,
                E_ML605_GPIO_C1,
                0
                );

        /* SPARE 16..23 -> SPARE 24..31 */
        cnt = AE52_SPARE_OFFS_OUT;
        while(cnt < AE52_SPARE_OFFS_IN)
        {
            out = 1<<cnt;
            ml605_gpio_dat_set(
                E_ML605_GPIO_1,
                E_ML605_GPIO_C2,
                out
                );
            WRS_ASM("sync;isync;eieio");
            AE52_NOM_DELAY();
            ml605_gpio_dat_get(
                E_ML605_GPIO_1,
                E_ML605_GPIO_C2,
                &in
                );

            /* mask */
            in &= ML605_GPIO_1_WORD_C2_TRI;
            
            /* check all pins */
            for(pin = AE52_SPARE_OFFS_OUT; pin < AE52_SPARE_OFFS_IN; pin++)
            {
                /* circuitous but better for error printing */
                if((out & (1<<pin)) != ((in>>8) & (1<<pin)))
                {
                    miss++;
                    err_AE52("SPARE %u -> SPARE %u mismatch (o:0x%x!=i:0x%x)\n",
                        pin,
                        pin+8,
                        out,
                        in);
                }else
                    dbg_AE52("SPARE %u -> SPARE %u ok (o:0x%x=i:0x%x)\n",
                        pin,
                        pin+8,
                        out,
                        in);
            }
            cnt++;
        }
        /* reset */
        ml605_gpio_dat_set(
                E_ML605_GPIO_1,
                E_ML605_GPIO_C2,
                0
                );
        
        
        if(miss)
            err += miss;
        else
            ok++;
        
        /* output only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
            print = 1; 
            outtick = tickGet();
        }else
            print = 0;
        /* in single mode output immediately */
        if(!mode)
            print = 1; 

        if(print)
        {
            ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
            print = 0;
        }   
    }while(mode);
        
    return (int) !!err;
}

#if 0
int ll_AE52M1553Test(int mode, int quiet)
{
int ret = 0;
u_int32 ok = 0;
u_int32 err = 0;
u_int32 outtick = tickGet();
u_int32 out = 0;
u_int32 in = 0;
uint8_t cnt = 0;
uint8_t miss = 0;
uint8_t fail = 0;
int print = 0;

int       DevMode = 0;
int       MajorFrames = 2;
int       MinorFrames = 5;
int       Status;
STestResult TestResult[RT_MAX_NUMBER];
STATUS    TestStatus;
int       i;

    do{
          
        Status = MsgTest(
                      DevMode,        /* Device mode (0: BC + RT, 1: BC, 2: RT) */
                      0,              /* BC device number */
                      1,              /* RT #1 device number (-1: no RT #1) */
                      2,              /* RT #2 device number (-1: no RT #2) */
                      -1,
                      /* g_AE52_MIL4?3:-1,  RT #3 device number (-1: no RT #3) */
                      MinorFrames,    /* Number of minor frames RT */
                      MajorFrames,    /* Number of major frames */
                      100,            /* Message GAP time: 100 µs */
                      1000,           /* Minor time frame: 100 ms */
                      3,              /* Message mode (1 = BC to RT, 2 = RT to BC, can be ORed together) */
                      0,              /* Bus selection: both (1 = Bus A, 2 = Bus B, 0 = both) */
                      &TestStatus,
                      TestResult);

        if (Status == ERROR)
        {
          err_AE52("Failed to start test\n");
          return ERROR;
        }

        dbg_AE52("Test %s\n", (TestStatus == OK) ? "passed" : "failed");

        for (i=0; i<RT_MAX_NUMBER; ++i)
        {
          if (TestResult[i].ValidRT)
          {
              dbg_AE52("RT %d: %d / %d messages received\n", i+1, TestResult[i].MessagesReceivedRT, MinorFrameCount * MajorFrameCount);
              if (TestResult[i].MessagesReceivedRT > 0)
              {
                  dbg_AE52(" (%d R, %d T\n)", TestResult[i].RMessagesReceivedRT, TestResult[i].TMessagesReceivedRT);
              }
              printf("\n");
              if (TestResult[i].NumberWrongSART > 0)      dbg_AE52("        %d messages with wrong sub-address (RT)\n", TestResult[i].NumberWrongSART);
              if (TestResult[i].NumberWrongBusRT > 0)     dbg_AE52("        %d messages at wrong bus (RT)\n", TestResult[i].NumberWrongBusRT);
              if (TestResult[i].NumberWrongDataRT)        dbg_AE52("        %d messages with wrong data (RT)\n", TestResult[i].NumberWrongDataRT);
              if ((MessageMode == MESSAGE_MODE_RTTOBC) && (TestResult[i].RMessagesReceivedRT > 0))    printf("      %d R messages instead of T messages (RT)\n", TestResult[i].RMessagesReceivedRT);
              if ((MessageMode == MESSAGE_MODE_BCTORT) && (TestResult[i].TMessagesReceivedRT > 0))    printf("      %d T messages instead of R messages (RT)\n", TestResult[i].TMessagesReceivedRT);
          }
          if (TestResult[i].ValidBC)
          {
              if (!TestResult[i].ValidRT)
              {
                  printf("RT %d: %d messages sent by BC\n", i+1, MinorFrames * MajorFrames);
              }
              if (TestResult[i].NumberWrongSABC > 0)      dbg_AE52("        %d messages with wrong sub-address (BC)\n", TestResult[i].NumberWrongSABC);
              if (TestResult[i].NumberWrongBusBC > 0)     dbg_AE52("        %d messages at wrong bus (BC)\n", TestResult[i].NumberWrongBusBC);
              if (TestResult[i].NumberWrongDataBC > 0)    dbg_AE52("        %d T messages with wrong data (BC)\n", TestResult[i].NumberWrongDataBC);
              if (TestResult[i].NumberBlkStsErrorBC > 0)  dbg_AE52("        %d messages with block status error at BC\n", TestResult[i].NumberBlkStsErrorBC);
          }
        }

        if(TestStatus == OK)
            ok++;
        else
            err++;;
        
        /* output only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
            print = 1; 
            outtick = tickGet();
        }else
            print = 0;
        /* in single mode output immediately */
        if(!mode)
            print = 1; 

        if(print)
        {
            ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
            print = 0;
        }   
    }while(mode);
    
return (int) !!err;
}
#endif


int ll_AE52SSDTest(int mode, int quiet)
{
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;      
    u_int32 outtick = tickGet();
    float t = 0;
    int print = 0;

    char params[500];
    char *hdtestResultP = NULL;
    char *hdtestResultTmpP = NULL;
    u_int32 errors = 0, writeXfer;
    u_int32 i = 0;

    /* allocate memory to get the outputs from hdtest */
    hdtestResultP = malloc(sizeof(char) * 100);
    if(hdtestResultP == NULL)
    {
        ST_AE52_ERR("hdtestResultP == NULL\n");
        return ERROR;
    }
    *hdtestResultP = 0;

    /* run hdtest in raw mode */
    sprintf(params, "-f=10000 -s=0 -r=0x%x /s0p0:1/testfile",
            hdtestResultP);
             
        do
        {
            ret = hdtest(params);
            /* read out the result from raw mode */
            sscanf(hdtestResultP, "-e=%d -w=%d -r=%d", &errors, &writeXfer, 
               &AE52SSDReadRate);
            dbg_AE52("E:%u W:%u R:%u\n", &errors, &writeXfer, &AE52SSDReadRate);
            if(ret)
                err++;
            else
                ok++;

            if(!quiet)
            {
                /* print only once per tick every AE52_SILENT_TICKS */
                if(tickGet() > (outtick + AE52_SILENT_TICKS))
                {
                  print = 1; 
                  outtick = tickGet();
                }else
                  print = 0; 
                /* in single mode output immediately */
                if(!mode)
                  print = 1; 
            }     

            /* have a break */
            OSS_Delay(NULL,1);

            if(print)
            {
              ST_AE52_PRINT("%s OK: %u ERR: %u\n",__FUNCTION__,ok,err);
              print = 0;
            }
        }while(mode);

    free(hdtestResultP);  
    return (int) !!err;
}

/* this test is not thread-save! (GPIOs are not saved/restored) */
int ll_AE52IoApiCpuTest(int mode, int quiet)
{
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;
    u_int32 outtick = tickGet();
    u_int8 in = 0;
    u_int8 out = 0;
    uint8_t miss = 0;
    int print = 0;

    /* init GPIOs */
    ret = AE52GPIOInit(quiet);
    if(ret)
    {
        return ret;

    }
    
    /* time to config pins */
    AE52_NOM_DELAY();

    do{
        /* check FPGA_DONE pin */
        gpioInputRead(G_vxbGpioDev,AE52_XM51_GPIO_PIN_FPGA_DONE,&in);
        dbg_AE52("FPGA_DONE %u\n", out, in);
        if(!in)
        {
            err_AE52("FPGA_DONE not set (0x%x)\n",
                      in);
                  miss = 1;
        }
        
       
        /* test API_OUT_0 */
        for(out=0;out<2;out++)
        {
            ml605_gpio_dat_set(
                  E_ML605_GPIO_0,
                  E_ML605_GPIO_C1,
                  out<<ML605_GPIO_0_BIT_APICPU_O
                  );
            AE52_NOM_DELAY();
            gpioInputRead(G_vxbGpioDev,AE52_XM51_GPIO_PIN_OUT,&in);
            dbg_AE52("O:%u I:%u\n", out, in);
            if(in != out)
            {
                err_AE52("API_CPU_OUT mismatch (0x%x!=0x%x)\n",
                          out,
                          in);
                miss = 1;
            }
        }
        if(miss)
          err++;
        else
          ok++;

        /* output only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
          print = 1; 
          outtick = tickGet();
        }else
          print = 0;
        /* in single mode output immediately */
        if(!mode)
          print = 1; 
        if(print)
        {
          ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
          print = 0;
        }   
    }while(mode);
      
  return (int) !!err;
}

#define AE52_P10_EEPROM_ADDR        0xA6
#define AE52_P10_EEPROM_TEST_SIZE   0x10
int ll_AE52P10EepromTest(int mode, int quiet)
{
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;
    u_int32 outtick = tickGet();
    u_int32 in = 0;
    u_int32 out = 0;
    uint8_t miss = 0;
    int print = 0;

    u_int8 smbdat1, smbdat2;
    u_int32 i;

    /* due to the ingenious "two byte" implementation of smbLib we proccess
    two bytes per loop */
    for(i = 0; i < AE52_P10_EEPROM_TEST_SIZE; i+=2)
    {
        /* read two byte */
        ret = sysSmbWriteReadTwoByte( XM51CFG_IIC_ONBOARD_1,
                AE52_P10_EEPROM_ADDR,
                i,
                &smbdat1,
                &smbdat2 );
        if(ret)
        {
            err_AE52("first read SMB reg %u  (0x%x)\n",i,ret);
            return ERROR;
        }
        dbg_AE52("first read %u=0x%x)\n",i,smbdat1);
        dbg_AE52("first read %u=0x%x)\n",i+1,smbdat2);
        AE52_NOM_DELAY();
        /* check and correct if neccessary */
        if(smbdat1 != (u_int8)i)
        {
            dbg_AE52("write %u=0x%x)\n",i,smbdat1);
            ret = sysSmbWriteTwoByte( XM51CFG_IIC_ONBOARD_1,
                                    AE52_P10_EEPROM_ADDR,
                                    i,
                                    i);
        }
        if(ret)
        {
            err_AE52("write SMB reg %u  (0x%x)\n",i,ret);
            return ERROR;
        }
        AE52_NOM_DELAY();
        if(smbdat2 != (u_int8)i+1)
        {
            dbg_AE52("write %u=0x%x)\n",i+1,smbdat2);
            ret = sysSmbWriteTwoByte( XM51CFG_IIC_ONBOARD_1,
                                    AE52_P10_EEPROM_ADDR,
                                    i+1,
                                    i+1);
        }
        AE52_NOM_DELAY();
        if(ret)
        {
            err_AE52("write SMB reg %u  (0x%x)\n",i+1,ret);
            return ERROR;
        }
    }
    
    /* now we can start the actual test */
    do{
        for(i = 0; i < AE52_P10_EEPROM_TEST_SIZE; i+=2)
        {
            /* read two byte */
            ret = sysSmbWriteReadTwoByte( XM51CFG_IIC_ONBOARD_1,
                    AE52_P10_EEPROM_ADDR,
                    i,
                    &smbdat1,
                    &smbdat2 );
            if(ret)
            {
                err_AE52("read SMB reg %u  (0x%x)\n",i,ret);
                miss++;
            }
            dbg_AE52("read %u=0x%x)\n",i,smbdat1);
            dbg_AE52("read %u=0x%x)\n",i+1,smbdat2);
            /* check */
            if(smbdat1 != (u_int8)i)
            {
                err_AE52("missmatch %u=0x%x)\n",i,smbdat1);
                err++;
            }else if(smbdat2 != (u_int8)i+1)
            {
                err_AE52("missmatch %u=0x%x)\n",i,smbdat2);
                err++;
            }else
                ok++;
            AE52_NOM_DELAY();
        }

        /* output only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
            print = 1; 
            outtick = tickGet();
        }else
            print = 0;
        /* in single mode output immediately */
        if(!mode)
            print = 1; 
        if(print)
        {
            ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
            print = 0;
        }   
    }while(mode);
        
    return (int) !!err;
}

int ll_AE52EeprodTest(int mode, int quiet)
{
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;
    u_int32 outtick = tickGet();
    u_int32 in = 0;
    u_int32 out = 0;
    uint8_t miss = 0;
    int print = 0;

    static EEPROD2 eeCPU, eeBrd;

    do{
        /* Read EEPROD */
        ret = sysReadEEPROD( &eeCPU,
                &eeBrd);
        switch(ret)
        {
            case 0:
                dbg_AE52("read OK\n"); 
                break;
            case 1:
                err_AE52("read EEPROD CPU (0x%x)\n",ret);
                break;
            case 2:
                err_AE52("read EEPROD AE52 (0x%x)\n",ret);
                break;                              
            default:
                err_AE52("read EEPROD (0x%x)\n",ret);
                break;              
        }

        if(!quiet)
        {   
            printf("\nCPU ");
            sysEEPROD_Print(&eeCPU);
            printf("\nAE52 ");
            sysEEPROD_Print(&eeBrd);
        }
        
        /* check */
        if(AE52EeprodCheck(&eeCPU))            
        {
            err_AE52("missmatch CPU\n");
            err++;
        }else
            ok++;
        AE52_NOM_DELAY();
        if(AE52EeprodCheck(&eeBrd))            
        {
            err_AE52("missmatch AE52\n");
            err++;
        }else
            ok++;

        OSS_Delay(NULL,500);
        
        /* output only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
            print = 1; 
            outtick = tickGet();
        }else
            print = 0;
        /* in single mode output immediately */
        if(!mode)
            print = 1; 
        if(print)
        {
            ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
            print = 0;
        }   
    }while(mode);
        
    return (int) !!err;
}


int ll_AE52RS232Test(int mode, int quiet)
{
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;
    u_int32 outtick = tickGet();
    int print = 0;

    char * tName;

    tName = taskName( taskIdSelf() );
	if( tName == NULL || !strncmp( "tShell0", tName, 7 ))
	{
		ST_AE52_ERR("This test must run from telnet.\n");
        return ERROR;
	}

    /* we brutally kill the shell task */
    taskDeleteForce(taskNameToId("tShell0"));

    do{
        /* Read EEPROD */
        ret = serTestRxTx("/tyCo/0","/tyCo/0");
        
        if(ret)            
        {
            err_AE52("serTestRxTx %d\n",ret);
            err++;
        }else
            ok++;

        OSS_Delay(NULL,50);
        
        /* output only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
            print = 1; 
            outtick = tickGet();
        }else
            print = 0;
        /* in single mode output immediately */
        if(!mode)
            print = 1; 
        if(print)
        {
            ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
            print = 0;
        }   
    }while(mode);
        
    return (int) !!err;
}


int ll_AE52EthTest(int mode, int quiet)
{
    int ret = 0;
    u_int32 ok = 0;
    u_int32 err = 0;
    u_int32 outtick = tickGet();
    int print = 0;

    do{
        /* Read EEPROD */
        ret = sysMenTimeGet(0,0);
        
        if(ret)            
        {
            err_AE52("sysMenTimeGet %d\n",ret);
            err++;
        }else
            ok++;

        OSS_Delay(NULL,1000);
        
        /* output only once per tick every AE52_SILENT_TICKS */
        if(tickGet() > (outtick + AE52_SILENT_TICKS))
        {
            print = 1; 
            outtick = tickGet();
        }else
            print = 0;
        /* in single mode output immediately */
        if(!mode)
            print = 1; 
        if(print)
        {
            ST_AE52_PRINT("%s OK: %lu ERR: %lu\n",__FUNCTION__,ok,err);
            print = 0;
        }   
    }while(mode);
        
    return (int) !!err;
}




/* 
#
# stress test functions 
# 
*/
void stressAE52IO( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
u_int32 delay = (100 - load) * sysClkRateGet() / 100;
u_int32 tick = 0;
int creat_tmp = 0;
u_int32 i = 0;
u_int8 k =0;
int fh = -1;


taskDelay( sysClkRateGet() + 5 * h->id );
h->startTick= tickGet();

while( h->enabled )
{
#ifdef _WRS_CONFIG_SMP                
h->cpu = vxCpuIndexGet();
#endif
#if 0   
    /* recalculate delay timing */
    if( h->load_act != load )
    {
        load = h->load_act;
        if( load == 99 )
        {
            delay = 1;
        }
        else
        {
            delay = (100 - load) * sysClkRateGet() / 100;
        }
    }
#endif
    /* were alive! */
    h->alive = 1;

    delay = 1;
    tick = tickGet();

    k = h->loopCnt + h->id;

    /**********************/
    /* Run test           */
    /**********************/


    /* BIT lamp */
    if(g_AE52_BitLampMode != AE51_BITLAMP_OFF)
    {
        if( ll_AE52IoBitLampTest(0,1) != OK)
            h->errCnt++;
        else
            h->bytenum += 1;
    }
    /* LRU fail */
#ifndef LRU_FAIL_DEBUG
    if( ll_AE52IoLRUFailTest(0,1) != OK)
        h->errCnt++;
    else
        h->bytenum += 1;
#else
#warning "No LRU Fail Test"
#endif
    /* BIT lamp */
    if(g_AE52_BitLampMode == AE51_BITLAMP_FAST)
    {
        if( ll_AE52IoBitLampTest(0,1) != OK)
            h->errCnt++;
        else
            h->bytenum += 1;
    }
    /* supression */
    if( ll_AE52IoSupressionTest(0,1) != OK )
        h->errCnt++;
    else
        h->bytenum += 1;
    /* BIT lamp */
    if(g_AE52_BitLampMode == AE51_BITLAMP_FAST)
    {
        if( ll_AE52IoBitLampTest(0,1) != OK)
            h->errCnt++;
        else
            h->bytenum += 1;
    }
    if( ll_AE52IoApiCpuTest(0,1) != OK )
        h->errCnt++;
    else
        h->bytenum += 1;

#ifdef STRESS_DELAY     
    taskDelay( delay );
#endif
    
    while( h->halt && h->enabled )
    {
        taskDelay(100);
    };

    h->loopCnt++;
}

h->isFinished = 1;

return;
}

void stressSysmon( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
u_int32 delay = (100 - load) * sysClkRateGet() / 100;
u_int32 tick = 0;
int creat_tmp = 0;
u_int32 i = 0;
u_int8 k =0;
int fh = -1;

taskDelay( sysClkRateGet() + 5 * h->id );
h->startTick= tickGet();

while( h->enabled )
{

#ifdef _WRS_CONFIG_SMP                
h->cpu = vxCpuIndexGet();
#endif

#if 0   
    /* recalculate delay timing */
    if( h->load_act != load )
    {
        load = h->load_act;
        if( load == 99 )
        {
            delay = 1;
        }
        else
        {
            delay = (100 - load) * sysClkRateGet() / 100;
        }
    }
#endif
    /* were alive! */
    h->alive = 1;

    delay = 1;
    tick = tickGet();

    k = h->loopCnt + h->id;

    /**********************/
    /* Run test           */
    /**********************/
    if( ll_AE52SysMonTest(0,1) != OK)
        h->errCnt++;
    else
        h->bytenum += 1;
    taskDelay(100);
    
    while( h->halt && h->enabled )
    {
        taskDelay(100);
    };

    h->loopCnt++;
}

h->isFinished = 1;

return;
}


void stressLRU( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
u_int32 delay = (100 - load) * sysClkRateGet() / 100;
u_int32 tick = 0;
int creat_tmp = 0;
u_int32 i = 0;
u_int8 k =0;
int fh = -1;

taskDelay( sysClkRateGet() + 5 * h->id );
h->startTick= tickGet();

while( h->enabled )
{

#ifdef _WRS_CONFIG_SMP                
    h->cpu = vxCpuIndexGet();
#endif

#if 0   
    /* recalculate delay timing */
    if( h->load_act != load )
    {
        load = h->load_act;
        if( load == 99 )
        {
            delay = 1;
        }
        else
        {
            delay = (100 - load) * sysClkRateGet() / 100;
        }
    }
#endif
    /* were alive! */
    h->alive = 1;

    delay = 1;
    tick = tickGet();

    k = h->loopCnt + h->id;

    /**********************/
    /* Run test           */
    /**********************/
    if( ll_AE52IoLRU58Test(0,1) != OK)
        h->errCnt++;
    else
        h->bytenum += 1;
#ifdef STRESS_DELAY     
    taskDelay( delay );
#endif
    
    while( h->halt && h->enabled )
    {
        taskDelay(100);
    };

    h->loopCnt++;
}

h->isFinished = 1;

return;
}

void stressSCL( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
u_int32 delay = (100 - load) * sysClkRateGet() / 100;
u_int32 tick = 0;
int creat_tmp = 0;
u_int32 i = 0;
int fh = -1;

taskDelay( sysClkRateGet() + 5 * h->id );
h->startTick = tickGet();

while( h->enabled )
{   

#ifdef _WRS_CONFIG_SMP                
        h->cpu = vxCpuIndexGet();
#endif

#if 0   
    /* recalculate delay timing */
    if( h->load_act != load )
    {
        load = h->load_act;
        if( load == 99 )
        {
            delay = 1;
        }
        else
        {
            delay = (100 - load) * sysClkRateGet() / 100;
        }
    }
#endif
    /* were alive! */
    h->alive = 1;

    delay = 1;
    tick = tickGet();

    /**********************/
    /* Run test           */
    /**********************/
    if( ll_AE52IoSclTest(0,1) != OK)
        h->errCnt++;
    else
        h->bytenum += 1;

#ifdef STRESS_DELAY             
    taskDelay( delay );
#endif
    
    while( h->halt && h->enabled )
    {
        taskDelay(100);
    };

    h->loopCnt++;
}

h->isFinished = 1;

return;
}

void stressAPI( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
u_int32 delay = (100 - load) * sysClkRateGet() / 100;
u_int32 tick = 0;
int creat_tmp = 0;
u_int32 i = 0;
u_int8 k =0;
int fh = -1;

taskDelay( sysClkRateGet() + 5 * h->id );
h->startTick= tickGet();

while( h->enabled )
{

#ifdef _WRS_CONFIG_SMP                
        h->cpu = vxCpuIndexGet();
#endif

#if 0   
    /* recalculate delay timing */
    if( h->load_act != load )
    {
        load = h->load_act;
        if( load == 99 )
        {
            delay = 1;
        }
        else
        {
            delay = (100 - load) * sysClkRateGet() / 100;
        }
    }
#endif
    /* were alive! */
    h->alive = 1;

    delay = 1;
    tick = tickGet();

    k = h->loopCnt + h->id;

    /**********************/
    /* Run test           */
    /**********************/
    if( ll_AE52IoApiTest(0,1) != OK)
        h->errCnt++;
    else
        h->bytenum += 1;
#ifdef STRESS_DELAY     
    taskDelay( delay );
#endif

    while( h->halt && h->enabled )
    {
        taskDelay(100);
    };

    h->loopCnt++;
}

h->isFinished = 1;

return;
}

#if 0
static void stressAE52Sdram( STRESS_TEST_HDL *hP, u_int32 dummy, u_int32 resvd )
{
STRESS_TEST_HDL *h = hP;
u_int32 delay = (100 - h->load_nom) * sysClkRateGet() / 100;
u_int32 tick = 0;
u_int32 *sdramP = NULL;
u_int32 i;
u_int32 k = 0;
u_int32 *tmpP = NULL;
u_int32 size = 0;
int     errorCount;
int     errorCountTotal = 0;


int ret = 0;
u_int32 ok = 0;
u_int32 err = 0;    
u_int32 outtick = tickGet();
int print = 0;

char params[50];
volatile u_int8 * const physStartAddr = AE52_FPGA_MEM_ADDR;
volatile u_int8 * const physEndAddr = AE52_FPGA_MEM_ADDR + AE52_FPGA_MEM_SIZE;
volatile u_int8 * startAddr = physStartAddr;
volatile u_int8 * endAddr;

u_int8 passes = 1;
u_int8 area = 0;

        
startAddr = physStartAddr;
if(g_AE52_slowtest)
    endAddr = physStartAddr + AE52_FPGA_MEM_CHUNKSIZE;
else
    endAddr = physStartAddr + 0x80000;

taskDelay( sysClkRateGet()  + 5 * h->id );

while( h->enabled )
{

#ifdef _WRS_CONFIG_SMP                
h->cpu = vxCpuIndexGet();
#endif

    /* were alive! */
    hP->alive = 1;
        

    /* set para string */
    sprintf(params, "-n=%d 0x%x 0x%x -t=bwlBWL -o=%d", 
                passes, 
                startAddr, 
                endAddr, 
                1);
    
    /* do mtest */
    ret = mtest(params);
                        
    if(g_AE52_slowtest)
        h->bytenum += AE52_FPGA_MEM_CHUNKSIZE;
    else
        h->bytenum +=  0x80000;

    /* check */
    if(ret)
    {
        stressDbg(NULL,STRESS_DBGLEV_1,"0x%x 0x%x\n",
            startAddr,
            endAddr);
        h->errCnt++;
    }
    else
    {
        stressDbg(NULL,STRESS_DBGLEV_3,"0x%x 0x%x OK\n",
            startAddr,
            endAddr);
        ok++;
    }

    startAddr += AE52_FPGA_MEM_CHUNKSIZE;
    startAddr = (startAddr >= physEndAddr) ? 0 : startAddr;
    if(g_AE52_slowtest)
        endAddr += AE52_FPGA_MEM_CHUNKSIZE;
    else
        endAddr = MIN(startAddr + 0x80000, physEndAddr); /* only 4k for fast results */
    
    h->loopCnt++;
    
    while( h->halt && h->enabled )
    {
        taskDelay(100);
    };
}

h->isFinished = 1; /* say finished to shutting down */

return;

}
#else
static void stressAE52Sdram( STRESS_TEST_HDL *hP, u_int32 dummy, u_int32 resvd )
{
STRESS_TEST_HDL *h = hP;
u_int32 delay = (100 - h->load_nom) * sysClkRateGet() / 100;
u_int32 tick = 0;
u_int32 *sdramP = NULL;
u_int32 i;
u_int32 k = 0;
u_int32 *tmpP = NULL;
u_int32 size = 0;
int     errorCount;
int     errorCountTotal = 0;

#ifdef _WRS_CONFIG_SMP
    h->cpu = vxCpuIndexGet();
#endif

taskDelay( sysClkRateGet()  + 5 * h->id );

size = AE52_FPGA_MEM_SIZE;
tmpP = (u_int32 *) AE52_FPGA_MEM_ADDR;

while( h->enabled )
{
#ifdef _WRS_CONFIG_SMP
    h->cpu = vxCpuIndexGet();
#endif

    
#if 0
    /* recalculate delay timing */
    if( h->load_act != h->load_nom )
    {
        h->load_act = h->load_nom;
        if( h->load_nom == 99 )
        {
            delay = 1;
        }
        else
        {
            delay = (100 - h->load_nom) * sysClkRateGet() / 100;
        }
    }
#endif

    tick = tickGet();

    /* write - linear pattern  - start value = loop count */

    k      = h->loopCnt;
    sdramP = tmpP;
    for( i=0; i < (size - 4);i += sizeof(u_int32) )
    {
        if( !(h->loopCnt % 13) )
            *sdramP = ~k; /* fill */
        else
            *sdramP = k; /* fill */
        sdramP++;
        k++;
        if(!(i%0x80000))
        {
            /* were alive! */
            hP->alive = 1;
            h->bytenum += 0x80000 << 2; /* long */
#ifdef STRESS_DELAY                     
            taskDelay( 1 ); /* be nice */
#endif
            if( !h->enabled )
                break;
        }
    }
    hP->alive = 1;
    /* read back */
    k      = h->loopCnt;
    sdramP = tmpP;
    for( i=0; i < (size - 4);i += sizeof(u_int32) )
    {
        int     verifyErr;
        u_int32 is; 
        u_int32 sb; 

        if( (h->bytenum ) <= 0 )
        {
            /* overflow occurred, clear startime to fix Xfer-Rate */
            h->startTick = tickGet();
        }

        is = *sdramP;
        if( !(h->loopCnt % 13) )
        {
            sb = ~k;
        }
        else
        {
            sb = k;
        }
        if( is != sb )
            verifyErr = 1;
        else
            verifyErr = 0;

        if( verifyErr )
        {
            err_AE52("***%s verify @%08x %08x/%08x %d;", h->name, sdramP, is, sb, h->loopCnt );
            stressDbg(NULL,STRESS_DBGLEV_1, "***%s verify @%08x %08x/%08x %d;", h->name, sdramP, is, sb, h->loopCnt );
            h->errCnt++;
        }

        sdramP++;
        k++;

        if(!(i%0x8000))
        {
            /* were alive! */
            hP->alive = 1;
            h->bytenum += 0x80000 << 2; /* long */
#ifdef STRESS_DELAY     
            taskDelay( 1 ); /* be nice */
#endif
            if( !h->enabled )
                break;
        }

        if(!(i%1000000))
        {
            stressDbg(NULL,STRESS_DBGLEV_3, "%s alive;",h->name);
        }
    }

#ifdef STRESS_DELAY 
    taskDelay( delay );
#endif
    
    while( h->halt && h->enabled )
    {
        /*printf("*** halt && enabled\n"); - for changing load ... */
        taskDelay(100);
    };

    h->loopCnt++;
}

/* printf( 
    \n>>> %s end after %d loops buff %08x size %08x errCnt %d errorCount %d\n", 
    h->name, 
    h->loopCnt, 
    tmpP, 
    size, 
    h->errCnt, 
    errorCountTotal );*/
h->isFinished = 1; /* say finished to shutting down */

return;
}
#endif

#if 0
void stressMIL( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
u_int32 delay = (100 - load) * sysClkRateGet() / 100;
u_int32 tick = 0;
int creat_tmp = 0;
u_int32 i = 0;
u_int8 k =0;
int fh = -1;

taskDelay( sysClkRateGet() + 5 * h->id );
h->startTick= tickGet();

#ifdef _WRS_CONFIG_SMP                
    h->cpu = vxCpuIndexGet();
#endif

while( h->enabled )
{
#ifdef _WRS_CONFIG_SMP                
    h->cpu = vxCpuIndexGet();
#endif

    /* were alive! */
    h->alive = 1;

    /**********************/
    /* Run test           */
    /**********************/
#if 0
    delay = sysClkRateGet();
    if( ll_AE52M1553Test(0,1) != OK)
#else
    delay = 0;
    if( MIL_looptest() != OK)
#endif
        h->errCnt++;
    else
        h->bytenum += 1;
    h->loopCnt++;
    taskDelay( 1 );
    
    while( h->halt && h->enabled )
    {
        taskDelay(100);
    };
}

h->isFinished = 1;

return;
}
#endif

#if 0
void stressMILRAM( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
u_int32 delay = (100 - load) * sysClkRateGet() / 100;
u_int32 tick = 0;
int creat_tmp = 0;
u_int32 i = 0;
u_int8 k =0;
int fh = -1;

taskDelay( sysClkRateGet() + 5 * h->id );
h->startTick= tickGet();

#ifdef _WRS_CONFIG_SMP                
    h->cpu = vxCpuIndexGet();
#endif

while( h->enabled )
{
#ifdef _WRS_CONFIG_SMP                
    h->cpu = vxCpuIndexGet();
#endif

    /* were alive! */
    h->alive = 1;

    /**********************/
    /* Run test           */
    /**********************/
#if 0
    delay = sysClkRateGet();
    if( ll_AE52M1553Test(0,1) != OK)
#else
    delay = 0;
    if( ll_AE52MilRamTest() != OK)
#endif
        h->errCnt++;
    else
        h->bytenum += 1;
    h->loopCnt++;
    taskDelay( 10 );
    
    while( h->halt && h->enabled )
    {
        taskDelay(100);
    };
}

h->isFinished = 1;

return;
}
#endif


static void stressMILRAM( STRESS_TEST_HDL *hP, u_int32 load, u_int32 resvd )
{
    STRESS_TEST_HDL *h = hP;
    u_int32 delay = (100 - load) * sysClkRateGet() / 100;
    u_int32 tick = 0;
    u_int32 *sdramP = NULL;
    u_int32 i;
    u_int32 k = 0;
    u_int32 * tmpP = AE52_MIL_MEM_ADDR;
    u_int32 size = AE52_MIL_MEM_SIZE;

#ifdef _WRS_CONFIG_SMP                
        h->cpu = vxCpuIndexGet();
#endif

    taskDelay( sysClkRateGet()  + 5 * h->id );

    sdramP = tmpP;

    while( h->enabled )
    {
        /* were alive! */
        h->alive = 1;
        
        /* recalculate delay timing */
        if( h->load_act!= load )
        {
            load = h->load_act;
            if( load == 99 )
            {
                delay = 1;
            }
            else
            {
                delay = (100 - load) * sysClkRateGet() / 100;
            }
        }
        tick = tickGet();

        k = h->loopCnt;
        sdramP = tmpP;
        for( i=0; i < (size - 4);i += sizeof(u_int32) )
        {
            if( !(h->loopCnt % 13) )
                *sdramP = ~k; /* fill */
            else
                *sdramP = k; /* fill */
            sdramP++;
            k++;
            if(!(i%0x8000))
            {
                h->bytenum += 0x8000 << 2; /* long */
                taskDelay( 1 ); /* be nice */
                if( !h->enabled )
                    break;
            }
        }
        
        /* were alive! */
        h->alive = 1;

        /* read back */
        k = h->loopCnt;
        sdramP = tmpP;
        for( i=0; i < (size - 4);i += sizeof(u_int32) )
        {
            int     verifyErr;
            u_int32 is; 
            u_int32 sb; 

            if( (h->bytenum ) <= 0 )
            {
                /* overflow occurred, clear startime to fix Xfer-Rate */
                h->startTick = tickGet();
            }

            is = *sdramP;
            if( !(h->loopCnt % 13) )
            {
                sb = ~k;
            }
            else
            {
                sb = k;
            }
            if( is != sb )
                verifyErr = 1;
            else
                verifyErr = 0;

            if( verifyErr )
            {
                stressDbg(NULL,STRESS_DBGLEV_1, "*** %s verify @%08x %08x/%08x %d;", h->name,sdramP, is, sb, h->loopCnt );
                h->errCnt++;
            }

            sdramP++;
            k++;

            if(!(i%0x8000))
            {
                h->bytenum += 0x8000 << 2; /* long */
                taskDelay( 1 ); /* be nice */
                if( !h->enabled )
                    break;
            }

            if(!(i%10000000))
            {
                stressDbg(NULL,STRESS_DBGLEV_3, "%s alive;",h->name);
                /* restart measurement */
                tick = tickGet();
            }
        }
        
        taskDelay( delay );
        
        while( h->halt && h->enabled )
        {
            taskDelay(1);
        };

        h->loopCnt++;
    }

    h->isFinished = 1; /* say finished to shutting down */

    return;
}



void stressSata( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
u_int32 delay = (100 - load) * sysClkRateGet() / 100;
u_int32 tick = 0;
int creat_tmp = 0;
u_int32 i = 0;
u_int8 k =0;
int fh = -1;

taskDelay( sysClkRateGet() + 5 * h->id );
h->startTick= tickGet();

#ifdef _WRS_CONFIG_SMP                
    h->cpu = vxCpuIndexGet();
#endif

while( h->enabled )
{
#ifdef _WRS_CONFIG_SMP                
    h->cpu = vxCpuIndexGet();
#endif

    /* were alive! */
    h->alive = 1;
    k = h->loopCnt + h->id;

    /**********************/
    /* Run test           */
    /**********************/
    if( ll_AE52SSDTest(0,1) != OK)
    {
        h->errCnt++;
    }
    else
        h->bytenum++;
    h->loopCnt++;
    taskDelay( delay );
    
    while( h->halt && h->enabled )
    {
        taskDelay(100);
    };
}

h->isFinished = 1;

return;
}

void stressEeprom( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
   u_int32 delay = (100 - load) * sysClkRateGet() / 100;
   u_int32 tick = 0;
   int creat_tmp = 0;
   u_int32 i = 0;
   u_int8 k =0;
   int fh = -1;
   
   taskDelay( sysClkRateGet() + 5 * h->id );
   h->startTick= tickGet();
   
#ifdef _WRS_CONFIG_SMP                
       h->cpu = vxCpuIndexGet();
#endif
   
   while( h->enabled )
   {
#ifdef _WRS_CONFIG_SMP                
       h->cpu = vxCpuIndexGet();
#endif
   
       /* were alive! */
       h->alive = 1;
       k = h->loopCnt + h->id;
   
       /**********************/
       /* Run test           */
       /**********************/
    if( ll_AE52EeprodTest(0,1) != OK)
          h->errCnt++;
    else
      h->bytenum += 1;
    taskDelay(100);
    if( ll_AE52P10EepromTest(0,1) != OK)
      h->errCnt++;
    else
      h->bytenum += 1;
    h->loopCnt++;
    taskDelay(100);
       
    while( h->halt && h->enabled )
    {
       taskDelay(100);
    };
   }
   
   h->isFinished = 1;
   
   return;
}


void stressRS232( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
   u_int32 delay = (100 - load) * sysClkRateGet() / 100;
   u_int32 tick = 0;
   int creat_tmp = 0;
   u_int32 i = 0;
   u_int8 k =0;
   int fh = -1;
   
   taskDelay( sysClkRateGet() + 5 * h->id );
   h->startTick= tickGet();
   
#ifdef _WRS_CONFIG_SMP                
       h->cpu = vxCpuIndexGet();
#endif
   
   while( h->enabled )
   {
#ifdef _WRS_CONFIG_SMP                
       h->cpu = vxCpuIndexGet();
#endif
   
        /* were alive! */
        h->alive = 1;
        k = h->loopCnt + h->id;

        /**********************/
        /* Run test           */
        /**********************/
        if( ll_AE52RS232Test(0,1) != OK)
            h->errCnt++;
        else
            h->bytenum += 1;
        h->loopCnt++;
        taskDelay(10);

        while( h->halt && h->enabled )
        {
           taskDelay(100);
        };
   }
   h->isFinished = 1;
   
   return;
}


void stressEth( STRESS_TEST_HDL *h, u_int32 load, u_int8 netId )
{
   u_int32 delay = (100 - load) * sysClkRateGet() / 100;
   u_int32 tick = 0;
   int creat_tmp = 0;
   u_int32 i = 0;
   u_int8 k =0;
   int fh = -1;
   
   taskDelay( sysClkRateGet() + 5 * h->id );
   h->startTick= tickGet();
   
#ifdef _WRS_CONFIG_SMP                
       h->cpu = vxCpuIndexGet();
#endif
   
   while( h->enabled )
   {
#ifdef _WRS_CONFIG_SMP                
        h->cpu = vxCpuIndexGet();
#endif

        /* were alive! */
        h->alive = 1;
        k = h->loopCnt + h->id;

        /**********************/
        /* Run test           */
        /**********************/
        if( ll_AE52EthTest(0,1) != OK)
            h->errCnt++;
        else
            h->bytenum += 1;
        h->loopCnt++;
        taskDelay(10);

        while( h->halt && h->enabled )
        {
           taskDelay(100);
        };
   }
   
   h->isFinished = 1;
   
   return;
}




