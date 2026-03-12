/*!
 *****************************************************************************
 *  @file:   UrtLib.h
 *  @brief:  Set of UART peripheral functions.
    @version    V0.1
    @date       March 2021
    @par Revision History:
    - V0.1, March 2021: initial version.
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2018 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#ifndef URT_LIB_H
#define URT_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "adi_processor.h"

// baud rate settings
#define B1200   1200
#define B2200   2200
#define B2400   2400
#define B4800   4800
#define B9600   9600
#define B19200  19200
#define B38400  38400
#define B57600  57600
#define B115200 115200
#define B230400 230400
#define B430800 430800

/*Urt Line Control Register */
#define COMLCR_BRK_EN        1
#define COMLCR_BRK_DIS       0
#define COMLCR_SP_EN         (0x20)
#define COMLCR_EPS_EVEN      (0x10)
#define COMLCR_PEN_EN        (0x08)
#define COMLCR_STOP_MULTIBIT (0x04)
#define COMLCR_STOP_ONEBIT   (0x00)

/*Urt Interrupt Enable Register*/
#define COMIEN_ERBFI (0x01)
#define COMIEN_ETBEI (0x02)
#define COMIEN_ELSI  (0x04)
#define COMIEN_EDSSI (0x08)
#define COMIEN_EDMAT (0x10)
#define COMIEN_EDMAR (0x20)

#define DEFAULT_ROOTCLK (20000000)
#define OSC32M_ROOTCLK  (32000000)
#ifdef FPGA_VALIDATION
#define OSC16M_ROOTCLK (32000000)
#else
#define OSC16M_ROOTCLK (16000000)
#endif
#define PLL_ROOTCLK (80000000)
#define EXT_GPIO_ROOTCLK                                                                                               \
    (80000000) // External clock is assumed to be 80MhZ, if different clock speed
               // is used, this should be changed

//------------------------------ Function prototypes

uint32_t UrtCfg(ADI_UART_TypeDef *pPort, uint32_t iBaud, uint32_t iBits, uint32_t iFormat);
uint32_t UrtFifoCfg(ADI_UART_TypeDef *pPort, uint32_t iFifoSize, uint32_t iFIFOEn);
uint32_t UrtFifoClr(ADI_UART_TypeDef *pPort, uint32_t iClrEn);
uint32_t UrtBrk(ADI_UART_TypeDef *pPort, uint32_t iBrk);
uint32_t UrtLinSta(ADI_UART_TypeDef *pPort);
uint32_t UrtTx(ADI_UART_TypeDef *pPort, uint32_t iTx);
uint8_t UrtRx(ADI_UART_TypeDef *pPort);
uint32_t UrtMod(ADI_UART_TypeDef *pPort, uint32_t iMcr, uint32_t iWr);
uint32_t UrtModSta(ADI_UART_TypeDef *pPort);
uint32_t UrtIntCfg(ADI_UART_TypeDef *pPort, uint32_t iIrq);
uint32_t UrtIntSta(ADI_UART_TypeDef *pPort);
uint32_t UrtSendString(ADI_UART_TypeDef *pPort, char *buffer);
void UrtPrint(char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // URT_LIB_H
