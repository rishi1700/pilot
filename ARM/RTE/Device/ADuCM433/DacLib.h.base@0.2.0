/*!
 *****************************************************************************
 * @file:  DACLib.h
 * @brief: header of Digital to Analog Voltage convertor
 * @version: V0.2
 * @date:    May 2022
 * @par:     Revision History:
 * - V0.1, March 2021: Initial version.
 * - V0.2, May 2021: API change for IDacCfg().
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/
#ifndef DAC_LIB_H
#define DAC_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "adi_processor.h"

typedef struct {
    bool pd;
    bool scale;
    bool outmux;
    bool pdN;
    bool scaleN;
} VDAC_CHAN_SETUP_t;

typedef struct {
    bool pd;
    bool clear;
    bool thermalShutDownEn;
    uint8_t range;
} IDAC_CHAN_SETUP_t;

//------------------------------ Function prototypes ------------------------------------------

extern uint32_t VDacCfg(uint32_t index, uint32_t fullScale, uint32_t outMux, uint32_t negGain);
extern uint32_t IDacCfg(uint32_t index, uint32_t fullScale, uint32_t res, uint32_t clrBit);

extern uint32_t VDacWr(uint32_t index, uint32_t data);
extern uint32_t VDacWrAutoSync(uint32_t index, uint32_t data);
extern uint32_t VDacSync(uint32_t index);

extern uint32_t IDacWr(uint32_t index, uint32_t data);
extern uint32_t IDacWrAutoSync(uint32_t index, uint32_t data);
extern uint32_t IDacSync(uint32_t index);
extern uint32_t IDacPdCh(uint32_t index);
extern uint32_t IDacEnCh(uint32_t index);

extern uint32_t VDacImonEn(uint16_t PosImonEn, uint16_t NegImonEn);

#ifdef __cplusplus
}
#endif

#endif //#DAC_LIB_H
