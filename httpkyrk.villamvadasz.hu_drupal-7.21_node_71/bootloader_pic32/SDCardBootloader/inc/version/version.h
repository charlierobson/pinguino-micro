#ifndef _VERSION_H_
#define _VERSION_H_

#include "k_stdtype.h"

typedef enum _SoftwareType {
	WT2000_RUN_IN_01 = 0x0001,
	LOCKINAMP_BX = 0x0002,
	IRB_TRIGGER = 0x0003,
	SIO_COMM = 0x0004,
	SPV_HEAD = 0x0005,
	IRB_EDDY = 0x0006,
	TSMC_ANALOG_10 = 0x0007,
	TSMC_LEDHeat = 0x0008,
	TSMC_LEDSupervisor = 0x0009,
	FPPIMCU = 0x000A,
	SIO_SPCL = 0x000B,
	SRP2100_PCU = 0x000C,
	VARI_VC0B_KINGSPAN = 0x000D,
	SIO_BOOTLOADER = 0x000E,
	BOOTLOADER_SDCARD = 0x000F,
} SoftwareType;

typedef struct _SoftwareIdentification {
	uint16 year;
	uint8 month;
	uint8 day;
	uint16 version;
	SoftwareType softwareType;
} SoftwareIdentification;

extern uint8 VERSION_ID[]; //user must implement this

#endif
