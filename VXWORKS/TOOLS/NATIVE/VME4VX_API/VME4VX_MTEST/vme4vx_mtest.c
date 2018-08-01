/*********************  P r o g r a m  -  M o d u l e ***********************/
/*
 *        \file  vme4vx_mtest.c cloned from VME4L API vme4l_mtest.c 
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2018 by MEN mikro elektronik GmbH, Nuremberg, Germany
 ******************************************************************************/
static const char RCSid[] = "$Id: vme4vx_mtest.c$";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <MEN/men_typs.h>
#include <MEN/usr_utl.h>
#include <MEN/usr_oss.h>

#include <MEN/sysMenPci2Vme.h>
#include <MEN/vme4vx.h>

/*--------------------------------------+
 |   DEFINES                             |
 +--------------------------------------*/
#define CHK(expression) \
 if( !(expression)) {\
	 printf("\n*** Error during: %s\nfile %s\nline %d\n", \
      #expression,__FILE__,__LINE__);\
      printf("%s\n",strerror(errno));\
     goto ABORT;\
 }

#define SWAP16(word)	( (((word)>>8) & 0xff) | (((word)<<8)&0xff00) )

#define SWAP32(dword)	( ((dword)>>24) | ((dword)<<24) | \
							  (((dword)>>8) & 0x0000ff00)   | \
							  (((dword)<<8) & 0x00ff0000)     )

#define ERR_READ	1
#define ERR_WRITE	2
#define ERR_BERR	3
#define ERR_MIRR	4
#define ERR_RW		5

#define MSGWDW_WIDTH 39
#define ERRWDW_WIDTH 40

#define INFOSTEP	262144
/* 32768 */

typedef struct
{
    char test_id;
    char *action_line;
    int errcnt;
} test_descr;

static test_descr test[] =
{
    {
        'b',
        "Byte access, random pattern" },
    {
        'w',
        "Word access, random pattern" },
    {
        'l',
        "Long access, random pattern" },
    {
        'B',
        "Byte access, linear pattern" },
    {
        'W',
        "Word access, linear pattern" },
    {
        'L',
        "Long access, linear pattern" },
    {
        'v',
        "VME4VX Read/Write block xfer, random pattern" },
    {
        'V',
        "VME4VX Read/Write block xfer, linear pattern" },
    {
        0 } };

/*-------------------+
 | Global variables  |
 +------------------*/
char *usage_str =
    "\
Syntax:   vme4vx_mtest [<opts>] <startaddr> <endaddr> [<opts>]\n\
Function: VME4VX memory test\n\
Options:\n\
    -s=<spc>   VME4VX space number [4=a24d16]\n\
    -w=<swp>   swap mode to use [0]\n\
    -n=<num>   number of runs\n\
    -q=<num>   abort after <num> errors\n\
    -m=<mod>   set address modifier (AM) for space <spc> (A21 only!) [0]\n\
                0 (00b) non-privileged data access (default)\n\
                1 (01b) non-privileged program access\n\
                2 (10b) supervisory data access\n\
                3 (11b) supervisory program access\n\
               (BLT: only 0 and 2 supported (supervisory=bit[1])\n\
                /!\\ Not supported yet ! \n\
    -v         don't write to RAM, just verify\n\
    -f         no fill info\n\
    -p         1s delay between read & verify\n\
    -x         do SW swapping (only for \"l\" test)\n\
    -t=<lst>   list of tests to execute:\n\n";

static void version( void );

static void usage( void )
{
    test_descr *test_p = test;
    fprintf( stderr,
             "%s",
             usage_str );

    while( test_p->test_id )
    {
        fprintf( stderr,
                 "       %c: %s\n",
                 test_p->test_id,
                 test_p->action_line );
        test_p++;
    }
    version();
}

static void version( void )
{
    char *rev = "$Revision: 0.1 $";
    char *p = rev + strlen( "$Revision: " );

    fprintf( stderr,
             "\nV " );
    while( *p != '$' )
        fprintf( stderr,
                 "%c",
                 *p++ );
    fprintf( stderr,
             " (c) Copyright 2018 by MEN GmbH\n" );
}

static int8 show_test_result( u_int32 tot_errors )
{
    printf( "\nTOTAL TEST RESULT: %d errors\n",
            (int)tot_errors );
    if( 0 < tot_errors )
    {
        return ERROR;
    }
    else
    {
        return OK;
    }
}

static void action_info( char *info )
{
    printf( "ACTION: %s\n",
            info );
}

static void fill_info( u_int32 bytes, int32 fillInfo )
{
    if( fillInfo )
    {
        printf( "FILLED: %6ldK\n",
                bytes / 1024 );
    }
}

