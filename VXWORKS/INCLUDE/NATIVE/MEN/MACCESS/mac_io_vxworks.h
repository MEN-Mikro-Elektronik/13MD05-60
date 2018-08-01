/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: mac_io_vxworks.h
 *
 *       Author: uf
 *        $Date: 1999/08/31 12:14:29 $
 *    $Revision: 1.1 $
 *
 *  Description: access macros for memory mapped devices
 *
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: mac_io_vxworks.h,v $
 * Revision 1.1  1999/08/31 12:14:29  Franke
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _MAC_IO_H
#   define _MAC_IO_H

#include <vxWorks.h>

/* can be set always */
#ifndef VX_SYS_IO_FUNCT_ACCESS
#define VX_SYS_IO_FUNCT_ACCESS
#endif

#ifdef VX_SYS_IO_FUNCT_ACCESS
#include <sysLib.h>

/*------------------------------------------------------------------------+
|  direct access mode - hardware access via pointer                       |
+------------------------------------------------------------------------*/
typedef volatile void* MACCESS;         /* access pointer */

#define MACCESS_CLONE(ma_src,ma_dst,offs)	ma_dst=sysInByte( (ma_src)+(offs) )

#define MREAD_D8(ma,offs)			sysInByte( (int)((u_int8*)(ma)+(offs))  ) 
#define MREAD_D16(ma,offs)			sysInWord( (int)((u_int8*)(ma)+(offs))  ) 
#define MREAD_D32(ma,offs)			sysInLong( (int)((u_int8*)(ma)+(offs))  ) 

#define MWRITE_D8(ma,offs,val)		sysOutByte( (int)((u_int8*)(ma)+(offs)), val ) 
#define MWRITE_D16(ma,offs,val)		sysOutWord( (int)((u_int8*)(ma)+(offs)), val ) 
#define MWRITE_D32(ma,offs,val)		sysOutLong( (int)((u_int8*)(ma)+(offs)), val ) 


#define MBLOCK_READ_D8(ma,offs,size,dst) \
        { int i;                           \
          for( i=0; i<size; i++ )          \
          {                                \
              *dst++ = sysInByte( (int)((u_int8*)(ma)+(offs)+i) );\
          }                                        \
        }

#define MBLOCK_READ_D16(ma,offs,size,dst) \
        { int i;                           \
          for( i=0; i<size; i+=2 )          \
          {                                \
              *((u_int16*)dst)++ = sysInWord( (int)((u_int8*)(ma)+(offs)+i) );\
          }                                        \
        }

#define MBLOCK_READ_D32(ma,offs,size,dst) \
        { int i;                           \
          for( i=0; i<size; i+=4 )          \
          {                                \
              *((u_int32*)dst)++ = sysInWord( (int)((u_int8*)(ma)+(offs)+i) );\
          }                                        \
        }



/*---------------------------------------------------------------------------+
|  Macros that uses only MWRITE_.. / MREAD_..                                |
+---------------------------------------------------------------------------*/
/*-------------------------------+
|  set block (uses MWRITE_..)    |
+-------------------------------*/
#define	MBLOCK_SET_D8(ma,offs,size,val)		\
		{									\
		for( ULONG i=0; i<(size); i++ )		\
			MWRITE_D8(ma,(offs)+i,val)		\
		}

#define MBLOCK_SET_D16(ma,offs,size,val)	\
		{									\
		for( ULONG i=0; i<(size); i+=2 )	\
			MWRITE_D16(ma,(offs)+i,val)		\
		}

#define MBLOCK_SET_D32(ma,offs,size,val)	\
		{									\
		for( ULONG i=0; i<(size); i+=4 )	\
			MWRITE_D32(ma,(offs)+i,val)		\
		}

/*-------------------------------+
|  set mask (uses MWRITE_..)     |
+-------------------------------*/
#define MSETMASK_D8(ma,offs,mask)	\
			MWRITE_D8(  ma,offs,(MREAD_D8( ma,offs)) | (mask) )

#define MSETMASK_D16(ma,offs,mask)	\
			MWRITE_D16( ma,offs,(MREAD_D16(ma,offs)) | (mask) )

#define MSETMASK_D32(ma,offs,mask)	\
			MWRITE_D32( ma,offs,(MREAD_D32(ma,offs)) | (mask) )

/*-------------------------------+
|  clear mask (uses MWRITE_..)   |
+-------------------------------*/
#define MCLRMASK_D8(ma,offs,mask)	\
			MWRITE_D8(  ma,offs,(MREAD_D8( ma,offs)) &~ (mask) )

#define MCLRMASK_D16(ma,offs,mask)	\
			MWRITE_D16( ma,offs,(MREAD_D16(ma,offs)) &~ (mask) )

#define MCLRMASK_D32(ma,offs,mask)	\
			MWRITE_D32( ma,offs,(MREAD_D32(ma,offs)) &~ (mask) )

/*-------------------------------+
|  FIFO read (uses MREAD_..)     |
+-------------------------------*/
#define MFIFO_MREAD_D8(ma,offs,count,dst)			\
	        {										\
				for( ULONG i=0; i<(count); i++ )	\
				    (dst) + i = MREAD_D8(ma, offs)	\
			}

#define MFIFO_MREAD_D16(ma,offs,count,dst)			\
	        {										\
				for( ULONG i=0; i<(count); i++ )	\
				    (dst) + i = MREAD_D16(ma, offs)	\
			}

#define MFIFO_MREAD_D32(ma,offs,count,dst)			\
	        {										\
				for( ULONG i=0; i<(count); i++ )	\
				    (dst) + i = MREAD_D32(ma, offs)	\
			}

/*-------------------------------+
|  FIFO write (uses MWRITE_..)   |
+-------------------------------*/
#define MFIFO_MWRITE_D8(ma,offs,count,src)			\
	        {										\
				for( ULONG i=0; i<(count); i++ )	\
					MWRITE_D8(ma, offs, (src)+i)	\
			}

#define MFIFO_MWRITE_D16(ma,offs,count,src)			\
	        {										\
				for( ULONG i=0; i<(count); i++ )	\
					MWRITE_D16(ma, offs, (src)+i)	\
			}

#define MFIFO_MWRITE_D32(ma,offs,count,src)			\
	        {										\
				for( ULONG i=0; i<(count); i++ )	\
					MWRITE_D32(ma, offs, (src)+i)	\
			}


#else /*VX_SYS_IO_FUNCT_ACCESS*/
# error \
"can't decide how to access IO - refer to your CPU reference manual\
 and include MEN/mac_mem.h for memory mapped access or\
 added your CPU to switch setting VX_SYS_IO_FUNCT_ACCESS\
 for function access (sysInByte()...)"
 
/*# include <MEM/mem_mapped */
#endif /*VX_SYS_IO_FUNCT_ACCESS*/



#endif /* _MAC_IO_H */

