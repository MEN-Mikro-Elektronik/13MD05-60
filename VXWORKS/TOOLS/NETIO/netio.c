/* netio.c
 *
 * Author:  Kai-Uwe Rommel <rommel@ars.de>
 * Created: Wed Sep 25 1996
 */

static char *rcsid =
	"$Id: vx_netio.c,v 1.2 2012/07/04 14:41:42 ts Exp $";
static char *rcsrev = "$Revision: 1.2 $";

/*
 * $Log: vx_netio.c,v $
 * Revision 1.2  2012/07/04 14:41:42  ts
 * R: usage too static, worked only as server
 * M: added simple option parsing( -u,-t, -b, -s, IP address)
 *
 *
 * Revision 1.26  2005/08/30 14:45:51  Rommel
 * minor fixes
 *
 * Revision 1.25  2004/05/26 07:23:04  Rommel
 * some more corrections from Oliver Lau and Frank Schnell
 *
 * Revision 1.24  2004/05/17 16:01:03  Rommel
 * fix for _send/_recv from Thomas Jahns
 *
 * Revision 1.23  2003/09/30 09:32:22  Rommel
 * corrections from Matthias Scheler for error handling
 * added socket buffer size setting
 * added htonl/ntohl code (hint from Oliver Lau)
 * restructured send/recv error/result checking
 * more verbose server side messages
 * other minor changes
 *
 * Revision 1.22  2003/09/22 14:58:33  Rommel
 * added server side progress messages
 *
 * Revision 1.21  2003/08/28 12:44:11  Rommel
 * fixed display of non-k-multiple packet sizes
 *
 * Revision 1.20  2003/08/27 11:05:48  Rommel
 * allow block size specifikation in bytes or k bytes
 *
 * Revision 1.19  2003/08/17 16:53:45  Rommel
 * added Unix/Linux pthreads support (required for UDP)
 *
 * Revision 1.18  2003/08/17 14:46:17  Rommel
 * added UDP benchmark
 * several minor changes (cleanup)
 * configurable binding address
 *
 * Revision 1.17  2003/07/12 17:25:00  Rommel
 * made block size selectable
 *
 * Revision 1.16  2003/02/10 09:06:59  Rommel
 * fixed sender algorithm
 *
 * Revision 1.15  2001/09/17 13:56:40  Rommel
 * changed to perform bidirectional benchmarks
 *
 * Revision 1.14  2001/04/19 12:20:55  Rommel
 * added fixes for Unix systems
 *
 * Revision 1.13  2001/03/26 11:37:41  Rommel
 * avoid integer overflows during throughput calculation
 *
 * Revision 1.12  2000/12/01 15:57:57  Rommel
 * *** empty log message ***
 *
 * Revision 1.11  2000/03/01 12:21:47  rommel
 * fixed _INTEGRAL_MAX_BITS problem for WIN32
 *
 * Revision 1.10  1999/10/28 17:36:57  rommel
 * fixed OS/2 timer code
 *
 * Revision 1.9  1999/10/28 17:04:12  rommel
 * fixed timer code
 *
 * Revision 1.8  1999/10/24 19:08:20  rommel
 * imported DOS support from G. Vanem <giva@bgnett.no>
 *
 *
 * Revision 1.8  1999/10/12 11:02:00  giva
 * added Watt-32 with djgpp support. Added debug mode.
 * G. Vanem <giva@bgnett.no>
 *
 * Revision 1.7  1999/06/13 18:42:25  rommel
 * added Linux port with patches from Detlef Plotzky <plo@bvu.de>
 *
 * Revision 1.6  1998/10/12 11:14:58  rommel
 * change to malloc'ed (and tiled) memory for transfer buffers
 * (hint from Guenter Kukkukk <kukuk@berlin.snafu.de>)
 * for increased performance
 *
 * Revision 1.5  1998/07/31 14:15:03  rommel
 * added random buffer data
 * fixed bugs
 *
 * Revision 1.4  1997/09/12 17:35:04  rommel
 * termination bug fixes
 *
 * Revision 1.3  1997/09/12 12:00:15  rommel
 * added Win32 port
 * (tested for Windows NT only)
 *
 * Revision 1.2  1997/09/12 10:44:22  rommel
 * added TCP/IP and a command line interface
 *
 * Revision 1.1  1996/09/25 08:42:29  rommel
 * Initial revision
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#if defined(UNIX) || defined(DJGPP)
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#else
#endif

#define DEFAULTPORT 		0x494F /* "IO" */
#define DEFAULTNBSRV 		"NETIOSRV"
#define DEFAULTNBCLT 		"NETIOCLT"
#define THREADSTACK 		65536

