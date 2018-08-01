#ifdef INCLUDE_MIPIOS_VX

#include <MEN/men_typs.h>

#include <vxWorks.h>
#include <stdio.h>
#include <string.h>
#include <taskLib.h>
#include <sysLib.h>

#include <in.h>		
#include <intLib.h>
#include <ioLib.h>	
#include <logLib.h>
#include <pingLib.h>
#include <socket.h>
#include <sockLib.h>



static char	strres[128];

typedef union
{
  int				i_val;
  long				l_val;
  struct linger		linger_val;
  struct timeval	timeval_val;
}VAL;

static char	*
sock_str_flag(VAL *ptr, int len)
{
	if (len != sizeof(int))
		sprintf(strres, "size (%d) not sizeof(int)", len);
	else
		sprintf(strres, "%s", (ptr->i_val == 0) ? "off" : "on");
	return(strres);
}

static char	*
sock_str_int(VAL *ptr, int len)
{
	if (len != sizeof(int))
		sprintf(strres, "size (%d) not sizeof(int)", len);
	else
		sprintf(strres, "%d", ptr->i_val);
	return(strres);
}

static char	*
sock_str_linger(VAL *ptr, int len)
{
	struct linger	*lptr = &ptr->linger_val;

	if (len != sizeof(struct linger))
		sprintf(strres, "size (%d) not sizeof(struct linger)", len);
	else
		sprintf(strres, "l_onoff = %d, l_linger = %d", lptr->l_onoff, lptr->l_linger);
	return(strres);
}

static char	*
sock_str_timeval(VAL *ptr, int len)
{
	struct timeval	*tvptr = &ptr->timeval_val;

	if (len != sizeof(struct timeval))
		sprintf(strres, "size (%d) not sizeof(struct timeval)", len);
	else
		sprintf(strres, "%d sec, %d usec", (int)tvptr->tv_sec, (int)tvptr->tv_usec);
	return(strres);
}


struct sock_opts {
  const char	   *opt_str;
  int		opt_level;
  int		opt_name;
  char   *(*opt_val_str)(VAL *, int);
} sock_opts[] = {
	{ "SO_BROADCAST",		SOL_SOCKET,	SO_BROADCAST,	sock_str_flag },
	{ "SO_DEBUG",			SOL_SOCKET,	SO_DEBUG,		sock_str_flag },
	{ "SO_DONTROUTE",		SOL_SOCKET,	SO_DONTROUTE,	sock_str_flag },
	{ "SO_ERROR",			SOL_SOCKET,	SO_ERROR,		sock_str_int },
	{ "SO_KEEPALIVE",		SOL_SOCKET,	SO_KEEPALIVE,	sock_str_flag },
	{ "SO_LINGER",			SOL_SOCKET,	SO_LINGER,		sock_str_linger },
	{ "SO_OOBINLINE",		SOL_SOCKET,	SO_OOBINLINE,	sock_str_flag },
	{ "SO_RCVBUF",			SOL_SOCKET,	SO_RCVBUF,		sock_str_int },
	{ "SO_SNDBUF",			SOL_SOCKET,	SO_SNDBUF,		sock_str_int },
	{ "SO_RCVLOWAT",		SOL_SOCKET,	SO_RCVLOWAT,	sock_str_int },
	{ "SO_SNDLOWAT",		SOL_SOCKET,	SO_SNDLOWAT,	sock_str_int },
	{ "SO_RCVTIMEO",		SOL_SOCKET,	SO_RCVTIMEO,	sock_str_timeval },
	{ "SO_SNDTIMEO",		SOL_SOCKET,	SO_SNDTIMEO,	sock_str_timeval },
	{ "SO_REUSEADDR",		SOL_SOCKET,	SO_REUSEADDR,	sock_str_flag },
	{ "SO_REUSEPORT",		SOL_SOCKET,	SO_REUSEPORT,	sock_str_flag },
	{ "SO_TYPE",			SOL_SOCKET,	SO_TYPE,		sock_str_int },
	{ "SO_USELOOPBACK",		SOL_SOCKET,	SO_USELOOPBACK,	sock_str_flag },
#if 0
	/*
	 * Option flags per-socket.
	 */
	#define	SO_ACCEPTCONN	0x0002		/* socket has had listen() */
	
	/*
	 * Additional options, not kept in so_options.
	 */
	#define SO_PROTOTYPE    0x1009		/* get/set protocol type */
#endif
	{ NULL,					0,			0,				NULL }
};


int mipios_show_socket_option( int socketFd )
{
	int					fd = socketFd;
	int					len;
	struct sock_opts	*ptr;
	VAL					val;

	for (ptr = sock_opts; ptr->opt_str != NULL; ptr++) 
	{
		printf("%s: ", ptr->opt_str);
		if (ptr->opt_val_str == NULL)
			printf("(undefined)\n");
		else 
		{

			len = sizeof(val);
			if (getsockopt(fd, ptr->opt_level, ptr->opt_name, (char*)&val, &len) == -1) 
			{
				printf("*** getsockopt error");
			} else 
			{
				printf("default = %s\n", (*ptr->opt_val_str)(&val, len));
			}
		}
	}
	return( 0 );
}

#endif /*INCLUDE_MIPIOS_VX*/

