/*!
 *****************************************************************************
 * @file:    ClkLib.h
 * @brief:   header file of clock
 * @version: V0.1
 * @date:    March 2021
 * @par:     Revision History:
 * - V0.1, March 2021: Initial version.
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/
#ifndef CLK_LIB_H
#define CLK_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "adi_processor.h"

typedef struct {
    // 0 - select Divided HCLK as PWM CLK source
    // 1 - select HCLK as PWM CLK source
    uint32_t pwmClkMux;

    // ENUM_CLOCK_CLKCON0_SPLLPD_ON  - enable pll, result in SPLL in 80MHz
    // ENUM_CLOCK_CLKCON0_SPLLPD_OFF - disable PLL.
    uint32_t pllEn;

    // ENUM_CLOCK_CLKCON0_PLLMUXSEL_OSC16M - 16MHz on chip OSC as PLL clock source
    // ENUM_CLOCK_CLKCON0_PLLMUXSEL_XTAL16M - 16MHz XTAL as pll clock source
    uint32_t pllClkMux;

    // 0 - XTAL interrupt will not be generated
    // 1 - XTAL interrupt will be generated
    uint32_t xtalIntEn;

    //---------Interrupt------------
    // 1 - enable interrupt related to PLL
    // 0 - disalbe interrrupt related to PLL
    uint32_t pllIntEn;

    // ENUM_CLOCK_CLKCON0_CLKMUX_HFOSC       /* High Frequency Internal Oscillator (HFOSC) */
    // ENUM_CLOCK_CLKCON0_CLKMUX_SPLL        /* System PLL is Selected (80 MHz) */
    // ENUM_CLOCK_CLKCON0_CLKMUX_EXTCLK      /* External GPIO Port is Selected (ECLKIN) */
    uint32_t rootClkMux;

    // 0 - Internal Oscillator as Gpt&PLA clock source
    // 1 - 16MHz XTAL
    // 2 - GPIO clock input (P2.2)
    uint32_t gptClkMux;

    //---------------Divider-----------
    // ENUM_CLOCK_CLKCON1_CDHCLK_DIV1     /* DIV1. Divide by 1 (HCLK is Equal
    // ENUM_CLOCK_CLKCON1_CDHCLK_DIV2     /* DIV2. Divide by 2 (HCLK is Half
    // ENUM_CLOCK_CLKCON1_CDHCLK_DIV4     /* DIV4. Divide by 4 (HCLK is Quart
    // ENUM_CLOCK_CLKCON1_CDHCLK_DIV8     /* DIV8. Divide by 8 */
    uint32_t hclkDiv;

    // ENUM_CLOCK_CLKCON1_CDPCLK0_DIV1    /* DIV1. Divide by 1 (PCLK is Equal t
    // ENUM_CLOCK_CLKCON1_CDPCLK0_DIV2    /* DIV2. Divide by 2 (PCLK is Half th
    // ENUM_CLOCK_CLKCON1_CDPCLK0_DIV4    /* DIV4. Divide by 4 (PCLK is Quarter
    // ENUM_CLOCK_CLKCON1_CDPCLK0_DIV8    /* DIV8. Divide by 8 */
    uint32_t pclk0Div;

    // ENUM_CLOCK_CLKCON1_CDPCLK1_DIV1    /* DIV1. Divide by 1 (PCLK is Equal t
    // ENUM_CLOCK_CLKCON1_CDPCLK1_DIV2    /* DIV2. Divide by 2 (PCLK is Half th
    // ENUM_CLOCK_CLKCON1_CDPCLK1_DIV4    /* DIV4. Divide by 4 (PCLK is Quarter
    // ENUM_CLOCK_CLKCON1_CDPCLK1_DIV8    /* DIV8. Divide by 8 */
    uint32_t pclk1Div;

    //-------TEST CLOCK -------
    // ENUM_CLOCK_CLKCON0_CLKOUT_SPLL_CLK  /* SPLL clock */
    // ENUM_CLOCK_CLKCON0_CLKOUT_HCLKBUS   /* Hclk_bus */
    // ENUM_CLOCK_CLKCON0_CLKOUT_T3        /* Timer 3 Clock */
    // ENUM_CLOCK_CLKCON0_CLKOUT_WUT       /* Wake up Timer Clock */
    // ENUM_CLOCK_CLKCON0_CLKOUT_T0        /* Timer 0 Clock */
    // ENUM_CLOCK_CLKCON0_CLKOUT_ANA_CLK   /* Analog Test Signal */
    // ENUM_CLOCK_CLKCON0_CLKOUT_B1_PCLK   /* Bridge 2 PCLK */
    // ENUM_CLOCK_CLKCON0_CLKOUT_B2_PCLK   /* Bridge 2 PCLK */
    // ENUM_CLOCK_CLKCON0_CLKOUT_B0_PCLK   /* Bridge 1 Pclk */
    // ENUM_CLOCK_CLKCON0_CLKOUT_CORE      /* Core Clock */
    uint32_t clkOutput;

    // 0 - OSC32K as GPT32 clock source
    // 1 - external clock as GPT32 clock source
    // 2 - PLACLK0 as GPT32 clock source
    // 3 - PLACLK1 as GPT32 clock source
    uint32_t gpt32ClkMux;

    // 0 - OSC32K as GPT32 clock source
    // 1 - external clock as GPT32 clock source
    // 2 - PLACLK0 as GPT32 clock source
    // 3 - PLACLK1 as GPT32 clock source
    uint32_t gpt2ClkMux;

    // 0 - OSC32K as GPT32 clock source
    // 1 - external clock as GPT32 clock source
    // 2 - PLACLK0 as GPT32 clock source
    // 3 - PLACLK1 as GPT32 clock source
    uint32_t gpt1ClkMux;

    // 0 - OSC32K as GPT32 clock source
    // 1 - external clock as GPT32 clock source
    // 2 - PLACLK0 as GPT32 clock source
    // 3 - PLACLK1 as GPT32 clock source
    uint32_t gpt0ClkMux;

} CLK_SETUP_t;

extern CLK_SETUP_t gClkSetup;

//---------------------function prototype---------------------------------
extern void ClkSetup(const CLK_SETUP_t *pSetup);

extern uint32_t ClkMuxCfg(uint32_t clockSource);
extern uint32_t GPIOClkOutCfg(uint32_t clockSource);
extern uint32_t PCLK0DivCfg(uint32_t clockSource);
extern uint32_t PCLK1DivCfg(uint32_t clockSource);
extern uint32_t SPLLIECfg(uint32_t pllIntEnable);
extern uint32_t PWMCLKCfg(uint32_t clockSource);
extern uint32_t PLLMUXCfg(uint32_t clockSource);
extern uint32_t HFXTALIEn(uint32_t enable);
extern uint32_t GPTMUXCfg(uint32_t clockSource);
extern uint32_t ClkSta(void);
extern void SPLLLOCKStickyStaCLR(void);
extern void SPLLUNLOCKStickyStaCLR(void);
extern void ClkSetup(const CLK_SETUP_t *pSetup);

#ifdef __cplusplus
}
#endif

#endif
