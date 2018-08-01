#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"

	FUNC_EXPORT(membenchRead)
	FUNC_EXPORT(membenchWrite)
	FUNC_EXPORT(membenchReg)

		
FUNC_BEGIN(membenchRead)
 srwi      r4,r4,5		/* blocksize /= 32 */
 subic     r3,r3,4		/* mem -= 4 */
 b rb10

rb5:	
 lwzu r5,4(r3) 
 lwzu r5,4(r3) 
 lwzu r5,4(r3) 
 lwzu r5,4(r3) 
 lwzu r5,4(r3) 
 lwzu r5,4(r3) 
 lwzu r5,4(r3) 
 lwzu r5,4(r3) 

rb10:	
 cmplwi r4,0
 subic  r4,r4,1
 bne rb5
 blr
FUNC_END(membenchRead)

FUNC_BEGIN(membenchWrite)
 srwi      r4,r4,5		/* blocksize /= 32 */
 lis	   r5,0xa55a	/* load testpattern */
 ori	   r5,r5,0x5555
 subic     r3,r3,4		/* mem -= 4 */
 b wb10

wb5:	
 stwu r5,4(r3) 
 stwu r5,4(r3) 
 stwu r5,4(r3) 
 stwu r5,4(r3) 
 stwu r5,4(r3) 
 stwu r5,4(r3) 
 stwu r5,4(r3) 
 stwu r5,4(r3) 

wb10:	
 cmplwi r4,0
 subic  r4,r4,1
 bne wb5
 blr

FUNC_END(membenchWrite)

FUNC_BEGIN(membenchReg)
 srwi      r3,r3,5		/* blocksize /= 32 */
 b reb10

reb5:	
 mr r5,r4
 mr r4,r5
 mr r5,r4
 mr r4,r5
 mr r5,r4
 mr r4,r5
 mr r5,r4
 mr r4,r5

reb10:	
 cmplwi r3,0
 subic  r3,r3,1
 bne reb5
 blr
FUNC_END(membenchReg)



