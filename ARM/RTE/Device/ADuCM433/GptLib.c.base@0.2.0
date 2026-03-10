/*!
 *****************************************************************************
 * @file:   GptLib.c
 * @brief:  library for general purpose timer
 * @version: V0.2
 * @date:    April 2022
 * @par:     Revision History:
 * - V0.1, March 2021: Initial version.
 * - V0.2, April 2022: Cleanup Doxygen Docs
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2018 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#include "GptLib.h"

/**
    @brief uint32_t GptCfg(ADI_TMR_TypeDef *pTMR, uint32_t iClkSrc, uint32_t iScale, uint32_t iMode)
        ======== Configures timer GPTx if not busy.
    @param pTMR :{pADI_GPT0,pADI_GPT1,pADI_GPT2}
            - pADI_GPT0 for timer 0.
            - pADI_GPT1 for timer 1.
    - pADI_GPT2 for timer 2.
    @param iClkSrc :{ENUM_TMR_CON_CLK_PCLK1,ENUM_TMR_CON_CLK_SYSCLK,ENUM_TMR_CON_CLK_GPTXMUX,ENUM_TMR_CON_CLK_HFXTAL}
            - TxCON.5,6
    - ENUM_TMR_CON_CLK_PCLK1   PCLK.
    - ENUM_TMR_CON_CLK_SYSCLK   SYSCLK
    - ENUM_TMR_CON_CLK_GPTXMUX  GPTxMUX
    - ENUM_TMR_CON_CLK_HFXTAL HFXTAL. 16 MHz OSC or XTAL (Dependent on CLKCON0.11)
    @param iScale :{ENUM_TMR_CON_PRE_DIV1OR4,ENUM_TMR_CON_PRE_DIV16,ENUM_TMR_CON_PRE_DIV256,ENUM_TMR_CON_PRE_DIV32768}
            - TxCON.0,1
            - ENUM_TMR_CON_PRE_DIV1OR4      Source_clock / [1 or 4]
            - ENUM_TMR_CON_PRE_DIV16     Source_clock / 16
            - ENUM_TMR_CON_PRE_DIV256    Source_clock / 256
            - ENUM_TMR_CON_PRE_DIV32768  Source_clock / 32,768
    @param iMode :{BITM_TMR_CON_MOD|BITM_TMR_CON_UP|BITM_TMR_CON_RLD|BITM_TMR_CON_ENABLE}
            - TxCON.2-4,7,12
            - BITM_TMR_CON_MOD   Timer Runs in Periodic Mode (default)
            - BITM_TMR_CON_UP  Timer is Set to Count up
            - BITM_TMR_CON_RLD Resets the Up/down Counter When GPTCLRI[0] is Set
            - BITM_TMR_CON_ENABLE   EN. Timer is Enabled
    @return 1 if write register successfully
**/

uint32_t GptCfg(ADI_TMR_TypeDef *pTMR, uint32_t iClkSrc, uint32_t iScale, uint32_t iMode)
{
    uint32_t i1 = 0;
    if (pTMR->STA & BITM_TMR_STA_BUSY)
        return 0;
    // i1 = pTMR->CON & BITM_TMR_CON_EVENTS; // to keep the selected event
    i1 |= (iClkSrc << BITP_TMR_CON_CLK);
    i1 |= (iScale << BITP_TMR_CON_PRE);
    i1 |= iMode;
    pTMR->CON = (uint16_t)i1;
    return 1;
}

/**
    @brief void GptLd(ADI_TMR_TypeDef *pTMR, uint32_t iTLd)
        ======== Sets timer reload value.
    @param pTMR :{pADI_GPT0,pADI_GPT1,pADI_GPT2}
            - pADI_GPT0 for timer 0.
            - pADI_GPT1 for timer 1.
    - pADI_GPT2 for timer 2.
    @param iTLd :{0-65535}
            - Sets reload value TxLD to iTLd.
**/

void GptLd(ADI_TMR_TypeDef *pTMR, uint32_t iTLd) { pTMR->LD = (uint16_t)iTLd; }

