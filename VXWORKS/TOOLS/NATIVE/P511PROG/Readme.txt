
P511Prog	 (c) Copyright 1995-2009 by MEN GmbH
================================================

	Author: mkolpak 
    $Date: 2009/04/07 11:56:54 $
    $Revision: 1.1 $

-------------------------------[ History ]---------------------------------

 $Log: Readme.txt,v $
 Revision 1.1  2009/04/07 11:56:54  MKolpak
 Initial Revision


---------------------------------------------------------------------------

FILE LIST:

    p511Prog.c

DESCRIPTION:

    This is a tool to program the onboard EEPROM of 15P511 - Dual Fast 
    Ethernet PMC. 
    
    Use it with a BSP which includes the z077End.c driver version > 1.18!

EXAMPLE COMPILE LINE:

    mk x86
    
TESTED ON:  

    OS:     VxWorks 6.2 / 6.4
    HW:     02D006 / 02F012 SBC

SYNTAX:

    -> p511prog "-?"

    P511Prog        (build: Apr  2 2009 16:31:06)
    =============================================
    
    Syntax  : p511Prog <paras>
    Function: program EEPROD structure on 15P511
       -b=<num>     PCI bus # of P511
       -d=<num>     PCI dev. # of P511
       -f=<num>     PCI func. # of P511
       -s=<num>     serial #
       -v=<str>     revision # (A.B.C)
       -n=<str>     name (P511)
       -m=<num>     model #
       -p=<str>     production date (DD.MM.YYYY)
       -r=<str>     repair date (DD.MM.YYYY)
       -?           print this help
       
EXAMPLE OUTPUT:
    
    -> ld < ../p511Prog.o
    value = 76395052 = 0x48db22c
    -> p511prog "-b=8 -d=5 -f=0 -s=42 -m=1 -v=1.2.3 -p=1.2.2004 -r=0.0.0 -n=P511"
    
    P511Prog        (build: Apr  2 2009 16:31:06)
    =============================================
    
    Init Z01_SMB core...done
    Writing EEPROD structure...done
    
    Reading EEPROD data from P511
    
    P511 EEPROD:
    
            EEPROD-ID    = 0xE
            HW-Name      = P511-01
            S/N          = 000042
            Revision     = 01.02.03
            ProDat       = 01.02.2004
            RepDat       = 00.00.1990
    value = 0 = 0x0

    
