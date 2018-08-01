/*
****************************************************************************
*
*       Author: mkolpak $
*        $Date: 2009/04/09 15:02:23 $
*    $Revision: 1.2 $
*
*  Description: This is a tool to program the onboard EEPROM of 15P511 - 
*               Dual Fast Ethernet PMC. 
*
*
*     Required:
*     Switches:
*
*-------------------------------[ History ]---------------------------------
*
* $Log: p511Prog.c,v $
* Revision 1.2  2009/04/09 15:02:23  MKolpak
* R: Copmiler error due to missing declarations
* M: Variable declarations added
*
* Revision 1.1  2009/04/07 11:57:02  MKolpak
* Initial Revision
*
*
*
*---------------------------------------------------------------------------
* (c) Copyright 2009 by MEN mikro elektronik GmbH, Nuernberg, Germany
****************************************************************************/


/* includes */

#include "vxWorks.h"
#include "version.h"
#include "stdlib.h"
#include "cacheLib.h"
#include "intLib.h"
#include "iv.h"
#include "end.h"            /* Common END structures. */
#include "endLib.h"
#include "stdio.h"
#include "sysLib.h"
#include "drv/pci/pciConfigLib.h"
#include "miiLib.h"         /* PHY regs */
#include "sys/times.h"
#include "types/vxCpu.h"
#include <MEN/men_typs.h>
#include <MEN/dbg.h>
#include <MEN/men_id.h>
#include <MEN/oss.h>
#include <MEN/chameleon.h>
#include "MEN/eeprod.h"
#include <MEN/smb2.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>

#include "z077End.h"


/* must be init with p511Init(<bus>,<device>,<function>)  */
static SMB_DESC_MENZ001 LZ1desc; 
static SMB_HANDLE *     LpZ1hdl;


/* Z001 instance # */
#define P511_Z001_INST  0

/* name len */
#define P511_NAME_LEN  5


STATUS p511Init(UINT32 ,UINT32 ,UINT32);
STATUS p511Write(
            UINT32 ser, 
            char * name, 
            UINT32 model, 
            UINT32 revA, 
            UINT32 revB, 
            UINT32 revC, 
            UINT32 prodDD, 
            UINT32 prodMM, 
            UINT32 prodYYYY, 
            UINT32 repDD, 
            UINT32 repMM, 
            UINT32 repYYYY);
STATUS p511Erase(void );
STATUS p511Print(void );
static void version(void);

static int usage(void)
{
    fprintf(stderr,"%s%s%s%s%s%s%s%s%s%s%s%s\n",
    "Syntax  : p511Prog <paras>\n",
    "Function: program EEPROD structure on 15P511\n",
    "   -b=<num>     PCI bus # of P511\n",
    "   -d=<num>     PCI dev. # of P511\n",
    "   -f=<num>     PCI func. # of P511\n",
    "   -s=<num>     serial #\n",
    "   -v=<str>     revision # (A.B.C)\n",
    "   -n=<str>     name (P511)\n",
    "   -m=<num>     model #\n",
    "   -p=<str>     production date (DD.MM.YYYY)\n",
    "   -r=<str>     repair date (DD.MM.YYYY)\n",
    "   -?           print this help\n"
    );
    version();
    return ERROR;
}

static void version(void)
{
    char *rev = "$Revision: 1.2 $";
    char *p = rev+strlen("$Revision: ");

    fprintf(stderr,"\nV ");
    while( *p != '$' ) fprintf(stderr, "%c", *p++);
    fprintf(stderr," (c) Copyright 1995-2009 by MEN GmbH\n");
}

int main( int argc, char *argv[] )
{
    char *errstr, *optp, *name = NULL, errbuf[40];
    UINT32 bus = 0, device = 0, function = 0;
    UINT32 ser = 0, model = 0, revA = 0, revB = 0, revC = 0;
    UINT32 prodDD = 0, prodMM = 0, prodYYYY = 0, repDD = 0, repMM = 0, repYYYY = 0;

    fprintf(stdout,
            "\nP511Prog\t(build: %s %s)\n"
            "=============================================\n\n",
              __DATE__, 
              __TIME__ ); 



    /* check paras */
    if ((errstr = UTL_ILLIOPT("?b=d=f=m=n=p=r=s=v=", errbuf))) {
        fprintf(stderr,"*** %s\n",errstr);
        return ERROR;
    }
    
    /* get paras */
    if(UTL_TSTOPT("?")) {
        usage();
        return ERROR;
    }
    /* PCI */
    if((optp = UTL_TSTOPT("b="))) bus = atoi(optp);
    if((optp = UTL_TSTOPT("d="))) device = atoi(optp);
    if((optp = UTL_TSTOPT("f="))) function = atoi(optp);
    /* stuff */    
    if((optp = UTL_TSTOPT("s="))) ser = atoi(optp);
    if((optp = UTL_TSTOPT("m="))) model = atoi(optp);
    /* rev */
    if((optp = UTL_TSTOPT("v=")))
    {
        if(!sscanf(optp,"%u.%u.%u",&revA,&revB,&revC))
            printf("Error rev. %s\n",optp);
    }
    /* prod */
    if((optp = UTL_TSTOPT("p=")))
    {
        if(!sscanf(optp,"%u.%u.%u",&prodDD,&prodMM,&prodYYYY))
            printf("Error prod. %s\n",optp);
    }
    /* rep */
    if((optp = UTL_TSTOPT("r=")))
    {
        if(!sscanf(optp,"%u.%u.%u",&repDD,&repMM,&repYYYY))
            printf("Error rep. %s\n",optp);/* dbg*/
    }
    /* name */
    if((optp = UTL_TSTOPT("n=")))
    {
        name = optp;
    }

    OSS_VxMikroDelayReInit(OSS_VXWORKS_OS_HDL);
    
    if(p511Init(bus, device, function))
        return ERROR;
    
    if(p511Write(
            ser,
            name,
            model,
            revA,
            revB,
            revC,
            prodDD,
            prodMM,
            prodYYYY,
            repDD,
            repMM,
            repYYYY))
        return ERROR;

    if(p511Print())
        return ERROR;

    return OK;
}


