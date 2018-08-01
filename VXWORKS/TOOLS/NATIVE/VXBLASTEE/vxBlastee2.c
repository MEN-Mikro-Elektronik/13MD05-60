/****************************************************************************
 ************                                                    ************
 ************                   vxBlastee2                       ************
 ************                                                    ************
 ****************************************************************************
 *
 *       Author: ck
 *        $Date: 2010/07/09 13:34:56 $
 *    $Revision: 1.1 $
 *
 *  Description: Utility to test UDP ethernet connection
 *               Server application that reads modifies and 
 *               returns given message back to the client
 *
 *     Required: - 
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: vxBlastee2.c,v $
 * Revision 1.1  2010/07/09 13:34:56  CKauntz
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 2010 MEN Mikro Elektronik GmbH. All rights reserved.
 ****************************************************************************/


#include <socket.h>
#include <sockLib.h>
#include <stdlib.h>
#include <sysLib.h>
#include <logLib.h>
#include <errno.h>
#include <string.h> 
#include <stdio.h> 
#include "wdLib.h"
#include "in.h"		
#include "ioLib.h"	

LOCAL int	blastNum;
LOCAL int 	wdIntvl = 60; /* default */

LOCAL WDOG_ID	blastWd = NULL;
int	blasteeStop;
LOCAL void blastRate ();
 

/*****************************************************************************
 * blastee - server task for VxWorks target
 *
 * DESCRIPTION
 *
 *     This is a server program which communicates with client through a
 *     TCP or UDP socket. It allows to configure the maximum size of socket-level 
 *     receive buffer. It repeatedly reads a given size message from the client
 *     and reports the number of bytes read every minute. It stops receiving 
 *     the message when the global variable blasteeStop is set to 1. 
 *
 * EXAMPLE:
 *
 *     To run this blastee task from the VxWorks shell do as follows: 
 *     -> sp (blastee, 7000, 1000, 16000, 0)
 *
 *     To stop blastee task from the VxWorks shell do as follows: 
 *     -> blasteeStop = 1 
 *
 */


void blastee2 
(
    int   port,     /* the port number to read from */
    int   size,     /* size of the meesage */
    int   blen,     /* maximum size of socket-level receive buffer */
    int   socktype  /* socket type 0 = TCP, 1 = UDP */
)
{
    struct sockaddr_in	serverAddr; /* server's address */
    struct sockaddr_in  clientAddr; /* client's address */
    int	   sock = 0;
    int    snew = 0;
    int    addrlen;
    char   *sendbuf, *recvbuf; 
    int    count = 0;
    long   rc;

    if ((recvbuf = (char *) malloc (size)) == NULL)
    {
        perror ("cannot allocate buffer of size ");
        goto END;
    }

    if ((sendbuf = (char *) malloc (size+30)) == NULL)
    {
        perror ("cannot allocate send buffer");
        goto END;
    }
    
    bzero (recvbuf, size);
    bzero (sendbuf, size+30);

    /* Create watchdog timer */
    if (blastWd == NULL && (blastWd = wdCreate ()) == NULL)
    {
        perror ("cannot create blast watchdog");
        goto END;
    }

    /* Start watchdog after all 10 sec */
    wdIntvl = 10;
    wdStart (blastWd, sysClkRateGet () * wdIntvl, (FUNCPTR) blastRate, 0);

    /* Zero out the sock_addr structures.
     * This MUST be done before the socket calls.
     */
    bzero ((char *) &serverAddr, sizeof (serverAddr));
    bzero ((char *) &clientAddr, sizeof (clientAddr));

   /* Open the socket. Use ARPA Internet address format and stream sockets. */
    if (socktype == 0) {
        /* TCP Socket uses the SOCK_STREAM bit without a protocol */
        sock = socket (AF_INET, SOCK_STREAM, 0);
    } else if (socktype == 1) {
        /* UPD Socket uses the SOCK_DGRAM bit without a protocol */
        sock = socket (AF_INET, SOCK_DGRAM, 0);
    } else {
        perror ("invalid socket type");
        goto END;;
    }

    if (sock < 0)
    {
        perror ("cannot open socket");
        goto END;
    }

    /* Set up our internet address, and bind it so the client can connect. */
    serverAddr.sin_family	= AF_INET;
    serverAddr.sin_port	    = htons (port);
    serverAddr.sin_addr.s_addr=INADDR_ANY; /*inet_addr(largv[2]); // Targetcomputer is ower own */

    if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) < 0)
    {
        perror ("bind error");
        goto END;
    }
    
    if (socktype == 0) { /* TCP / SOCK_STREM mode */
        /* Listen, for the client to connect to us. */
        if (listen (sock, 10) < 0)
        {
            perror ("listen failed");
            goto END;
        }
    }
    addrlen = sizeof (clientAddr);

    blastNum = 0;
    blasteeStop = FALSE;
    
    while (1) 
    {
        if (blasteeStop == TRUE)
            break;
        if (socktype == 0) {/* TCP / SOCK_STREM mode */
            snew = accept(sock,(struct sockaddr*) &clientAddr, &addrlen);
            if( snew == ERROR){
                printf("Error: accept error\n");
                goto END;
            } else {    
                count++;
                printf(" Server is connected to client! (%d)\n",count);
            }
        }
         rc = 0;
        /* exchange data */
        while(rc!=ERROR) {
            
            if (blasteeStop == TRUE)
                break;
            
            if (socktype == 0) {/* TCP / SOCK_STREM mode */
                rc=recv(snew,recvbuf,size,0); 
            } else {            /* UDP / SOCK_DGRAM mode */
                rc=recvfrom(sock,recvbuf,size,0,
                    (struct sockaddr*)&clientAddr,&addrlen);
            }
            if(rc==0) { 
                printf("Server has disconnected...\n"); 
                break; 
            }
            if(rc==ERROR) { 
                printf("Error: recv\n"); 
                break; 
            } 
            
            blastNum += rc;
            *(recvbuf+rc) ='\0';
            sprintf(sendbuf,"Client sends %s",recvbuf);
            
            if (socktype == 0) {/* TCP / SOCK_STREM mode */
                rc=send(snew,sendbuf,(int)strlen(sendbuf),0); 
            } else {            /* UDP / SOCK_DGRAM mode */
                rc=sendto(sock,sendbuf,(int)strlen(sendbuf),0,
                    (struct sockaddr*)&clientAddr,addrlen); 
            }
            if(rc==0) { 
                printf("Server has disconnected...\n"); 
                break; 
            }
            if(rc==ERROR) { 
                printf("Error: send\n"); 
                break; 
            } 
        } 
        close(snew);
    }

END:
    wdCancel (blastWd);

    if(recvbuf)
        free(recvbuf);
    if(sendbuf)
        free(sendbuf);
    if(sock>= 0)
        close(sock);
    if (snew>= 0)
        close (snew);
    printf ("blastee2 end.\n");
}

/*****************************************************************************
 * blastRate - watchdog routine executed every one minute to report number of
 *             bytes read   
 *
 */

LOCAL void blastRate ()
{
    if (blastNum > 0)
    {
        logMsg ("%d bytes/sec\n", blastNum / wdIntvl,0, 0, 0, 0, 0);
        blastNum = 0;
    }
    else
    {
        logMsg ("No bytes read in the last %d seconds.\n", wdIntvl, 0, 0, 0, 0, 0);
    }
    wdStart (blastWd, sysClkRateGet () * wdIntvl, (FUNCPTR) blastRate, 0);
}
