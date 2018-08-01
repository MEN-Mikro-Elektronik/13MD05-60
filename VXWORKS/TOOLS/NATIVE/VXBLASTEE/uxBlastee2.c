/****************************************************************************
 ************                                                    ************
 ************                   uxBlastee2                       ************
 ************                                                    ************
 ****************************************************************************
 *
 *       Author: ck
 *        $Date: 2010/07/13 10:18:05 $
 *    $Revision: 1.1 $
 *
 *  Description: Utility to test UDP/TCP ethernet connection.
 *               Server application that reads modifies and 
 *               returns given message back to the client
 *
 *     Required: - 
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: uxBlastee2.c,v $
 * Revision 1.1  2010/07/13 10:18:05  CKauntz
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 2010 MEN Mikro Elektronik GmbH. All rights reserved.
 ****************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/times.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

int	blastNum;
int 	wdIntvl = 60;

#define	FALSE	0
#define TRUE	1

static void blastRate ();


/*****************************************************************************
 * blastee - server process on UNIX host
 *
 * DESCRIPTION
 *
 *     This is a server program which communicates with client through a
 *     TCP socket. It allows to configure the maximum size of socket-level 
 *     receive buffer. It repeatedly reads a given size message from the client
 *     and reports the number of bytes read every minute. 
 *
 * EXAMPLE:
 *
 *     To run this blastee task from UNIX host do the following: 
 *     % blastee   7000  1000  16000  0 &
 *
 *
 */


main (int argc, char	*argv[])
{
    struct sockaddr_in	serverAddr; /* server's address */
    struct sockaddr_in  clientAddr; /* client's address */
    char   *sendbuf;
    char   *recvbuf; 
    int	   sock;
    int    snew;
    int    len;
    int	   size;
    int	   blen;
    int    socktype = 0;
    int    count = 0;
    long   rc;

    if (argc < 5)
	{
	printf ("usage: %s port size bufLen sockettype\n"
            "         sockettype : 0 SOCKET_STREAM for TCP transfer\n"
            "                      1 SOCKET_DGRAM for UDP transfer\n", argv [0]);
	return (1);
	}

    size     = atoi (argv [2]);
    blen     = atoi (argv [3]);
    socktype = atoi (argv [4]);
    
    sendbuf = (char *) malloc (size);

    if (sendbuf == NULL)
    {
        perror ("cannot allocate sendbuf");
        return (1);
    }
    
    recvbuf = (char *) malloc (size);
    if (recvbuf == NULL)
    {
        perror ("cannot allocate recvbuf");
        free(sendbuf);
        return (1);
    }

	wdIntvl = 10;
    /* Associate SIGALARM signal with blastRate signal handler */
    signal (SIGALRM, blastRate);
    alarm (wdIntvl); /* Start signal handler after a minute */

    /* Zero out the sock_addr structures.
     * This MUST be done before the socket calls.
     */
    bzero (&serverAddr, sizeof (serverAddr));
    bzero (&clientAddr, sizeof (clientAddr));
 
   /* Open the socket. Use ARPA Internet address format and stream or dgram 
      sockets. */ 
    if (socktype == 0)  /* TCP transfer */
    {
        sock = socket (AF_INET, SOCK_STREAM, 0);
        printf(" TCP transfer \n");
    } 
    else if (socktype == 1)  /* UDP transfer */
    { 
        sock = socket (AF_INET, SOCK_DGRAM, 0);
        printf(" UDP transfer \n");
    }

    if (sock < 0)
	{
        perror ("cannot open socket");
        free(sendbuf);
        free(recvbuf);
        return (1);
	}

   /* Set up our internet address, and bind it so the client can connect. */   
    serverAddr.sin_family	= AF_INET;
    serverAddr.sin_port	= htons (atoi (argv [1]));

    if (bind (sock, (struct sockaddr *) &serverAddr, sizeof (serverAddr)) < 0)
	{
        perror ("bind error");
        close(sock);
        free(sendbuf);
        free(recvbuf);
        exit (1);
	}

   if (socktype == 0) { /* TCP / SOCK_STREM mode */
       /* Listen, for the client to connect to us. */
        if (listen (sock, 2) < 0)
        {
            perror ("listen failed");
            close(sock);
            free(sendbuf);
            free(recvbuf);
            exit (1);
        }
    }
    
    len = sizeof (clientAddr);
    
    blastNum = 0;
    
    while (1) 
    {
        if (socktype == 0)  /* TCP / SOCK_STREM mode */
        {    
            snew = accept (sock, (struct sockaddr *) &clientAddr, &len);
            if (snew == -1)
            {
                perror ("accept failed");
                close (sock);
                free(sendbuf);
                free(recvbuf);
                exit (1);
            }
        }         
        rc = 0;
        /* exchange data */
        while(rc!=-1) 
        {
            if (socktype == 0) /* TCP / SOCK_STREM mode */
            {
                rc=recv(snew,recvbuf,size,0); 
            }
            else              /* UDP / SOCK_DGRAM mode */
            {            
                rc=recvfrom(sock,recvbuf,size,0,
                    (struct sockaddr*)&clientAddr,&len);
            }
            if(rc==0) { 
                printf("Server has disconnected...\n"); 
                break; 
            }
            if(rc==-1) { 
                printf("Error: recv\n"); 
                break; 
            } 
            
      	    blastNum += rc;
            *(recvbuf+rc) ='\0';
            sprintf(sendbuf,"Client sends %s",recvbuf);
                        
            if (socktype == 0) /* TCP / SOCK_STREM mode */
            {
                rc=send(snew,sendbuf,(int)strlen(sendbuf),0); 
            } 
            else               /* UDP / SOCK_DGRAM mode */
            {     
                rc=sendto(sock,sendbuf,(int)strlen(sendbuf),0,
                    (struct sockaddr*)&clientAddr,len); 
            }
            if(rc==0) { 
                printf("Server has disconnected...\n"); 
                break; 
            }
            if(rc==-1) { 
                printf("Error: send\n"); 
                break; 
            } 
        } 
        close(snew);
    }


    if(recvbuf)
        free(recvbuf);
    if(sendbuf)
        free(sendbuf);
    if(sock>= 0)
        close(sock);
    if (snew>= 0)
        close (snew);
    printf ("blastee end.\n");
    }

/*****************************************************************************
 * blastRate - signal handler routine executed every one minute which reports
 *             number of  bytes read   
 *
 */

static void blastRate ()
    {
    if (blastNum > 0)
	{
	printf ("%d bytes/sec tot %d\n", blastNum / wdIntvl, blastNum);
	blastNum = 0;
	}
    else
	{
	printf ("No bytes read in the last %d seconds.\n", wdIntvl);
	}
    alarm (wdIntvl);
    }
