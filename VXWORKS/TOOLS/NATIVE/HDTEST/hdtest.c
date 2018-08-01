/****************************************************************************
 ************                                                    ************
 ************   HDTEST                                          ************
 ************                                                    ************
 ****************************************************************************
 *
 *       Author: kp
 *        $Date: 2011/01/19 09:37:53 $
 *    $Revision: 1.10 $
 *
 *  Description: hdtest for VxWorks
 *
 *
 *     Required:
 *     Switches:
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: hdtest.c,v $
 * Revision 1.10  2011/01/19 09:37:53  UFranke
 * R: missing to be nice to other tasks
 * M: option -w=delay calls taskDelay( delay ) before write/read a subblock
 *
 * Revision 1.9  2009/01/07 11:09:28  Rlange
 * M: Option -r is not well documented
 * R: Cosmetic for -r Option documentation
 *
 * Revision 1.8  2008/05/09 14:00:05  ufranke
 * fixed
 *  - hdtest.c:169: warning: too many arguments for format
 *
 * Revision 1.7  2007/02/09 14:17:05  RLange
 * Added Raw mode option -r (without outputs)
 *
 * Revision 1.6  2007/01/05 19:56:13  cs
 * changed:
 *   - support multiple instances running at a time,
 *     move global variables into handle
 *
 * Revision 1.5  2006/10/20 11:54:55  cs
 * adaptions for VxWorks 6.3 (NFS creat() is POSIX conform!)
 *    !!! more than one pass on NFS doesn't work (yet) !!!
 *
 * Revision 1.4  2000/02/07 13:56:25  Franke
 * bugfix external_buffer initialisation
 *
 * Revision 1.3  2000/02/03 16:19:17  kp
 * allow to specify linear test pattern steps
 *
 * Revision 1.2  2000/01/14 08:36:22  kp
 * fixed problems with abort keys
 * cosmetics
 *
 * Revision 1.1  2000/01/12 10:56:30  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000..2011 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static char RCSid[]="$Id: hdtest.c,v 1.10 2011/01/19 09:37:53 UFranke Exp $";


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ioLib.h>
#include <errno.h>
#include <string.h>
#include <types.h>
#include <version.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/testutil.h>

#define SECTSIZE 256
#define KILO 1024
#define MEGA (KILO*KILO)

#define TSTOUT_Y	3

#define SUB_BLOCK_SIZE (h->buffersize/h->devidebuffer)

#define NUMTEST 4


typedef struct HDTEST_HANDLE_T{
	int abort_on_error;
	int linear_pat;

	int globerror;
	int passcount;
	int numberofpass;
	u_int32	starttick;
	u_int32 endtick;
	int maxseeks;
	int tapemode;
	int flashmode;
	int return_teststatus;
	int dont_delete_file;
	int scrmode;				/* use tu_ functions */
	u_int32 outAddr;
	u_int32 rdXfer;
	u_int32 wrtXfer;
	int customer_mode;
	int dont_fill_buffer;
	int dont_do_read_test;
	char *external_buffer;

	char *testfile;
	char *buffer;
	int  filesize;
	int  buffersize;
	int  devidebuffer;
	int  filedesc;
	FILE *logptr;
	struct TEST *tp;
	struct TEST *tpall;

	int minseek;
	int maxseek;

	int delay;

} HDTEST_HANDLE_T;

typedef struct TEST{
	char *name;
	int errorcnt;
	int lsterror;	/* write  scat  rand  read */
	int counter1;	/* byte   seeks seeks byte */
	int counter2;	/* bps    mst   mst   bps  */
	int counter3;
	int counter4;
	int (*func)(struct HDTEST_HANDLE_T *);
	void (*dispfunc)(struct HDTEST_HANDLE_T *, FILE *fp, int i);
} TEST;