STATUS p511Init(UINT32 pci_bus ,UINT32 pci_device ,UINT32 pci_function)
{
    Z077_PCI_DESC pciDesc = {0};
    CHAMELEONV2_HANDLE * chamHdl = NULL;
    CHAMELEONV2_UNIT chamUnit = {0};
    INT8 inst = P511_Z001_INST;
    EEPROD2 EEPR;
    UINT32 err = 0;

    printf("Init Z01_SMB core...");

    if(z077ModInit("p511prog"))
    {
        printf("\n#Error - ModInit!\n");
        return ERROR;
    }

    bzero((char*) &EEPR, sizeof(EEPROD2));

    pciDesc.busNo = pci_bus;
    pciDesc.deviceNo = pci_device;
    pciDesc.funcNo = pci_function;
    
    if(z077CheckChameleonDevice(&pciDesc))
    {
        printf("\n#Error - No chameleon pci_device found at %u/%u/%u!\n",
            pci_bus,
            pci_device,
            pci_function);
        return ERROR;
    }
    
    if(z077InitCham(&pciDesc, &chamHdl))
    {
        printf("\n#Error - Could not init. chameleon device at %u/%u/%u!\n",
            pci_bus,
            pci_device,
            pci_function);
          return ERROR;
    }

    if(z077FindZxCore(
            &inst, 
            Z077_CHAMID_16Z001_SMB,
            &chamHdl,
            &chamUnit))
    {
        printf("\n#Error - Could not find Z01_SMB core in chamelon device!\n");
        return ERROR;
    }
    
    if(z077_P511InitZ1(
            (UINT32 * ) &chamUnit.addr,
            &LZ1desc,
            &LpZ1hdl))
    {
        printf("\n#Error - Could not init. Z01_SMB core in chamelon device!\n");
        return ERROR;
    }
    OSS_Delay(OSS_VXWORKS_OS_HDL,1);
    if((err = z077_P511ReadEEPROM(&LZ1desc, LpZ1hdl, &EEPR)) != OK)
    {
        printf("\n#Error - Could not read eeprom (err=%u)!\n", err);
        return err;
    }    

    if(z077_P511EEPROD_Check(&EEPR))
    {
        printf("\n#Warning - Actual eeprod not valid (see DBG_Show)!\n");     
    }
    printf("done\n");
    return OK;
}


