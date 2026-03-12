/*!
 *****************************************************************************
 * @file:    ClkLib.c
 * @brief:   source file of clock
 * @version: V0.3
 * @date:    Apr 2022
 * @par:     Revision History:
 * - V0.1, March 2021: Initial version.
 * - V0.2, Feb 2022: Load and clear userkey on CLKCON0 related functions
 * - V0.3, April 2022: Cleanup Doxygen Docs
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#include "ClkLib.h"

/**
    @brief uint32_t ClkMuxCfg(uint32_t clockSource)
        ======== select root clock source
    @param clockSource:
        -ENUM_CLOCK_CLKCON0_CLKMUX_HFOSC       High Frequency Internal Oscillator (HFOSC 16MHz)
        -ENUM_CLOCK_CLKCON0_CLKMUX_SPLL         System PLL is Selected (80 MHz)
        -ENUM_CLOCK_CLKCON0_CLKMUX_EXTCLK       External GPIO Port is Selected (ECLKIN P2.2)
    @return 1
**/
uint32_t ClkMuxCfg(uint32_t clockSource)
{
    uint32_t reg;
    uint32_t status;
    pADI_MISC->USERKEY = 0x9FE5;
    if (clockSource == ENUM_CLOCK_CLKCON0_CLKMUX_SPLL) {
        do {
            status = pADI_CLK->CLKSTAT0;
        } while (!(status & BITM_CLOCK_CLKSTAT0_SPLLSTATUS)); // check PLL ready
    }
    reg = pADI_CLK->CLKCON0;
    reg &= ~BITM_CLOCK_CLKCON0_CLKMUX;
    reg |= (clockSource << BITP_CLOCK_CLKCON0_CLKMUX);
    pADI_CLK->CLKCON0 = (uint16_t)reg;
    pADI_MISC->USERKEY = 0;
    return 1;
}

/**
    @brief uint32_t GPIOClkOutCfg(uint32_t clockSource)
        ======== Used to select which clock will be output on selected P2.1
    @param clockSource:
        -ENUM_CLOCK_CLKCON0_CLKOUT_SPLL_CLK     SPLL clock
        -ENUM_CLOCK_CLKCON0_CLKOUT_HCLKBUS      Hclk_bus
        -ENUM_CLOCK_CLKCON0_CLKOUT_T3           Timer 3 Clock
        -ENUM_CLOCK_CLKCON0_CLKOUT_WUT          Wake up Timer Clock
        -ENUM_CLOCK_CLKCON0_CLKOUT_T0           Timer 0 Clock
        -ENUM_CLOCK_CLKCON0_CLKOUT_ANA_CLK      Analog Test Signal
        -ENUM_CLOCK_CLKCON0_CLKOUT_B1_PCLK      Bridge 2 PCLK
        -ENUM_CLOCK_CLKCON0_CLKOUT_B2_PCLK      Bridge 2 PCLK
        -ENUM_CLOCK_CLKCON0_CLKOUT_B0_PCLK      Bridge 1 Pclk
        -ENUM_CLOCK_CLKCON0_CLKOUT_CORE         Core Clock
        -ENUM_CLOCK_CLKCON0_CLKOUT_LFOSC        32K OSC
        -ENUM_CLOCK_CLKCON0_CLKOUT_HXTAL        Crystal clok
        -ENUM_CLOCK_CLKCON0_CLKOUT_ROOT         Root Clock
        -ENUM_CLOCK_CLKCON0_CLKOUT_HFOSC        HFOSC (16 MHz)
    @return 1
**/
uint32_t GPIOClkOutCfg(uint32_t clockSource)
{
    uint32_t reg;
    pADI_MISC->USERKEY = 0x9FE5;
    reg = pADI_CLK->CLKCON0;
    reg &= ~BITM_CLOCK_CLKCON0_CLKOUT;
    reg |= (clockSource << BITP_CLOCK_CLKCON0_CLKOUT);
    pADI_CLK->CLKCON0 = (uint16_t)reg;
    pADI_MISC->USERKEY = 0;
    return 1;
}

