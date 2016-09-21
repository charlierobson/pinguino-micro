#include <plib.h>
#include <string.h>
#include <stdlib.h>
#include "global.h"
#include "mal.h"
#include "main.h"
#include "c_main.h"
#include "i_main.h"
#include "w_main.h"
#include "k_stdtype.h"
#include "version.h"
#include "sleep.h"

#include "FSIO.h"
#include "NVMem.h"
#include "sd_bootloader.h"
#include "Bootloader.h"
#include "HardwareProfile.h"

// Do not change this
#define FLASH_PAGE_SIZE 0x1000
#define AUX_FLASH_BASE_ADRS				(0x7FC000)
#define AUX_FLASH_END_ADRS				(0x7FFFFF)
#define DEV_CONFIG_REG_BASE_ADDRESS 	(0xF80000)
#define DEV_CONFIG_REG_END_ADDRESS   	(0xF80012)

#define SW_YEAR 0x2015
#define SW_MONTH 0x12
#define SW_DAY 0x04
#define SW_TYPE BOOTLOADER_SDCARD
#define SW_VERSION 0x0001

SoftwareIdentification softwareIdentification = {SW_YEAR, SW_MONTH, SW_DAY, SW_VERSION, SW_TYPE};
unsigned char VERSION_ID[] = "pic_time_2_0_1";

FSFILE * myFile;
BYTE myData[512];
size_t numBytes;
UINT pointer = 0;
UINT readBytes;
UINT8 asciiBuffer[1024];
UINT8 asciiRec[200];
UINT8 hexRec[100];
T_REC record;

// Configuration Bit settings
// SYSCLK = "80 MHz (8MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 40 MHz
// Primary Osc w/PLL (XT+,HS+,EC+PLL)
// WDT OFF
// Other options are don't care
//
    #pragma config UPLLEN   = ON        // USB PLL Enabled
    #pragma config FPLLMUL  = MUL_20        // PLL Multiplier
    #pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider
    #pragma config FPLLIDIV = DIV_2         // PLL Input Divider
    #pragma config FPLLODIV = DIV_1         // PLL Output Divider
    #pragma config FPBDIV   = DIV_2         // Peripheral Clock divisor
    #pragma config FWDTEN   = OFF           // Watchdog Timer
    #pragma config WDTPS    = PS1           // Watchdog Timer Postscale
    #pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
    #pragma config OSCIOFNC = OFF           // CLKO Enable
    #pragma config POSCMOD  = HS            // Primary Oscillator
    #pragma config IESO     = OFF           // Internal/External Switch-over
    #pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable (KLO was off)
    #pragma config FNOSC    = PRIPLL        // Oscillator Selection
    #pragma config CP       = OFF           // Code Protect
    #pragma config BWP      = OFF           // Boot Flash Write Protect
    #pragma config PWP      = OFF           // Program Flash Write Protect

static void ConvertAsciiToHex(UINT8* asciiRec, UINT8* hexRec);
static void InitializeBoard(void);
static void JumpToApp(void);
static BOOL ValidAppPresent(void);
static void do_app(void);
static void LeaveBootloaderToAppOrReset(void);
static void DestroyApplication(void);

int main (void) {
	DDPCON = 0; //Turn off JTAG and TRACE
	SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

	AD1PCFG = 0xFFFF;

	while (1) {
		do_app();
	}
	return 0;
}

static void do_app(void) {
	volatile UINT i;
    volatile BYTE led = 0;

    // Initialize the File System
   	if(!FSInit()) {
		LeaveBootloaderToAppOrReset();
    } else {
		myFile = FSfopen("image.hex","r");
	    if(myFile == NULL) {
			LeaveBootloaderToAppOrReset();
		} else {
			if (FSrename ("burn.hex", myFile) != 0) {
				LeaveBootloaderToAppOrReset();
			} else {
			    // Erase Flash (Block Erase the program Flash)
			    EraseFlash();
			    // Initialize the state-machine to read the records.
			    record.status = REC_NOT_FOUND;
				while(1) {
					// For a faster read, read 512 bytes at a time and buffer it.
					readBytes = FSfread((void *)&asciiBuffer[pointer], 1, 512, myFile);
					if(readBytes == 0) {
						// Nothing to read. Come out of this loop
						// break;
						FSfclose(myFile);
						// Something fishy. The hex file has ended abruptly, looks like there was no "end of hex record".
						//Indicate error and stay in while loop.
						LeaveBootloaderToAppOrReset();            
					}
					
					for(i = 0; i < (readBytes + pointer); i ++) {
						// This state machine seperates-out the valid hex records from the read 512 bytes.
						switch(record.status) {
							case REC_FLASHED:
							case REC_NOT_FOUND: {
								if(asciiBuffer[i] == ':') {
									// We have a record found in the 512 bytes of data in the buffer.
									record.start = &asciiBuffer[i];
									record.len = 0;
									record.status = REC_FOUND_BUT_NOT_FLASHED;
								}
								break;
							}
							case REC_FOUND_BUT_NOT_FLASHED: {
								if((asciiBuffer[i] == 0x0A) || (asciiBuffer[i] == 0xFF)) {
									// We have got a complete record. (0x0A is new line feed and 0xFF is End of file)
									// Start the hex conversion from element
									// 1. This will discard the ':' which is
									// the start of the hex record.
									ConvertAsciiToHex(&record.start[1],hexRec);
									WriteHexRecord2Flash(hexRec);
									record.status = REC_FLASHED;
								}
								break;
							}
						}
						// Move to next byte in the buffer.
						record.len ++;
					}
					
					if(record.status == REC_FOUND_BUT_NOT_FLASHED) {
						// We still have a half read record in the buffer. The next half part of the record is read 
						// when we read 512 bytes of data from the next file read. 
						memcpy(asciiBuffer, record.start, record.len);
						pointer = record.len;
						record.status = REC_NOT_FOUND;
					} else {
						pointer = 0;
					}
				}//while(1)
			}	
		}
	}
}

