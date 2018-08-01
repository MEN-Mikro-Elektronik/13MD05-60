#define M85XX_PVR(base)                (CAST(VUINT32 *)((base) + 0xE00A0))
#define M85XX_SVR(base)                (CAST(VUINT32 *)((base) + 0xE00A4))
#define  M85XX_BR0(base)         (CAST(VUINT32 *)((base) + 0x5000))
#define  M85XX_OR0(base)         (CAST(VUINT32 *)((base) + 0x5004))
#define  M85XX_BR1(base)         (CAST(VUINT32 *)((base) + 0x5008))
#define  M85XX_OR1(base)         (CAST(VUINT32 *)((base) + 0x500c))
#define  M85XX_BR2(base)         (CAST(VUINT32 *)((base) + 0x5010))
#define  M85XX_OR2(base)         (CAST(VUINT32 *)((base) + 0x5014))
#define  M85XX_BR3(base)         (CAST(VUINT32 *)((base) + 0x5018))
#define  M85XX_OR3(base)         (CAST(VUINT32 *)((base) + 0x501c))
#define  M85XX_BR4(base)         (CAST(VUINT32 *)((base) + 0x5020))
#define  M85XX_OR4(base)         (CAST(VUINT32 *)((base) + 0x5024))
#define  M85XX_BR5(base)         (CAST(VUINT32 *)((base) + 0x5028))
#define  M85XX_OR5(base)         (CAST(VUINT32 *)((base) + 0x502C))
#define  M85XX_BR6(base)         (CAST(VUINT32 *)((base) + 0x5030))
#define  M85XX_OR6(base)         (CAST(VUINT32 *)((base) + 0x5034))
#define  M85XX_BR7(base)         (CAST(VUINT32 *)((base) + 0x5038))
#define  M85XX_OR7(base)         (CAST(VUINT32 *)((base) + 0x503C))
/* LBC Clock Configuration */
#define  M85XX_LBCR(base)         (CAST(VUINT32 *)((base) + 0x50D0))
#define  M85XX_LCRR(base)         (CAST(VUINT32 *)((base) + 0x50D4))






/* CPM regs */
#define M85XX_CPM_SICR(base)         (CAST(VUINT16 *)((base) + 0x90c00))
#define M85XX_CPM_SIVEC(base)        (CAST(VUINT8  *)((base) + 0x90c04))
#define M85XX_CPM_SIPNR_H(base)      (CAST(VUINT32 *)((base) + 0x90c08))
#define M85XX_CPM_SIPNR_L(base)      (CAST(VUINT32 *)((base) + 0x90c0c))
#define M85XX_CPM_SCPRR_H(base)      (CAST(VUINT32 *)((base) + 0x90c14))
#define M85XX_CPM_SCPRR_L(base)      (CAST(VUINT32 *)((base) + 0x90c18))
#define M85XX_CPM_SIMR_H(base)       (CAST(VUINT32 *)((base) + 0x90c1c))
#define M85XX_CPM_SIMR_L(base)       (CAST(VUINT32 *)((base) + 0x90c20))
#define M85XX_CPM_SIEXR(base)        (CAST(VUINT32 *)((base) + 0x90c24))
#define M85XX_CPM_SCCR(base)         (CAST(VUINT32 *)((base) + 0x90c80))

#define M8260_BRGC_DIVISOR	BRGCLK_DIV_FACTOR