/**
    @brief uint32_t PCLK0DivCfg(uint32_t clockSource)
        ======== select Analog Clock Source PCLK0 must be <= HCLK
    @param clockSource:
        - ENUM_CLOCK_CLKCON1_CDPCLK0_DIV2   Divide by 2  recommend setting for ADC and analog blocks
        - ENUM_CLOCK_CLKCON1_CDPCLK0_DIV4   Divide by 4 (PCLK is Quarter of Root Clock)
        - ENUM_CLOCK_CLKCON1_CDPCLK0_DIV1   Divide by 1 (PCLK is Equal to Root Clock)
    @return 1
**/

uint32_t PCLK0DivCfg(uint32_t clockSource)
{
    uint32_t reg;

    reg = pADI_CLK->CLKCON1;
    reg &= ~BITM_CLOCK_CLKCON1_CDPCLK0;
    reg |= (clockSource << BITP_CLOCK_CLKCON1_CDPCLK0);
    pADI_CLK->CLKCON1 = (uint16_t)reg;
    return 1;
}

/**
    @brief uint32_t PCLK1DivCfg(uint32_t clockSource)
        ======== select Analog Clock Source PCLK0 must be <= HCLK
    @param clockSource:
        - ENUM_CLOCK_CLKCON1_CDPCLK1_DIV1   Divide by 1 (PCLK is Equal to Root Clock)
        - ENUM_CLOCK_CLKCON1_CDPCLK1_DIV2   Divide by 2 (PCLK is Half the Frequency of Root Clock)
        - ENUM_CLOCK_CLKCON1_CDPCLK1_DIV4   Divide by 4 (PCLK is Quarter of Root Clock)

        - ENUM_CLOCK_CLKCON1_CDPCLK0_DIV8   Divide by 8
      Not recommended. Analog Devices has not characterized for these clock divide settings
    @return 1
**/
uint32_t PCLK1DivCfg(uint32_t clockSource)
{
    uint32_t reg;

    reg = pADI_CLK->CLKCON1;
    reg &= ~BITM_CLOCK_CLKCON1_CDPCLK1;
    reg |= (clockSource << BITP_CLOCK_CLKCON1_CDPCLK1);
    pADI_CLK->CLKCON1 = (uint16_t)reg;
    return 1;
}

/**
    @brief uint32_t SPLLIECfg(uint32_t pllIntEnable)
        ======== PLL Interrupt Enable
    @param pllIntEnable:
        -ENUM_CLOCK_CLKCON0_SPLLIE_DIS          PLL Interrupt Will Not Be Generated
        -ENUM_CLOCK_CLKCON0_SPLLIE_EN           PLL Interrupt Will Be Generated
    @return 1
**/
uint32_t SPLLIECfg(uint32_t pllIntEnable)
{
    uint32_t reg;
    pADI_MISC->USERKEY = 0x9FE5;
    reg = pADI_CLK->CLKCON0;
    reg &= ~BITM_CLOCK_CLKCON0_SPLLIE;
    reg |= (pllIntEnable << BITP_CLOCK_CLKCON0_SPLLIE);
    pADI_CLK->CLKCON0 = (uint16_t)reg;
    pADI_MISC->USERKEY = 0;
    return 1;
}

/**
    @brief uint32_t PWMCLKCfg(uint32_t clockSource)
        ======== PWM Clock source selection
    @param clockSource:
        -ENUM_CLOCK_CLKCON0_PWMCLKSEL_DIVCLK
        -ENUM_CLOCK_CLKCON0_PWMCLKSEL_HCLK
    @return 1
**/
uint32_t PWMCLKCfg(uint32_t clockSource)
{
    uint32_t reg;
    pADI_MISC->USERKEY = 0x9FE5;
    reg = pADI_CLK->CLKCON0;
    reg &= ~BITM_CLOCK_CLKCON0_PWMCLKSEL;
    reg |= (clockSource << BITP_CLOCK_CLKCON0_PWMCLKSEL);
    pADI_CLK->CLKCON0 = (uint16_t)reg;
    pADI_MISC->USERKEY = 0;
    return 1;
}