static void version(void);
static int cleanup(HDTEST_HANDLE_T *h, int no);
static int test_it(HDTEST_HANDLE_T *h, int testno);
static int writetest(HDTEST_HANDLE_T *h);
static int readtest(HDTEST_HANDLE_T *h);
static int scatseek(HDTEST_HANDLE_T *h);
static int randseek(HDTEST_HANDLE_T *h);
static int seekandread(HDTEST_HANDLE_T *h, int pos);
static int error(HDTEST_HANDLE_T *h, int epos, int argc, ... );
static void compute_buffer(HDTEST_HANDLE_T *h, int start, long *buf);
static int verify_buffer(HDTEST_HANDLE_T *h, int start, long *buf, int noprint);
static void dispio(HDTEST_HANDLE_T *h, FILE *fp, int i);
static void dispseek(HDTEST_HANDLE_T *h, FILE *fp, int i);
static void statlog(HDTEST_HANDLE_T *h);
static void globstat(HDTEST_HANDLE_T *h);
static void printstats(HDTEST_HANDLE_T *h, FILE *fp, int i);
static void printscreen(HDTEST_HANDLE_T *h);
static void rewind_tape(HDTEST_HANDLE_T *h);
static void format_flash(HDTEST_HANDLE_T *h);
static u_int32 random( u_int32 n );
static int checkabort(HDTEST_HANDLE_T *h);

extern void wvEvent(int num);

static int usage(int no)
{
	fprintf(stderr, "%s%s",
	"Syntax  : hdtest2 [<opts>] <testfile> [<opts>]\n"
	"Function: harddisk read/write/seek tests\n"
	"   -a           abort on error\n"
	"   -c=<num>     special customer modes (1=AFT)\n"
	"   -d           don't delete testfile on exit\n"
    "   -e           return status of test as exitcode\n"
	"   -f=<num>     filesize in kilobyte          (10 MB)\n"
	"   -g=<incr>    use linear testpattern with increment 1,2,4\n"
	"   -l[=<path>]  create logfile                (./hdtest.log)\n"
	"   -n[=<num>]   number of passes (0=indef.)   (1)\n"
	"   -m=<num>     read/write buffer using <num> read/write calls\n"
	"   -p=<num>     buffersize for read&write     (100 kB)\n"
	"   -s=<num>     max. number of seeks          (200)\n"
	"   -r=<bufptr>  raw terminal (non screen) mode\n"
	"                bufptr: char pointer to output buffer\n"
	"   -t           tape mode\n"
	"   -w=<delay>   wait/taskDelay(delay)\n"
  "   -x           flash eprom mode\n"
  "   -z=<adr>     use external buffer (2k min)\n",
	RCSid
	);
	fprintf(stdout, "built: %s %s\n",__DATE__,__TIME__);
	version();
	return 1;
}

static void version(void)
{
	char *rev = "$Revision: 1.10 $";
	char *p = rev+strlen("$Revision: ");

	fprintf(stderr, "\nV ");
	while( *p != '$' ) fprintf(stderr, "%c", *p++);
	fprintf(stderr, " (c) Copyright 1995-2011 by MEN GmbH\n");
	fprintf(stderr, " built: %s %s\n",__DATE__, __TIME__);
}


static void init_tests(TEST *tests)
{
	int i;
	for(i=0; i<NUMTEST; i++ ){
		tests[i].errorcnt = 0;
		tests[i].lsterror = 0;
		tests[i].counter1 = 0;
		tests[i].counter2 = 0;
		tests[i].counter3 = 0;
		tests[i].counter4 = 0;
	}
	tests[0].name = "Write File";
	tests[0].func = writetest;
	tests[0].dispfunc = dispio;

	tests[1].name = "Scatter Seeks";
	tests[1].func = scatseek;
	tests[1].dispfunc = dispseek;
	tests[1].counter3 = 0x7fffffff;

	tests[2].name = "Random Seeks";
	tests[2].func = randseek;
	tests[2].dispfunc = dispseek;
	tests[2].counter3 = 0x7fffffff;

	tests[3].name = "Read & Verify";
	tests[3].func = readtest;
	tests[3].dispfunc = dispio;
}


