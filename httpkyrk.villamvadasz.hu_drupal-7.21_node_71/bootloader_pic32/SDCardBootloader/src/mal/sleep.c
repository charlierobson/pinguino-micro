#include <plib.h>
#include "global.h"
#include "sleep.h"
#include "c_sleep.h"
#include "i_sleep.h"
#include "w_sleep.h"

#include "task.h"
#include "k_stdtype.h"

uint32 sleepTime = 0;
uint32 sleepRequestLock = 0;

void executeSleep(void);

void init_sleep(void) {
}

void idle_Request(void) {
	PowerSaveIdle();
}

void sleep_Request(uint32 time) {
	if (sleepRequestLock == 0) {
		sleepRequestLock = 1;
		sleepTime = time;
		#ifndef SLEEPREQUEST_DISABLE_TASK_EXECUTE
			addSingleShootTask(executeSleep, "executeSleep");
		#endif
	}
}

void executeSleep(void) {
	uint32 x = 0;
	beforeSleepUser();

	if (sleepTime == 0) {
		DisableWDT();
		ClearWDT();
		while (backToSleep()) {
			PowerSaveSleep();
		}
	} else {
		for (x = 0; x < sleepTime; x++) {
			ClearWDT();
			EnableWDT();
			ClearWDT();
			PowerSaveSleep();
			DisableWDT();
		}
	}
	afterSleepUser();
	sleepRequestLock = 0;
}
