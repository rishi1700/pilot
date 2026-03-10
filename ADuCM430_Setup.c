/*!
 *****************************************************************************
 * @file:   ADuCM430_Setup.c
 * @brief:  setup for ADuCM430
 * @version    V0.2
 * @date       March 2022
 * @par Revision History:
 * -V0.2, fixed UART pin configuration
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/
#include <stdarg.h>

#include "AdcLib.h"
#include "ClkLib.h"
#include "Common.h"
#include "DacLib.h"
#include "DioLib.h"
#include "UrtLib.h"
#include "WdtLib.h"

/*
   User can change following cofiguraion according to application
*/

void ADuCM430Setup(void)
{
    pADI_MISC->USERKEY = 0x9FE5;

    WdtGo(false);
    //WdtCfg( ENUM_WDT_CON_MDE_FREE, ENUM_WDT_CON_PRE_DIV256, \
    //        ENUM_WDT_CON_IRQ_INTERRUPT, ENUM_WDT_CON_PDSTOP_STOP );
    // WdtClkCfg(1, ENUM_WDT_CON_PRE_DIV256);
    // WdtWindowCfg(0x100, 1);
    // WdtGo(true);

    // Clock setup
    // use internal OSC to configure clock setting
    pADI_CLK->CLKCON0 &= ~BITM_CLOCK_CLKCON0_CLKMUX;
    for (volatile uint32_t i = 0; i < 1000; i++)
        ;

    uint16_t clkCfg;
    // HCLK, PCLK0, PCLK1
    clkCfg = (ENUM_CLOCK_CLKCON1_CDHCLK_DIV1 << BITP_CLOCK_CLKCON1_CDHCLK) |
             (ENUM_CLOCK_CLKCON1_CDPCLK0_DIV2 << BITP_CLOCK_CLKCON1_CDPCLK0) |
             (ENUM_CLOCK_CLKCON1_CDPCLK1_DIV2 << BITP_CLOCK_CLKCON1_CDPCLK1);
    pADI_CLK->CLKCON1 = clkCfg;

    // Enable PLL
    // If use xtal, Dio Config needed.
    // Use HFOSC
    pADI_CLK->CLKCON0 &= ~BITM_CLOCK_CLKCON0_PLLMUXSEL;
    pADI_CLK->CLKCON0 |= ENUM_CLOCK_CLKCON0_PLLMUXSEL_OSC16M << BITP_CLOCK_CLKCON0_PLLMUXSEL;

    pADI_CLK->CLKCON0 &= ~BITM_CLOCK_CLKCON0_SPLLPD;
    for (volatile uint32_t i = 0; i < 1000; i++)
        ;
    do {
        clkCfg = pADI_CLK->CLKSTAT0;
    }

    // check PLL ready
    while (!(clkCfg & BITM_CLOCK_CLKSTAT0_SPLLSTATUS));
    // Set System Pll as Clk source
    pADI_CLK->CLKCON0 &= ~BITM_CLOCK_CLKCON0_CLKMUX;
    pADI_CLK->CLKCON0 |= ENUM_CLOCK_CLKCON0_CLKMUX_SPLL << BITP_CLOCK_CLKCON0_CLKMUX;

    // set CLKOUT mux, at P2.1
    // pADI_CLK->CLKCON0 &= ~BITM_CLOCK_CLKCON0_CLKOUT;
    // pADI_CLK->CLKCON0 |= ENUM_CLOCK_CLKCON0_CLKOUT_HFOSC << BITP_CLOCK_CLKCON0_CLKOUT;
    // DioCfgPin(pADI_GPIO2, PIN1, P2_1_CLKOUT);

    // UrtSetup
    DioCfgPin(pADI_GPIO0, PIN4, P0_4_UART0_RX);
    DioCfgPin(pADI_GPIO0, PIN5, P0_5_UART0_TX);

    UrtCfg(pADI_UART, B115200, ENUM_UART_LCR_WLS_BITS8, 0);
    UrtIntCfg(pADI_UART, COMIEN_ERBFI);
    // NVIC_EnableIRQ(UART0_IRQn);

    DioCfgPin(pADI_GPIO2, PIN0, P2_2_GPIO);
    DioOenPin(pADI_GPIO2, PIN0, 1);

    UrtPrint("Binary Compiled at %s", __TIMESTAMP__);
    UrtPrint("-------> Init ");
   UrtPrint("All Done!\r\n");
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
        for (int i = 0; i < ret; i++) {
            do {
                status = UrtTx(pADI_UART, UrtPrBuf[i]);
            } while (status == 0);
        }
    }
    va_end(ap);
}
