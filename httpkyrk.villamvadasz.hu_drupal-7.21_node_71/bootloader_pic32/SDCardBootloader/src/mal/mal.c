#include "global.h"
#include "mal.h"

volatile unsigned int gieTemp = 0;

void mal_reset(void) {
	SoftReset();
}