int main( int argc, char *argv[] )
{
	char *logfile = NULL;
	char *optp,*errstr,errbuf[40], *tmp;
	int i,abort;
	HDTEST_HANDLE_T hdtH;
	TEST tests[NUMTEST];

	/* initialize handle to 0 */
	for(tmp = (char*)&hdtH, i = 0; i < sizeof(hdtH); i++)
		tmp[i] = 0;

	hdtH.testfile = "testf";
	hdtH.globerror = 0;
	hdtH.passcount = 0;
	hdtH.numberofpass = 1;
	hdtH.maxseeks = 200;
	hdtH.tapemode = 0;
	hdtH.flashmode = 0;
	hdtH.return_teststatus = 0;
	hdtH.dont_delete_file = 0;
	hdtH.scrmode = 1;
	hdtH.outAddr = 0x0;
	hdtH.rdXfer = 0;
	hdtH.wrtXfer = 0;
	hdtH.dont_fill_buffer = 0;
	hdtH.dont_do_read_test = 0;
	hdtH.external_buffer = NULL;
	hdtH.buffer = NULL;
	hdtH.filesize = 10*MEGA;
	hdtH.buffersize = 100 * KILO;
	hdtH.devidebuffer = 1;
	hdtH.filedesc = -1;
	hdtH.logptr  = NULL;
	hdtH.tpall = tests;
	hdtH.delay = 0;

	init_tests( tests );

	if ((errstr = UTL_ILLIOPT("?ac=def=g=n=m=l=p=s=r=txz=w=", errbuf))) {
		printf("*** %s\n",errstr);
		return(1);
	}

	/*
	 * get options
	 */
	if(UTL_TSTOPT("?")) {
		usage(0);
		return 1;
	}

	logfile = UTL_TSTOPT("l=");

   	if((optp = UTL_TSTOPT("w="))) hdtH.delay = atoi(optp);
   	if((optp = UTL_TSTOPT("n="))) hdtH.numberofpass = atoi(optp);
	if((optp = UTL_TSTOPT("p="))) hdtH.buffersize = atoi(optp) * KILO;
	if((optp = UTL_TSTOPT("f="))) hdtH.filesize = atoi(optp) * KILO;
	if((optp = UTL_TSTOPT("s="))) hdtH.maxseeks = atoi(optp);
	if((optp = UTL_TSTOPT("m="))) hdtH.devidebuffer = atoi(optp);

	if((optp = UTL_TSTOPT("c="))) {
		hdtH.customer_mode = atoi(optp);
		switch( hdtH.customer_mode ){
		case 1:					/* AFT, test DMA from single M-module cell */
								/* requires special HD driver */
			hdtH.dont_fill_buffer++;
			hdtH.dont_do_read_test++;
			hdtH.maxseeks=0;
			break;
		default:
			fprintf(stderr,"unkown customer mode\n");
			exit(1);
		}
	}
	if(UTL_TSTOPT("a")) hdtH.abort_on_error++;
	if((optp=UTL_TSTOPT("g="))) hdtH.linear_pat=atoi(optp);
	if(UTL_TSTOPT("e")) hdtH.return_teststatus++;
	if((UTL_TSTOPT("d"))) hdtH.dont_delete_file++;
	if((optp = UTL_TSTOPT("r="))) {
		hdtH.scrmode = 0;
		sscanf( optp, "%x", (int*)&hdtH.outAddr);
		printf("option -r = 0x%x\n",(unsigned int)hdtH.outAddr);
	}

	if((optp = UTL_TSTOPT("z="))){
		sscanf( optp, "%x", (int*)&hdtH.external_buffer);
	}

	if(UTL_TSTOPT("t")) {
		hdtH.tapemode++;
		hdtH.maxseeks = 0;
		hdtH.testfile = "/mt0";
	}
	if(UTL_TSTOPT("x")) {
		hdtH.flashmode++;
		hdtH.maxseeks = 0;
		hdtH.testfile = "/mc0";
	}

	/*
	 * get name of the testfile
	 */

	for(i=1; i<argc; i++) {
		if(*argv[i] == '-') continue;
		hdtH.testfile = argv[i];
	}

	/*
	 * get buffer memory
	 */
	if( hdtH.external_buffer ){
		hdtH.buffer = hdtH.external_buffer;
	}

	else {
		if(!(hdtH.buffer = malloc(hdtH.buffersize))){
			printf("can't get buffer memory!");
			return 1;
		}
	}

	/*
	 * create the file
	 */
	if( !hdtH.flashmode ){
#if ( ( _WRS_VXWORKS_MAJOR == 6 ) && ( _WRS_VXWORKS_MINOR >= 3 ) )
		/* for VxWorks >= 6.3 the NFS file systems creat() function takes POSIX
		   conform permission parameters (unix like). Therefore use open() with
		   permission to create file, internally it is handled correctly */
		remove(hdtH.testfile);
		if( (hdtH.filedesc = open( hdtH.testfile, (O_CREAT | O_RDWR), 0) ) < 0 )
#else
		if( (hdtH.filedesc = creat( hdtH.testfile, O_RDWR )) < 0 )
#endif
 		{
			printf("can't create file");
			return 1;
		}

	}
	else {
		char nbuf[100];
		strcpy( nbuf, hdtH.testfile );
		strcat( nbuf, "@" );

		if(( hdtH.filedesc = open(nbuf, O_RDWR, 0)) < 0  ){
			printf("can't open %s", nbuf);
			return 1;
		}
	}
	/*
	 * open the logfile if required
	 */
	if(logfile)
		if((hdtH.logptr = fopen(logfile, "w")) == NULL){
			printf("can't open logfile %s", logfile);
			return 1;
		}

	/*
	 * init screen I/O
	 */
	if( hdtH.scrmode ){
		tu_init_screen( hdtH.testfile, 46, 7, 1 );

		printscreen(&hdtH);
	}

	hdtH.starttick = tu_get_tick();

	abort = 0;
	for(hdtH.passcount = 1; (hdtH.passcount <= hdtH.numberofpass || (hdtH.numberofpass == 0)) &&
			!abort;
		hdtH.passcount++){
		hdtH.tp = tests;
		for(i=0; i<NUMTEST && !abort; i++, hdtH.tp++){
			globstat(&hdtH);
			if( test_it(&hdtH, i)) {
				abort++;
				break;
			}
			if( checkabort(&hdtH)) {
				abort++;
				break;
			}
			printstats(&hdtH, stdout, i);
		}
	}

	return cleanup(&hdtH, 0);
}

