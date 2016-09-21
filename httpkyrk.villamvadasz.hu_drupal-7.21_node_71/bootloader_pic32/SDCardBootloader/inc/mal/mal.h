#ifndef _MAL_H_
#define _MAL_H_

#include	<p32xxxx.h>
#include <plib.h>

	extern volatile unsigned int gieTemp;

	#define MAL_NOP() Nop()

	#ifndef ClrWdt
		#define ClrWdt()	WDTCONSET = 0x0001;
	#endif
	
	#define lock_isr() 	gieTemp = INTDisableInterrupts();
	#define unlock_isr() INTRestoreInterrupts(gieTemp);

	extern void mal_reset(void);


#endif
