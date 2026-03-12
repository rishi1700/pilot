/*!
 *****************************************************************************
 * @file:    AdcLib.h
 * @brief:   header file of Adc
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
#ifndef ADC_LIB_H
#define ADC_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "adi_processor.h"

#define ENUM_ADC_AIN0_AIN1_DIFF (ENUM_ADC_AIN0 | BITM_ADC_ADCCH_DIFFSINGLE0)
#define ENUM_ADC_AIN2_AIN3_DIFF (ENUM_ADC_AIN2 | BITM_ADC_ADCCH_DIFFSINGLE1)
#define ENUM_ADC_AIN4_AIN5_DIFF (ENUM_ADC_AIN4 | BITM_ADC_ADCCH_DIFFSINGLE2)

#define FAST_CONVERSIONS 1
#define SLOW_CONVERSIONS 0

// extern ADC_SETUP_t gAdcSetup;

// ADC sequencer setup data struct
typedef struct {
    // repeat seqnuece interval
    uint32_t repeatInterval;

    // sequence Interrupt enable
    uint32_t seqIntEn;

} ADC_SEQ_SETUP_t;

extern ADC_SEQ_SETUP_t gAdcSeqSetup;

//------------------------ Function Declaration ---------------------
extern void AdcPowerDown(bool flag);
extern void AdcIntEn(bool flag);
extern void AdcDmaEn(bool flag);

extern void AdcPinExt(uint32_t index);
extern void AdcPinInt(uint16_t uiAdcCh, uint16_t uiVmonMux, uint16_t uiImonMux);

extern void AdcGo(uint32_t flag, uint8_t PinMode);
extern void AdcSeqChan(uint32_t *const pSeqChx, uint32_t num);
extern void AdcSeqGo(void);
extern void AdcSeqRestart(void);
extern void AdcSeqStall(bool flag);

extern uint32_t AdcRd(uint32_t index);

extern void AdcSeqSetup(ADC_SEQ_SETUP_t *pAdcSeqSetup);

extern uint16_t AdcSpeed(uint16_t uiDiv, uint8_t uiFastHSlowL);
extern int AdcOverSampling(uint16_t uiOSR);
extern int AdcCalibrationSource(uint16_t uiDiffSource, uint16_t uSeSource);
extern int AdcGptTrigSetup(uint16_t uiGptSelect, uint16_t uiTimeoutLCountH);
#ifdef __cplusplus
}
#endif

#endif //#ADC_LIB_H