static int checkabort(HDTEST_HANDLE_T *h)
{
	int c;

	if( h->scrmode ){
		switch( c = tu_keybd_hit()){
		case 0x3:					/* CTRL-C */
		case 0x4:					/* CTRL-D */
		case 0x5:					/* CTRL-E */
		case 0x1b:					/* ESC */
			return 1;				/* abort */
		}
	}
	return 0;
}

static int cleanup(HDTEST_HANDLE_T *h, int no)
{
	h->endtick = tu_get_tick();
	h->passcount--;
	globstat(h);
	if(h->filedesc){
		close(h->filedesc);
		if( !h->tapemode && !h->flashmode && !h->dont_delete_file)
			remove(h->testfile);/* delete the testfile */
	}
	if(h->logptr){
		statlog(h);
		fclose(h->logptr);
	}

	if( !h->external_buffer && h->buffer)
		free( h->buffer );

	if( h->scrmode ){
		tu_exit();
	}
	else{
		if( h->outAddr ){
			sprintf((char*)h->outAddr, "-e=%d -w=%d -r=%d ", h->globerror,
				(int)h->wrtXfer, (int)h->rdXfer );
		}

		/* printf("%d total errors\n", h->globerror ); */
	}

	return no;
}

static int test_it(HDTEST_HANDLE_T *h, int testno)
{
	/* clear test output area */
	if( h->scrmode ){
		tu_clear_wdw(WDW_MSG);
		tu_print(WDW_MSG, 0, 1, "Test #%d: %s", testno, h->tp->name);
	}
	else{
		/* printf( "Test #%d: %s\n", testno, h->tp->name); */
	}
	return (*h->tp->func)(h); /* call test function */
}