static void ok_after_retries( u_int32 address, int num_tries )
{
    printf( "Addr %08lx ok after %d retries\n",
            address,
            num_tries );
}

static void is_mirroring( u_int32 address, u_int32 size )
{
    printf( "Addr %08lx mirroring after %08lx\n",
            address,
            size );
}

static char* get_errtype_str( int err_type )
{
    switch( err_type )
    {
        case ERR_READ:
            return "READ";
        case ERR_WRITE:
            return "WRIT";
        case ERR_BERR:
            return "BERR";
        case ERR_MIRR:
            return "MIRR";
        case ERR_RW:
            return "RDWR";
        default:
            return "????";
    }
}

static void out_error( 
    int err_type, 
    u_int32 vmeAddress, 
    u_int32 is, 
    u_int32 shouldbe, 
    int access_size, 
    int32 max_errors, 
    int32* tot_errors )
{
    u_int32 mask = ( 1LL << ( access_size * 8 ) ) - 1;
    char fmt[200], *p = fmt;

    shouldbe &= mask; /* mask unwanted bits */
    is &= mask;

    p += sprintf( fmt,
                  "ERROR: %s",
                  "%-4s %08x" );

    if( err_type == ERR_READ || err_type == ERR_WRITE || err_type == ERR_RW )
    {
        switch( access_size )
        {
            case 1:
                sprintf( p,
                         "%s",
                         " is %02lx shouldbe %02lx" );
                break;
            case 2:
                sprintf( p,
                         "%s",
                         " is %04lx shouldbe %04lx" );
                break;
            case 4:
                sprintf( p,
                         "%s",
                         " is %08lx shouldbe %08lx" );
                break;
        }
    }

    printf( fmt,
            get_errtype_str( err_type ),
            vmeAddress,
            is,
            shouldbe );
    printf( "\n" );

    * ( tot_errors ) += 1;
    if( * ( tot_errors ) >= max_errors )
    {
        show_test_result( * ( tot_errors ) );
    }
}

/********************************** mk_rand_pat *****************************
 *
 *  Description:  Creates a random pattern using a fast random algorithm.
 *				
 *
 *---------------------------------------------------------------------------
 *  Input......:  address - only used to dermine when to create a new random
 *							pattern. If address is long aligned, a new
 *							random pattern is created, otherwise the old
 *							random value is shifted according to size.
 *				  oldpat  - previous pattern returned by mk_rand_pat or
 *							starting value for pattern
 *				  size    - required size for random pattern
 *					        1=byte 2=word 4=long
 *
 *  Output.....:  return  - 8/16/32 bit random pattern
 *				
 *  Globals....:  ---
 ****************************************************************************/
static u_int32 mk_rand_pat( u_int32 address, u_int32 oldpat, u_int8 size, long swSwap )
{
    u_int32 pattern;

    if( size != 4 )
    {
        pattern = oldpat << ( size << 3 ); /* emulates rol.l #y,xx */
        pattern |= ( oldpat >> ( 32 - ( size << 3 ) ) ) & ( ( 1 << ( size << 3 ) ) - 1 );
    }
    else
    {
        pattern = oldpat;
        if( swSwap ) pattern = SWAP32( pattern );
    }

    if( ( address & 0x3 ) == 0 )
    {
        register u_int32 a = pattern;

        a <<= 11;
        a += pattern;
        a <<= 2;
        pattern += a;
        pattern += 13849;
    }

    if( swSwap && size == 4 ) pattern = SWAP32( pattern );

    return pattern;
}

/********************************** mk_lin_pat *****************************
 *
 *  Description:  Creates a linear testpattern which corresponds to the
 *				  given address
 *
 *---------------------------------------------------------------------------
 *  Input......:  address - used to produce the test pattern
 *
 *				  oldpat  - not used, just for compat. with mk_rand_pat
 *				  size    - required size for pattern
 *					        1=byte 2=word 4=long
 *
 *  Output.....:  return  - 8/16/32 bit linear pattern
 *				
 *  Globals....:  ---
 ****************************************************************************/
static u_int32 mk_lin_pat( u_int32 oldpat )
{
    return oldpat + 1;
}

static int berr_check( u_int32 vmeaddr, int32 busErrCnt, int32* tot_errors, int32 max_errors )
{
    if( busErrCnt )
    {
        printf( "*** VME bus error detected "
                "(can be another program)\n" );
        out_error( ERR_BERR,
                   vmeaddr,
                   0,
                   0,
                   0,
                   max_errors,
                   tot_errors );
        busErrCnt = 0;
        return ERROR;
    }
    return OK;
}

