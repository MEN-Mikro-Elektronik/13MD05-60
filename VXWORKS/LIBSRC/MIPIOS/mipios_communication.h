extern int32 MIPIOS_COM_RxInit( MIPIOS_LINE *lineHdl );
extern int32 MIPIOS_COM_TxInit( MIPIOS_LINE *lineHdl );
extern u_int32 MIPIOS_COM_Send( int fhTxSocket, u_int32 ip, u_int16 port, MIPIOS_FRAME *txFrame );