static int writetest(HDTEST_HANDLE_T *h)
{
	int i;
	int filepos = 0;
	int bufferpos;
	int sttick;
	int tottick = 0;
	int bytesout = 0;
	int sub_block_size = SUB_BLOCK_SIZE;
	char *bp;
	char *line="Bytes-Out Time     XferRate";

	if( h->tapemode ) rewind_tape(h);
	if( h->flashmode) format_flash(h);

	if( h->scrmode ){
		tu_print(WDW_MSG, 0, TSTOUT_Y+2, line );
		tu_print(WDW_MSG, 0, TSTOUT_Y+3, tu_underline_string(line) );
	}

	for(; filepos <= (h->filesize-h->buffersize); filepos += h->buffersize){

		if( checkabort(h)) return 1;

		if( !h->dont_fill_buffer ){
			if( h->scrmode ){
				tu_print(WDW_MSG, 0, TSTOUT_Y, " computing sector #%08d...",
					 (filepos)/SECTSIZE);
			}

			for(bufferpos = 0, bp=h->buffer; bufferpos < h->buffersize;
				bufferpos += SECTSIZE,	 bp += SECTSIZE)
				compute_buffer(h, bufferpos + filepos, (long *)bp);
		}

		if( h->scrmode ){
			tu_print(WDW_MSG, 0, TSTOUT_Y, "writing to sector #%08d...",
				 filepos/SECTSIZE);
		}
		else{
			/* printf( "writing to sector #%08d...", filepos/SECTSIZE); */
		}

		if( !h->tapemode && lseek(h->filedesc, filepos, 0) == -1){
			if( error(h, filepos, 0)) return 1;
		}
		else {
			sttick = tu_get_tick();

			for(bp=h->buffer, i=0; i<h->devidebuffer; i++, bp+=sub_block_size )
			{
				if( h->delay )
				{
					taskDelay( h->delay );
				}
				if(write(h->filedesc, bp, sub_block_size) != sub_block_size)
				{
					if( error(h, filepos, 0)) return 1;
					break;
				}
			}

			if( i==h->devidebuffer ){
				tottick += (tu_get_tick() - sttick);
				bytesout += sub_block_size*h->devidebuffer/SECTSIZE ;

				if( h->scrmode ){
					tu_print(WDW_MSG, 0, TSTOUT_Y+4, " %8d %s  %s",
						 bytesout*SECTSIZE, tu_tick2string(2,tottick),
						 tu_bytes_per_sec(bytesout*SECTSIZE,tottick));
				}
				else{
					/* printf(" %8d %s %s\n",
					 bytesout*SECTSIZE, tu_tick2string(2,tottick),
					 tu_bytes_per_sec(bytesout*SECTSIZE,tottick)); */
				}
			}
		}
	}
	h->tp->counter1 += bytesout;
	h->tp->counter2 += tottick;

	return 0;
}

static int readtest(HDTEST_HANDLE_T *h)
{
	int i;
	int filepos = 0;
	int bufferpos;
	int sttick;
	int tottick = 0;
	int bytesin = 0;
	int vecnt   ;
	int sub_block_size = SUB_BLOCK_SIZE;
	char *bp;
	char *line="Bytes-In  Time     XferRate";


	if( h->dont_do_read_test ) return 0;

	if(!h->tapemode) lseek(h->filedesc, 0, 0);
	else rewind_tape(h);

	if( h->scrmode ){
		tu_print(WDW_MSG, 0, TSTOUT_Y+2, line );
		tu_print(WDW_MSG, 0, TSTOUT_Y+3, tu_underline_string(line) );
	}

	for(; filepos <= (h->filesize-h->buffersize); filepos += h->buffersize){

		if( checkabort(h)) return 1;

		if( h->scrmode ){
			tu_print(WDW_MSG, 0, TSTOUT_Y, "  reading sector #%08d...",
				 filepos/SECTSIZE);
		}
		else{
			/* printf( "  reading sector #%08d...", filepos/SECTSIZE); */
		}

		if(!h->tapemode && lseek(h->filedesc, filepos, 0) == -1){
			if( error(h, filepos, 0)) return 1;
		}
		else {
			sttick = tu_get_tick();
			for(bp=h->buffer, i=0; i<h->devidebuffer; i++, bp+=sub_block_size)
			{
				if( h->delay )
				{
					taskDelay( h->delay );
				}
				if(read(h->filedesc, bp, sub_block_size) != sub_block_size)
				{
					if( error(h, filepos, 0)) return 1;
						break;
				}
			}

			if( i==h->devidebuffer) {

				tottick += (tu_get_tick() - sttick);
				bytesin += sub_block_size*h->devidebuffer/SECTSIZE ;

				if( h->scrmode ){
					tu_print(WDW_MSG, 0, TSTOUT_Y+4, " %8d %s  %s",
							 bytesin*SECTSIZE, tu_tick2string(2,tottick),
							 tu_bytes_per_sec(bytesin*SECTSIZE,tottick));

					tu_print(WDW_MSG, 0, TSTOUT_Y,
							 "verifying sector #%08d...",
							 filepos/SECTSIZE);
				}
				else{
					/*
					printf( " %8d %s  %s\n",
							 bytesin*SECTSIZE, tu_tick2string(2,tottick),
							 tu_bytes_per_sec(bytesin*SECTSIZE,tottick)); */
				}

				vecnt=0;
				for(bufferpos = 0, bp=h->buffer; bufferpos < h->buffersize;
				 bufferpos += SECTSIZE,	 bp += SECTSIZE){
					int rv;
					rv = verify_buffer(h, bufferpos+filepos, (long *)bp,
										   vecnt>50);
					if( rv < 0 ) return 1;
					vecnt += rv;
				}
				if(vecnt>50){
					if( h->scrmode ){
						tu_clear(WDW_MSG, 0, TSTOUT_Y+6, 0);
						tu_clear(WDW_MSG, 0, TSTOUT_Y+7, 0);
						tu_print(WDW_MSG, 0, TSTOUT_Y+6, "More Verify Errors %d",
								 vecnt-50);
					}
					else{
						/* printf( "More Verify Errors %d", vecnt-50); */
					}

					errno = EFAULT;
					if( error(h, bufferpos+filepos, 1, vecnt)) return 1;
				}
			}
		}
	}
	h->tp->counter1 += bytesin;
	h->tp->counter2 += tottick;

	return 0;
}

