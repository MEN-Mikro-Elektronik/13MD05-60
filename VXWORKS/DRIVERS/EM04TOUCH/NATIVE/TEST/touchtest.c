/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  touchtest.c
 *
 *      \author  rla
 *        $Date: 2012/02/15 10:05:09 $
 *    $Revision: 1.4 $
 *
 *        \brief  Install and read touch device using select() routine
 *
 *     Switches: -
 */
/*---------------------------[ Public Functions ]----------------------------
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: touchtest.c,v $
 * Revision 1.4  2012/02/15 10:05:09  ts
 * R: exception occured when /touchScreen/0 wasnt created
 * M: fixed bug from Rev. 1.2: check if open() returns file handle < 0, not == 0
 *
 * Revision 1.3  2011/07/08 17:43:47  ts
 * R: After executing touchtest usage was unclear (returns at once with OK)
 * M: added describing printf that asks users to press the touch
 *
 * Revision 1.2  2007/11/23 15:13:13  cs
 * added:
 *   + when device already created through WindMl init use this one
 *     else: create device before open
 * changed:
 *   - name of created device from /touch to /touchScreen/0
 *
 * Revision 1.1  2003/04/10 17:21:34  Rlange
 * Initial Revision
 *
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2003 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/
static const char RCSid[]="$Id: touchtest.c,v 1.4 2012/02/15 10:05:09 ts Exp $";

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
LOCAL void touchread(int fd_touch);
int sysTouchDevCreate(char *name);

/**********************************************************************/
/** Routine to test touch panel select function
 *	/return 0
 */
int testtouch(void)
{
	int fd_touch = 0;
	int taskId;

	fd_touch = open("/touchScreen/0",2,0);

	if( fd_touch < 0 ) { /* touch was not created before */
		/* create Touch device */
		if( (sysTouchDevCreate("/touchScreen/0")) == ERROR ) {
			printf("*** sysTouchDevCreate(\"/touchScreen/0\") failed, exiting.\n" );
			return ERROR;
		}

		/* device existed but still error ? bail out */
		fd_touch = open("/touchScreen/0",2,0);
		if( fd_touch < 0 ) { 
			printf("*** open(\"/touchScreen/0\",2,0) failed, exiting.\n" );
			return ERROR;
		}
	}

	/* add task to select list */
	ioctl(fd_touch, FIOSELECT, SELREAD);

	taskId = taskSpawn("ttouchread",
						1,
						0,
						8192,
						(FUNCPTR) touchread,
						fd_touch,
						0,0,0,0,0,0,0,0,0);

	printf("Touch task spawned. Hit touch to generate events.\n" );

	return OK;
}


/**********************************************************************/
/** Routine to read touch data continuously
 *	/param	fd_touch	file descriptor of open touch device
 */
LOCAL void touchread(int fd_touch )
{
	TOUCH_PACKET pbuf;
	fd_set fdset;

	for(;;){
		/* init fdset varaiable */
		FD_ZERO(&fdset);
		FD_SET(fd_touch, &fdset);

		select(FD_SETSIZE, &fdset, NULL, NULL, NULL);
		read(fd_touch, (char*)&pbuf, sizeof(TOUCH_PACKET));
		printf(" touch hit : x=%d\ty=%d\tpen %s\n",pbuf.x,pbuf.y,
			(pbuf.penStatus == TOUCH_PEN_UP) ? "UP" : "DOWN");
	}
}