/**
    @brief uint32_t GptVal(ADI_TMR_TypeDef *pTMR)
        ======== Reads timer value.
    @param pTMR :{pADI_GPT0,pADI_GPT1,pADI_GPT2}
            - pADI_GPT0 for timer 0.
            - pADI_GPT1 for timer 1.
    - pADI_GPT2 for timer 2.
    @return timer value TxVAL.
**/

uint32_t GptVal(ADI_TMR_TypeDef *pTMR) { return pTMR->VAL; }

/**
    @brief uint32_t GptSta(ADI_TMR_TypeDef *pTMR)
        ======== Reads timer status register.
    @param pTMR :{pADI_GPT0,pADI_GPT1,pADI_GPT2}
            - pADI_GPT0 for timer 0.
            - pADI_GPT1 for timer 1.
    - pADI_GPT2 for timer 2.
    @return TxSTA.
**/

uint32_t GptSta(ADI_TMR_TypeDef *pTMR) { return pTMR->STA; }

/**
    @brief void GptClrInt(ADI_TMR_TypeDef *pTMR, uint32_t iSource)
        ======== clears current Timer interrupt by writing to TxCLRI.
    @param pTMR :{pADI_GPT0,pADI_GPT1,pADI_GPT2}
            - pADI_GPT0 for timer 0.
            - pADI_GPT1 for timer 1.
    - pADI_GPT2 for timer 2.
    @param iSource :{BITM_TMR_CLRI_TMOUT}
            - BITM_TMR_CLRI_TMOUT for time out.
**/
void GptClrInt(ADI_TMR_TypeDef *pTMR, uint32_t iSource) { pTMR->CLRI = (uint16_t)iSource; }

/**
    @brief uint32_t GptBsy(ADI_TMR_TypeDef *pTMR)
        ======== Checks the busy bit.
    @param pTMR :{pADI_GPT0,pADI_GPT1,pADI_GPT2}
            - pADI_GPT0 for timer 0.
            - pADI_GPT1 for timer 1.
    - pADI_GPT2 for timer 2.
    @return busy bit: 0 is not busy, 1 is busy.
**/
uint32_t GptBsy(ADI_TMR_TypeDef *pTMR)
{
    if (pTMR->STA & BITM_TMR_STA_BUSY) {
        return 1;
    }
    else {
        return 0;
    }
}

/**
    @brief void GptSetup(ADI_TMR_TypeDef *pTMR, GPT_SETUP_t *pSetup)
        ======== setup for 16 bit general purpose timer

    @param pTMR :{pADI_GPT0,pADI_GPT1,pADI_GPT2}
            - pADI_GPT0 for timer 0.
            - pADI_GPT1 for timer 1.
            - pADI_GPT2 for timer 2.
    @param pSetup: pointer to timer setup structure
**/
void GptSetup(ADI_TMR_TypeDef *pTMR, GPT_SETUP_t *pSetup)
{
    uint32_t regCon;

    regCon = (pSetup->prescaler << BITP_TMR_CON_PRE) | (pSetup->countUp << BITP_TMR_CON_UP) |
             (pSetup->periodicMode << BITP_TMR_CON_MOD) | (pSetup->clockSource << BITP_TMR_CON_CLK) |
             (pSetup->reload << BITP_TMR_CON_RLD);
    pTMR->CON = (uint16_t)regCon;
    pTMR->LD = (uint16_t)(pSetup->loadValue);
}

//***********************************************
//   32BIT GPT FUNCTION
//***********************************************