#define SCAT_STEP (10*KILO)
static int scatseek(HDTEST_HANDLE_T *h)
{
	int lopos, highpos, i;
	char *line = "Seek-Time";

	h->minseek = h->tp->counter3;
	h->maxseek = h->tp->counter4;

	if( h->scrmode ){
		tu_print(WDW_MSG,0,TSTOUT_Y+2, line );
		tu_print(WDW_MSG,0,TSTOUT_Y+3, tu_underline_string(line) );
	}

	highpos = h->filesize - SCAT_STEP;
	lopos = 0;

	for(i=h->maxseeks; i>0 && lopos < highpos;
	 lopos+=SCAT_STEP, highpos-=SCAT_STEP, i--){
		if( seekandread(h, lopos)) return 1;
		if( seekandread(h, highpos)) return 1;
	}
	h->tp->counter3 = h->minseek;
	h->tp->counter4 = h->maxseek;

	return 0;
}


static int randseek(HDTEST_HANDLE_T *h)
{
	int i;
	static int sect = 0;
	char *line = "Seek-Time";

	h->minseek = h->tp->counter3;
	h->maxseek = h->tp->counter4;

	if( h->scrmode ){
		tu_print(WDW_MSG,0,TSTOUT_Y+2, line );
		tu_print(WDW_MSG,0,TSTOUT_Y+3, tu_underline_string(line) );
	}

	for(i=0; i<h->maxseeks; i++){
		sect = random(sect);
		if( seekandread(h,((unsigned)(sect & 0xffff)) * (h->filesize/KILO)/0x40))
			return 1;
	}
	h->tp->counter3 = h->minseek;
	h->tp->counter4 = h->maxseek;
	return 0;
}

#define SEEK_RSIZE 2048
static int seekandread(HDTEST_HANDLE_T *h, int pos)
{
	int sttick, rv;

	if( checkabort(h)) return 1;

	if( h->scrmode ){
		tu_print(WDW_MSG,0,TSTOUT_Y,"seeking to byte %8d...", pos);
	}

	sttick = tu_get_tick();

	if((rv = lseek(h->filedesc, pos, 0)) != pos){
/*printf("lseek err rv=%d ", rv );*/
		if( error(h, pos, 0)) return 1;
	}
	else {
		if((rv =read(h->filedesc, h->buffer, SEEK_RSIZE)) == -1){
/*printf("read err rv=%d ", rv );*/
			if( error(h, pos, 0)) return 1;
		}
		else {
			sttick = tu_get_tick() - sttick;
			if(sttick > h->maxseek) h->maxseek = sttick;
			if(sttick < h->minseek) h->minseek = sttick;
			h->tp->counter2 += sttick;
			h->tp->counter1++;

			if( h->scrmode ){
				tu_print(WDW_MSG,0,TSTOUT_Y+4, tu_tick2string(3,sttick));
			}
		}
	}
	return 0;
}