/********************************************************************
* Function: 	JumpToApp()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		
*
* Side Effects:	No return from here.
*
* Overview: 	Jumps to application.
*
*			
* Note:		 	None.
********************************************************************/
void JumpToApp(void) {
	void (*fptr)(void);
	fptr = (void (*)(void))USER_APP_RESET_ADDRESS;
	fptr();
}	

/********************************************************************
* Function: 	ConvertAsciiToHex()
*
* Precondition: 
*
* Input: 		Ascii buffer and hex buffer.
*
* Output:		
*
* Side Effects:	No return from here.
*
* Overview: 	Converts ASCII to Hex.
*
*			
* Note:		 	None.
********************************************************************/
void ConvertAsciiToHex(UINT8* asciiRec, UINT8* hexRec) {
	UINT8 i = 0;
	UINT8 k = 0;
	UINT8 hex;

	while((asciiRec[i] >= 0x30) && (asciiRec[i] <= 0x66)) {
		// Check if the ascci values are in alpha numeric range.
		
		if(asciiRec[i] < 0x3A) {
			// Numerical reperesentation in ASCII found.
			hex = asciiRec[i] & 0x0F;
		} else {
			// Alphabetical value.
			hex = 0x09 + (asciiRec[i] & 0x0F);						
		}
	
		// Following logic converts 2 bytes of ASCII to 1 byte of hex.
		k = i % 2;
		
		if(k) {
			hexRec[i/2] |= hex;	
		} else {
			hexRec[i/2] = (hex << 4) & 0xF0;
		}	
		i++;		
	}
}

/********************************************************************
* Function: 	EraseFlash()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		
*
* Side Effects:	No return from here.
*
* Overview: 	Erases Flash (Block Erase).
*
*			
* Note:		 	None.
********************************************************************/
void EraseFlash(void) {
	void * pFlash;
    UINT result;
    INT i;

    pFlash = (void*)APP_FLASH_BASE_ADDRESS;									
    for( i = 0; i < ((APP_FLASH_END_ADDRESS - APP_FLASH_BASE_ADDRESS + 1)/FLASH_PAGE_SIZE); i++ ) {
	     result = NVMemErasePage( pFlash + (i*FLASH_PAGE_SIZE) );
        // Assert on NV error. This must be caught during debug phase.
        if(result != 0) {
			LeaveBootloaderToAppOrReset();
        }
    }
}

