/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: oxeprom.c
 *      Project: P10 VxWorks Driver
 *
 *       Author: rl
 *        $Date: 2003/04/22 13:05:21 $
 *    $Revision: 1.2 $
 *
 *  Description: microwire bus protocol and P10 eeprom reader/writer
 *				 
 *     Required: -
 *     Switches: -
 *
 *		   Note: 
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: oxprom.c,v $
 * Revision 1.2  2003/04/22 13:05:21  UFranke
 * bugfix
 *   -address cast to UINT16 removed
 *
 * Revision 1.1  2000/03/14 14:34:59  loesel
 * Initial Revision
 *---------------------------------------------------------------------------
 * (c) Copyright 2000..2003 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
#include "vxWorks.h"
/*#include "config.h"
*/
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "memLib.h"
#include "sysLib.h"
#include "tickLib.h"
#include "taskLib.h"
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciIntLib.h"
#include "oxprom.h"
#include "MEN/men_typs.h"

/*-----------------------------------------+
|  TYPEDEFS                                |
+------------------------------------------*/


/*-----------------------------------------+
|  DEFINES & CONST                         |
+------------------------------------------*/
/*--- instructions for serial EEPROM ---*/
#define     _READ_   0x80    /* read data */
#define     EWEN    0x30    /* enable erase/write state */
#define     ERASE   0xc0    /* erase cell */
#define     _WRITE_  0x40    /* write data */
#define     ERAL    0x20    /* chip erase */
#define     WRAL    0x10    /* chip write */
#define     EWDS    0x00    /* disable erase/write state */

#define     T_WP    100   /* max. time required for write/erase (us) */
#define		DELAY	20	                /* m_clock's delay time */
/* bit definition */
#define B_DAT_IN	0x08				/* data input on OX16C950 */
#define B_DAT_OUT	0x04				/* data output on OX16C950 */
#define B_CLK		0x01				/* clock				*/
#define B_SEL		0x02				/* chip-select			*/

#ifndef CPU_PCI_IO_ADRS
	#if ((CPU == I80486) | (CPU == I80386) | (CPU == PENTIUM))
		#define CPU_PCI_IO_ADRS		0
	#else	
		#define CPU_PCI_IO_ADRS     0x80000000               /* base of PCI I/O addr */
	#endif
#endif /* CPU_PCI_IO_ADRS */
/*-----------------------------------------+
|  GLOBALS                                 |
+------------------------------------------*/
LOCAL UINT16 p10eedata [] =
    {
    0x9505,
    0x8800,
    0x8900,
    0x8a00,
    0x8b00,
    0x8d00,
    0x8e40,
    0x9e0f,
    0x1f00,
    0x8000,
    0x8600,
    0x8902,
    0xae15,
    0x2f14,
    0x8001,
    0x8211,
    0x8600,
    0x8900,
    0x8a80,
    0x8b06,
    0x3d02,
    0x0000
    };
/*-----------------------------------------+
|  STATICS                                 |
+------------------------------------------*/
/*-----------------------------------------+
|  PROTOTYPES                              |
+------------------------------------------*/

UINT16 m_read( int base, UINT8 index, int freq );


/******************************* _delay ******************************
 *
 *  Description:  Delay (at least) one microsecond
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  -
 *  Globals....:  ---
 ***************************************************************************/
static void _delay( void )
{
    volatile i,n;

    for(i=DELAY; i>0; i--)
        n=10*10;
}
/************************************* m_delay ********************************
 *  Description:  m_delay	for read / write cycles.
 *
 *		   Note:  busClock descriptor value
 *					 0 - max speed - no m_delay
 *					 1 -  1kHz  delay 1ms + 
 *					10 - 10kHz  delay 0.1ms
 *
 *---------------------------------------------------------------------------
 *  Input......:  mcrwHdl	pointer to mcrw handle
 *  Output.....:  -
 *  Globals....:  -
 ****************************************************************************/
static void m_delay
(
	UINT freq
)
{
	int ticksOld;

	switch( freq )
	{
		case 1:  /* 1kHz max */
			ticksOld = sysClkRateGet();
			sysClkRateSet (1000);			 /* 1 [ms] */
			taskDelay(1);
			sysClkRateSet (ticksOld);
			break;
		case 10: /* 10kHz max */
			_delay();
			break;
		default: /* max speed */
			break;
	}/*switch*/
}/*delay*/


/*----------------------------------------------------------------------
 * LOW-LEVEL ROUTINES FOR SERIAL EEPROM
 *--------------------------------------------------------------------*/