static int error(HDTEST_HANDLE_T *h, int epos, int argc, ... )
{
	int i;
	va_list ap;
	char buf[80];
	char *p;

	h->globerror++;
	h->tp->errorcnt++;
	h->tp->lsterror = errno;

	p=buf;
	p+=sprintf(p, " %8d  %8x", errno, epos);
	va_start(ap,argc);

	for(i=0;i<argc;i++)
		p+=sprintf(p, " %08x", (int)va_arg(ap,u_int32));

	va_end(ap);

	if( h->scrmode ){
		if(tu_print_error(1, 1, buf)){
			return 1;
		}

		if(h->abort_on_error) return 1;
	}
	else{
		printf( "\n*** Error %s\n", buf );
		if(h->abort_on_error) return 1;
	}

	if(h->logptr){
		va_start(ap,argc);
		fprintf(h->logptr, "%s", tu_tick2string(0,tu_get_tick() - h->starttick));
		fprintf(h->logptr, " %d:%-20s %8d %08x", h->globerror, h->tp->name, errno, epos);
		for(i=1; i<=argc; i++)
			fprintf(h->logptr, " %08x", va_arg(ap,int));
		fprintf(h->logptr, "\n");
		va_end(ap);
	}
	return 0;
}

static void compute_buffer(HDTEST_HANDLE_T *h, int start, long *buf)
{
	register int i;

	if(! h->linear_pat){
		for(i=0; i<SECTSIZE; i+=4, buf++)
			start = *buf = random(start);
	}
	else {
		switch( h->linear_pat ){
		case 1:
		{
			u_int8 *p = (u_int8 *)buf;

			for(i=0; i<SECTSIZE; i+=1)
				*p++ = i+start;
			break;
		}

		case 2:
		{
			u_int16 *p = (u_int16 *)buf;

			for(i=0; i<SECTSIZE; i+=2)
				*p++ = i+start;
			break;
		}

		case 4:
		{
			u_int32 *p = (u_int32 *)buf;

			for(i=0; i<SECTSIZE; i+=4)
				*p++ = i+start;
			break;
		}
		default:
			break;
		}
	}
}

static int verify_buffer(HDTEST_HANDLE_T *h, int start, long *buf, int noprint)
{
	register int i, n=0;
	register int ecnt=0;
	int epos = start;

	for(i=0; i<SECTSIZE; i+=4, buf++){
		if( !h->linear_pat ){
			start = n = random(start);
		}
		else {
			switch( h->linear_pat ){
			case 1:
				n = ((((start+i)&0xff))<<24) + ((((start+i+1)&0xff))<<16) +
					((((start+i+2)&0xff))<<8) + ((((start+i)&0xff)+3));
				break;
			case 2:
				n = ((((start+i)&0xffff))<<16) + ((((start+i+2)&0xffff)));
				break;
			case 4:
				n = start+i;
				break;
			}
		}

		if(*buf != n){
			ecnt++;
			if(noprint == 0){
				if( h->scrmode ){
					tu_clear(WDW_MSG,0,TSTOUT_Y+6,0);
					tu_clear(WDW_MSG,0,TSTOUT_Y+7,0);
					tu_print(WDW_MSG,0,TSTOUT_Y+6,
							 "VerifyErr at %8d ($%08x)", epos+i, epos+i);
					tu_print(WDW_MSG,0,TSTOUT_Y+7,"should be $%08x, is $%08x",
							 n, *buf);
				}
				else{
					printf( "VerifyErr at %8d ($%08x) ", epos+i, epos+i);
					printf( "should be $%08x, is $%08x",
							 n, (int)*buf);
				}

				errno = EFAULT;
				if( error(h, epos+i, 2, n, *buf))
					return -1;
			}
		}
	}
	return ecnt;
}

static void dispio(HDTEST_HANDLE_T *h, FILE *fp, int i)
{
	if( fp!=stdout ){
		if( h->scrmode ){
			fprintf(fp, "%4d.%02d MB / xfer rate: %s",
					h->tp->counter1/(KILO*4),
					(h->tp->counter1 % (4*KILO)) * 100 / (KILO*4),
					tu_bytes_per_sec(h->tp->counter1*SECTSIZE, h->tp->counter2));
		}
	}
	else {
		tu_print(WDW_RESULT,42,2+i,"%4d.%02d MB / xfer rate: %s",
				h->tp->counter1/(KILO*4),
				(h->tp->counter1 % (4*KILO)) * 100 / (KILO*4),
				tu_bytes_per_sec(h->tp->counter1*SECTSIZE, h->tp->counter2));
	}
}

