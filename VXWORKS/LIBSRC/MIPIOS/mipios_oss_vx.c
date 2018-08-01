#ifdef INCLUDE_MIPIOS_VX_TARGET

#include <stdio.h>
#include <logLib.h>
#include <strLib.h>
#include <MEN/men_typs.h>
#include <MEN/mipios.h>



#define SIG_QUEUE_MAX 	((MIPIOS_MAX_DATA_SIZE/4)+1) /* one frame can transfer 250 + 1 single signal - do not touch !!! */
#define SIG_MAX_HDL 	0x40

typedef struct
{
	u_int32	sigNo;
	void	*sigQueue[SIG_QUEUE_MAX];
	int		sigReadIdx;
	int		sigWriteIdx;
}OSS_MIPIOS_VX_SIG;


#define OSS_MIPIOS_VX_INTERN
#include <MEN/oss.h>
#include <mipios_oss_vx.h>

#define DBG_MYLEVEL         OSS_MIPIOS_DbgLev
#include <MEN/dbg.h>        /* debug module */

u_int32 OSS_MIPIOS_DbgLev = 0;



static OSS_MIPIOS_VX_SIG _SigHdl[SIG_MAX_HDL];
static u_int32		sigInstCnt = 0;

static int32				newSigInstalled = 0;
static OSS_MIPIOS_VX_SIG	*sigInstalledHdl= NULL;

static int32				newSigRemoved = 0;
static OSS_MIPIOS_VX_SIG	*sigRemovedHdl= NULL;


int32 OSS_MIPIOS_VX_SigCreate
(
    OSS_HANDLE     			*osHdl,
    int32           		value,
    OSS_MIPIOS_VX_SIG  		**sigHandleP
)
{
    DBGCMD( static const char functionName[] = __FUNCTION__; )
    
    sigInstCnt++;
    if( sigInstCnt == SIG_MAX_HDL )
    	sigInstCnt = 0;
    
    
    bzero( (char*)&_SigHdl[sigInstCnt], sizeof(_SigHdl[sigInstCnt]) );
    _SigHdl[sigInstCnt].sigNo = value;
    *sigHandleP = &_SigHdl[sigInstCnt];

	newSigInstalled = 1;
	sigInstalledHdl = *sigHandleP;

	printf("%s sigInstCnt %d sigHandle/sigNo %08x/%d\n", functionName, (int)sigInstCnt, (int)*sigHandleP, (int)value );
    
    return( 0 );
}

int32 OSS_MIPIOS_VX_SigRemove
(
    OSS_HANDLE     *osHdl,
    OSS_MIPIOS_VX_SIG **sigHandleP
)
{
    DBGCMD( static const char functionName[] = __FUNCTION__; )

	printf("%s sigHandle %08x\n", functionName, (int)*sigHandleP );

	newSigRemoved = 1;
	sigRemovedHdl = *sigHandleP;

    *sigHandleP = NULL;

    return( 0 );
}

int32 OSS_MIPIOS_VX_SigSend
(
    OSS_HANDLE *osHdl,
    OSS_MIPIOS_VX_SIG *sigHandle
)
{
    DBGCMD( static const char functionName[] = __FUNCTION__; )

	/*logMsg(">>>%s sigHandle %08x\n", (int)functionName, (int)sigHandle, 3,4,5,6 );*/

	/* /*protect overrun ??? */
	
	sigHandle->sigQueue[sigHandle->sigWriteIdx] = sigHandle;

	sigHandle->sigWriteIdx++;
	if( sigHandle->sigWriteIdx == SIG_QUEUE_MAX )
		sigHandle->sigWriteIdx = 0;

	/*logMsg(">>>%s next wrIndex %d   rdIndex %d  sigHandle %08x\n", (int)functionName, (int)sigHandle->sigWriteIdx, sigHandle->sigReadIdx, (int)sigHandle,5,6 );*/

	return( 0 );
}

int32 OSS_MIPIOS_VX_SigCheckInstalled
(
    OSS_MIPIOS_VX_SIG  		**sigHandleP,
    int32           		*sigNoP
)
{
    DBGCMD( static const char functionName[] = __FUNCTION__; )
	int32 retCode = newSigInstalled;

	if( retCode )
	{
		*sigHandleP = sigInstalledHdl;
		*sigNoP		= sigInstalledHdl->sigNo;
		newSigInstalled = 0;
		sigInstalledHdl = NULL;

		printf("%s sigHandle/sigNo %08x/%d\n", functionName, (int)*sigHandleP, (int)*sigNoP );

	}
	
	return( retCode );
}