/********************************** BWL_TEST ********************************
 *
 *  Description:  Macro for Byte/Word/Long tests
 *
 ****************************************************************************/
static u_int32 BWL_TEST_LIN( 
    u_int8 accessSize, 
    int32 pass, 
    void* mapStart, 
    void* mapEnd, 
    long swSwap, 
    long just_verify, 
    int32 fillInfo, 
    VME4VX_BUSERR_PARAMS* busErrorParams, 
    int32* tot_errors, 
    int32 max_errors, 
    u_int32 vmeAddr,
    int8 opt_pause )
{
    u_int32 oldVal = 0;
    u_int32 count = 0;
    u_int32 size = (u_int32) ( ( mapEnd - mapStart ) );
    u_int32 step;
    u_int32 i;
    int32 err_type;

    u_int32* localBuffer;
    u_int32* localBufferVerify;
    u_int32* localBufferCmp;
    int32* errCount = (int32*) ( busErrorParams->parameters );

    localBuffer         = calloc( size,
                          1 );
    localBufferVerify   = calloc( size,
                                1 );
    localBufferCmp      = calloc( size,
                                  1 );

    /*---------------+
     |  Fill memory  |
     +--------------*/
    if( !just_verify )
    {
        action_info( "Filling Memory" );
        for( i = 0; count < size; i++ )
        {
            /* create a new test pattern */
            oldVal = mk_lin_pat( oldVal );
            /* store value */
            * ( localBuffer + i ) = oldVal;
            count += sizeof(u_int32);
        }
        fill_info( count,
                   fillInfo );
    }

    switch( accessSize )
    {
        case 1:
            bcopyBytes( (char*)localBuffer,
                        (char*)mapStart,
                        size );
            break;

        case 2:
            bcopyWords( (char*)localBuffer,
                        (char*)mapStart,
                        size / 2 );
            break;

        case 4:
            bcopyLongs( (char*)localBuffer,
                        (char*)mapStart,
                        size / 4 );
            break;

        default:
            bcopy( (char*)localBuffer,
                   (char*)mapStart,
                   size );
    }

    if( ERROR == berr_check( busErrorParams->busErrorAddrs,
                            * ( errCount ),
                            tot_errors,
                            max_errors ) )
    {
        return * ( tot_errors );
    }

    /*-----------------+
     |  Verify Memory  |
     +----------------*/
    if( opt_pause )
    {
        UOS_Delay( 1000 );
    }

    /*-------------------------------------------+
     | Verify error detected...                  |
     | 1) Set HW Trigger (read and compare)      |
     | 3) retry access 3 times                   |
     | 2) Check for mirrored memory              |
     | 4) Display error to user                  |
     +------------------------------------------*/
    action_info( "Verify Memory" );
    switch( accessSize )
    {
        case 1:
            bcopyBytes( (char*)mapStart,
                        (char*)localBufferVerify,
                        size );
            break;

        case 2:
            bcopyWords( (char*)mapStart,
                        (char*)localBufferVerify,
                        size / 2 );
            break;

        case 4:
            bcopyLongs( (char*)mapStart,
                        (char*)localBufferVerify,
                        size / 4 );
            break;

        default:
            bcopy( (char*)mapStart,
                   (char*)localBufferVerify,
                   size );
    }

    if( OK != memcmp( localBufferVerify,
                      localBuffer,
                      size ) )
    {
        /*--- if memory comparison is wrong we assume this is a write error ---*/
        err_type = ERR_WRITE;

        /*--- Re-read the buffer 3 times to confirm it is a write error ---*/
        for( i = 1; i <= 3; i++ )
        {
            switch( accessSize )
            {
                case 1:
                    bcopyBytes( (char*)mapStart,
                                (char*)localBufferCmp,
                                size );
                    break;

                case 2:
                    bcopyWords( (char*)mapStart,
                                (char*)localBufferCmp,
                                size / 2 );
                    break;

                case 4:
                    bcopyLongs( (char*)mapStart,
                                (char*)localBufferCmp,
                                size / 4 );
                    break;

                default:
                    bcopy( (char*)mapStart,
                           (char*)localBufferCmp,
                           size );
            }

            if( OK == memcmp( localBufferCmp,
                              localBuffer,
                              size ) )
            {
                /*--- We finally had a read error ---*/
                ok_after_retries( vmeAddr,
                                  i );
                err_type = ERR_READ;
                break;
            }
        }

        if( err_type == ERR_WRITE )
        {
            /*--- detect mirrored RAM ---*/
            i = 0;
            while( i < size )
            {
                step = 1;
                while( step < size )
                {
                    if( localBufferVerify[step] == localBuffer[i] )
                    {
                        is_mirroring( vmeAddr,
                                      ( &localBufferVerify[step] ) - localBufferVerify );
                        err_type = ERR_MIRR;
                    }
                    step = step << 1;
                }
                i++;
            }
        }
        else
        {
            
        }
        
        i = 0;
        while( i < ( size / sizeof(u_int32) ) )
        {
            if( localBufferVerify[i] != localBuffer[i] )
            {
                out_error( err_type,
                           vmeAddr + (i * accessSize),
                           localBufferVerify[i],
                           localBuffer[i],
                           sizeof(u_int32),
                           max_errors,
                           tot_errors );
            }
            i++;
        }

    }
    fill_info( count * sizeof ( accessSize ),
               fillInfo );

    berr_check( busErrorParams->busErrorAddrs,
                * ( errCount ),
                tot_errors,
                max_errors );
    
    free( localBuffer );
    free( localBufferCmp );
    free( localBufferVerify );

    return * ( tot_errors );
}

