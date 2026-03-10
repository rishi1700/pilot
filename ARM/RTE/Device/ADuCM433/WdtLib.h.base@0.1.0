/*!
 *****************************************************************************
 * @file:   WdtLib.h
 * @brief:  header file of watch dog timer
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
#ifndef WDT_LIB_H
#define WDT_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "adi_processor.h"
#include <stdint.h>

#define WDT_REFRESH_VALUE 0xCCCC

//------------------------------ Function prototypes ------------------------------------------

extern uint32_t WdtLd(uint16_t Tld);
extern uint32_t WdtVal(void);
extern uint32_t WdtCfg(uint32_t Mod, uint32_t Pre, uint32_t Int, uint32_t Pd);
extern uint32_t WdtClkCfg(uint32_t div2En, uint32_t pre);
extern uint32_t WdtGo(uint32_t enable);
extern uint32_t WdtRefresh(void);
extern uint32_t WdtSta(void);
extern uint32_t WdtWindowCfg(uint32_t minLd, uint32_t enable);

#ifdef __cplusplus
}
#endif

#endif //#WDT_LIB_H
