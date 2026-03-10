/*!
 *****************************************************************************
 * @file:
 * @brief:
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2018 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#ifndef IRAMCODE_H
#define IRAMCODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "adi_processor.h"

void ADC_Int_Handler() __attribute__((section(".IRAM_INTERRUPT")));
void PLL_Int_Handler() __attribute__((section(".IRAM_INTERRUPT")));
void toggleLedIRam() __attribute__((section(".IRAM_CODE")));

#ifdef __cplusplus
}
#endif

#endif /* IRAMCODE_H */