static u_int32 BWL_TEST_RAND( 
    u_int8 accessSize, 
    int32 pass, 
    void* mapStart, 
    void* mapEnd, 
    long swSwap, 
    long just_verify, 
    int32 fillInfo, 
    VME4VX_BUSERR_PARAMS* busErrorParams, 
    int32* tot_errors, 
    int32 max_errors, 
    u_int32 vmeAddr,
    int8 opt_pause )
{
    u_int32 randval = 0xABCDEF02 + pass;
    u_int32 count = 0;
    u_int32 size = (u_int32) ( ( mapEnd - mapStart ) );
    u_int32 step;
    u_int32 i;
    int32 err_type;

    u_int32* localBuffer;
    u_int32* localBufferVerify;
    u_int32* localBufferCmp;
    int32* errCount = (int32*) ( busErrorParams->parameters );

    if( swSwap )
    {
        randval = SWAP32( randval );
    }

    localBuffer         = calloc( size,
                          1 );
    localBufferVerify   = calloc( size,
                                1 );
    localBufferCmp      = calloc( size,
                                  1 );

    /*---------------+
     |  Fill memory  |
     +--------------*/
    if( !just_verify )
    {
        action_info( "Filling Memory" );
        for( i = 0; count < size; i++ )
        {
            /* create a new test pattern */
            randval = mk_rand_pat( (u_int32)localBuffer,
                                   randval,
                                   accessSize,
                                   swSwap );
            /* store value */
            * ( localBuffer + i ) = randval;
            count += sizeof(u_int32);
        }
        fill_info( count,
                   fillInfo );
    }

    switch( accessSize )
    {
        case 1:
            bcopyBytes( (char*)localBuffer,
                        (char*)mapStart,
                        size );
            break;

        case 2:
            bcopyWords( (char*)localBuffer,
                        (char*)mapStart,
                        size / 2 );
            break;

        case 4:
            bcopyLongs( (char*)localBuffer,
                        (char*)mapStart,
                        size / 4 );
            break;

        default:
            bcopy( (char*)localBuffer,
                   (char*)mapStart,
                   size );
    }

    if( ERROR == berr_check( busErrorParams->busErrorAddrs,
                            * ( errCount ),
                            tot_errors,
                            max_errors ) )
    {
        return * ( tot_errors );
    }

    /*-----------------+
     |  Verify Memory  |
     +----------------*/
    if( opt_pause )
    {
        UOS_Delay( 1000 );
    }

    /*-------------------------------------------+
     | Verify error detected...                  |
     | 1) Set HW Trigger (read and compare)      |
     | 3) retry access 3 times                   |
     | 2) Check for mirrored memory              |
     | 4) Display error to user                  |
     +------------------------------------------*/
    action_info( "Verify Memory" );
    switch( accessSize )
    {
        case 1:
            bcopyBytes( (char*)mapStart,
                        (char*)localBufferVerify,
                        size );
            break;

        case 2:
            bcopyWords( (char*)mapStart,
                        (char*)localBufferVerify,
                        size / 2 );
            break;

        case 4:
            bcopyLongs( (char*)mapStart,
                        (char*)localBufferVerify,
                        size / 4 );
            break;

        default:
            bcopy( (char*)mapStart,
                   (char*)localBufferVerify,
                   size );
    }

    if( OK != memcmp( localBufferVerify,
                      localBuffer,
                      size ) )
    {
        /*--- if memory comparison is wrong we assume this is a write error ---*/
        err_type = ERR_WRITE;

        /*--- Re-read the buffer 3 times to confirm it is a write error ---*/
        for( i = 1; i <= 3; i++ )
        {
            switch( accessSize )
            {
                case 1:
                    bcopyBytes( (char*)mapStart,
                                (char*)localBufferCmp,
                                size );
                    break;

                case 2:
                    bcopyWords( (char*)mapStart,
                                (char*)localBufferCmp,
                                size / 2 );
                    break;

                case 4:
                    bcopyLongs( (char*)mapStart,
                                (char*)localBufferCmp,
                                size / 4 );
                    break;

                default:
                    bcopy( (char*)mapStart,
                           (char*)localBufferCmp,
                           size );
            }

            if( OK == memcmp( localBufferCmp,
                              localBuffer,
                              size ) )
            {
                /*--- We finally had a read error ---*/
                ok_after_retries( vmeAddr,
                                  i );
                err_type = ERR_READ;
                break;
            }
        }

        if( err_type == ERR_WRITE )
        {
            /*--- detect mirrored RAM ---*/
            i = 0;
            while( i < size )
            {
                step = 1;
                while( step < size )
                {
                    if( localBufferVerify[step] == localBuffer[i] )
                    {
                        is_mirroring( vmeAddr,
                                      ( &localBufferVerify[step] ) - localBufferVerify );
                        err_type = ERR_MIRR;
                    }
                    step = step << 1;
                }
                i++;
            }
        }
        else
        {
            
        }
        
        i = 0;
        while( i < ( size / sizeof(u_int32) ) )
        {
            if( localBufferVerify[i] != localBuffer[i] )
            {
                out_error( err_type,
                           vmeAddr + (i * accessSize),
                           localBufferVerify[i],
                           localBuffer[i],
                           sizeof(u_int32),
                           max_errors,
                           tot_errors );
            }
            i++;
        }

    }
    fill_info( count * sizeof ( accessSize ),
               fillInfo );

    berr_check( busErrorParams->busErrorAddrs,
                * ( errCount ),
                tot_errors,
                max_errors );
    
    free( localBuffer );
    free( localBufferCmp );
    free( localBufferVerify );

    return * ( tot_errors );
}

