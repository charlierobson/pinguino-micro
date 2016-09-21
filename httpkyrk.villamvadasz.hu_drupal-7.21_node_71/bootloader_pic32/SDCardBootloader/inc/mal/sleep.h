#ifndef _SLEEP_H_
#define _SLEEP_H_

#include "k_stdtype.h"

extern void idle_Request(void);
extern void sleep_Request(uint32 time);
extern uint8 backToSleep(void); //Callout

#endif
