/**************************************************************************/
/**
 * @file     system_ADuCM430.c
 * @brief    CMSIS Device System Source File for
 *           ADuCM430 Device
 * @version  V1.1.0
 * @date     01 June 2022
 ******************************************************************************/
/*
 * Copyright (c) 2009-2019 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "adi_processor.h"

/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/
#define SYSTEM_CLOCK (80000000UL)

/*----------------------------------------------------------------------------
  Define Block checksum
 *----------------------------------------------------------------------------*/
#if defined(__ICCARM__) || defined(__ARMCC_VERSION)
const uint32_t page0_checksum __attribute__((used, section(".ARM.__at_0x00000FFC"))) = 0xFFFFFFFF;
#elif defined(__GNUC__)
__attribute__((__section__(".PAGE0_CHECKSUM"))) const uint32_t page0_checksum = 0xFFFFFFFF;
#endif

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/

// The following definitions are available in IAR 8.50 from cmsis_iccarm.h VV5.2.0
// defining them again here for older IAR compilers in case the definitions are not
// already available
#ifdef __ICCARM__

#ifndef __VECTOR_TABLE
#define __VECTOR_TABLE __vector_table
#endif

#ifndef __VECTOR_TABLE_ATTRIBUTE
#define __VECTOR_TABLE_ATTRIBUTE @".intvec"
#endif

#endif

extern const VECTOR_TABLE_Type __VECTOR_TABLE[240];
#define SIZEOF_IVT sizeof(__VECTOR_TABLE)

__ALIGNED(0x200)
#if defined(__ICCARM__) || defined(__ARMCC_VERSION)
VECTOR_TABLE_Type __relocated_vector_table[SIZEOF_IVT / 4] __attribute__((used, section(".bss.IRAM_IVT")));
#elif defined(__SES_ARM)
__attribute__((__section__(".vectors_ram"))) VECTOR_TABLE_Type __relocated_vector_table[SIZEOF_IVT / 4];
#elif defined(__GNUC__)
__attribute__((__section__(".IRAM_IVT"))) VECTOR_TABLE_Type __relocated_vector_table[SIZEOF_IVT / 4];
#endif

/*----------------------------------------------------------------------------
  System Core Clock Variable
 *----------------------------------------------------------------------------*/
uint32_t SystemCoreClock = SYSTEM_CLOCK; /* System Core Clock Frequency */

/*----------------------------------------------------------------------------
  System Core Clock update function
  Function to be updated with code that reads the registers and calculates the
  clock speed
 *----------------------------------------------------------------------------*/
void SystemCoreClockUpdate(void) { SystemCoreClock = SYSTEM_CLOCK; }

/*----------------------------------------------------------------------------
  System initialization function
 *----------------------------------------------------------------------------*/
void SystemInit(void)
{
    uint32_t i;
    uint32_t *pSrc, *pDst;

    /* copy the IVT (avoid use of memcpy here so it does not become locked into flash) */
    for (i = 0, pSrc = (uint32_t *)__VECTOR_TABLE, pDst = (uint32_t *)__relocated_vector_table; i < SIZEOF_IVT / 4; i++)
        *pDst++ = *pSrc++;

    /* relocate vector table */
    __disable_irq();
    SCB->VTOR = (uint32_t)(__relocated_vector_table);
    __DSB();

    /* Enable Cache */
    pADI_CACHE->KEY = 0xF123F456;
    pADI_CACHE->SETUP = 0x10001;
    __NOP();

    __enable_irq();

    // bugfix clear pending FLash DMA Interrupts
    NVIC_ClearPendingIRQ(DMA_FLASH0_IRQn);
    NVIC_ClearPendingIRQ(DMA_FLASH1_IRQn);

    SystemCoreClockUpdate();
}