/*------------------------------------------+
 | VMEbus block transfer using vmeAddress   |
 +-----------------------------------------*/
static void vmeblt( u_int32 spc, void *src, void *dst, u_int32 len, int direction, int32* tot_errors )
{
    if( direction )
    {
        CHK( printf( "Write BLT\n" ) >= 0 );
        if( ERROR == sysMenPci2VmeDma( VX_WRITE,
                                       src,
                                       dst,
                                       len,
                                       FALSE,
                                       spc,
                                       500 ) )
        {
            goto ABORT;
        }
    }
    else
    {
        CHK( printf( "Read BLT\n" ) >= 0 );
        if( ERROR == sysMenPci2VmeDma( VX_READ,
                                       src,
                                       dst,
                                       len,
                                       FALSE,
                                       spc,
                                       500 ) )
        {
            goto ABORT;
        }
    }
    return;

    ABORT: * ( tot_errors ) += 1;
    show_test_result( * ( tot_errors ) );
}

/************************************ blk_test *****************************
 *
 *  Description:  block test func, only for DMA spaces (VME4VX_SPC_AXX_DXX_BLT)
 *
 *---------------------------------------------------------------------------
 *  Input......:  pBuf    		- the data buffer to read to / write from
 *				  rand_pattern  - if 1, then pseudo random pattern is used
 *				  readfunc      - func pointer to a function reading from VME
 *				  writefunc      - func pointer to a function reading from VME *
 *
 *  Output.....:  return  		- error count, 0 if successful test
 *
 *  Globals....:  ---
 *  
 *  important: blk_size represents # of bytes as in malloc call in main! 
 ****************************************************************************/
