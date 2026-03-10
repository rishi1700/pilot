/******************************************************************************
 * @file     startup_ARMCM3.c
 * @brief    CMSIS-Core(M) Device Startup File for a Cortex-M3 Device
 * @version  V2.0.3
 * @date     31. March 2020
 ******************************************************************************/
/*
 * Copyright (c) 2009-2020 Arm Limited. All rights reserved.
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
  External References
 *----------------------------------------------------------------------------*/
extern uint32_t __INITIAL_SP;

extern __NO_RETURN void __PROGRAM_START(void);

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler(void);
void Default_Handler(void);

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
/* Exceptions */
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

void WakeUp_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Ext_Int0_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Ext_Int1_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Ext_Int2_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Ext_Int3_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Ext_Int4_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Ext_Int5_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Ext_Int6_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Ext_Int7_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Ext_Int8_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Ext_Int9_Handler(void) __attribute__((weak, alias("Default_Handler")));
void WDog_Tmr_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void GP_Tmr0_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void GP_Tmr1_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void GP_Tmr2_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void GP_Tmr3_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MDIO_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Flash0_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void Flash1_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UART_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SPI0_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SPI1_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void I2C0_Slave_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void I2C0_Master_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void I2C1_Slave_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void I2C1_Master_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PLA0_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PLA1_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PLA2_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PLA3_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PWMTrip_Int_Handle(void) __attribute__((weak, alias("Default_Handler")));
void PWM0_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PWM1_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PWM2_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PWM3_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SRAM_Err_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_Err_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_SPI0_TX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_SPI0_RX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_SPI1_TX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_SPI1_RX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_UART_TX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_UART_RX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_I2C0_STX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_I2C0_SRX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_I2C0_MTX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_I2C1_STX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_I2C1_SRX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_I2C1_MTX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_MDIO_TX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_MDIO_RX_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_Flash0_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_Flash1_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_ADC_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_TRIG0_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DMA_TRIG1_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PLL_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HFOSC_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void ADC_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SEQ_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DIGCOMP0_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DIGCOMP1_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DIGCOMP2_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DIGCOMP3_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void COMP0_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void COMP1_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void COMP2_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void COMP3_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MCODEC_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void OVERTEMP_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IDAC_RES_LOW_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void TEC_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void GPIO_INTA_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void GPIO_INTB_Int_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UnUsed_Handler(void) __attribute__((weak, alias("Default_Handler")));

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