/******************************* _select ************************************
 *
 *  Description:  Select EEPROM:
 *                 output DI/CLK/CS low
 *                 m_delay
 *                 output CS high
 *                 m_delay
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  -
 *  Globals....:  ---
 ***************************************************************************/
static void _select( int base, int freq )
{
    sysOutByte ( base, (UINT8)(0x00));			/* everything inactive */
    m_delay (freq);
    
    sysOutByte ( base, (UINT8)(B_SEL | 0x00));	/* activate chip select */
    m_delay (freq);
    
}

/******************************* _deselect **********************************
 *
 *  Description:  Deselect EEPROM
 *                 output CS low
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  -
 *  Globals....:  ---
 ***************************************************************************/
static void _deselect( int base )
{
    sysOutByte ( base, 0x00);			/* everything inactive */
}



/******************************* _clock ************************************
 *
 *  Description:  Output data bit:
 *                 output clock low
 *                 output data bit
 *                 m_delay
 *                 output clock high
 *                 m_delay
 *                 return state of data serial eeprom's DO - line
 *                 (Note: keep CS asserted)
 *---------------------------------------------------------------------------
 *  Input......:  data bit to send
 *  Output.....:  state of DO line
 *  Globals....:  ---
 ***************************************************************************/
static int _clock( int base, UINT8 dbs, int freq )
{
    sysOutByte ( base, (dbs<<2) |B_SEL );  /* output clock low */
                                            /* output data high/low */
    m_delay(freq);                               /* m_delay    */
	

    sysOutByte ( base, (dbs<<2)|B_CLK|B_SEL );  /* output clock high */
    m_delay(freq);                               /* m_delay    */
	
	
    return( ((sysInByte( base ) & B_DAT_IN)>>3) & 0x01 );  /* get data */
}


/*----------------------------------------------------------------------
 * HIGH-LEVEL ROUTINES FOR SERIAL EEPROM
 *--------------------------------------------------------------------*/
/******************************* _opcode ************************************
 *
 *  Description:  Output opcode with leading startbit
 *
 *---------------------------------------------------------------------------
 *  Input......:  code     opcode to write
 *  Output.....:  -
 *  Globals....:  ---
 ***************************************************************************/
void _opcode( int base, UINT8 code, int freq )
{
    register int i;

    _select( base, freq );
    _clock( base, 1, freq );                         /* output start bit */

    for(i=7; i>=0; i--)
        _clock( base, (UINT8)((code>>i)&0x01), freq );        /* output instruction code  */
}




/******************************* _write ************************************
 *
 *  Description:  Write a specified word into EEPROM at 'base'.
 *
 *---------------------------------------------------------------------------
 *  Input......:  index    index to write (0..63)
 *                data     word to write
 *  Output.....:  return   0=ok 1=write err 2=verify err
 *  Globals....:  ---
 ***************************************************************************/
int _write( int base, UINT8 index, UINT16 data, int freq )
{
    register int    i,j;                    /* counters     */

	/*printf ("\nwrite enable\n");*/
    _opcode( base, EWEN, freq);                     /* write enable */
    _deselect( base );                        /* deselect     */

	/*printf ("write select\n");*/
    _opcode( base, (UINT8)(_WRITE_+index), freq );             /* select write */
    for(i=15; i>=0; i--)
        _clock( base, (UINT8)((data>>i)&0x01), freq );        /* write data   */
    _deselect( base );                        /* deselect     */

    _select( base, freq );
    
    /*printf ("wait for low\n");*/
    for(i=T_WP; i>0; i--)                   /* wait for low */
    {   if(!_clock( base, 0, freq ))
            break;
        m_delay(freq);
    }
    
    /*printf ("wait for high\n");*/
    for(j=T_WP; j>0; j--)                   /* wait for high*/
    {   if(_clock( base, 0, freq ))
            break;
        m_delay(freq);
    }

    _opcode( base, EWDS, freq );                    /* write disable*/
    _deselect( base );                        /* disable      */

    
    if( data != m_read( base, index, freq ) )        /* verify data  */
        return MCRW_ERR_WRITE_VERIFY;                           /* ..error      */

    return 0;                               /* ..no         */
}



/******************************* _erase ************************************
 *
 *  Description:  Erase a specified word into EEPROM
 *
 *---------------------------------------------------------------------------
 *  Input......:  index    index to write (0..15)
 *  Output.....:  return   0=ok 1=error
 *  Globals....:  ---
 ***************************************************************************/
