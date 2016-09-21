/********************************************************************
 FileName:      HardwareProfile.h
 Dependencies:  See INCLUDES section
 Processor:     PIC32
 Hardware:      The code is natively intended to be used on the 
                  following hardware platforms: 
                  Explorer 16
                  USB Starter Kit
                  Ethernet Starter Kit
                  The firmware may be modified for use on other 
                  platforms by editing this file (HardwareProfile.h)
 Compiler:  	Microchip C32 (for PIC32)
 Company:       Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the �Company�) for its PIC� Microcontroller is intended and
 supplied to you, the Company�s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN �AS IS� CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************/


#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H


//   Part number defining Macro
#if   (((__PIC32_FEATURE_SET__ >= 100) && (__PIC32_FEATURE_SET__ <= 299)))
    #define __PIC32MX1XX_2XX__
#elif (((__PIC32_FEATURE_SET__ >= 300) && (__PIC32_FEATURE_SET__ <= 799)))
    #define __PIC32MX3XX_7XX__
#else
    #error("Controller not supported")
#endif


#if defined(__PIC32MX1XX_2XX__)
    // Maximum System frequency of 40MHz for PIC32MX1xx and PIC32MX2xx devices.
    #define SYS_FREQ (40000000L)
#elif defined(__PIC32MX3XX_7XX__)
   // Maximum System frequency of 80MHz for PIC32MX3xx, PIC32MX4xx, 
    // PIC32MX5xx, PIC32MX6xx and PIC32MX7xx devices.
    #define SYS_FREQ (80000000L)
	#define PB_DIV         		2
#endif


// Common macros 
// Clock frequency values
// These directly influence timed events using the Tick module.  They also are used for UART and SPI baud rate generation.
#define GetSystemClock()		SYS_FREQ			// Hz
#define GetInstructionClock()	(GetSystemClock()/1)	// 
#define GetPeripheralClock()	(GetSystemClock()/PB_DIV)	// Divisor is dependent on the value of FPBDIV set(configuration bits).



// Demo board hardware profiles
#include "HardwareProfile_PIC32MX_OlimexPIC32.h"	
#include "HardwareProfile_PIC32MX_SD_OlimexPIC32.h"	

#endif  //HARDWARE_PROFILE_H
