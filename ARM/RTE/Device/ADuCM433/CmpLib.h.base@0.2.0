/*!
 *****************************************************************************
 * @file:  CmpLib.h
 * @brief: header of Analog comparator library
 * @version: V0.2
 * @date:    August 2021
 * @par:     Revision History:
 * - V0.1, March 2021: Initial version.
 * - V0.2, August 2021: Added Digital Comparator Functions
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/
#ifndef CMP_LIB_H
#define CMP_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "adi_processor.h"

typedef struct {
    // latchsel
    // 0 select analog comparator to pla & gpio
    // 1 select analog comparator latch signal to pla & gpio
    uint32_t latch_sel;

    // Enable Comparator
    // 0 - disable
    // 1 - enable
    uint32_t cmp_en;

    // Comparator Positive Input Source
    uint32_t cmp_input_pos;

    // Comparator Negative Input Source
    uint32_t cmp_input_neg;

    // Select Output Logic State
    uint32_t cmp_invert;

    // Hysteresis Voltage
    uint32_t cmp_hys;

    // interrupt mode
    uint32_t int_mode;

    uint32_t cmp_out;
} CMP_SETUP_t;

#define DIGCHAN0 0
#define DIGCHAN1 1
#define DIGCHAN2 2
#define DIGCHAN3 3

extern uint32_t CmpHysCfg(uint32_t CmpNum, uint32_t iHysVoltage);
extern uint32_t CmpEnable(uint32_t CmpNum, uint32_t iEn);
extern uint32_t CmpIntCfg(uint32_t CmpNum, uint32_t intMode);
extern uint32_t CmpOutputCfg(uint32_t CmpNum, uint32_t iInvert);
extern uint32_t CmpInputCfg(uint32_t CmpNum, uint32_t iInPos, uint32_t iInNeg);
extern void CmpSetup(const CMP_SETUP_t *pSetup, uint32_t CmpNum);
extern uint32_t OscPd(uint32_t enable);
extern uint32_t XtalEn(uint32_t enable);

// ************************* Digital Comparator Functions *************************************************
extern uint32_t DigCompInputSelect(uint8_t ChanNum, uint32_t AinSelect);
extern uint32_t DigCompThreshSetup(uint8_t ChanNum, uint32_t LowThresh, uint32_t HighThresh);
extern uint32_t DigCompEnable_Disable(uint8_t ChanNum, uint8_t EnableDisable);
extern uint32_t DigCompIrq(uint8_t ChanNum, uint8_t EnableDisable, uint8_t RiseEdgeH_FallEdgeL);
extern uint32_t DigCompToPlaSetup(uint8_t ChanNum, uint8_t IrqOrCompOut);
#ifdef __cplusplus
}
#endif

#endif //#CMP_LIB_H
