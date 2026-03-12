/*!
 *****************************************************************************
 * @file:  DioLib.h
 * @brief: header of Digital to Analog Voltage convertor
 * @version: V0.2
 * @date:    June 2021
 * @par:     Revision History:
 * - V0.1, March 2021: Initial version.
 * - V0.2, June 2021: Fixed #defines used with DioPwrCfgPin().
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#ifndef DIO_LIB_H
#define DIO_LIB_H

#include "adi_processor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PIN0 (1u << 0)
#define PIN1 (1u << 1)
#define PIN2 (1u << 2)
#define PIN3 (1u << 3)
#define PIN4 (1u << 4)
#define PIN5 (1u << 5)
#define PIN6 (1u << 6)
#define PIN7 (1u << 7)

// GPIO Pin mux configuration
#define P0_0_GPIO      0
#define P0_0_SPI0_SCLK 1
#define P0_0_COMP0_OUT 2
#define P0_0_PLA0_IN0  3
#define P0_1_GPIO      0
#define P0_1_SPI0_MISO 1
#define P0_1_COMP1_OUT 2
#define P0_1_PLA1_IN   3
#define P0_2_GPIO      0
#define P0_2_SPI0_MOSI 1
#define P0_2_PLA1_CLK  2
#define P0_2_PLA2_IN   3
#define P0_3_GPIO_IRQ0 0
#define P0_3_SPI0_CS   1
#define P0_3_COMPD1_IN 2
#define P0_3_PLA3_IN   3
#define P0_4_GPIO      0
#define P0_4_I2C0_SCL  1
#define P0_4_UART0_RX  2
#define P0_4_PLA4_IN   3
#define P0_5_GPIO      0
#define P0_5_I2C0_SDA  1
#define P0_5_UART0_TX  2
#define P0_5_PLA5_IN   3
#define P0_6_GPIO_IRQ3 0
#define P0_6_I2C1_SCL  1
#define P0_6_PWM5      2
#define P0_6_PLA4_OUT  3
#define P0_7_GPIO_IRQ4 0
#define P0_7_I2C1_SDA  1
#define P0_7_PWM4      2
#define P0_7_PLA5_OUT  3
#define P1_0_GPIO      0
#define P1_0_I2C1_SCL  1
#define P1_0_COMP2_OUT 2
#define P1_0_PLA2_OUT  3
#define P1_1_GPIO      0
#define P1_1_I2C1_SDA  1
#define P1_1_COMP3_OUT 2
#define P1_1_PLA3_OUT  3
#define P1_2_GPIO      0
#define P1_2_I2C1_SCL  1
#define P1_2_PWM0      2
#define P1_2_PLA6_IN   3
#define P1_3_GPIO      0
#define P1_3_I2C1_SDA  1
#define P1_3_PWM2      2
#define P1_3_PLA7_IN   3
#define P1_4_GPIO      0
#define P1_4_SPI1_SCLK 1
#define P1_4_PWM1      2
#define P1_4_PLA10_OUT 3
#define P1_5_GPIO      0
#define P1_5_SPI1_MISO 1
#define P1_5_PWM3      2
#define P1_5_PLA11_OUT 3
#define P1_6_GPIO      0
#define P1_6_SPI1_MOSI 1
#define P1_6_COMPD3_IN 2
#define P1_6_PLA12_OUT 3
#define P1_7_GPIO_IRQ1 0
#define P1_7_SPI1_CS   1
#define P1_7_COMPD2_IN 2
#define P1_7_PLA13_OUT 3
#define P2_0_GPIO_IRQ2 0
#define P2_0_ADCCONV   1
#define P2_0_MCI       2
#define P2_0_PLA8_IN   3
#define P2_1_GPIO_DM   0
#define P2_1_POR       1
#define P2_1_CLKOUT    2
#define P2_2_GPIO      0
#define P2_2_ECLKIN    1
#define P2_2_PLA0_CLK  2
#define P2_2_PLA9_IN   3
#define P2_3_GPIO_BM   0
#define P2_3_PLA10_IN  3
#define P2_4_GPIO      0
#define P2_4_SPI1_MOSI 1
#define P2_4_PLA18_OUT 3
#define P2_5_GPIO      0
#define P2_5_SPI1_MISO 1
#define P2_5_PLA19_OUT 3
#define P2_6_GPIO_IRQ5 0
#define P2_6_SPI1_SCLK 1
#define P2_6_PLA20_OUT 3
#define P2_7_GPIO_IRQ6 0
#define P2_7_SPI1_CS   1
#define P2_7_COMPD0_IN 2
#define P2_7_PLA21_OUT 3
#define P3_0_GPIO_IRQ8 0
#define P3_0_PRTADDR0  1
#define P3_0_SRDY0     2
#define P3_1_GPIO      0
#define P3_1_PRTADDR1  1
#define P3_1_PWMSYNC   2
#define P3_2_GPIO      0
#define P3_2_PRTADDR2  1
#define P3_2_PWMTRIP   2
#define P3_2_MCO       3
#define P3_3_GPIO      0
#define P3_3_PRTADDR3  1
#define P3_3_UART0_RX  2
#define P3_4_GPIO_IRQ9 0
#define P3_4_PRTADDR4  1
#define P3_4_UART0_TX  2
#define P3_4_PLA26_OUT 3
#define P3_5_GPIO      0
#define P3_5_MCK       1
#define P3_5_SRDY1     2
#define P3_5_PLA27_OUT 3
#define P3_6_GPIO      0
#define P3_6_MDIO      1
#define P3_6_PLA28_OUT 3
#define P3_7_GPIO      0
#define P3_7_PLA29_OUT 3
#define P4_0_GPIO      0
#define P4_0_IDACDIS   1
#define P4_1_GPIO      0
#define P4_1_PWM6      1
#define P4_2_GPIO      0
#define P4_2_PLA30_OUT 3
#define P4_3_GPIO_IRQ7 0
#define P4_3_AIN10     1
#define P4_4_GPIO      0
#define P4_4_AIN11     1
#define P4_5_GPIO      0
#define P4_5_AIN12     1
#define P4_6_GPIO      0
#define P4_6_AIN13     1
#define P4_7_GPIO      0
#define P4_7_AIN14     1
#define P5_0_GPIO      0
#define P5_0_VDAC0     1
#define P5_1_GPIO      0
#define P5_1_VDAC1     1
#define P5_2_GPIO      0
#define P5_2_VDAC2     1
#define P5_3_GPIO      0
#define P5_3_VDAC3     1
#define P5_4_GPIO      0
#define P5_4_VDAC4     1
#define P5_5_GPIO      0
#define P5_5_VDAC5     1
#define P5_6_GPIO      0
#define P5_6_VDAC6     1
#define P5_7_GPIO      0
#define P5_7_VDAC7     1
#define P6_0_GPIO      0
#define P6_0_VDAC8     1
#define P6_1_GPIO      0
#define P6_1_AIN15     1
#define P6_2_GPIO      0
#define P6_2_SWCLK     1
#define P6_3_GPIO      0
#define P6_3_SWDIO     1

#if 0 // not used in s2
#define P5_0_FULLMUX0 1
#define P5_1_FULLMUX1 1
#define P5_2_FULLMUX2 1
#define P5_3_FULLMUX3 1
#define P5_4_FULLMUX4 1
#define P5_5_FULLMUX5 1
#define P5_6_FULLMUX6 1
#endif

#define GPIO_PWR_1V2      0
#define GPIO_PWR_1V8      1
#define GPIO_PWR_3V3      2
#define GPIO_PULLSEL_UP   1
#define GPIO_PULLSEL_DOWN 0

// =================================== Function API =====================
void DioCfg(ADI_GPIO_TypeDef *pPort, uint16_t Mpx);
void DioCfgPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Mode);
void DioOen(ADI_GPIO_TypeDef *pPort, uint8_t Oen);
void DioOenPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Oen);
void DioPul(ADI_GPIO_TypeDef *pPort, uint8_t Pul);
void DioPulPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Pul);
void DioIen(ADI_GPIO_TypeDef *pPort, uint8_t Ien);
void DioIenPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Ien);
void DioSet(ADI_GPIO_TypeDef *pPort, uint8_t PinMsk);
void DioClr(ADI_GPIO_TypeDef *pPort, uint8_t PinMsk);
void DioTgl(ADI_GPIO_TypeDef *pPort, uint8_t PinMsk);
void DioDsPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Ds);
void DioOpenDrainPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t En);
void DioPwrCfgPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Opt);
void DioWr(ADI_GPIO_TypeDef *pPort, uint8_t Val);
uint8_t DioRd(ADI_GPIO_TypeDef *pPort);

#ifdef __cplusplus
}
#endif

#endif // __DIO_H__