/**
    @brief uint32_t PLLMUXCfg(uint32_t clockSource)
        ======== PLL Clock source selection
    @param clockSource:
        -ENUM_CLOCK_CLKCON0_PLLMUXSEL_OSC16M
        -ENUM_CLOCK_CLKCON0_PLLMUXSEL_XTAL16M
    @return 1
**/
uint32_t PLLMUXCfg(uint32_t clockSource)
{
    uint32_t reg;
    pADI_MISC->USERKEY = 0x9FE5;
    reg = pADI_CLK->CLKCON0;
    reg &= ~BITM_CLOCK_CLKCON0_PLLMUXSEL;
    reg |= (clockSource << BITP_CLOCK_CLKCON0_PLLMUXSEL);
    pADI_CLK->CLKCON0 = (uint16_t)reg;
    pADI_MISC->USERKEY = 0;
    return 1;
}

/**
    @brief uint32_t HFXTALIEn(uint32_t enable)
        ======== HFXTAL (High Frequency Crystal) Interrupt Enable
    @param enable:
        -ENUM_CLOCK_CLKCON0_HFXTALIE_DIS
        -ENUM_CLOCK_CLKCON0_HFXTALIE_EN
    @return 1
**/
uint32_t HFXTALIEn(uint32_t enable)
{
    uint32_t reg;
    pADI_MISC->USERKEY = 0x9FE5;
    reg = pADI_CLK->CLKCON0;
    reg &= ~BITM_CLOCK_CLKCON0_HFXTALIE;
    reg |= (enable << BITP_CLOCK_CLKCON0_HFXTALIE);
    pADI_CLK->CLKCON0 = (uint16_t)reg;
    pADI_MISC->USERKEY = 0;
    return 1;
}

/**
    @brief uint32_t GPTMUXCfg(uint32_t clockSource)
        ======== GPT/PLA Clock source selection
    @param clockSource:
        -ENUM_CLOCK_CLKCON0_GPTCLK_HFOSC
        -ENUM_CLOCK_CLKCON0_GPTCLK_GPIOCLK
        -ENUM_CLOCK_CLKCON0_CLKOUT_HFOSC
    @return 1
**/
uint32_t GPTMUXCfg(uint32_t clockSource)
{
    uint32_t reg;
    pADI_MISC->USERKEY = 0x9FE5;
    reg = pADI_CLK->CLKCON0;
    reg &= ~BITM_CLOCK_CLKCON0_GPTCLK;
    reg |= (clockSource << BITP_CLOCK_CLKCON0_GPTCLK);
    pADI_CLK->CLKCON0 = (uint16_t)reg;
    pADI_MISC->USERKEY = 0;
    return 1;
}

/**
    @brief  uint32_t ClkSta()
        ========== Read the status register for the Clk.
    @return value of pADI_CLK->CLKSTAT0
        - SPLLSTATUS,System PLL Status.
        - SPLLLOCKCLR ,System PLL Lock.
        - SPLLUNLOCKCLR, System PLL Unlock
        - SPLLLOCK ,Sticky System PLL Lock Flag.
        - SPLLUNLOCK ,Sticky System PLL Unlock Flag.
        - [15:5]  RESERVED
**/
uint32_t ClkSta() { return pADI_CLK->CLKSTAT0; }

/**
    @brief void SPLLLOCKStickyStaCLR(void)
        ======== Writing a one to this bit clear sticky status and Lock IRQ source, this bit can be auto-cleared to 0
after writing a 1.

**/
void SPLLLOCKStickyStaCLR(void)
{
    uint32_t reg;

    reg = pADI_CLK->CLKSTAT0;
    reg &= ~BITM_CLOCK_CLKSTAT0_SPLLLOCKCLR;
    reg |= (1 << BITP_CLOCK_CLKSTAT0_SPLLLOCKCLR);
    pADI_CLK->CLKSTAT0 = (uint16_t)reg;
}