/* TCP/IP system specific details */

#ifdef UNIX
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <time.h>
# include <netinet/in.h>
# include <netdb.h>
# define psock_errno(x) perror(x)
# define soclose(x) close(x)

int sock_init(void)
{
	return 0;
}
# include <pthread.h>
pthread_t thread;
# define newthread(entry) (pthread_create(&thread, 0, entry, 0) != 0)
# define THREAD void*
# define THREADRESULT ((void*)0)
#endif /* UNIX */


#ifdef VXWORKS
# include <types.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <netinet/in.h>
# include <netdb.h>
# include <sockLib.h>
# include <hostLib.h>
# include <inetLib.h>
# include <selectLib.h>
# include <time.h>
# include <wrapper/wrapperHostLib.h>  /* struct hostent * gethostbyname(char *) */
# include <netinet/in.h>
# define psock_errno(x) perror(x)
# define soclose(x) close(x)

int sock_init(void)
{
	return 0;
}
# include <pthread.h>
pthread_t thread;
# define newthread(entry) (pthread_create(&thread, 0, entry, 0) != 0)
/* # define gethostbyname(x) hostGetByName(x) */
# define THREAD void*
# define THREADRESULT ((void*)0)
#endif


/* global data */

#ifndef max
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef EINTR
#define EINTR 0
#endif

int nSizes[] = {1024, 2048, 4096, 8192, 16384, 32768};
size_t nnSizes = sizeof(nSizes) / sizeof(int);
#define NMAXSIZE 65536

int tSizes[] = {1024, 2048, 4096, 8192, 16384, 32767};
size_t ntSizes = sizeof(tSizes) / sizeof(int);
#define TMAXSIZE 65536

#define INTERVAL 6

/* you may need to adapt this to your platform/compiler */
typedef unsigned int uint32;

typedef struct
{
	uint32 cmd;
	uint32 data;
}
	CONTROL;

#define CMD_QUIT  0
#define CMD_C2S   1
#define CMD_S2C   2
#define CMD_RES   3

#define CTLSIZE sizeof(CONTROL)

/* timer code */

int bTimeOver;

#if defined(UNIX) || defined(DJGPP) || defined(VXWORKS)

void on_alarm(int signum)
{
	if (signum)
		;

	alarm(0);
	bTimeOver = 1;
}

int StartAlarm(long nSeconds)
{
	bTimeOver = 0;
	signal(SIGALRM, on_alarm);
	alarm(nSeconds);
	return 0;
}

/*
 * we use UOS_MsecTimerGet() instead gettimeofday, does the same.
 * 32bit Msec Time can wrap after 4,294,000 s .. enough to test.
 */

int StartTimer(struct timeval *nStart)
{
	long msectime	= 	UOS_MsecTimerGet();
	long sec	 	= 	msectime/1000;
	long usec		=	(msectime - (sec*1000)) * 1000;

	nStart->tv_sec	= sec;
	nStart->tv_usec	= usec;

	return 0;
}

long StopTimer(struct timeval *nStart/* , int nAccuracy */)
{
	struct timeval nStop;
	long msectime	= 	UOS_MsecTimerGet();
	long sec	 	= 	msectime/1000;
	long msec		=	msectime - (sec*1000);

	nStop.tv_sec	= sec;
	nStop.tv_usec	= msec * 1000;		

	return (nStop.tv_sec  - nStart->tv_sec) + 
		(((nStop.tv_usec - nStart->tv_usec))  / 1000);
}

#endif /* UNIX || DJGPP */




/*******************************************************************************

 *******************************************************************************/

/* initialize data to transfer */

char *InitBuffer(size_t nSize)
{
	char *cBuffer = malloc(nSize);

	if (cBuffer != NULL)
	{
		size_t i;

		cBuffer[0] = 0;
		srand(1);

		for (i = 1; i < nSize; i++)
			cBuffer[i] = (char) rand();
	}

	return cBuffer;
}