static int blk_test( 
    unsigned char* pBuf, 
    int rand_pattern, 
    void (*readfunc)(), 
    void (*writefunc)(), 
    int32 pass, 
    long just_verify, 
    u_int32 spc, 
    int32 startadr, 
    int32 endadr, 
    long swSwap, 
    int32 fillInfo, 
    int8 opt_pause, 
    int32 max_errors, 
    int32* tot_errors, 
    VME4VX_BUSERR_PARAMS* busErrorParams )
{
    u_int32 blk_size, cur_size = 0;
    u_int32 pattern = 0xabcdef02 + pass;
    int32* errcnt = 0, i = 0;
    unsigned long address;
    unsigned long *p;
    unsigned long *buf = (unsigned long *)pBuf;

    errcnt = (int32*) ( busErrorParams->parameters );

    blk_size = endadr - startadr;

    /*----------------+
     |  Write memory  |
     +---------------*/
    if( !just_verify )
    {
        address = startadr;

        action_info( "Filling Memory" );
        while( address < endadr )
        {
            /*---------------------+
             |  Fill write buffer  |
             +--------------------*/
            if( rand_pattern )
            {
                for( p = buf, i = 0; i < blk_size / sizeof(long); i++ )
                {
                    pattern = mk_rand_pat( 0,
                                           pattern,
                                           4,
                                           swSwap );
                    *p = pattern;
                    p++;
                }
            }
            else
            {
                for( p = buf, i = 0; i < blk_size / sizeof(long); i++ )
                {
                    *p++ = address + ( i << 2 );
                }
            }

            /*-----------------------+
             |  Write buffer to mem  |
             +----------------------*/
            cur_size = blk_size;

            if( cur_size > endadr - address )
            {
                cur_size = endadr - address;
            }
            ( *writefunc )( spc,
                            buf,
                            address,
                            cur_size,
                            1 );

            address += cur_size;

            fill_info( ( address - startadr ),
                       fillInfo );
        }
    }

    if( ERROR == berr_check( busErrorParams->busErrorAddrs,
                             * ( errcnt ),
                             tot_errors,
                             max_errors ) )
    {
        return * ( tot_errors );
    }

    address = startadr;
    pattern = 0xabcdef02 + pass;

    if( opt_pause ) UOS_Delay( 1000 );

    /*----------------+
     |  Verify memory |
     +---------------*/
    action_info( "Verify Memory" );
    while( address < endadr )
    {
        /*-------------------------+
         |  Read to buffer memory  |
         +------------------------*/
        cur_size = blk_size;
        if( cur_size > endadr - address )
        {
            cur_size = endadr - address;
        }

        memset( buf,
                0,
                cur_size ); /* clear before reading */

        /* here the data block with len <cur_size> is read to address <address> */
        ( *readfunc )( spc,
                       address,
                       buf,
                       cur_size,
                       0 );

        /*------------------+
         |  Compare buffer  |
         +-----------------*/
        for( p = buf, i = 0; i < cur_size / sizeof(long); p++, i++ )
        {
            if( rand_pattern )
            {
                pattern = mk_rand_pat( 0,
                                       pattern,
                                       4,
                                       swSwap );
            }
            else
            {
                pattern = address + ( i << 2 );
            }

            if( *p != pattern )
            {
                * ( errcnt ) += 1;
                out_error( ERR_RW,
                           address + ( i << 2 ),
                           *p,
                           pattern,
                           4,
                           max_errors,
                           tot_errors );
            }
        }
        address += cur_size;

        fill_info( ( address - startadr ),
                   fillInfo );
    }

    if( ERROR == berr_check( busErrorParams->busErrorAddrs,
                             * ( errcnt ),
                             tot_errors,
                             max_errors ) )
    {
        return * ( tot_errors );
    }

    return * ( errcnt );
}