/**
    @brief void Gpt32Setup(ADI_TIMER_TypeDef *pTMR, GPTH_SETUP_t *pSetup)
        ======== setup for 32 bit general purpose timer

    @param pTMR :{pADI_GPTH}
                    - pADI_GPTH for 32 bit timer
    @param pSetup: pointer to timer setup structure
**/
void Gpt32Setup(ADI_TIMER_TypeDef *pTMR, GPTH_SETUP_t *pSetup)
{
    uint32_t regCon, regCfg0, regCfg1, regCfg2, regCfg3;

    regCon = (pSetup->prescaler << BITP_TIMER_CTL_PRE) | (pSetup->clock_source << BITP_TIMER_CTL_SEL);
    regCfg0 = (pSetup->mode0 << BITP_TIMER_CFG_N__MODE) | (pSetup->event0 << BITP_TIMER_CFG_N__EVENTSEL) |
              (pSetup->ccEn0 << BITP_TIMER_CFG_N__CC_EN);
    regCfg1 = (pSetup->mode1 << BITP_TIMER_CFG_N__MODE) | (pSetup->event1 << BITP_TIMER_CFG_N__EVENTSEL) |
              (pSetup->ccEn1 << BITP_TIMER_CFG_N__CC_EN);
    regCfg2 = (pSetup->mode2 << BITP_TIMER_CFG_N__MODE) | (pSetup->event2 << BITP_TIMER_CFG_N__EVENTSEL) |
              (pSetup->ccEn2 << BITP_TIMER_CFG_N__CC_EN);
    regCfg3 = (pSetup->mode3 << BITP_TIMER_CFG_N__MODE) | (pSetup->event3 << BITP_TIMER_CFG_N__EVENTSEL) |
              (pSetup->ccEn3 << BITP_TIMER_CFG_N__CC_EN);

    pTMR->CNT = pSetup->count;
    pTMR->CC0 = pSetup->cc0_value;
    pTMR->CC1 = pSetup->cc1_value;
    pTMR->CC2 = pSetup->cc2_value;
    pTMR->CC3 = pSetup->cc3_value;
    pTMR->CFG0 = regCfg0;
    pTMR->CFG1 = regCfg1;
    pTMR->CFG2 = regCfg2;
    pTMR->CFG3 = regCfg3;
    pTMR->CTL = regCon;
}

/**
    @brief void GptGo(ADI_TMR_TypeDef *pTMR)
        ======== Run Gpt timer.
    @param pTMR :{pADI_GPT0,pADI_GPT1,pADI_GPT2}
            - pADI_GPT0 for timer 0.
            - pADI_GPT1 for timer 1.
            - pADI_GPT2 for timer 2.
**/
void GptGo(ADI_TMR_TypeDef *pTMR) { pTMR->CON |= BITM_TMR_CON_ENABLE; }

/**
    @brief void GptStop(ADI_TMR_TypeDef *pTMR)
        ======== Run Gpt timer.
    @param pTMR :{pADI_GPT0,pADI_GPT1,pADI_GPT2}
            - pADI_GPT0 for timer 0.
            - pADI_GPT1 for timer 1.
            - pADI_GPT2 for timer 2.
**/
void GptStop(ADI_TMR_TypeDef *pTMR) { pTMR->CON &= ~BITM_TMR_CON_ENABLE; }

/**
    @brief void Gpt32Go(ADI_TIMER_TypeDef *pTMR)
        ========  Run Gpt timer.
    @param pTMR :{}
            - pADI_GPTH for 32 bit timer
**/
void Gpt32Go(ADI_TIMER_TypeDef *pTMR) { pTMR->CTL |= BITM_TIMER_CTL_EN; }

/**
    @brief void Gpt32Stop(ADI_TIMER_TypeDef *pTMR)
        ========  Run Gpt timer.
    @param pTMR :{}
            - pADI_GPTH for 32 bit timer
**/
void Gpt32Stop(ADI_TIMER_TypeDef *pTMR) { pTMR->CTL &= ~BITM_TIMER_CTL_EN; }

