/*!
 *****************************************************************************
 * @file:  GptLib.h
 * @brief: header file of general purpose timer
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
#ifndef GPT_LIB_H
#define GPT_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "adi_processor.h"

#define T0CON_EVENT_WUT  (0x0 << BITP_TMR_CON_EVENTS)
#define T0CON_EVENT_EXT0 (0x1 << BITP_TMR_CON_EVENTS)
#define T0CON_EVENT_EXT1 (0x2 << BITP_TMR_CON_EVENTS)
#define T0CON_EVENT_EXT2 (0x3 << BITP_TMR_CON_EVENTS)
#define T0CON_EVENT_EXT3 (0x4 << BITP_TMR_CON_EVENTS)
#define T0CON_EVENT_EXT4 (0x5 << BITP_TMR_CON_EVENTS)
#define T0CON_EVENT_EXT5 (0x6 << BITP_TMR_CON_EVENTS)
#define T0CON_EVENT_EXT6 (0x7 << BITP_TMR_CON_EVENTS)
#define T0CON_EVENT_EXT7 (0x8 << BITP_TMR_CON_EVENTS)
#define T0CON_EVENT_EXT8 (0x9 << BITP_TMR_CON_EVENTS)
#define T0CON_EVENT_T3   (0xA << BITP_TMR_CON_EVENTS)

#define T0CON_EVENT_LV0 (0xD << 8)

#define T0CON_EVENT_T1 (0xF << 8)

#define T1CON_EVENT_EXT4    (0x0 << 8)
#define T1CON_EVENT_EXT5    (0x1 << 8)
#define T1CON_EVENT_EXT6    (0x2 << 8)
#define T1CON_EVENT_FLASH   (0x3 << 8)
#define T1CON_EVENT_UART    (0x4 << 8)
#define T1CON_EVENT_SPI0    (0x5 << 8)
#define T1CON_EVENT_PLA0    (0x6 << 8)
#define T1CON_EVENT_PLA1    (0x7 << 8)
#define T1CON_EVENT_DMAERR  (0x8 << 8)
#define T1CON_EVENT_DMADONE (0x9 << 8)

#define T1CON_EVENT_I2C1S (0xD << 8)
#define T1CON_EVENT_I2C1M (0xE << 8)
#define T1CON_EVENT_T2    (0xF << 8)

#define T2CON_EVENT_EXT7  (0x0 << 8)
#define T2CON_EVENT_EXT8  (0x1 << 8)
#define T2CON_EVENT_SPI1  (0x2 << 8)
#define T2CON_EVENT_I2C0S (0x3 << 8)
#define T2CON_EVENT_I2C0M (0x4 << 8)
#define T2CON_EVENT_PLA2  (0x5 << 8)
#define T2CON_EVENT_PLA3  (0x6 << 8)
#define T2CON_EVENT_PWMT  (0x7 << 8)
#define T2CON_EVENT_PWM0  (0x8 << 8)
#define T2CON_EVENT_PWM1  (0x9 << 8)
#define T2CON_EVENT_PWM2  (0xA << 8)
#define T2CON_EVENT_PWM3  (0xB << 8)
#define T2CON_EVENT_LV1   (0xC << 8)
#define T2CON_EVENT_EXT0  (0xD << 8)
#define T2CON_EVENT_EXT1  (0xE << 8)
#define T2CON_EVENT_T1    (0xF << 8)

typedef struct {
    // ENUM_TMR_CON_CLK_PCLK0        PCLK.
    // ENUM_TMR_CON_CLK_HCLK        ROOT_CLK
    // ENUM_TMR_CON_CLK_LFOSC       LFOSC. 32 KHz OSC
    // ENUM_TMR_CON_CLK_HFXTAL      HFXTAL. 16 MHz OSC or XTAL (Dependent on CLKCON0.11)
    uint32_t clockSource;
    // ENUM_TMR_CON_PRE_DIV1      Source_clock / [1 or 4] , divide by 4 when PCLK or ROOT_CLK selected as source
    // ENUM_TMR_CON_PRE_DIV16       Source_clock / 16
    // ENUM_TMR_CON_PRE_DIV256      Source_clock / 256
    // ENUM_TMR_CON_PRE_DIV32768    Source_clock / 32,768
    uint32_t prescaler;
    // 1 - timer counting up
    // 0 - timer counting down
    uint32_t countUp;
    // 1 or ENUM_TMR_CON_MOD_PERIODIC- timer runs in periodic mode
    // 0 or ENUM_TMR_CON_MOD_FREERUN - tiemr runs in free running mode
    uint32_t periodicMode;
    // laodValue is only used for periodic mode
    uint32_t loadValue;
    // reload property is only used for periodic mode
    // 1 - reload enabled
    // 0 - disable
    uint32_t reload;

    // Event/Capture configuration -----
    // unused now, no need to configure
    uint32_t eventSource;
    // 1 - enable event capture feature
    // 0 - disable event capture feature
    uint32_t eventEn;
} GPT_SETUP_t;

extern GPT_SETUP_t gGpt0Setup;
extern GPT_SETUP_t gGpt1Setup;
extern GPT_SETUP_t gGpt2Setup;

// setup for 32bit timer
typedef struct {
    uint32_t count; // count value for 32bit timer
    // avaible options for clock source
    // ENUM_TIMER_CTL_SEL_PCLK       PCLK
    // ENUM_TIMER_CTL_SEL_SYSCLK     HCLK
    // ENUM_TIMER_CTL_SEL_LFOSC      32KHz Oscillator
    uint32_t clock_source;
    uint32_t prescaler; // prescaler of timer

    // 0 - disable compare and capture function
    // 1 - enable compare or capture function
    uint32_t ccEn0;
    uint32_t ccEn1;
    uint32_t ccEn2;
    uint32_t ccEn3;
    // ENUM_TIMER_CFG_N__MODE_CMP  working in compare mode
    // ENUM_TIMER_CFG_N__MODE_CAP  working in capture mode
    uint32_t mode0;
    uint32_t mode1;
    uint32_t mode2;
    uint32_t mode3;
    uint32_t cc0_value; // compare or capture value for 32bit timer
    uint32_t cc1_value;
    uint32_t cc2_value;
    uint32_t cc3_value;

    // event only useful when mode selected as capture mode ENUM_TIMER_CFG_N__MODE_CAP
    uint32_t event0;
    uint32_t event1;
    uint32_t event2;
    uint32_t event3;
} GPTH_SETUP_t;

extern GPTH_SETUP_t gGpth0Setup;
extern GPTH_SETUP_t gGpth1Setup;

//------------------------------ Function Declaration --------------------------
extern void GptSetup(ADI_TMR_TypeDef *pTMR, GPT_SETUP_t *pSetup);
extern uint32_t GptCfg(ADI_TMR_TypeDef *pTMR, uint32_t iClkSrc, uint32_t iScale, uint32_t iMode);
extern void GptLd(ADI_TMR_TypeDef *pTMR, uint32_t iTLd);
extern uint32_t GptVal(ADI_TMR_TypeDef *pTMR);
extern uint32_t GptSta(ADI_TMR_TypeDef *pTMR);
extern void GptClrInt(ADI_TMR_TypeDef *pTMR, uint32_t iSource);
extern uint32_t GptBsy(ADI_TMR_TypeDef *pTMR);
extern void Gpt32Setup(ADI_TIMER_TypeDef *pTMR, GPTH_SETUP_t *pSetup);
extern void GptGo(ADI_TMR_TypeDef *pTMR);
extern void GptStop(ADI_TMR_TypeDef *pTMR);
void GptClkCfg(uint32_t gpt0ClkMux, uint32_t gpt1ClkMux, uint32_t gpt2ClkMux, uint32_t gpt32ClkMux);

#ifdef __cplusplus
}
#endif

#endif //#GPT_LIB_H