static int do_test( 
    unsigned char* pBuf, 
    u_int32 vmeAddr, 
    u_int32 spc, 
    u_int32 size, 
    char test_id, 
    int32 pass, 
    long swSwap, 
    long just_verify, 
    int32* tot_errors, 
    int32 max_errors, 
    int32 fillInfo, 
    int8 opt_pause, 
    VME4VX_BUSERR_PARAMS* busErrorParams )
{
    u_int32 errorcount = 0;
    void* mapStart;
    void* mapEnd;

    switch( spc )
    {
        case VME4VX_SPC_A16_D32:
        case VME4VX_SPC_A16_D16:
            CHK( OK == sysBusToLocalAdrs( OSS_VME_AM_16UD, vmeAddr, &mapStart ) );
            CHK( OK == sysBusToLocalAdrs( OSS_VME_AM_16UD, (vmeAddr + size), &mapEnd ) );
            break;

        case VME4VX_SPC_A24_D64_BLT:
        case VME4VX_SPC_A24_D32_BLT:
        case VME4VX_SPC_A24_D16_BLT:
            CHK( OK == sysBusToLocalAdrs( OSS_VME_AM_24UB, vmeAddr, &mapStart ) );
            CHK( OK == sysBusToLocalAdrs( OSS_VME_AM_24UB, (vmeAddr + size), &mapEnd ) );
            break;

        case VME4VX_SPC_A24_D32:
        case VME4VX_SPC_A24_D16:
            CHK( OK == sysBusToLocalAdrs( OSS_VME_AM_24UD, vmeAddr, &mapStart ) );
            CHK( OK == sysBusToLocalAdrs( OSS_VME_AM_24UD, (vmeAddr + size), &mapEnd ) );
            break;

        case VME4VX_SPC_A32_D64_BLT:
        case VME4VX_SPC_A32_D32_BLT:
            CHK( OK == sysMenPci2VmeSetA32MasterPrefix( vmeAddr ) );
            CHK( OK == sysBusToLocalAdrs( OSS_VME_AM_32UB, vmeAddr, &mapStart ) );
            CHK( OK == sysBusToLocalAdrs( OSS_VME_AM_32UB, (vmeAddr + size), &mapEnd ) );
            break;

        case VME4VX_SPC_A32_D32:
            CHK( OK == sysMenPci2VmeSetA32MasterPrefix( vmeAddr ) );
            CHK( OK == sysBusToLocalAdrs( OSS_VME_AM_32UD, vmeAddr, &mapStart ) );
            CHK( OK == sysBusToLocalAdrs( OSS_VME_AM_32UD, (vmeAddr + size), &mapEnd ) );
            break;
    }

    switch( test_id )
    {
        case 'b':
            errorcount = BWL_TEST_RAND( sizeof(u_int8),
                                        pass,
                                        mapStart,
                                        mapEnd,
                                        swSwap,
                                        just_verify,
                                        fillInfo,
                                        busErrorParams,
                                        tot_errors,
                                        max_errors,
                                        vmeAddr,
                                        opt_pause );
            break;
        case 'w':
            errorcount = BWL_TEST_RAND( sizeof(u_int16),
                                        pass,
                                        mapStart,
                                        mapEnd,
                                        swSwap,
                                        just_verify,
                                        fillInfo,
                                        busErrorParams,
                                        tot_errors,
                                        max_errors,
                                        vmeAddr,
                                        opt_pause );
            break;
        case 'l':
            errorcount = BWL_TEST_RAND( sizeof(u_int32),
                                        pass,
                                        mapStart,
                                        mapEnd,
                                        swSwap,
                                        just_verify,
                                        fillInfo,
                                        busErrorParams,
                                        tot_errors,
                                        max_errors,
                                        vmeAddr,
                                        opt_pause );
            break;
        case 'B':
            errorcount = BWL_TEST_LIN( sizeof(u_int8),
                                       pass,
                                       mapStart,
                                       mapEnd,
                                       swSwap,
                                       just_verify,
                                       fillInfo,
                                       busErrorParams,
                                       tot_errors,
                                       max_errors,
                                       vmeAddr,
                                       opt_pause );
            break;
        case 'W':
            errorcount = BWL_TEST_LIN( sizeof(u_int16),
                                       pass,
                                       mapStart,
                                       mapEnd,
                                       swSwap,
                                       just_verify,
                                       fillInfo,
                                       busErrorParams,
                                       tot_errors,
                                       max_errors,
                                       vmeAddr,
                                       opt_pause );
            break;
        case 'L':
            errorcount = BWL_TEST_LIN( sizeof(u_int32),
                                       pass,
                                       mapStart,
                                       mapEnd,
                                       swSwap,
                                       just_verify,
                                       fillInfo,
                                       busErrorParams,
                                       tot_errors,
                                       max_errors,
                                       vmeAddr,
                                       opt_pause );
            break;
        case 'v':
            errorcount = blk_test( pBuf,
                                   TRUE,
                                   vmeblt,
                                   vmeblt,
                                   pass,
                                   just_verify,
                                   spc,
                                   vmeAddr,
                                   ( vmeAddr + size ),
                                   swSwap,
                                   fillInfo,
                                   opt_pause,
                                   max_errors,
                                   tot_errors,
                                   busErrorParams );
            break;
        case 'V':
            errorcount = blk_test( pBuf,
                                   FALSE,
                                   vmeblt,
                                   vmeblt,
                                   pass,
                                   just_verify,
                                   spc,
                                   vmeAddr,
                                   ( vmeAddr + size ),
                                   swSwap,
                                   fillInfo,
                                   opt_pause,
                                   max_errors,
                                   tot_errors,
                                   busErrorParams );
            break;
    }
    return errorcount;

ABORT: 
    tot_errors++;
    show_test_result( * ( tot_errors ) );
    return OK;
}

static void vmeBusErrorHandler( VME4VX_BUSERR_PARAMS* pBusErrorParams )
{
    int32* busErrorCntP = (int32*)pBusErrorParams->parameters;
    ( * ( busErrorCntP ) ) += 1;
}