/**
    @brief void GptClkCfg(uint32_t gpt0ClkMux, uint32_t gpt1ClkMux, uint32_t gpt2ClkMux, uint32_t gpt32ClkMux)
        ========  Configure the timer clock source
    @param gpt0ClkMux :{ENUM_CLOCK_CLKCON3_GPT0CLKMUX_OSC32K,ENUM_CLOCK_CLKCON3_GPT0CLKMUX_EXTCLK,
                        ENUM_CLOCK_CLKCON3_GPT0CLKMUX_PLACLK0,ENUM_CLOCK_CLKCON3_GPT0CLKMUX_PLACLK1}
        - ENUM_CLOCK_CLKCON3_GPT0CLKMUX_OSC32K   32 KHz OSC
        - ENUM_CLOCK_CLKCON3_GPT0CLKMUX_EXTCLK   External Clock
        - ENUM_CLOCK_CLKCON3_GPT0CLKMUX_PLACLK0  PLA Clock 0
        - ENUM_CLOCK_CLKCON3_GPT0CLKMUX_PLACLK1  PLA Clock 1
    @param gpt1ClkMux :{ENUM_CLOCK_CLKCON3_GPT1CLKMUX_OSC32K,ENUM_CLOCK_CLKCON3_GPT1CLKMUX_EXTCLK,
                        ENUM_CLOCK_CLKCON3_GPT1CLKMUX_PLACLK0,ENUM_CLOCK_CLKCON3_GPT1CLKMUX_PLACLK1}
        - ENUM_CLOCK_CLKCON3_GPT1CLKMUX_OSC32K   32 KHz OSC
        - ENUM_CLOCK_CLKCON3_GPT1CLKMUX_EXTCLK   External Clock
        - ENUM_CLOCK_CLKCON3_GPT1CLKMUX_PLACLK0  PLA Clock 0
        - ENUM_CLOCK_CLKCON3_GPT1CLKMUX_PLACLK1  PLA Clock 1
    @param gpt2ClkMux :{ENUM_CLOCK_CLKCON3_GPT2CLKMUX_OSC32K,ENUM_CLOCK_CLKCON3_GPT2CLKMUX_EXTCLK,
                        ENUM_CLOCK_CLKCON3_GPT2CLKMUX_PLACLK0,ENUM_CLOCK_CLKCON3_GPT2CLKMUX_PLACLK1}
        - ENUM_CLOCK_CLKCON3_GPT2CLKMUX_OSC32K   32 KHz OSC
        - ENUM_CLOCK_CLKCON3_GPT2CLKMUX_EXTCLK   External Clock
        - ENUM_CLOCK_CLKCON3_GPT2CLKMUX_PLACLK0  PLA Clock 0
        - ENUM_CLOCK_CLKCON3_GPT2CLKMUX_PLACLK1  PLA Clock 1
    @param gpt32ClkMux :{ENUM_CLOCK_CLKCON3_GPT32CLKMUX_OSC32K,ENUM_CLOCK_CLKCON3_GPT32CLKMUX_EXTCLK,
                        ENUM_CLOCK_CLKCON3_GPT32CLKMUX_PLACLK0,ENUM_CLOCK_CLKCON3_GPT32CLKMUX_PLACLK1}
        - ENUM_CLOCK_CLKCON3_GPT32CLKMUX_OSC32K   32 KHz OSC
        - ENUM_CLOCK_CLKCON3_GPT32CLKMUX_EXTCLK   External Clock
        - ENUM_CLOCK_CLKCON3_GPT32CLKMUX_PLACLK0  PLA Clock 0
        - ENUM_CLOCK_CLKCON3_GPT32CLKMUX_PLACLK1  PLA Clock 1
**/
void GptClkCfg(uint32_t gpt0ClkMux, uint32_t gpt1ClkMux, uint32_t gpt2ClkMux, uint32_t gpt32ClkMux)
{
    pADI_CLK->CLKCON3 = gpt0ClkMux << BITP_CLOCK_CLKCON3_GPT0CLKMUX | gpt1ClkMux << BITP_CLOCK_CLKCON3_GPT1CLKMUX |
                        gpt2ClkMux << BITP_CLOCK_CLKCON3_GPT2CLKMUX | gpt32ClkMux << BITP_CLOCK_CLKCON3_GPT32CLKMUX;
}