static void dispseek(HDTEST_HANDLE_T *h, FILE *fp, int i)
{
	if(h->tp->counter1){
		if(fp!=stdout ){
			if( h->scrmode ){
				fprintf(fp, "%8d seeks, %5d / %5d / %5d",
						h->tp->counter1, h->tp->counter3*10,
						h->tp->counter2*10/h->tp->counter1,
						h->tp->counter4*10 );
			}
		}
		else {
			tu_print(WDW_RESULT,42,2+i,"%8d seeks, %5d / %5d / %5d",
					h->tp->counter1, h->tp->counter3*10,
					h->tp->counter2*10/h->tp->counter1,
					h->tp->counter4*10 );
		}
	}
}

static void statlog(HDTEST_HANDLE_T *h)
{
	int i;
	TEST *tp = h->tpall;

	fprintf(h->logptr, "\n***** Test completed ******\n");
	fprintf(h->logptr, "testfile: %s, filesize: %d, buffersize: %d\n",
     		h->testfile, h->filesize, h->buffersize);
   	fprintf(h->logptr, "\nTotal Time: %s",
			tu_tick2string(0, h->endtick - h->starttick));
	fprintf(h->logptr, ", passes: %d, Errors: %d\n\n",
			h->passcount, h->globerror);

	for(i=0; i<NUMTEST; i++, tp++){
		fprintf(h->logptr, "%d. %-15s ", i, tp->name);
		h->tp = tp;
		printstats(h, h->logptr, i);
	}
}


static void globstat(HDTEST_HANDLE_T *h)
{
	if( h->scrmode ){
		tu_print_elapsed_time();
		tu_print_pass(h->passcount,h->numberofpass);
	}
}

static void printstats(HDTEST_HANDLE_T *h, FILE *fp, int i)
{
	if(fp != stdout )
		if( h->scrmode ){
			fprintf(fp, "%8d  %8d\n", h->tp->errorcnt, h->tp->lsterror);
		}
		else{
			switch( i ){
				case 0:
					h->wrtXfer = (h->tp->counter1*SECTSIZE/h->tp->counter2) * 100;
					break;
				case 3:
					h->rdXfer = (h->tp->counter1*SECTSIZE/h->tp->counter2) * 100;
					break;
			}
		}
	else{
		if( h->scrmode ){
			tu_print(WDW_RESULT,20,2+i,"%8d  %8d  ", h->tp->errorcnt, h->tp->lsterror);
			(*h->tp->dispfunc)(h, fp, i);
		}
		else{
			switch( i ){
				case 0:
					h->wrtXfer = (h->tp->counter1*SECTSIZE/h->tp->counter2) * 100;
					break;
				case 3:
					h->rdXfer = (h->tp->counter1*SECTSIZE/h->tp->counter2) * 100;
					break;
			}
		}
	}
}

static void printscreen(HDTEST_HANDLE_T *h)
{
	int i;
	char *res="statistics:         Errcount  Last-Err";
	TEST *test=h->tpall;

    tu_print_errhead("Error     ErrorPos  Info-1   Info-2  ");


    tu_print(WDW_RESULT,0,0,res);
    tu_print(WDW_RESULT,0,1,tu_underline_string(res));

    for(i=0; i<NUMTEST; i++)
		tu_print(WDW_RESULT,0,2+i,"%-20s\n", test[i].name);
}



static void rewind_tape(HDTEST_HANDLE_T *h)
{
	if( h->scrmode ){
		tu_print(WDW_MSG, 0, TSTOUT_Y, "rewinding tape...");
	}
	else{
		printf( "rewinding tape...\n");
	}

#if 0
	/*???*/
	if( _ss_rest( h->filedesc ) < 0 )
		error(h->tp, 0, 0);
#endif
	if( h->scrmode ){
		tu_clear(WDW_MSG, 0, TSTOUT_Y, 0);
	}
}

static void format_flash(HDTEST_HANDLE_T *h)
{
#if 0
	int err;
#endif
	if( h->scrmode ){
		tu_print(WDW_MSG, 0, TSTOUT_Y, "formatting flash...");
	}
	else{
		printf("formatting flash...\n");
	}

#if 0
	if(err = _os_ss_wtrack( h->filedesc, NULL, NULL, 0, 0, 0 )){
		errno = err;
		error(h->tp, 0, 0);
	}
#endif
	if( h->scrmode ){
		tu_clear(WDW_MSG, 0, TSTOUT_Y, 0);
	}
}

static u_int32 random( u_int32 n )
{
    register u_int32 a = n;

    a <<= 11;
    a += n;
    a <<= 2;
    n += a;
    n += 13849;
	return n;
}