static int _erase( int base, UINT8 index, int freq )
{
    register int    i,j;                    /* counters     */

	/*printf ("\nerase enable\n");*/
    _opcode( base, EWEN, freq );					/* erase enable */
    for(i=0;i<4;i++) _clock( base, 0, freq );
    _deselect( base );                        		/* deselect     */
	

	/*printf ("erase select\n");*/
    _opcode( base, (UINT8)(ERASE+index), freq );	/* select erase */
    _deselect( base );								/* deselect     */
	
	
	/*printf ("wait for low\n");*/
    _select( base, freq );
    for(i=T_WP; i>0; i--)							/* wait for low */
    {   
    	if(!_clock( base, 0, freq ))
            break;
        m_delay(freq);
       
    }

	/*printf ("wait for high\n");*/
    for(j=T_WP; j>0; j--)							/* wait for high*/
    {   
    	if(_clock( base, 0, freq ))
            break;
		m_delay(freq);
	
    }
	
	/*printf ("erase disable\n");*/
    _opcode( base, EWDS, freq );			/* erase disable*/
    _deselect( base );								/* disable      */

    if( m_read( base, index, freq ) != 0xffff )        /* verify data  */
        return MCRW_ERR_ERASE;                           /* ..error      */
    
    return 0;
}


/******************************* m_read *************************************
 *
 *  Description:  Read a specified word from EEPROM at 'base'.
 *
 *---------------------------------------------------------------------------
 *  Input......:  addr     base address pointer
 *                index    index to read
 *  Output.....:  return   readed word
 *  Globals....:  ---
 ****************************************************************************/
UINT16 m_read( int base, UINT8 index, int freq )
{
    register UINT16    wx;                 /* data word    */
    register int        i;                  /* counter      */

    _opcode( base, (UINT8)(_READ_+index), freq );
    for(wx=0, i=0; i<16; i++)
        wx = (wx<<1)+_clock( base, 0, freq );
    _deselect( base );

    return(wx);
}


/******************************* m_write ************************************
 *
 *  Description:  Write a specified word into EEPROM at 'base'.
 *
 *---------------------------------------------------------------------------
 *  Input......:  addr     base address pointer
 *                index    index to write
 *                data     word to write
 *  Output.....:  return   readed word
 *  Globals....:  ---
 ***************************************************************************/
int m_write( int base, UINT8  index, UINT16 data, int freq )
{
    if( _erase( base, index, freq ))              /* erase cell first */
        return( MCRW_ERR_ERASE );

    return _write( base, index, data, freq );
}




/*----------------------------------------------------------------------
 * USER-LEVEL ROUTINES FOR SERIAL EEPROM
 *--------------------------------------------------------------------*/

STATUS p10DumpEEPROM( int address )
{
	/* sys pci find device must be inserted */
	int wordCount;
	int size = sizeof(p10eedata), temp;
	
	printf ("Dump Size[%d]: ", size ); scanf ("%d", &temp);
	
	if( 0< temp && temp < 0x100 )
		size = temp;
	
	printf("\n\nP10 EEPROM Dump\n");
	for ( wordCount = 0; wordCount < size; wordCount++)
	{
		printf ("%d: 0x%04x\n", wordCount, m_read(address, wordCount, 1));
	}
	return (OK);
}



STATUS p10VerifyEEPROM( int address )
{
	UINT16 	buffer;
	int		i;
	
	printf("Verifying EEPROM");
	
	for (i = 0; i < NELEMENTS(p10eedata); i++)
	{
		/* compare data until error or end of data */
		buffer = m_read ( address, i, 1 );
		if (buffer != p10eedata[i])
		{
			printf ("\n*** ERROR in EEPROM!\n");
			return ERROR;
		}
		else printf (".");
	}
	printf ("OK\n");
	printf ("%d bytes verified\n", i);
	
	
	return (OK);
}



STATUS p10WriteEEPROM( int address )
{
	int		i;
	int		retVal;
	
	printf ("Writing EEPROM");
	
	for (i = 0; i < NELEMENTS(p10eedata); i++)
	{
		/* write data from table to eeprom */
		retVal = m_write ( address, i, p10eedata[i], 1 );
		if (retVal == MCRW_ERR_ERASE)
		{
			printf ("\n\nERROR erasing EEPROM\n");
			return ERROR;
		}
		else if (retVal == MCRW_ERR_WRITE)
		{
			printf ("\n\nERROR writing EEPROM\n");
			return ERROR;
		}
		else if (retVal == MCRW_ERR_WRITE_VERIFY)
		{
			printf ("\n\nERROR verifing EEPROM\n");
			return ERROR;
		}
		else printf (".");
		
	}
	
	printf ("OK\n");
	printf ("%d bytes written\n", i);
	
	return OK;
}