/**
    @brief void SPLLUNLOCKStickyStaCLR(void)
        ======== Writing a one to this bit clear sticky status and Lock IRQ source, this bit can be auto-cleared to 0
after writing a 1.
**/

void SPLLUNLOCKStickyStaCLR(void)
{
    uint32_t reg;

    reg = pADI_CLK->CLKSTAT0;
    reg &= ~BITM_CLOCK_CLKSTAT0_SPLLUNLOCKCLR;
    reg |= (1 << BITP_CLOCK_CLKSTAT0_SPLLUNLOCKCLR);
    pADI_CLK->CLKSTAT0 = (uint16_t)reg;
}

/**
    @brief void ClkSetup(const CLK_SETUP_t *pSetup)
        ======== setup clock
    @param pSetup: pointer to CLK_SETUP_t structure
**/
void ClkSetup(const CLK_SETUP_t *pSetup)
{
    pADI_MISC->USERKEY = 0x9FE5;
    uint32_t regCLKCON1, regCLKCON3, status;

    uint32_t preClkCfg = pADI_CLK->CLKCON0;
    pADI_CLK->CLKCON0 =
        (uint16_t)(preClkCfg & (~BITM_CLOCK_CLKCON0_CLKMUX)); // use internal OSC to configure clock setting
    for (volatile uint32_t i = 0; i < 1000; i++)
        ;

    preClkCfg = pADI_CLK->CLKCON0 & (~BITM_CLOCK_CLKCON0_CLKOUT);

    /* before config clkcon0 register, config clkcon1 first */
    /* casue the flash clk from hclk */
    regCLKCON1 = (pSetup->hclkDiv << BITP_CLOCK_CLKCON1_CDHCLK) | (pSetup->pclk0Div << BITP_CLOCK_CLKCON1_CDPCLK0) |
                 (pSetup->pclk1Div << BITP_CLOCK_CLKCON1_CDPCLK1);

    pADI_CLK->CLKCON1 = (uint16_t)regCLKCON1;

    if (!pSetup->pllEn) {
        // config pll input source clock
        pADI_CLK->CLKCON0 &= ~BITM_CLOCK_CLKCON0_PLLMUXSEL;
        pADI_CLK->CLKCON0 |= pSetup->pllClkMux << BITP_CLOCK_CLKCON0_PLLMUXSEL;

        // enable pll
        pADI_CLK->CLKCON0 &= ~BITM_CLOCK_CLKCON0_SPLLPD;
        for (volatile uint32_t i = 0; i < 1000; i++)
            ;

        do {
            status = pADI_CLK->CLKSTAT0;
        }
        // check PLL ready
        while (!(status & BITM_CLOCK_CLKSTAT0_SPLLLOCK));
    }
    else {
        // disable PLL.
        pADI_CLK->CLKCON0 |= ~BITM_CLOCK_CLKCON0_SPLLPD;
    }

    // set sys clock input mux. PLL/OSC/External CLK
    pADI_CLK->CLKCON0 &= ~BITM_CLOCK_CLKCON0_CLKMUX;
    pADI_CLK->CLKCON0 |= pSetup->rootClkMux;

    // set CLKOUT input mux
    pADI_CLK->CLKCON0 &= ~BITM_CLOCK_CLKCON0_CLKOUT;
    pADI_CLK->CLKCON0 |= pSetup->clkOutput;

    // set pll interupt
    pADI_CLK->CLKCON0 &= ~BITM_CLOCK_CLKCON0_SPLLIE;
    pADI_CLK->CLKCON0 |= pSetup->pllIntEn;

    regCLKCON3 =
        pSetup->gpt0ClkMux << BITP_CLOCK_CLKCON3_GPT0CLKMUX | pSetup->gpt1ClkMux << BITP_CLOCK_CLKCON3_GPT1CLKMUX |
        pSetup->gpt2ClkMux << BITP_CLOCK_CLKCON3_GPT2CLKMUX | pSetup->gpt32ClkMux << BITP_CLOCK_CLKCON3_GPT32CLKMUX;

    pADI_CLK->CLKCON3 = regCLKCON3;
    pADI_MISC->USERKEY = 0;
}