/********************************************************************
* Function: 	WriteHexRecord2Flash()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		
*
* Side Effects:	No return from here.
*
* Overview: 	Writes Hex Records to Flash.
*
*			
* Note:		 	None.
********************************************************************/
void WriteHexRecord2Flash(UINT8* HexRecord) {
	static T_HEX_RECORD HexRecordSt;
	UINT8 Checksum = 0;
	UINT8 i;
	UINT WrData;
	UINT RdData;
	void* ProgAddress;
	UINT result;
		
	HexRecordSt.RecDataLen = HexRecord[0];
	HexRecordSt.RecType = HexRecord[3];	
	HexRecordSt.Data = &HexRecord[4];	
	
	// Hex Record checksum check.
	for(i = 0; i < HexRecordSt.RecDataLen + 5; i++) {
		Checksum += HexRecord[i];
	}	
	
    if(Checksum != 0) {
		DestroyApplication();
		LeaveBootloaderToAppOrReset();
	} else {
		// Hex record checksum OK.
		switch(HexRecordSt.RecType) {
			case DATA_RECORD: { //Record Type 00, data record.
				HexRecordSt.Address.byte.MB = 0;
				HexRecordSt.Address.byte.UB = 0;
				HexRecordSt.Address.byte.HB = HexRecord[1];
				HexRecordSt.Address.byte.LB = HexRecord[2];
				
				// Derive the address.
				HexRecordSt.Address.Val = HexRecordSt.Address.Val + HexRecordSt.ExtLinAddress.Val + HexRecordSt.ExtSegAddress.Val;
						
				while(HexRecordSt.RecDataLen) {
					// Convert the Physical address to Virtual address. 
					ProgAddress = (void *)PA_TO_KVA0(HexRecordSt.Address.Val);
					// Make sure we are not writing boot area and device configuration bits.
					if(((ProgAddress >= (void *)APP_FLASH_BASE_ADDRESS) && (ProgAddress <= (void *)APP_FLASH_END_ADDRESS))
					   && ((ProgAddress < (void*)DEV_CONFIG_REG_BASE_ADDRESS) || (ProgAddress > (void*)DEV_CONFIG_REG_END_ADDRESS))) {
						if(HexRecordSt.RecDataLen < 4) {
							// Sometimes record data length will not be in multiples of 4. Appending 0xFF will make sure that..
							// we don't write junk data in such cases.
							WrData = 0xFFFFFFFF;
							memcpy(&WrData, HexRecordSt.Data, HexRecordSt.RecDataLen);	
						} else {
							memcpy(&WrData, HexRecordSt.Data, 4);
						}		
						// Write the data into flash.	
						result = NVMemWriteWord(ProgAddress, WrData);	
						// Assert on error. This must be caught during debug phase.		
						if(result != 0) {
							DestroyApplication();
							LeaveBootloaderToAppOrReset();
   						}									
					}	
					
					// Increment the address.
					HexRecordSt.Address.Val += 4;
					// Increment the data pointer.
					HexRecordSt.Data += 4;
					// Decrement data len.
					if(HexRecordSt.RecDataLen > 3) {
						HexRecordSt.RecDataLen -= 4;
					} else {
						HexRecordSt.RecDataLen = 0;
					}	
				}
				break;
			}	
			case EXT_SEG_ADRS_RECORD: {  // Record Type 02, defines 4th to 19th bits of the data address.
			    HexRecordSt.ExtSegAddress.byte.MB = 0;
				HexRecordSt.ExtSegAddress.byte.UB = HexRecordSt.Data[0];
				HexRecordSt.ExtSegAddress.byte.HB = HexRecordSt.Data[1];
				HexRecordSt.ExtSegAddress.byte.LB = 0;
				// Reset linear address.
				HexRecordSt.ExtLinAddress.Val = 0;
				break;
			}
			case EXT_LIN_ADRS_RECORD: {   // Record Type 04, defines 16th to 31st bits of the data address. 
				HexRecordSt.ExtLinAddress.byte.MB = HexRecordSt.Data[0];
				HexRecordSt.ExtLinAddress.byte.UB = HexRecordSt.Data[1];
				HexRecordSt.ExtLinAddress.byte.HB = 0;
				HexRecordSt.ExtLinAddress.byte.LB = 0;
				// Reset segment address.
				HexRecordSt.ExtSegAddress.Val = 0;
				break;
			}	
			case END_OF_FILE_RECORD: {  //Record Type 01, defines the end of file record.
				HexRecordSt.ExtSegAddress.Val = 0;
				HexRecordSt.ExtLinAddress.Val = 0;
				// Disable any interrupts here before jumping to the application.
				if (FSrename ("finished.hex", myFile) != 0) {
					LeaveBootloaderToAppOrReset();
				} else {
					LeaveBootloaderToAppOrReset();
				}
				break;
			}		
			default: {
				HexRecordSt.ExtSegAddress.Val = 0;
				HexRecordSt.ExtLinAddress.Val = 0;
				break;
			}
		}		
	}		
}	

/********************************************************************
* Function: 	ValidAppPresent()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		TRUE: If application is valid.
*
* Side Effects:	None.
*
* Overview: 	Logic: Check application vector has 
				some value other than "0xFFFFFF"
*
*			
* Note:		 	None.
********************************************************************/
BOOL ValidAppPresent(void) {
	volatile UINT32 *AppPtr;
	
	AppPtr = (UINT32*)USER_APP_RESET_ADDRESS;

	if(*AppPtr == 0xFFFFFFFF) {
		return FALSE;
	} else {
		return TRUE;
	}
}			

static void LeaveBootloaderToAppOrReset(void) {
    if(ValidAppPresent()) {
        JumpToApp();        
	} else {
		mal_reset();
	}
}

static void DestroyApplication(void) {
	EraseFlash();
}
