extern int32 OSS_MIPIOS_VX_SigCreate
(
    OSS_HANDLE     			*osHdl,
    int32           		value,
    OSS_MIPIOS_VX_SIG  		**sigHandleP
);

extern int32 OSS_MIPIOS_VX_SigRemove
(
    OSS_HANDLE     *osHdl,
    OSS_MIPIOS_VX_SIG **sigHandleP
);

extern int32 OSS_MIPIOS_VX_SigSend
(
    OSS_HANDLE *osHdl,
    OSS_MIPIOS_VX_SIG *sigHandle
);

extern int32 OSS_MIPIOS_VX_SigCheckInstalled
(
    OSS_MIPIOS_VX_SIG  		**sigHandleP,
    int32           		*sigNoP
);

extern int32 OSS_MIPIOS_VX_SigCheckRemoved
(
    OSS_MIPIOS_VX_SIG  		**sigHandleP,
    int32           		*sigNoP
);

extern int32 OSS_MIPIOS_VX_SigCheckGet
(
    OSS_MIPIOS_VX_SIG *sigHandle,
	u_int32 *sigHandleP
);

extern int32 OSS_MIPIOS_VX_SigGetAll
(
    OSS_MIPIOS_VX_SIG *sigHandle,
	u_int32 *sigHandleP, /* singleSignal */
	u_int32 *sigBuffer,
	int		sigBufferSize
);