int main( int argc, char *argv[] )
{
    int i, total_pass;
    char *optp, *testlist;
    unsigned char* pDat = NULL;
    test_descr* test_p;
    char* test_id;
    int errors = 0;
    int opt_mod = -1;
    int swapMode = 0;
    int32 busErrorCnt = 0;

    u_int32 startadr, endadr; /* start and end address of VME memory */
    int32 pass; /* Nb of test passes */
    long just_verify; /* don't write RAM, just verify */
    int32 max_errors = (u_int32) ( 1 << 31 ) - 1; /* max. errorcount until program abort */
    int32 tot_errors = 0; /* total errors occured */
    u_int8 spaceNum = 4;
    int8 opt_pause = 0;
    int32 fillInfo = 1;
    long swSwap = 0;
    u_int32 size;
    char spcName[16];

    VME4VX_BUSERR_PARAMS busErrorParams;

    if( UTL_TSTOPT( "?" ) )
    {
        usage();
        return EXIT_SUCCESS;
    }

    /*------------------------------+
     |  Parse command line options  |
     +-----------------------------*/
    startadr = endadr = 0xffffffff;

    for( i = 1; i < argc; i++ )
        if( *argv[i] != '-' )
        {
            if( startadr == 0xffffffff ) sscanf( argv[i],
                                                 "%lx",
                                                 &startadr );
            else sscanf( argv[i],
                         "%lx",
                         &endadr );
        }

    if( startadr == 0xffffffff || endadr == 0xffffffff )
    {
        printf( "missing start or end address\n" );
        usage();
        return EXIT_FAILURE;
    }

    size = endadr - startadr;
    printf( "start 0x%x end 0x%x size = 0x%x\n",
            startadr,
            endadr,
            size );

    CHK( (pDat = malloc( size )) != NULL );

    total_pass = 1;
    if( ( optp = UTL_TSTOPT( "n=" ) ) )
    {
        total_pass = atoi( optp );
    }

    just_verify = (long)UTL_TSTOPT( "v" );
    fillInfo = !(long)UTL_TSTOPT( "f" );
    testlist = UTL_TSTOPT( "t=" );
    if( !testlist )
    {
        usage();
        return EXIT_FAILURE;
    }

    swSwap = (long)UTL_TSTOPT( "x" );
    if( ( optp = UTL_TSTOPT( "q=" ) ) ) max_errors = atoi( optp );

    if( ( optp = UTL_TSTOPT( "s=" ) ) ) spaceNum = atoi( optp );

    if( ( optp = UTL_TSTOPT( "m=" ) ) ) opt_mod = atoi( optp );

    if( ( optp = UTL_TSTOPT( "w=" ) ) ) swapMode = atoi( optp );

    opt_pause = UTL_TSTOPT( "p" ) ? 1 : 0;

    CHK( ( ( opt_mod < 4 ) && ( opt_mod >= 0 ) ) || ( opt_mod == -1 ) );

    /*-- open space */
    sysMenPci2VmeSpaceName( spaceNum,
                            spcName );
    printf( "Memory test on spc %d: %s VME space\n",
            spaceNum,
            spcName );

    if( opt_mod >= 0 )
    {
        printf( "change AM to 0x%x\n",
                opt_mod & 0xff );
        /* TODO: add when BSP support it */
        /*CHK( sysVmeAddrModifierSet( (char ) ( opt_mod & 0xff ) ) == 0 );*/
    }

    /*--- install buserr handler ---*/
    busErrorParams.parameters = (void*) &busErrorCnt;
    CHK( sysMdisIntConnect( VME4VX_IRQVEC_BUSERR,
                            vmeBusErrorHandler,
                            (void* ) &busErrorParams ) == 0 );

    /*-----------------+
     |  Execute tests  |
     +----------------*/
    printf( "Testing VME memory %08x .. %08x\n",
            startadr,
            endadr );

    for( pass = 0; pass < total_pass || total_pass == 0; pass++ )
    {
        printf( "PASS %d...\n",
                pass );

        for( i = 0, test_id = testlist; *test_id; test_id++, i++ )
        {
            for( test_p = test; test_p->test_id; test_p++ )
                if( test_p->test_id == *test_id ) break;
            if( test_p->test_id )
            {
                printf( "%s\n",
                        test_p->action_line );

                errors = do_test( pDat,
                                  startadr,
                                  spaceNum,
                                  size,
                                  *test_id,
                                  pass,
                                  swSwap,
                                  just_verify,
                                  &tot_errors,
                                  max_errors,
                                  fillInfo,
                                  opt_pause,
                                  &busErrorParams );

                test_p->errcnt += errors;
                printf( "TEST result: %-32s %5d errors\n",
                        test_p->action_line,
                        test_p->errcnt );
            }
        }
    }
    if( tot_errors == 0 )
    {
        return EXIT_SUCCESS;
    }

ABORT: 
    show_test_result( tot_errors );
    return EXIT_FAILURE;
}
