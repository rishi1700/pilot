/*!
 *****************************************************************************
 * @file:   common.h
 * @brief:  Common utilities for testing.
 * @version: V0.2
 * @date:    May 2021
 * @par:     Revision History:
 * - V0.2, May 2021: change DEBUG_PIN0 to pin0.
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_PORT0         (pADI_GPIO2)
#define DEBUG_PIN0          (PIN0)
#define DEBUG_PIN0_TOGGLE() DioTgl(DEBUG_PORT0, DEBUG_PIN0)
#define DEBUG_PIN0_SET()    DioSetPin(DEBUG_PORT0, DEBUG_PIN0)
#define DEBUG_PIN0_CLR()    DioClrPin(DEBUG_PORT0, DEBUG_PIN0)
#define DEBUG_PIN1          (PIN4)
#define DEBUG_PIN1_TOGGLE() DioTgl(DEBUG_PORT0, DEBUG_PIN1)
#define DEBUG_PIN1_SET()    DioSetPin(DEBUG_PORT0, DEBUG_PIN1)
#define DEBUG_PIN1_CLR()    DioClrPin(DEBUG_PORT0, DEBUG_PIN1)
#define DEBUG_TOGGLE_DELAY  1000

/* Enable REDIRECT_OUTPUT_TO_UART to send the output to UART terminal */
#define REDIRECT_OUTPUT_TO_UART

/*Disable ENABLE_DEBUG_PIN to prevent debug pin*/
#define ENABLE_DEBUG_PIN

/********************************************************************************
 * API function prototypes
 *********************************************************************************/
void common_Init(void);
void SafetyWait(void);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