char *PacketSize(int nSize)
{
	static char szBuffer[64];

	if ((nSize % 1024) == 0 || (nSize % 1024) == 1023)
		sprintf(szBuffer, "%2dk", (nSize + 512) / 1024);
	else
		sprintf(szBuffer, "%d", nSize);

	return szBuffer;
}

/* TCP/IP code */

int send_data(int socket, void *buffer, size_t size, int flags)
{
	int rc = send(socket, buffer, size, flags);

	if (rc < 0)
	{
		psock_errno("send()");
		return -1;
	}

	if (rc != size)
		return 1;

	return 0;
}

int recv_data(int socket, void *buffer, size_t size, int flags)
{
	int /* size_t */ rc = recv(socket, buffer, size, flags);

	if (rc < 0)
	{
		psock_errno("recv()");
		return -1;
	}

	if (rc != (int)size)
		return 1;

	return 0;
}

const int sobufsize = 131072;
int nPort = DEFAULTPORT;
int nAuxPort = DEFAULTPORT + 1;
struct in_addr addr_server;
struct in_addr addr_local;

int udpsocket, udpd;
unsigned long nUDPCount;
long long nUDPData;

THREAD TCP_Server(void *arg)
{
	char *cBuffer;
	CONTROL ctl;
	long long nData;
	struct sockaddr_in sa_server, sa_client;
	int server, client;
	size_t length;
	struct timeval tv;
	fd_set fds;
	int rc;
	int nByte;

	if ((cBuffer = InitBuffer(TMAXSIZE)) == NULL)
	{
		perror("malloc()");
		return THREADRESULT;
	}

	if ((server = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		psock_errno("socket()");
		if (cBuffer)
			free(cBuffer);
		return THREADRESULT;
	}

	setsockopt(server, SOL_SOCKET, SO_RCVBUF, (char *) &sobufsize, sizeof(sobufsize));
	setsockopt(server, SOL_SOCKET, SO_SNDBUF, (char *) &sobufsize, sizeof(sobufsize));

	sa_server.sin_family 	= AF_INET;
	sa_server.sin_port 		= (unsigned short)(htons(nPort));
	sa_server.sin_addr 		= addr_local;

	if (bind(server, (struct sockaddr *) &sa_server, sizeof(sa_server)) < 0)
	{
		psock_errno("bind()");
		soclose(server);
		free(cBuffer);
		return THREADRESULT;
	}

	if (listen(server, 2) != 0)
	{
		psock_errno("listen()");
		soclose(server);
		free(cBuffer);
		return THREADRESULT;
	}

	for (;;)
	{
		printf("TCP server listening.\n");

		FD_ZERO(&fds);
		FD_SET(server, &fds);
		tv.tv_sec  = 3600;
		tv.tv_usec = 0;

		if ((rc = select(FD_SETSIZE, &fds, 0, 0, &tv)) < 0)
		{
			psock_errno("select()");
			break;
		}

		if (rc == 0 || FD_ISSET(server, &fds) == 0)
			continue;

		length = sizeof(sa_client);
		if ((client = accept(server, 
							 (struct sockaddr *) &sa_client, 
							 (int*)(&length) )) == -1)
			continue;

		setsockopt(client, SOL_SOCKET, SO_RCVBUF, (char *) &sobufsize, sizeof(sobufsize));
		setsockopt(client, SOL_SOCKET, SO_SNDBUF, (char *) &sobufsize, sizeof(sobufsize));

		printf("TCP connection established ... ");
		fflush(stdout);

		for (;;)
		{
			if (recv_data(client, (void *) &ctl, CTLSIZE, 0))
				break;

			ctl.cmd = ntohl(ctl.cmd);
			ctl.data = ntohl(ctl.data);

			if (ctl.cmd == CMD_C2S)
			{
				printf("\nReceiving from client, packet size %s ... ", PacketSize(ctl.data));
				nData = 0;

				do
				{
					for (nByte = 0; nByte < ctl.data; )
					{
						rc = recv(client, cBuffer + nByte, ctl.data - nByte, 0);

						if (rc < 0 && errno != EINTR)
						{
							psock_errno("recv()");
							break;
						}
	    
						if (rc > 0)
							nByte += rc;
					}

					nData += ctl.data;
				}
				while (cBuffer[0] == 0 && rc > 0);
			}
			else if (ctl.cmd == CMD_S2C)
			{
				if (StartAlarm(INTERVAL) == 0)
				{
					printf("\nSending to client, packet size %s ... ", PacketSize(ctl.data));
					cBuffer[0] = 0;
					nData = 0;

					while (!bTimeOver)
					{
						for (nByte = 0; nByte < ctl.data; )
						{
							rc = send(client, cBuffer + nByte, ctl.data - nByte, 0);

							if (rc < 0 && errno != EINTR)
							{
								psock_errno("send()");
								break;
							}

							if (rc > 0)
								nByte += rc;
						}

						nData += ctl.data;
					}

					cBuffer[0] = 1;

					if (send_data(client, cBuffer, ctl.data, 0))
						break;
				}
			}
			else /* quit */
				break;
		}

		printf("\nDone.\n");

		soclose(client);

		if (rc < 0)
			break;
	}

	soclose(server);
	free(cBuffer);

	return THREADRESULT;
}

void TCP_Bench(void *arg)
{
	char *cBuffer;
	CONTROL ctl;
	struct timeval nTimer;
	long nTime, nResult;
	long long nData;
	int i;
	struct sockaddr_in sa_server;
	int server;
	int rc;
	int nByte;

	if ((cBuffer = InitBuffer(TMAXSIZE)) == NULL)
	{
		perror("malloc()");
		return;
	}

	if ((server = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		psock_errno("socket()");
		free(cBuffer);
		return;
	}

	setsockopt(server, SOL_SOCKET, SO_RCVBUF, (char *) &sobufsize, sizeof(sobufsize));
	setsockopt(server, SOL_SOCKET, SO_SNDBUF, (char *) &sobufsize, sizeof(sobufsize));

	sa_server.sin_family = AF_INET;
	sa_server.sin_port = (unsigned short)(htons(nPort));
	sa_server.sin_addr = addr_server;

	if (connect(server, (struct sockaddr *) &sa_server, sizeof(sa_server)) < 0)
	{
		psock_errno("connect()");
		soclose(server);
		free(cBuffer);
		return;
	}

	printf("\nTCP connection established.\n");

	for (i = 0; i < ntSizes; i++)
	{
		printf("Packet size %s bytes: ", PacketSize(tSizes[i]));
		fflush(stdout);

		/* tell the server we will send it data now */

		ctl.cmd = htonl(CMD_C2S);
		ctl.data = htonl(tSizes[i]);

		if (send_data(server, (void *) &ctl, CTLSIZE, 0))
			break;

		/* 1 - Tx test */

		if (StartAlarm(INTERVAL) == 0 && StartTimer(&nTimer) == 0)
		{
			nData = 0;
			cBuffer[0] = 0;

			while (!bTimeOver)
			{
				for (nByte = 0; nByte < tSizes[i]; )
				{
					rc = send(server, cBuffer + nByte, tSizes[i] - nByte, 0);

					if (rc < 0 && errno != EINTR)
					{
						psock_errno("send()");
						break;
					}
	  
					if (rc > 0)
						nByte += rc;
				}

				nData += tSizes[i];
			}

			if ((nTime = StopTimer(&nTimer/* , 1024 */)) == -1)
				printf(" (failed)");

			if (nData < 100 * 1024 * INTERVAL)
			{
				nResult = (long)(nData * 1024 / nTime);
				printf(" %ld Byte/s", nResult);
			}
			else
			{
				nResult = (long)(nData / nTime);
				printf(" %ld KByte/s", nResult);
			}

			printf(" Tx, ");
			fflush(stdout);

			cBuffer[0] = 1;

			if (send_data(server, cBuffer, tSizes[i], 0))
				break;
		}

		/* tell the server we expect him to send us data now */

		ctl.cmd = htonl(CMD_S2C);
		ctl.data = htonl(tSizes[i]);

		if (send_data(server, (void *) &ctl, CTLSIZE, 0))
			break;

		/* 2 - Rx test */

		if (StartTimer(&nTimer) == 0)
		{
			nData = 0;

			do
			{
				for (nByte = 0; nByte < tSizes[i]; )
				{
					rc = recv(server, cBuffer + nByte, tSizes[i] - nByte, 0);

					if (rc < 0 && errno != EINTR)
					{
						psock_errno("recv()");
						break;
					}
	  
					if (rc > 0)
						nByte += rc;
				}

				nData += tSizes[i];
			}
			while (cBuffer[0] == 0 && rc > 0);

			if ((nTime = StopTimer(&nTimer/* , 1024 */)) == -1)
				printf(" (failed)");

			if (nData < 100 * 1024 * INTERVAL)
			{
				nResult = nData * 1024 / nTime;
				printf(" %ld Byte/s", nResult);
			}
			else
			{
				nResult = nData / nTime;
				printf(" %ld KByte/s", nResult);
			}

			printf(" Rx.\n");
		}
	}

	ctl.cmd = htonl(CMD_QUIT);
	ctl.data = 0;

	send_data(server, (void *) &ctl, CTLSIZE, 0);

	printf("Done.\n");

	soclose(server);
	free(cBuffer);
}

THREAD UDP_Receiver(void *arg)
{
	char *cBuffer;
	struct sockaddr_in sa_server, sa_client;
	int rc;
	size_t nBytes;

	if ((cBuffer = InitBuffer(TMAXSIZE)) == NULL)
	{
		perror("malloc()");
		return THREADRESULT;
	}

	if ((udpsocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		psock_errno("socket(DGRAM)");
		free(cBuffer);
		return THREADRESULT;
	}
  
	setsockopt(udpsocket, 
		   SOL_SOCKET, 
		   SO_RCVBUF, 
		   (char *) &sobufsize, 
		   sizeof(sobufsize));
	setsockopt(udpsocket, 
		   SOL_SOCKET, 
		   SO_SNDBUF, 
		   (char *) &sobufsize, 
		   sizeof(sobufsize));

	sa_server.sin_family = AF_INET;
	sa_server.sin_port = htons(nAuxPort);
	sa_server.sin_addr = addr_local;

	if (bind(udpsocket, (struct sockaddr *) &sa_server, sizeof(sa_server)) < 0)
	{
		psock_errno("bind(DGRAM)");
		soclose(udpsocket);
		free(cBuffer);
		return THREADRESULT;
	}

	udpd = 1;

	for (;;)
	{
		nBytes = sizeof(sa_client);
		rc = recvfrom(udpsocket, 
					  cBuffer, 
					  TMAXSIZE, 
					  0, 
					  (struct sockaddr *) &sa_client, 
					  (int*)(&nBytes));

		if (rc < 0 && errno != EINTR)
			psock_errno("recvfrom()");

		if (rc > 0)
		{
			nUDPCount++;
			nUDPData += rc;
		}
	}

	soclose(udpsocket);
	free(cBuffer);
	return THREADRESULT;
}

THREAD UDP_Server(void *arg)
{
	char *cBuffer;
	CONTROL ctl;
	struct sockaddr_in sa_server, sa_client;
	int server, client;
	struct timeval tv;
	fd_set fds;
	int rc, nByte;
	size_t nLength;

	if ((cBuffer = InitBuffer(TMAXSIZE)) == NULL)
	{
		perror("malloc()");
		return THREADRESULT;
	}

	if ((server = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		psock_errno("socket(STREAM)");
		free(cBuffer);
		return THREADRESULT;
	}

	setsockopt(server, SOL_SOCKET, SO_RCVBUF, (char *) &sobufsize, sizeof(sobufsize));
	setsockopt(server, SOL_SOCKET, SO_SNDBUF, (char *) &sobufsize, sizeof(sobufsize));

	sa_server.sin_family = AF_INET;
	sa_server.sin_port = htons(nAuxPort);
	sa_server.sin_addr = addr_local;

	if (bind(server, (struct sockaddr *) &sa_server, sizeof(sa_server)) < 0)
	{
		psock_errno("bind(STREAM)");
		soclose(server);
		free(cBuffer);
		return THREADRESULT;
	}

	if (listen(server, 2) != 0)
	{
		psock_errno("listen()");
		soclose(server);
		free(cBuffer);
		return THREADRESULT;
	}

	for (;;)
	{
		printf("UDP server listening.\n");

		FD_ZERO(&fds);
		FD_SET(server, &fds);
		tv.tv_sec  = 3600;
		tv.tv_usec = 0;

		if ((rc = select(FD_SETSIZE, &fds, 0, 0, &tv)) < 0)
		{
			psock_errno("select()");
			break;
		}

		if (rc == 0 || FD_ISSET(server, &fds) == 0)
			continue;

		nLength = sizeof(sa_client);
		if ((client = accept(server, 
							 (struct sockaddr *) &sa_client, 
							 (int*)(&nLength) )) == -1)
			continue;

		setsockopt(client, SOL_SOCKET, SO_RCVBUF, (char *) &sobufsize, sizeof(sobufsize));
		setsockopt(client, SOL_SOCKET, SO_SNDBUF, (char *) &sobufsize, sizeof(sobufsize));

		printf("UDP connection established ... ");
		fflush(stdout);

		sa_client.sin_port = htons(nAuxPort);

		for (;;)
		{
			if (recv_data(client, (void *) &ctl, CTLSIZE, 0))
				break;

			ctl.cmd = ntohl(ctl.cmd);
			ctl.data = ntohl(ctl.data);

			if (ctl.cmd == CMD_C2S)
			{
				printf("\nReceiving from client, packet size %s ... ", PacketSize(ctl.data));
				nUDPCount = 0;
				nUDPData = 0;

				ctl.cmd = htonl(ctl.cmd);
				ctl.data = htonl(ctl.data);

				if (send_data(client, (void *) &ctl, CTLSIZE, 0))
					break;
			}
			else if (ctl.cmd == CMD_RES)
			{
				ctl.cmd = htonl(ctl.cmd);
				ctl.data = htonl(nUDPCount);

				if (send_data(client, (void *) &ctl, CTLSIZE, 0))
					break;
			}
			else if (ctl.cmd == CMD_S2C)
			{
				if (StartAlarm(INTERVAL) == 0)
				{
					printf("\nSending to client, packet size %s ... ", PacketSize(ctl.data));
					cBuffer[0] = 0;
					nLength = ctl.data;

					ctl.cmd = htonl(CMD_RES);
					ctl.data = 0;

					while (!bTimeOver)
					{
						for (nByte = 0; nByte < nLength; )
						{
							rc = sendto(udpsocket, cBuffer + nByte, nLength - nByte, 0, 
										(struct sockaddr *) &sa_client, sizeof(sa_client));

							if (rc < 0 && errno != EINTR)
							{
								psock_errno("sendto()");
								break;
							}
	      
							if (rc > 0)
								nByte += rc;
						}

						ctl.data++;
					}

					ctl.data = htonl(ctl.data);

					if (send_data(client, (void *) &ctl, CTLSIZE, 0))
						break;
				}
			}
			else /* quit */
				break;
		}

		printf("\nDone.\n");

		soclose(client);

		if (rc < 0)
			break;
	}

	soclose(server);
	free(cBuffer);

	return THREADRESULT;
}

void UDP_Bench(void)
{
	char *cBuffer;
	CONTROL ctl;
	struct timeval nTimer;
	long nTime, nResult, nCount;
	long long nData;
	int i;
	struct sockaddr_in sa_server;
	int server;
	int rc, nByte;

	if ((cBuffer = InitBuffer(TMAXSIZE)) == NULL)
	{
		perror("malloc()");
		return;
	}

	if ((server = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		psock_errno("socket()");
		free(cBuffer);
		return;
	}

	setsockopt(server, SOL_SOCKET, SO_RCVBUF, (char *) &sobufsize, sizeof(sobufsize));
	setsockopt(server, SOL_SOCKET, SO_SNDBUF, (char *) &sobufsize, sizeof(sobufsize));

	sa_server.sin_family = AF_INET;
	sa_server.sin_port = htons(nAuxPort);
	sa_server.sin_addr = addr_server;

	if (connect(server, (struct sockaddr *) &sa_server, sizeof(sa_server)) < 0)
	{
		psock_errno("connect()");
		soclose(server);
		free(cBuffer);
		return;
	}

	printf("\nUDP connection established.\n");

	for (i = 0; i < ntSizes; i++)
	{
		printf("Packet size %s bytes: ", PacketSize(tSizes[i]));
		fflush(stdout);

		/* tell the server we will send it data now */

		ctl.cmd = htonl(CMD_C2S);
		ctl.data = htonl(tSizes[i]);

		if (send_data(server, (void *) &ctl, CTLSIZE, 0))
			break;

		if (recv_data(server, (void *) &ctl, CTLSIZE, 0))
			break;

		/* 1 - Tx test */

		if (StartAlarm(INTERVAL) == 0 && StartTimer(&nTimer) == 0)
		{
			cBuffer[0] = 0;
			nData = 0;
			nCount = 0;

			while (!bTimeOver)
			{
				for (nByte = 0; nByte < tSizes[i]; )
				{
					rc = sendto(udpsocket, cBuffer + nByte, tSizes[i] - nByte, 0, 
								(struct sockaddr *) &sa_server, sizeof(sa_server));

					if (rc < 0)
					{
						if (errno != EINTR)
						{
							psock_errno("sendto()");
							break;
						}
					}
					else
						nByte += rc;
				}

				nData += tSizes[i];
				nCount++;
			}
      
			if ((nTime = StopTimer(&nTimer/* , 1024 */)) == -1)
				printf(" (failed)");

			ctl.cmd = htonl(CMD_RES);

			if (send_data(server, (void *) &ctl, CTLSIZE, 0))
				break;

			if (recv_data(server, (void *) &ctl, CTLSIZE, 0))
				break;

			ctl.data = ntohl(ctl.data);
			nData = (long long) tSizes[i] * ctl.data;

			if (nData < 100 * 1024 * INTERVAL)
			{
				nResult = nData * 1024 / nTime;
				printf(" %ld Byte/s", nResult);
			}
			else
			{
				nResult = nData / nTime;
				printf(" %ld KByte/s", nResult);
			}

			nResult = (nCount - ctl.data) * 100 / nCount;
			printf(" (%ld%%) Tx, ", nResult);
			fflush(stdout);
		}

		/* tell the server we expect him to send us data now */

		ctl.cmd = htonl(CMD_S2C);
		ctl.data = htonl(tSizes[i]);
		nUDPCount = 0;
		nUDPData = 0;

		if (send_data(server, (void *) &ctl, CTLSIZE, 0))
			break;

		/* 2 - Rx test */

		if (StartTimer(&nTimer) == 0)
		{
			if (recv_data(server, (void *) &ctl, CTLSIZE, 0))
				break;

			if ((nTime = StopTimer(&nTimer/* , 1024 */)) == -1)
				printf(" (failed)");

			ctl.data = ntohl(ctl.data);
			nData = nUDPData;

			if (nData < 100 * 1024 * INTERVAL)
			{
				nResult = nData * 1024 / nTime;
				printf(" %ld Byte/s", nResult);
			}
			else
			{
				nResult = nData / nTime;
				printf(" %ld KByte/s", nResult);
			}

			nResult = (ctl.data - nUDPCount) * 100 / ctl.data;
			printf(" (%ld%%) Rx.\n", nResult);
		}
	}

	ctl.cmd = htonl(CMD_QUIT);
	ctl.data = 0;

	send_data(server, (void *) &ctl, CTLSIZE, 0);

	printf("Done.\n");

	soclose(server);
	free(cBuffer);
}


void handler(int sig)
{
	if (sig);

	return;
}

void usage(void)
{
    printf(
	"\nUsage: netio [options] [<server>]"
	"\n (server IP must be last argument!)\n"
	"\n  -s            run server side of benchmark (otherwise run client)"
	"\n  -b <size> (k) use this block size (otherwise run with 1,2,4,8,16 and 32k)"	
	"\n  -t            use TCP protocol for benchmark"
	"\n  -u            use UDP protocol for benchmark"
	"\n  -h <addr>     bind TCP and UDP servers to this local host address/name only"
	"\n                (default is to bind to all local addresses)"
	"\n  -p <port>     bind TCP and UDP servers to this port (default is %d)\n"
	
	"\n  <server>      If the client side of the benchmark is running,"
	"\n                a server name or address is required.\n"
	
	"\nThe server side can run either TCP (-t) or UDP (-u) protocol or both"
	"\n(default, if neither -t or -u is specified). The client runs one of"
	"\nthese protocols only (must specify -t or -u).\n");
}



/****************************************************************************************
 *  netio main function
 *  
 *  Original has main(int argc, char **argv) while we have to put arguments in brackets
 */
int netio(char *argval)
{


	char szVersion[32], *szLocal = 0, *szEnd;
	char *targetIp=NULL, *optarg=NULL;
	int bSRV=0, bTCP=0, bUDP=0;
	struct in_addr ia;
	struct hostent *host;
	long nSize;

	/* check arguments 
	   netio "-t 192.1.1.147"
	   netio "-s"
	   netio "-u 192.1.1.147 -b 1k"
	 */
	
	if ((strlen(argval) == 0) || (argval == NULL)) {
		usage();
		return(0);
	}
	printf("\nNETIO - Network Benchmark Version %s %s\n(C) 1997-2005 Kai Uwe Rommel\n", 
		   rcsid, rcsrev);

	/* parse arguments */
	if ((optarg = strstr(argval,"-s"))) {
		printf("arg   -s found \n");
		bSRV = 1;
	}

	if ((optarg = strstr(argval,"-t"))) {
		printf("arg   -t found \n");
		bTCP = 1;
	}

	if ((optarg = strstr(argval,"-u"))) {
		printf("arg   -u found \n");
		bUDP = 1;
	}

	if ((optarg = strstr(argval,"-b"))) {
		nSize = strtol(optarg+3, &szEnd, 10);
		nSize *= 1024;
		nSizes[0] = min(max(nSize, 1), NMAXSIZE);
		tSizes[0] = min(max(nSize, 1), TMAXSIZE);
		nnSizes = ntSizes = 1;
		printf("arg   -b found. size: %ld \n", nSize);
	}

	/* specific port used ? */
	if ((optarg = strstr(argval,"-p"))) {
		nPort = atoi(optarg+3);
		nAuxPort = nPort + 1;
		printf("arg   -p found. port: %ld \n", nPort);
	}

	/* check server IP "xxx.xxx.xxx.xxx"  */
	if ((optarg = strstr(argval,"."))) {
		/* skip to begin of IP before 1st dot */
		while( ((*optarg--) != ' ') && (optarg >= argval));
		optarg+=2;
		if(inet_aton(optarg,&ia)) {
			printf("*** %s is an INVALID ip address\n", optarg);
			exit(ERROR);
		} else {
			printf("%s is a VALID ip address. 0x%08x\n", optarg, ia.s_addr);
		}
		targetIp = optarg;
	}



	if ((bTCP) && (bUDP)) {
		fprintf(stderr,"*** only -t or -u may be specified.\n");
		return(-1);
	}

	if (bSRV == 1 && bTCP == 0 && bUDP == 0 )
		bTCP = bUDP = 1;

	/* initialize NetBIOS and/or TCP/IP */
	if (bTCP || bUDP)
	{
		if (sock_init())
			return psock_errno("sock_init()"), 1;


		addr_local.s_addr = INADDR_ANY;

		if (!bSRV) {
			/* targetIp shall always be given as 4 digit IP */
			addr_server.s_addr = inet_addr(targetIp );
		}
	}

	/* do work */

	signal(SIGINT, handler);

	if (bSRV)
	{
		printf("\n");

		if (bTCP)
		{
			if (newthread(TCP_Server))
				return printf("Cannot create additional thread.\n"), 2;
		}
		if (bUDP)
		{
			if (newthread(UDP_Receiver))
				return printf("Cannot create additional thread.\n"), 2;
			if (newthread(UDP_Server))
				return printf("Cannot create additional thread.\n"), 2;
		}

		while (udpd == 0) 
			sleep(1);
		for (;;) 
			sleep(86400);
	}
	else
	{
		if (bTCP)
			TCP_Bench(0);
		else if (bUDP)
		{
			if (newthread(UDP_Receiver))
				return printf("Cannot create additional thread.\n"), 2;
			while (udpd == 0)	sleep(1);
			UDP_Bench();
		}

	}

	/* terminate */
	printf("\n");

	return 0;
}

/* end of netio.c */
