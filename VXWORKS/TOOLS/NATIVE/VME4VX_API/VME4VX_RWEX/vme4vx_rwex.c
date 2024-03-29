/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file    vme4vx_rwex cloned from VME4L API vme4l_rwex.c
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2018 by MEN mikro elektronik GmbH, Nuremberg, Germany
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <MEN/men_typs.h>
#include <MEN/usr_utl.h>
#include <MEN/usr_oss.h>
#include <MEN/oss.h>

#include <MEN/sysMenPci2Vme.h>
#include <MEN/vme4vx.h>

/*------------------+
 |  DEFINES         |
 +-----------------*/
#define CHK(expression)                                         \
    if( !(expression)) {                                        \
        printf("\n*** Error during: %s\nfile %s\nline %d\n",    \
               #expression,__FILE__,__LINE__);                  \
        printf("%s\n",strerror(errno));                         \
        return EXIT_FAILURE;                                    \
    }

#define _1G     1000000000      /* for ns->s */
#define _1MB    (1*1024*1024)   /* for MB/s */

#define VERIFY_FILE_POSTFIX ".bad"
/*------------------+
 |  TYPDEFS         |
 +-----------------*/

/*------------------+
 |  GLOBALS         |
 +-----------------*/

/*------------------+
 |  PROTOTYPES      |
 +-----------------*/
static void MemDump( uint8_t *buf, uint32_t n, uint32_t fmt );

static void vmeBusErrorHandler( VME4VX_BUSERR_PARAMS* pBusErrorParams );

/* Simple memcmp with positon returned */
int memcmp_pos( uint8_t *buf1, uint8_t *buf2, size_t size )
{
    size_t i;
    uint8_t *buf1_start = buf1;
    for( i = 0; i < size; i++ )
    {
        if( *buf1 != *buf2 ) return buf1 - buf1_start;
        buf1++;
        buf2++;
    }
    return 0;
}

static void usage( void )
{
    printf( "Syntax:        vme4l_rwex [<opts>] <vmeaddr> <size> [<opts>]\n" );
    printf( "Function:      perform basic VME read/write from and to VME spaces\n" );
    printf( "Options:\n\n" );
    printf( "-s=<spc>       VME4VX space number\n" );
    printf( "-a=<width>     access width in bytes (1/2/4/8)                                     [4]\n" );
    printf( "-v=<init.Val>  initial value for write buffer fill, each next byte is incremented  [0]\n" );
    printf( "-V=<Val>       value for write buffer fill, value is repeted                       [0]\n" );
    printf( "-n=<# runs>    nr. of runs reading/writing is done                                 [1]\n" );
    printf( "               (shown MB/s is average of all runs)\n" );
    printf( "-e             don't rewind a file after each read/write (only with -n option)\n" );
    printf( "-i             FIFO mode, don't increment VME address\n" );
    printf( "-x             use SRAM for BLT transfers\n" );
    printf( "-r             read from VME space into CPU\n" );
    printf( "-w             write from CPU to VME space\n" );
    printf( "-d             do *NOT* dump data (if reading big size)\n" );
    printf( "-l             use page-aligned memory as buffer\n" );
    printf( "-t             use VME4VX_RW_USE_SGL_DMA for non-BLT spaces (uses DMA BDs with SGL bit set)\n" );
    printf( "-f=<file>      with -r dump binary data into a file\n" );
    printf( "-k=<offset>    for writes use an offset in an allocated buffer,\n" );
    printf( "               it loads the entire file into a buffer, reads data from a VME\n" );
    printf( "               and compares against accidental overwrites\n" );
    printf( "-c             in conjuction with -w and -f, verifies written data\n" );
    printf( "               if fails, store read data in <file>\n" );
    printf( "\n\n" );
    printf( "/!\\ The tool will owerwrite any bus error handler!\n" );
}

int vme_read( void* vmeAddr, int spcNumber, int accWidth, size_t size, void* localBuffer, int flags )
{
    int rv = 0;
    void* localMemAddrs = NULL;
    int useDmaBlt = FALSE;
    u_int32 addressModifier = 0x00000000;
    u_int32 dmaTout = 100; /* vxWWorks ticks */

    switch( spcNumber )
    {
        case VME4VX_SPC_A16_D32:
            addressModifier += 0x00010000;
        case VME4VX_SPC_A16_D16:
            rv = sysBusToLocalAdrs( OSS_VME_AM_16UD,
                                    vmeAddr,
                                    &localMemAddrs );
            break;

        case VME4VX_SPC_A24_D64_BLT:
        case VME4VX_SPC_A24_D32_BLT:
            addressModifier += 0x01000000;
        case VME4VX_SPC_A24_D16_BLT:
            useDmaBlt = TRUE;
            rv = sysBusToLocalAdrs( OSS_VME_AM_24UB,
                                    vmeAddr,
                                    &localMemAddrs );
            break;

        case VME4VX_SPC_A24_D32:
            addressModifier += 0x01000000;
        case VME4VX_SPC_A24_D16:
            rv = sysBusToLocalAdrs( OSS_VME_AM_24UD,
                                    vmeAddr,
                                    &localMemAddrs );
            break;

        case VME4VX_SPC_A32_D64_BLT:
        case VME4VX_SPC_A32_D32_BLT:
            useDmaBlt = TRUE;
            sysMenPci2VmeSetA32MasterPrefix( vmeAddr );
            rv = sysBusToLocalAdrs( OSS_VME_AM_32UB,
                                    vmeAddr,
                                    &localMemAddrs );
            break;

        case VME4VX_SPC_A32_D32:
            sysMenPci2VmeSetA32MasterPrefix( vmeAddr );
            rv = sysBusToLocalAdrs( OSS_VME_AM_32UD,
                                    vmeAddr,
                                    &localMemAddrs );
            break;
    }

    if( ERROR == rv )
    {
        return rv;
    }
    
    /*--- Update local address offset depending of the access width---*/
    localMemAddrs += addressModifier;

    /*--- VME Single Accesses ---*/
    if( FALSE == useDmaBlt )
    {
        switch( accWidth )
        {
            case 1:
                bcopyBytes( localMemAddrs,
                            localBuffer,
                            size );
                break;

            case 2:
                bcopyWords( localMemAddrs,
                            localBuffer,
                            size / 2 );
                break;

            case 4:
                bcopyLongs( localMemAddrs,
                            localBuffer,
                            size / 4 );
                break;

            default:
                bcopy( localMemAddrs,
                       localBuffer,
                       size );
        }
    }
    /*--- VME DMA burst transfer ---*/
    else
    {
        if( flags & VME4VX_RW_SRAM_DMA )
        {
            rv = sysMenPci2VmeDma( VX_READ,
                                   vmeAddr,
                                   localBuffer,
                                   size,
                                   TRUE,
                                   spcNumber,
                                   dmaTout );
        }
        else
        {
            rv = sysMenPci2VmeDma( VX_READ,
                                   vmeAddr,
                                   localBuffer,
                                   size,
                                   FALSE,
                                   spcNumber,
                                   dmaTout );
        }
    }

    if( ERROR != rv )
    {
        rv = size;
    }

    return rv;
}

int vme_write( void* vmeAddr, int spcNumber, int accWidth, size_t size, void* localBuffer, int flags )
{
    int rv = 0;
    void* localMemAddrs = NULL;
    int useDmaBlt = FALSE;
    u_int32 addressModifier = 0x00000000;
    u_int32 dmaTout = 100; /* vxWWorks ticks */

    switch( spcNumber )
    {
        case VME4VX_SPC_A16_D32:
            addressModifier += 0x00010000;
        case VME4VX_SPC_A16_D16:
            rv = sysBusToLocalAdrs( OSS_VME_AM_16UD,
                                    vmeAddr,
                                    &localMemAddrs );
            break;

        case VME4VX_SPC_A24_D64_BLT:
        case VME4VX_SPC_A24_D32_BLT:
            addressModifier += 0x01000000;
        case VME4VX_SPC_A24_D16_BLT:
            useDmaBlt = TRUE;
            rv = sysBusToLocalAdrs( OSS_VME_AM_24UB,
                                    vmeAddr,
                                    &localMemAddrs );
            break;

        case VME4VX_SPC_A24_D32:
            addressModifier += 0x01000000;
        case VME4VX_SPC_A24_D16:
            rv = sysBusToLocalAdrs( OSS_VME_AM_24UD,
                                    vmeAddr,
                                    &localMemAddrs );
            break;

        case VME4VX_SPC_A32_D64_BLT:
        case VME4VX_SPC_A32_D32_BLT:
            useDmaBlt = TRUE;
            sysMenPci2VmeSetA32MasterPrefix( vmeAddr );
            rv = sysBusToLocalAdrs( OSS_VME_AM_32UB,
                                    vmeAddr,
                                    &localMemAddrs );
            break;

        case VME4VX_SPC_A32_D32:
            sysMenPci2VmeSetA32MasterPrefix( vmeAddr );
            rv = sysBusToLocalAdrs( OSS_VME_AM_32UD,
                                    vmeAddr,
                                    &localMemAddrs );
            break;
    }

    if( ERROR == rv )
    {
        return rv;
    }

    /*--- Update local address offset depending of the access width---*/
    localMemAddrs += addressModifier;

    /*--- VME Single Accesses ---*/
    if( FALSE == useDmaBlt )
    {
        switch( accWidth )
        {
            case 1:
                bcopyBytes( localBuffer,
                            localMemAddrs,
                            size );
                break;

            case 2:
                bcopyWords( localBuffer,
                            localMemAddrs,
                            size / 2 );
                break;

            case 4:
                bcopyLongs( localBuffer,
                            localMemAddrs,
                            size / 4 );
                break;

            default:
                bcopy( localBuffer,
                       localMemAddrs,
                       size );
        }
    }
    /*--- VME DMA burst transfer ---*/
    else
    {
        if( flags & VME4VX_RW_SRAM_DMA )
        {
            rv = sysMenPci2VmeDma( VX_WRITE,
                                   localBuffer,
                                   vmeAddr,
                                   size,
                                   TRUE,
                                   spcNumber,
                                   dmaTout );
        }
        else
        {
            rv = sysMenPci2VmeDma( VX_WRITE,
                                   localBuffer,
                                   vmeAddr,
                                   size,
                                   FALSE,
                                   spcNumber,
                                   dmaTout );
        }
    }

    if( ERROR != rv )
    {
        rv = size;
    }

    return rv;
}

/**********************************************************************/
/** Program entry point
 *
 * \return success (0) always
 */
int main( int argc, char *argv[] )
{
    int rv, i;
    void* buf = NULL;
    void* buf_ver = NULL;
    int spc = 0;
    void* vmeAddr;
    u_int32 startaddr = 0xffffffff;
    int accWidth = 4;
    size_t size = 0xffffffff;
    size_t file_size = 0;
    size_t buf_size = 0;
    char* optp = NULL;
    char* file_name = NULL;
    char* file_name_ver = NULL;
    char spcName[16];
    int f_desc = -1;
    int f_ver_desc = -1;
    int opt_read = -1, opt_dump = 1, opt_align = 0, opt_runs = 1;
    uint32_t opt_use_startval = 0, opt_startval = 0;
    uint32_t opt_use_fillval = 0;
    uint64_t opt_fillval = 0;
    int opt_verify_write = 0;
    int opt_disable_file_rewind = 0;
    int opt_mmap = 0;
    int opt_bufoffset = 0;
    int opt_bufoffset_val = 0;
    int opt_rw_flags = VME4VX_RW_NOFLAGS;
    struct timespec t1, t2;
    double transferRate = 0, timePerRun = 0, timeTotal = 0;
    ssize_t ret;
    struct stat st;
    uint8_t* p;
    int j;
    int pos;
    int return_global = 0;
    VME4VX_BUSERR_PARAMS busErrorParams;
    uint8_t gotBusError;

    if( UTL_TSTOPT( "?" ) || UTL_TSTOPT( "h" ) || ( argc == 1 ) )
    {
        usage();
        return EXIT_SUCCESS;
    }

    /*----------------------------------+
     |    Parse command line options    |
     +---------------------------------*/
    for( i = 1; i < argc; i++ )
    {
        if( *argv[i] != '-' )
        {
            if( startaddr == 0xffffffff ) startaddr = strtoul( argv[i],
                                                               NULL,
                                                               0 );
            else size = strtoul( argv[i],
                                 NULL,
                                 0 );
        }
    }
    /* Correctness of startaddr and size is done later */
    vmeAddr = (void*)startaddr;

    /* Check / parse args */
    if( ( optp = UTL_TSTOPT( "m" ) ) ) opt_mmap = 1;

    if( UTL_TSTOPT( "r" ) ) opt_read = 1;
    else if( UTL_TSTOPT( "w" ) ) opt_read = 0;

    opt_dump = ( optp = UTL_TSTOPT( "d" ) ) ? 0 : 1;

    if( ( optp = UTL_TSTOPT( "a=" ) ) ) accWidth = strtoul( optp,
                                                            NULL,
                                                            0 );

    if( ( optp = UTL_TSTOPT( "s=" ) ) ) spc = strtoul( optp,
                                                       NULL,
                                                       0 );

    if( UTL_TSTOPT( "x" ) )
    {
        opt_rw_flags |= VME4VX_RW_SRAM_DMA;
        printf( "Use SRAM for DMA block transfers\n" );
    }

    opt_align = ( optp = UTL_TSTOPT( "l" ) ) ? 1 : 0;

    if( UTL_TSTOPT( "t" ) )
    {
        opt_rw_flags |= VME4VX_RW_USE_SGL_DMA;
        printf( "Use DMA for single-mode accesses\n" );
    }

    if( UTL_TSTOPT( "i" ) )
    { /* fifo mode */
        opt_rw_flags |= VME4VX_RW_NOVMEINC;
        printf( "Use FIFO/novmeinc mode\n" );
    }

    if( ( optp = UTL_TSTOPT( "v=" ) ) )
    {
        opt_use_startval = 1;
        opt_startval = strtoul( optp,
                                NULL,
                                0 );
    }

    if( ( optp = UTL_TSTOPT( "V=" ) ) )
    {
        opt_use_fillval = 1;
        opt_fillval = strtoul( optp,
                               NULL,
                               0 );
        if( ( accWidth < 8 ) && ( opt_fillval > ( 1LL << ( accWidth * 8 ) ) ) )
        {
            printf( "Value given to the -V parameter (0x%llx) is bigger than word width (%d)!\n",
                    opt_fillval,
                    accWidth );
            return EXIT_FAILURE;
        }
    }

    if( !opt_read && !opt_use_fillval && !opt_use_startval )
    {
        printf( "In Write mode -v or -V has to be specified!\n" );
        usage();
        return EXIT_FAILURE;
    }

    if( opt_use_fillval && opt_use_startval )
    {
        printf( "Parameters -v and -V cannot be used at the same time!\n" );
        usage();
        return EXIT_FAILURE;
    }

    if( ( optp = UTL_TSTOPT( "n=" ) ) )
    {
        opt_runs = strtoul( optp,
                            NULL,
                            0 );

        if( opt_runs <= 0 )
        {
            printf( "*** nr. of runs must be at least 1 !\n" );
            usage();
            return EXIT_FAILURE;
        }
    }

    if( opt_read == -1 )
    {
        printf( "*** specify either -r or -w!\n" );
        usage();
        return EXIT_FAILURE;
    }

    if( ( optp = UTL_TSTOPT( "c" ) ) )
    {
        opt_verify_write = 1;
    }

    if( ( optp = UTL_TSTOPT( "f=" ) ) )
    {
        file_name = optp;
        if( opt_read == 0 )
        { /* read from file */
            printf( "Read data from a file %s\n",
                    file_name );
        }
        if( opt_read == 1 )
        { /* write to file */
            printf( "Write output to a file %s\n",
                    file_name );
        }
    }

    if( ( optp = UTL_TSTOPT( "k=" ) ) )
    {
        if( !file_name || !opt_verify_write || opt_read )
        {
            printf( "You have to use parameter -c, -w and -f when -k is used\n" );
            return EXIT_FAILURE;
        }
        opt_bufoffset = 1;
        opt_bufoffset_val = strtoul( optp,
                                     NULL,
                                     0 );
        printf( "Skip 0x%x (%d) bytes in allocated buffer\n",
                opt_bufoffset_val,
                opt_bufoffset_val );
    }

    if( opt_runs > 1 && UTL_TSTOPT( "e" ) )
    {
        opt_disable_file_rewind = 1;
        printf( "Don't rewind a file\n" );
    }

    if( file_name )
    {
        if( opt_read == 1 )
        { /* read from VME */
            f_desc = open( file_name,
                           O_CREAT | O_SYNC | O_TRUNC | O_WRONLY,
                           0666 );
        }
        else if( opt_read == 0 )
        { /* write to VME */
            f_desc = open( file_name,
                           O_RDONLY,
                           0666 );
        }
        if( !f_desc )
        {
            printf( "Unable to open file %s\n",
                    file_name );
            return EXIT_FAILURE;
        }

        if( opt_read == 0 )
        { /* write to VME */
            /* get file size */
            fstat( f_desc,
                   &st );
            file_size = st.st_size;
            printf( "file size %d bytes\n",
                    file_size );

            if( size == 0xffffffff )
            {
                size = file_size;
            }
        }
    }

    if( startaddr == 0xffffffff || size == 0xffffffff )
    {
        printf( "*** missing vmeaddr or size!\n" );
        usage();
        return EXIT_FAILURE;
    }

    if( opt_bufoffset ) buf_size = file_size;
    else buf_size = size;

    CHK( (buf = calloc(1, buf_size)) != NULL );
    if( opt_verify_write )
    { /* allocate second buffer for write verification */
        CHK( (buf_ver = calloc(1, buf_size)) != NULL );
    }

    if( f_desc > -1 && !opt_read )
    {
        if( opt_disable_file_rewind && ( size * opt_runs > file_size ) )
        {
            printf( "File too small to feed %d runs! Transfer size %d (0x%x), file size %d (0x%x)\n",
                    opt_runs,
                    size,
                    size,
                    file_size,
                    file_size );
            return EXIT_FAILURE;
        }
        if( !opt_disable_file_rewind && ( size > file_size ) )
        {
            printf( "File too small! Transfer size %d (0x%x), file size %d (0x%x)\n",
                    size,
                    size,
                    file_size,
                    file_size );
            return EXIT_FAILURE;
        }
    }

    /* Inform the user of which windows he accesses */
    sysMenPci2VmeSpaceName( spc,
                            spcName );
    busErrorParams.parameters = (void*)&gotBusError;
    sysMdisIntConnect( VME4VX_IRQVEC_BUSERR,
                       vmeBusErrorHandler,
                       (void*) &busErrorParams );

    if( opt_read )
    {
        printf( "=> Read from VME in %s mode.\n",
                spcName );
    }
    else
    {
        printf( "=> Write to VME in %s mode.\n",
                spcName );
    }

    for( i = 1; i <= opt_runs; i++ )
    {
        /*--- read ---*/
        if( opt_read )
        {
            /* measure time right before and after VME access without mem dumps */
            clock_gettime( CLOCK_MONOTONIC,
                           &t1 );
            gotBusError = 0;
            rv = vme_read( vmeAddr,
                           spc,
                           accWidth,
                           size,
                           buf,
                           opt_rw_flags );
            clock_gettime( CLOCK_MONOTONIC,
                           &t2 );

            if( ERROR == rv || TRUE == gotBusError )
            {
                printf( "Unable to read data from vme ! (VME Bus Error at address 0x%08x)\n",
                        busErrorParams.busErrorAddrs );
                return EXIT_FAILURE;
            }

            if( opt_dump ) MemDump( buf,
                                    rv,
                                    1 );

            if( file_name )
            {
                ret = write( f_desc,
                             buf,
                             rv );
                if( ret != rv )
                {
                    printf( "Unable to write to a file %s (ret=%d)\n",
                            file_name,
                            ret );
                    return EXIT_FAILURE;
                }
            }
        }
        /*--- write ---*/
        else
        {
            if( file_name )
            {
                ret = read( f_desc,
                            buf,
                            buf_size );
                if( ret != buf_size )
                {
                    printf( "Unable to read from a file %s (ret=%d)\n",
                            file_name,
                            ret );
                    return EXIT_FAILURE;
                }
            }
            else
            {
                p = buf;
                /* If filename is not specified check if buffer shall be filled */
                if( opt_use_startval )
                {
                    for( j = 0; j < size; j++ )
                    {
                        *p++ = ( j + opt_startval ) & 0xff;
                    }
                }
                if( opt_use_fillval )
                {
                    uint64_t word_swap;
                    printf( "Fill with data 0x%llx\n",
                            opt_fillval );
                    word_swap = opt_fillval;
                    for( j = 0; j < size; j += accWidth )
                    {
                        memcpy( p,
                                ( (uint8_t *) &word_swap ) + 8 - accWidth,
                                accWidth );
                        p += accWidth;
                    }
                }
            }

            if( opt_verify_write && opt_bufoffset )
            {
                /* prepare buffer for comparison */
                memcpy( buf_ver,
                        buf,
                        buf_size );
                /* clear only the part of a buffer to be used for comparison */
                memset( buf_ver + opt_bufoffset_val,
                        0,
                        size );
            }

            clock_gettime( CLOCK_MONOTONIC,
                           &t1 );
            gotBusError = 0;
            rv = vme_write( vmeAddr,
                            spc,
                            accWidth,
                            size,
                            buf + opt_bufoffset_val,
                            opt_rw_flags );
            clock_gettime( CLOCK_MONOTONIC,
                           &t2 );

            if( ERROR == rv || TRUE == gotBusError )
            {
                printf( "Unable to read data from vme ! (VME Bus Error at address 0x%08x)\n",
                        busErrorParams.busErrorAddrs );
                return EXIT_FAILURE;
            }

            if( opt_verify_write )
            {
                /* verify written data */
                rv = vme_read( vmeAddr,
                               spc,
                               accWidth,
                               size,
                               buf_ver + opt_bufoffset_val,
                               opt_rw_flags );
                if( rv != size )
                {
                    printf( "Unable to read data from vme %d (ret=%d)\n",
                            rv,
                            size );
                    return EXIT_FAILURE;
                }

                if( ( pos = memcmp_pos( buf,
                                        buf_ver,
                                        buf_size ) ) )
                {
                    printf( "Error during write verification at the position %d.\n",
                            abs( pos ) );
                    return_global = 1;
                    file_name_ver = calloc( 1,
                                            strlen( file_name ) + strlen( VERIFY_FILE_POSTFIX ) + 1
                                                + 9 );
                    if( !file_name_ver )
                    {
                        printf( "Unable to allocate memory\n" );
                        return EXIT_FAILURE;
                    }
                    if( opt_runs > 1 ) sprintf( file_name_ver,
                                                "%s" VERIFY_FILE_POSTFIX "%d",
                                                file_name,
                                                i );
                    else sprintf( file_name_ver,
                                  "%s" VERIFY_FILE_POSTFIX,
                                  file_name );
                    printf( "Writing read data into a file %s.\n",
                            file_name_ver );
                    f_ver_desc = open( file_name_ver,
                                       O_CREAT | O_SYNC | O_TRUNC | O_WRONLY,
                                       0666 );
                    if( !f_ver_desc )
                    {
                        printf( "Unable to open file %s\n",
                                file_name_ver );
                        return EXIT_FAILURE;
                    }
                    /* write bad data */
                    ret = write( f_ver_desc,
                                 buf_ver,
                                 buf_size );
                    if( ret != buf_size )
                    {
                        printf( "Unable to write to a file %s (ret=%d)\n",
                                file_name_ver,
                                ret );
                        return EXIT_FAILURE;
                    }
                    /* close descritor */
                    if( f_ver_desc )
                    {
                        close( f_ver_desc );
                    }
                    free( file_name_ver );
                    file_name_ver = NULL;
                }
                else
                {
                    printf( "Verification OK (%d bytes)\n",
                            size );
                }
            }
        }
        if( buf ) memset( buf,
                          0,
                          buf_size );
        if( buf_ver ) memset( buf_ver,
                              0,
                              buf_size );

        if( f_desc > -1 && !opt_disable_file_rewind )
        {
            /* rewind the file if an option is set */
            lseek( f_desc,
                   0,
                   SEEK_SET );
        }
        /* timeTotal(max) =  +1.7E+308, enough till eternity.. */
        timePerRun =
            (float) ( ( t2.tv_sec - t1.tv_sec ) * _1G + ( t2.tv_nsec - t1.tv_nsec ) ) / _1G;
        timeTotal += timePerRun;
    }

    if( f_desc )
    {
        close( f_desc );
    }

    transferRate = ( ( (float)size * (float)opt_runs ) / timeTotal ) / _1MB;

    printf( "%d %s access%s finished. average Time: %.3f s => average transfer rate: %.3f MB/s\n",
            opt_runs,
            opt_read ? "read" : "write",
            ( opt_runs > 1 ) ? "es" : "",
            timeTotal / opt_runs,
            transferRate );

    if( opt_verify_write && buf_ver )
    {
        free( buf_ver );
        buf_ver = NULL;
    }

    if( buf )
    {
        free( buf );
        buf = NULL;
    }
    return return_global;
}

static void MemDump( uint8_t *buf, uint32_t n, uint32_t fmt )
{
    uint8_t *k, *k0, *kmax = buf + n;
    int32_t i;
    printf( "----\n" );
    for( k = k0 = buf; k0 < kmax; k0 += 16 )
    {
        printf( "%04x: ",
                (unsigned int) ( k - buf ) );

        switch( fmt )
        { /* dump hex: */
            case 4: /* long aligned */
                for( k = k0, i = 0; i < 16; i += 4, k += 4 )
                    if( k < kmax ) printf( "%08lx ",
                                           *(uint32_t*)k );
                    else printf( "         " );
                break;
            case 2: /* word aligned */
                for( k = k0, i = 0; i < 16; i += 2, k += 2 )
                    if( k < kmax ) printf( "%04x ",
                                           *(uint16_t*)k & 0xffff );
                    else printf( "    " );
                break;
            default: /* byte aligned */
                for( k = k0, i = 0; i < 16; i++, k++ )
                    if( k < kmax ) printf( "%02x ",
                                           *k & 0xff );
                    else printf( "   " );
        }

        for( k = k0, i = 0; i < 16 && k < kmax; i++, k++ ) /* dump ascii */
            if( *(uint8_t*)k >= 32 && *(uint8_t*)k <= 127 ) printf( "%c",
                                                                    *k );
            else printf( "." );

        printf( "\n" );
    }
    printf( "----\n" );
}

static void vmeBusErrorHandler( VME4VX_BUSERR_PARAMS* pBusErrorParams )
{
    uint8_t* gotBusErrorP = (uint8_t*)pBusErrorParams->parameters;
    *(gotBusErrorP) = TRUE;
}