STATUS p511Write(
            UINT32 ser, 
            char * name, 
            UINT32 model, 
            UINT32 revA, 
            UINT32 revB, 
            UINT32 revC, 
            UINT32 prodDD, 
            UINT32 prodMM, 
            UINT32 prodYYYY, 
            UINT32 repDD, 
            UINT32 repMM, 
            UINT32 repYYYY)
{   
    int err;
    EEPROD2 EEPR;

    printf("Writing EEPROD structure...");

    if(LpZ1hdl == NULL)
    {
        printf("\n#Error - Call p511Init() first!\n");
        return ERROR;
    }   
    
    bzero((char*) &EEPR, sizeof(EEPROD2));

    /* serial */
    if(ser >= 0xFFF)
        printf("\n#Warning - S/N > 4095 (MAC address roll-over)\n");
    if(!ser)
    {
        printf("\n#Error -  S/N = 0\n");
        return ERROR;
    }
    /* name */
    if(name == NULL)
    {
        printf("\n#Error - name: NULL\n");
        return ERROR;
    }
    if((name[0] == 0) ||
      (strlen(name) > P511_NAME_LEN))
    {
        printf("\n#Error - name: \"%s\"\n", name);
        return ERROR;
    }
    /* model */
    if(model > 0xff)
    {
        printf("\n#Error - model > %u\n", 0xff);
        return ERROR;
    }
    /* revA */
    if(revA > 0xff)
    {
        printf("\n#Error - revision A > %u\n", 0xff);
        return ERROR;
    }
    /* revB */
    if(revB > 0xff)
    {
        printf("\n#Error - revision B > %u\n", 0xff);
        return ERROR;
    }
    /* revC */
    if(revC > 0xff)
    {
        printf("\n#Error - revision C > %u\n", 0xff);
        return ERROR;
    }
    /* prodDD */
    if(!prodDD || (prodDD > 31))
    {
        printf("\n#Error - prod. day = %u\n", prodDD);
        return ERROR;
    }
    /* prodMM */
    if(!prodMM || (prodMM > 12))
    {
        printf("\n#Error - prod. month = %u\n", prodMM);
        return ERROR;
    }
    /* prodYYY */
    if(!prodYYYY || (prodYYYY < EEPROD2_DATE_YEAR_BIAS))
    {
        printf("\n#Error - prod. year = %u\n", prodYYYY);
        return ERROR;
    }
    /* repDD */
    if(repDD > 31)
    {
        printf("\n#Error - rep. day = %u\n", repDD);
        return ERROR;
    }
    /* repMM */
    if((repMM > 12))
    {
        printf("\n#Error - rep. month = %u\n", repMM);
        return ERROR;
    }

    /* set struct */
    EEPR.pd_revision[0] = revA;
    EEPR.pd_revision[1] = revB;
    EEPR.pd_revision[2] = revC;
    EEPR.pd_serial = ser;
    strcpy(EEPR.pd_hwName,name);
    EEPR.pd_model = model;

    /* proDat */
    EEPR.pd_prodat |= prodDD & 0x1F;
    EEPR.pd_prodat |= ((UINT16) prodMM & 0x000F) << 5;
    prodYYYY -= EEPROD2_DATE_YEAR_BIAS;    
    EEPR.pd_prodat |= ((UINT16) prodYYYY & 0x007F) << 9;

    /* RepDat */
    EEPR.pd_repdat |= repDD & 0x1F;
    EEPR.pd_repdat |= ((UINT16) repMM & 0x000F) << 5;
    repYYYY = (repYYYY < EEPROD2_DATE_YEAR_BIAS) ? 0 : (repYYYY - EEPROD2_DATE_YEAR_BIAS);
    EEPR.pd_repdat |= ((UINT16) repYYYY & 0x007F) << 9;

    if((err = z077_P511WriteEEPROM(&LZ1desc, LpZ1hdl, &EEPR)) != OK)
    {
        printf("\n#Error - Could not write eeprom (err=%u)!\n", err);
        return err;
    }
    printf("done\n\n");
    return OK;
}

STATUS p511Erase(void )
{   
    EEPROD2 EEPR;

    if(LpZ1hdl == NULL)
    {
        printf("\n#Error - Call p511Init() first!\n");
        return ERROR;
    } 

    bfill((char*) &EEPR, sizeof(EEPROD2), 0xffffffff);
    return z077_P511WriteEEPROM(&LZ1desc, LpZ1hdl, &EEPR);
}

STATUS p511Print(void )
{   
    EEPROD2 EEPR;
    int err = 0;
    UINT8 day = 0, month = 0;
    UINT16 year = 0;

    printf("Reading EEPROD data from P511\n");

    if(LpZ1hdl == NULL)
    {
        printf("\n#Error - Call p511Init() first!\n");
        return ERROR;
    }     

    bzero((char*) &EEPR, sizeof(EEPROD2));
    if((err = z077_P511ReadEEPROM(&LZ1desc, LpZ1hdl, &EEPR)) != OK)
    {
        printf("\n#Error - Could not read eeprom (err=%u)!\n", err);     
        return err;
    }

    printf( "\nP511 EEPROD:\n\n"
            "\tEEPROD-ID    = 0x%X\n"           
            "\tHW-Name      = %s-%02u\n"           
            "\tS/N          = %06u\n"           
            "\tRevision     = %02u.%02u.%02u\n",  
            (EEPR.pd_id >> 4),
            EEPR.pd_hwName,
            EEPR.pd_model,
            EEPR.pd_serial,
            EEPR.pd_revision[0],
            EEPR.pd_revision[1],
            EEPR.pd_revision[2]);

    /* ProDat */
    day = EEPR.pd_prodat & 0x001F;
    month = (EEPR.pd_prodat >> 5)  & 0x000F;
    year = (EEPR.pd_prodat >> 9)  & 0x007F;
    year += EEPROD2_DATE_YEAR_BIAS;
    printf("\tProDat       = %02u.%02u.%04u\n", day, month, year);
    /* RepDat */
    day = EEPR.pd_repdat& 0x001F;
    month = (EEPR.pd_repdat>> 5)  & 0x000F;
    year = (EEPR.pd_repdat>> 9)  & 0x007F;
    year += EEPROD2_DATE_YEAR_BIAS;
    printf("\tRepDat       = %02u.%02u.%04u\n", day, month, year);

    return OK;
    
}