/******************************** p10EEPROM **********************************
 *
 *  Description: Find all p10 devices and write eeprom data
 *
 *----------------------------------------------------------------------------
 *  Input......:  
 *
 *  Output.....:  Return 		OK | ERROR
 *
 *  Globals....:  p10Rsrc
 *****************************************************************************/
STATUS p10Utility( int base )
{
	char		buf[20];
	
	do {
		
		printf("Utility to write and verify EEPROM, press ESC to quit\n");
				
		printf("1 - Verify EEPROM data on P10 base 0x%04x\n", base);
					
		printf("2 - Write EEPROM data on P10 base 0x%04x\n", base);
				
		printf("3 - Dump EEPROM data on P10 base 0x%04x\n", base);
		
		gets (buf);
	   			
		switch( buf[0] ){
		case '1':	p10VerifyEEPROM ( base );
					break;				

		case '2':	p10WriteEEPROM ( base );
					break;
		
		case '3':	p10DumpEEPROM ( base );
					break;
		}
		
	} while (buf[0] != 27); /* ESC (ASCII 27) */
		

	



	return OK;
}





/******************************** p10EEPROM **********************************
 *
 *  Description: Find all p10 devices and write eeprom data
 *
 *----------------------------------------------------------------------------
 *  Input......:  pciToIoOffset 0 - use default
 *
 *  Output.....:  Return 		OK | ERROR
 *
 *  Globals....:  p10Rsrc
 *****************************************************************************/
STATUS oxprom( u_int32 pciToIoOffset )
{
	int     	pciBus;
    int     	pciDevice;
    int  		pciFunc;
    int			unit;
    int			found = 0;
    UINT32		iobaseCsr[4] = {0, 0, 0, 0};
	int			p10_units = 0;
	char		buf[20];
	int			i;

	if( pciToIoOffset == 0 )
		pciToIoOffset =  CPU_PCI_IO_ADRS; /* default */

  	/* try to find P10 */
	for (unit = 0; unit < 4; unit++)
	{
    	if ( pciFindDevice(	0x1415,
							0x9501,
    						p10_units,
                           	&pciBus, &pciDevice, &pciFunc ) == OK)
			
		{
			
			/* board detected */
			found = TRUE;
			
			pciConfigInLong (	pciBus, 
			 					pciDevice, 
			 					pciFunc,
	         					PCI_CFG_BASE_ADDRESS_2,
	         					&iobaseCsr[unit]);
			
			iobaseCsr[unit] &= PCI_IOBASE_MASK;
			iobaseCsr[unit] += pciToIoOffset + 3;
			
			pciConfigOutByte (	pciBus, 
			  					pciDevice, 
		  						pciFunc,
                   	      		PCI_CFG_COMMAND,
                       	  		PCI_CMD_IO_ENABLE | 
                       			PCI_CMD_MEM_ENABLE | 
                       			PCI_CMD_MASTER_ENABLE);

        

   			pciConfigOutByte (	pciBus, 
			  					pciDevice, 
			  					pciFunc, 
		  						PCI_CFG_MODE, 
                   	      		SLEEP_MODE_DIS);
			p10_units++;  /* number of P10 units found */
						
		}
		else
		{
			break;
		}
	} /* for */

	

    if ((found != TRUE) || (pciDevice > 32))
		return (ERROR);


	
	do {
		printf("\nP10 EEPROM UTILITY (c) 2000 MEN Microelektronik $Revision: 1.2 $\n");
		printf("Utility to write and verify EEPROM, press ESC to quit\n");
		printf("%d P10 modules found\n", p10_units);
		
		
		for (i = 0; i < p10_units; i++)
		{
			printf("%d - EEPROM utilities for P10 on base 0x%04x\n", i+1, iobaseCsr[i]);
		}
		
		gets (buf);
	   			
		switch( buf[0] ){
		case '1':	p10Utility ( iobaseCsr [0] );
					break;
					
		case '2':	p10Utility ( iobaseCsr [1] );
					break;			
					
		case '3':	p10Utility ( iobaseCsr [2] );
					break;			
					
		case '4':	p10Utility ( iobaseCsr [3] );
					break;			
								
		}
	
	
	} while (buf[0] != 27); /* ESC (ASCII 27) */
		
		
	return OK;		
}