extern const VECTOR_TABLE_Type __VECTOR_TABLE[240];
const VECTOR_TABLE_Type __VECTOR_TABLE[240] __VECTOR_TABLE_ATTRIBUTE = {
    (VECTOR_TABLE_Type)(&__INITIAL_SP), /*     Initial Stack Pointer */
    Reset_Handler,                      /*     Reset Handler */
    NMI_Handler,                        /* -14 NMI Handler */
    HardFault_Handler,                  /* -13 Hard Fault Handler */
    MemManage_Handler,                  /* -12 MPU Fault Handler */
    BusFault_Handler,                   /* -11 Bus Fault Handler */
    UsageFault_Handler,                 /* -10 Usage Fault Handler */
    0,                                  /*     Reserved */
    0,                                  /*     Reserved */
    0,                                  /*     Reserved */
    0,                                  /*     Reserved */
    SVC_Handler,                        /*  -5 SVCall Handler */
    DebugMon_Handler,                   /*  -4 Debug Monitor Handler */
    0,                                  /*     Reserved */
    PendSV_Handler,                     /*  -2 PendSV Handler */
    SysTick_Handler,                    /*  -1 SysTick Handler */

    /* Interrupts */
    WakeUp_Int_Handler,       /*   Interrupt 0 */
    Ext_Int0_Handler,         /*   Interrupt 1 */
    Ext_Int1_Handler,         /*   Interrupt 2 */
    Ext_Int2_Handler,         /*   Interrupt 3 */
    Ext_Int3_Handler,         /*   Interrupt 4 */
    Ext_Int4_Handler,         /*   Interrupt 5 */
    Ext_Int5_Handler,         /*   Interrupt 6 */
    Ext_Int6_Handler,         /*   Interrupt 7 */
    Ext_Int7_Handler,         /*   Interrupt 8 */
    Ext_Int8_Handler,         /*   Interrupt 9 */
    Ext_Int9_Handler,         /*   Interrupt 10 */
    WDog_Tmr_Int_Handler,     /*   Interrupt 11 */
    GP_Tmr0_Int_Handler,      /*   Interrupt 12 */
    GP_Tmr1_Int_Handler,      /*   Interrupt 13 */
    GP_Tmr2_Int_Handler,      /*   Interrupt 14 */
    GP_Tmr3_Int_Handler,      /*   Interrupt 15 */
    MDIO_Int_Handler,         /*   Interrupt 16 */
    Flash0_Int_Handler,       /*   Interrupt 17 */
    Flash1_Int_Handler,       /*   Interrupt 18 */
    UART_Int_Handler,         /*   Interrupt 19 */
    SPI0_Int_Handler,         /*   Interrupt 20 */
    SPI1_Int_Handler,         /*   Interrupt 21 */
    I2C0_Slave_Int_Handler,   /*   Interrupt 22 */
    I2C0_Master_Int_Handler,  /*   Interrupt 23 */
    I2C1_Slave_Int_Handler,   /*   Interrupt 24 */
    I2C1_Master_Int_Handler,  /*   Interrupt 25 */
    PLA0_Int_Handler,         /*   Interrupt 26 */
    PLA1_Int_Handler,         /*   Interrupt 27 */
    PLA2_Int_Handler,         /*   Interrupt 28 */
    PLA3_Int_Handler,         /*   Interrupt 29 */
    PWMTrip_Int_Handle,       /*   Interrupt 30 */
    PWM0_Int_Handler,         /*   Interrupt 31 */
    PWM1_Int_Handler,         /*   Interrupt 32 */
    PWM2_Int_Handler,         /*   Interrupt 33 */
    PWM3_Int_Handler,         /*   Interrupt 34 */
    SRAM_Err_Int_Handler,     /*   Interrupt 35 */
    DMA_Err_Int_Handler,      /*   Interrupt 36 */
    DMA_SPI0_TX_Int_Handler,  /*   Interrupt 37 */
    DMA_SPI0_RX_Int_Handler,  /*   Interrupt 38 */
    DMA_SPI1_TX_Int_Handler,  /*   Interrupt 39 */
    DMA_SPI1_RX_Int_Handler,  /*   Interrupt 40 */
    DMA_UART_TX_Int_Handler,  /*   Interrupt 41 */
    DMA_UART_RX_Int_Handler,  /*   Interrupt 42 */
    DMA_I2C0_STX_Int_Handler, /*   Interrupt 43 */
    DMA_I2C0_SRX_Int_Handler, /*   Interrupt 44 */
    DMA_I2C0_MTX_Int_Handler, /*   Interrupt 45 */
    DMA_I2C1_STX_Int_Handler, /*   Interrupt 46 */
    DMA_I2C1_SRX_Int_Handler, /*   Interrupt 47 */
    DMA_I2C1_MTX_Int_Handler, /*   Interrupt 48 */
    DMA_MDIO_TX_Int_Handler,  /*   Interrupt 49 */
    DMA_MDIO_RX_Int_Handler,  /*   Interrupt 50 */
    DMA_Flash0_Int_Handler,   /*   Interrupt 51 */
    DMA_Flash1_Int_Handler,   /*   Interrupt 52 */
    DMA_ADC_Int_Handler,      /*   Interrupt 53 */
    DMA_TRIG0_Int_Handler,    /*   Interrupt 54 */
    DMA_TRIG1_Int_Handler,    /*   Interrupt 55 */
    PLL_Int_Handler,          /*   Interrupt 56 */
    HFOSC_Int_Handler,        /*   Interrupt 57 */
    ADC_Int_Handler,          /*   Interrupt 58 */
    SEQ_Int_Handler,          /*   Interrupt 59 */
    DIGCOMP0_Int_Handler,     /*   Interrupt 60 */
    DIGCOMP1_Int_Handler,     /*   Interrupt 61 */
    DIGCOMP2_Int_Handler,     /*   Interrupt 62 */
    DIGCOMP3_Int_Handler,     /*   Interrupt 63 */
    COMP0_Int_Handler,        /*   Interrupt 64 */
    COMP1_Int_Handler,        /*   Interrupt 65 */
    COMP2_Int_Handler,        /*   Interrupt 66 */
    COMP3_Int_Handler,        /*   Interrupt 67 */
    MCODEC_Int_Handler,       /*   Interrupt 68 */
    OVERTEMP_Int_Handler,     /*   Interrupt 69 */
    IDAC_RES_LOW_Int_Handler, /*   Interrupt 70 */
    TEC_Int_Handler,          /*   Interrupt 71 */
    GPIO_INTA_Int_Handler,    /*   Interrupt 72 */
    GPIO_INTB_Int_Handler,    /*   Interrupt 73 */
    0,                        /*   Interrupt 74 */
    0,                        /*   Interrupt 75 */
    0,                        /*   Interrupt 76 */
    0,                        /*   Interrupt 77 */
    0,                        /*   Interrupt 78 */
    UnUsed_Handler            /*   Interrupt 79 */
};

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/*----------------------------------------------------------------------------
  Reset Handler called on controller reset
 *----------------------------------------------------------------------------*/
__NO_RETURN void Reset_Handler(void)
{
    /* reset the main SP to clean up any leftovers from the boot rom. */
    __set_MSP((uint32_t)__VECTOR_TABLE[0]);

    SystemInit();      /* CMSIS System Initialization */
    __PROGRAM_START(); /* Enter PreMain (C library entry point) */
    while (1)
        ;
}

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif

/*----------------------------------------------------------------------------
  Hard Fault Handler
 *----------------------------------------------------------------------------*/
void HardFault_Handler(void)
{
    while (1)
        ;
}

/*----------------------------------------------------------------------------
  Default Handler for Exceptions / Interrupts
 *----------------------------------------------------------------------------*/
void Default_Handler(void)
{
    while (1)
        ;
}

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic pop
#endif
