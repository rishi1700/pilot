/*!
 *****************************************************************************
 * @file:   common.c
 * @brief:  Common utilities for testing.
 * @version    V0.2
 * @date       July 2021
 * @par Revision History:
 * -V0.1, March 2021: initial version.
 * -V0.2, July 2021: change i from int to uint32_t
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2010-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/
#include "common.h"
#include <stdarg.h>

#if defined(ENABLE_DEBUG_PIN) || defined(REDIRECT_OUTPUT_TO_UART)
#include "DioLib.h"
#include "UrtLib.h"

#endif

#ifdef REDIRECT_OUTPUT_TO_UART

//#define USE_UART_PORT0
#define USE_UART_PORT3

#endif /* REDIRECT_OUTPUT_TO_UART */

/**
 * Test Initialization
 *
 * @brief  Test initialization
 *
 */
void common_Init(void)
{
#ifdef REDIRECT_OUTPUT_TO_UART
#ifdef USE_UART_PORT0
    // Configure  UART on port0 pins
    DioCfgPin(pADI_GPIO0, PIN4, P0_4_UART0_RX);
    DioCfgPin(pADI_GPIO0, PIN5, P0_5_UART0_TX);
    DioPulPin(pADI_GPIO0, PIN4, GPIO_PULLSEL_UP);
    DioPulPin(pADI_GPIO0, PIN5, GPIO_PULLSEL_UP);
    DioDsPin(pADI_GPIO0, PIN5, ENUM_GPIO_DS_DS5_STRENGTH3);
    DioDsPin(pADI_GPIO0, PIN4, ENUM_GPIO_DS_DS4_STRENGTH3);
#else
    // Configure  UART on port3 pins
    DioCfgPin(pADI_GPIO3, PIN3, P3_3_UART0_RX);
    DioCfgPin(pADI_GPIO3, PIN4, P3_4_UART0_TX);
    DioPulPin(pADI_GPIO3, PIN3, GPIO_PULLSEL_UP);
    DioPulPin(pADI_GPIO3, PIN4, GPIO_PULLSEL_UP);
    DioDsPin(pADI_GPIO3, PIN3, ENUM_GPIO_DS_DS3_STRENGTH3);
    DioDsPin(pADI_GPIO3, PIN4, ENUM_GPIO_DS_DS4_STRENGTH3);

#endif
#endif

#ifdef ENABLE_DEBUG_PIN
    DioCfgPin(DEBUG_PORT0, DEBUG_PIN0, 0); // gpio
    DioOenPin(DEBUG_PORT0, DEBUG_PIN0, 1); // enable output
    DioPulPin(DEBUG_PORT0, DEBUG_PIN0, 0); // enable pull up
    DioSet(DEBUG_PORT0, DEBUG_PIN0);       // set to 1 as default
    DioCfgPin(DEBUG_PORT0, DEBUG_PIN1, 0); // gpio
    DioOenPin(DEBUG_PORT0, DEBUG_PIN1, 1); // enable output
    DioPulPin(DEBUG_PORT0, DEBUG_PIN1, 0); // enable pull up
    DioSet(DEBUG_PORT0, DEBUG_PIN1);       // set to 1 as default

#endif
}

/**
 * UrtPrint
 *
 * Use stdarg lib vsprint lib to format string, write str to uart output
 *
 */
char UrtPrBuf[256];
void UrtPrint(char *fmt, ...)
{
    va_list ap;
    uint32_t ret;
    uint32_t status;

    va_start(ap, fmt);

    ret = vsprintf(UrtPrBuf, fmt, ap);

    if (ret < 256 && ret > 0) {
        for (uint32_t i = 0; i < ret; i++) {
            do {
                status = UrtTx(pADI_UART, UrtPrBuf[i]);
            } while (status == 0);
        }
    }
    va_end(ap);
}

/**
 * SafetyWait
 *
 * @brief  Blink and wait until KEIL/IAR halt CPU successfully
 *
 */
void SafetyWait(void)
{
    volatile uint32_t i, j;
    // LED setup
    DioOenPin(DEBUG_PORT0, DEBUG_PIN0, 1);
    DioSet(DEBUG_PORT0, DEBUG_PIN0);

    for (i = 0; i < 50; i++) {
        DEBUG_PIN0_TOGGLE();
        for (j = 0; j < DEBUG_TOGGLE_DELAY; j++) {
        }
    }
}
