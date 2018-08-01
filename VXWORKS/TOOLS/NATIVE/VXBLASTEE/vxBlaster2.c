/****************************************************************************
 ************                                                    ************
 ************                   vxBlaster2                       ************
 ************                                                    ************
 ****************************************************************************
 *
 *       Author: ck
 *        $Date: 2010/07/12 09:22:44 $
 *    $Revision: 1.1 $
 *
 *  Description: Utility to test UDP/TCP ethernet connection
 *               Client application that sends a message, receives
 *               and checks the content returned from the server
 *
 *     Required: - 
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: vxBlaster2.c,v $
 * Revision 1.1  2010/07/12 09:22:44  CKauntz
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 2010 MEN Mikro Elektronik GmbH. All rights reserved.
 ****************************************************************************/


/*****************************************************************************
 * blaster2 - client task for VxWorks target
 *
 * DESCRIPTION
 *
 *     This is a client task which connects to the server via TCP/UDP socket.
 *     It allows to configure the maximum size of the socket-level send and
 *     receive buffer. It repeatedly sends a given size message to the given port
 *     at destination target and waits for the reply to compare the changed
 *     content from the server. It stops sending the message when the global
 *     variable blaster2Stop is set to 1. 
 *
 * EXAMPLE:
 *
 *     To run this blaster2 task from the VxWorks shell do as follows: 
 *     -> sp (blaster2, "192.2.200.42", 7000, 1000, 16000, 0, 0) for infinite runs
 *     or only defined number of cycles (e.g.2): 
 *     -> blaster2 "192.2.200.42", 7000, 1000, 16000, 0, 2
 *
 *     To stop blaster2 task from the VxWorks shell do as follows: 
 *     -> blaster2Stop = 1 
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "vxWorks.h"
#include "socket.h"
#include "in.h"
#include <sockLib.h>
#include "arpa/inet.h"

int blaster2Stop;


int blaster2 
    (
    char    *targetAddr, /* targetAddress is the IP address of destination 
                            target that needs to be blasted */
    int	     port,       /* port number to send to */
    int      size,       /* size of the message to be sent */
    int      blen,       /* maximum size of socket-level send buffer */
    int      socktype,   /* socket type 0 = TCP, 1 = UDP */
    int      runs        /* number of runs, 0 = infinite runs */
    )
    {

    struct sockaddr_in	sin;
    int    sock = 0;
    char   *buffer, *recvbuf;
    /*int    optvar;*/   /* optional variable to store the boolean for SO_BROADCAST */
    int    currentruns = 0;
    int    recverr = 0, senderr = 0, compareerror = 0;

    bzero ((char *) &sin, sizeof (sin));

    if (socktype == 0) {   
        /* TCP Socket uses the SOCK_STREAM bit */
        sock = socket (AF_INET, SOCK_STREAM, 0);
    } else if (socktype == 1) {
        /* UPD Socket uses the SOCK_DGRAM bit without a protocol */
        sock = socket (AF_INET, SOCK_DGRAM, 0);
    } else {
        perror ("invalid socket type");
        return(1);
    }


    if (sock < 0)
    {
        perror ("cannot open socket");
        return (1);
    }

    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = inet_addr (targetAddr);
    sin.sin_port        = htons (port);


    if ((buffer = (char *) malloc (size)) == NULL)
    {
        perror ("cannot allocate buffer of size ");
        close (sock);
        return (1);
    }
    
    if ((recvbuf = (char *) malloc (size+30)) == NULL)
    {
        perror ("cannot allocate receive buffer of size ");
        free(buffer);
        close (sock);
        return (1);
    }
    if (setsockopt (sock, SOL_SOCKET, SO_SNDBUF, 
                    (char *)&blen, sizeof (blen)) < 0)
    {
        perror ("setsockopt SO_SNDBUF failed");
        free (buffer);
        free (recvbuf);
        close (sock);
        return (1);
    }
    if (setsockopt (sock, SOL_SOCKET, SO_RCVBUF,
                    (char*) &blen, sizeof (blen)) < 0)
    {
        perror ("setsockopt SO_RCVBUF failed");
        free (buffer);
        free (recvbuf);
        close (sock);
        return (1);
    }
    /*
    optvar = 1;
    if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST,
                    (char*) &optvar, 1) < 0)
    {
        perror ("setsockopt SO_BROADCAST failed");
        free (buffer);
        free (recvbuf);
        close (sock);
        return (1);
    }
*/
    if (connect (sock, (struct sockaddr  *)&sin, sizeof (sin)) < 0)
    {
        perror ("connect failed");
        printf ("host %s port %d\n", inet_ntoa (sin.sin_addr),
                ntohs (sin.sin_port));
        free (buffer);
        free (recvbuf);
        close (sock);
	    return (1);
	}

    blaster2Stop = FALSE;

    /* Send a data buffer of length size to the server repeatedly */ 
    for (;;)
    {
	    int nBytes;
	    
	    if (blaster2Stop == TRUE)
            break;
            
        /* fill the send buffer with defined values! */
        sprintf(buffer,"%d This is a Testframe to be send to the VxBlaster "
            "server!\n",currentruns);

        if ((nBytes = send(sock, buffer, size, 0)) < 0)
        {
            perror ("blaster2 write error");
            senderr++;
        }
 
        if ((nBytes = recv(sock, recvbuf, size, 0)) < 0)
        {
            perror ("blaster2 receive error");
            printf(" Receive failed! \n");
            recverr++;
        }
        if(nBytes > 0)
             recvbuf[nBytes]='\0';
        /* Check incoming frame against the sended frame. 13 bytes will
         *  be added by the server "Client sends "
         */
        if( strncmp(buffer, &recvbuf[13], nBytes) != 0){
            printf (" Receive error! Received frame: %s\n", recvbuf);
            compareerror++;
        }
        
        currentruns++;
        if ((runs == currentruns) && (runs != 0))
            break;
        if( !(currentruns % 200) )
            printf( "Curent status: %d Receive errors, %d Send errors, %d Cycles done"
            ", Compare errors %d.\n", recverr, senderr, currentruns, compareerror);
    }
    
    printf( "Status: %d Receive errors, %d Send errors, %d Cycles done, "
        "Compare errors %d.\n",recverr, senderr, currentruns, compareerror);

    close(sock);
    
    free (buffer);
    free (recvbuf);
    printf ("blaster2 exit.\n");
    if (compareerror || recverr || senderr){
        return 1; /* ERROR */
    } else {
        return 0; /* OK */
    }
            
}