int32 OSS_MIPIOS_VX_SigCheckRemoved
(
    OSS_MIPIOS_VX_SIG  		**sigHandleP,
    int32           		*sigNoP
)
{
    DBGCMD( static const char functionName[] = __FUNCTION__; )
	int32 retCode = newSigRemoved;

	if( retCode )
	{
		*sigHandleP = sigRemovedHdl;
		*sigNoP		= sigRemovedHdl->sigNo;
		newSigRemoved = 0;
		sigRemovedHdl = NULL;
		printf("%s sigHandle/sigNo %08x/%d\n", functionName, (int)*sigHandleP, (int)*sigNoP );
	}
	
	return( retCode );
}

int32 OSS_MIPIOS_VX_SigCheckGet
(
    OSS_MIPIOS_VX_SIG *sigHandle,
	u_int32 *sigHandleP
)
{
    DBGCMD( static const char functionName[] = __FUNCTION__; )
    int32 nbrOfSignals = 0;

	if( sigHandle == NULL )
	{
		/* logMsg("*** %s sigHandle %08x\n", (int)__FUNCTION__, (int)sigHandle, 3,4,5,6 ); */
		goto CLEANUP;
	}

	*sigHandleP = (u_int32)NULL;

	if( sigHandle->sigReadIdx == sigHandle->sigWriteIdx )
		goto CLEANUP;

	if( sigHandle->sigReadIdx > sigHandle->sigWriteIdx )
		nbrOfSignals = sigHandle->sigWriteIdx + (SIG_QUEUE_MAX - sigHandle->sigReadIdx);
	else
		nbrOfSignals = sigHandle->sigWriteIdx - sigHandle->sigReadIdx;
		
	*sigHandleP = (int)sigHandle->sigQueue[sigHandle->sigReadIdx];

	sigHandle->sigReadIdx++;

	if( sigHandle->sigReadIdx == SIG_QUEUE_MAX )
		sigHandle->sigReadIdx = 0;
		

	/* logMsg(">>>%s next r/w idx %d/%d sigHandle %08x nbrOfSignals %d\n", (int)functionName, (int)sigHandle->sigReadIdx, (int)sigHandle->sigWriteIdx,
			(int)*sigHandleP, nbrOfSignals,6 ); */

CLEANUP:
	return( nbrOfSignals );
}

int32 OSS_MIPIOS_VX_SigGetAll
(
    OSS_MIPIOS_VX_SIG *sigHandle,
	u_int32 *sigHandleP, /* singleSignal */
	u_int32 *sigBuffer,
	int		sigBufferSize
)
{
    DBGCMD( static const char functionName[] = __FUNCTION__; )
    int32 nbrOfSignals = 0;
	int   loopy;

	if( sigHandle == NULL || sigBuffer == NULL )
	{
		logMsg("*** %s sigHandle %08x  sigBuffer %08x\n", (int)__FUNCTION__, (int)sigHandle, (int)sigBuffer, 4,5,6 );
		goto CLEANUP;
	}

	/* cleanup the target */
	*sigHandleP = (u_int32)NULL;
	bzero( (char*)sigBuffer, sigBufferSize );

	/* snapshoot - nbr of signals to read from queue */
	if( sigHandle->sigReadIdx > sigHandle->sigWriteIdx )
		nbrOfSignals = sigHandle->sigWriteIdx + (SIG_QUEUE_MAX - sigHandle->sigReadIdx);
	else
		nbrOfSignals = sigHandle->sigWriteIdx - sigHandle->sigReadIdx;

	if( nbrOfSignals == 0 )
		goto CLEANUP;

	if( (nbrOfSignals-1)*sizeof(u_int32) > sigBufferSize )
	{
		logMsg("*** %s nbrOfSignals %d  sigBufferSize %d\n", (int)__FUNCTION__, (int)nbrOfSignals, (int)sigBufferSize, 4,5,6 );
		goto CLEANUP;
	}

	loopy = 0;
	while( sigHandle->sigReadIdx != sigHandle->sigWriteIdx && loopy < nbrOfSignals )
	{
		if( loopy == 0 )
			*sigHandleP = (int)sigHandle->sigQueue[sigHandle->sigReadIdx];
		else
			*sigBuffer++ = (int)sigHandle->sigQueue[sigHandle->sigReadIdx];

		sigHandle->sigReadIdx++;

		if( sigHandle->sigReadIdx == SIG_QUEUE_MAX )
			sigHandle->sigReadIdx = 0;

		loopy++;
	}/*while*/
		
	/*logMsg(">>>%s next r/w idx %d/%d sigHandle %08x nbrOfSignals %d\n", (int)functionName, (int)sigHandle->sigReadIdx, (int)sigHandle->sigWriteIdx,
			(int)*sigHandleP, nbrOfSignals,6 );*/

CLEANUP:
	return( nbrOfSignals );
}


#endif /*INCLUDE_MIPIOS_VX_TARGET*/
