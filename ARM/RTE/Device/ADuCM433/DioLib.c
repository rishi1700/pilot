/*!
 *****************************************************************************
 * @file:    DioLib.c
 * @brief:   source file of digital I/O.
 * @version: V0.3
 * @date:    April 2022
 * @par:     Revision History:
 * - V0.1, March 2021: Initial version.
 * - V0.2, June 2021: Fixed bug with DioPwrCfgPin().
 * - V0.3, April 2022: Cleanup Doxygen Docs
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2010-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#include "DioLib.h"

/**
    @brief void DioCfgPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Mode)
        ======== Configures the mode of 1 GPIO of the specified port.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param PinMsk :{PIN0|PIN1|PIN2|PIN3|PIN4|PIN5|PIN6|PIN7}
        - PIN0 to configure Px.0.
        - PIN1 to configure Px.1.
        - PIN2 to configure Px.2.
        - PIN3 to configure Px.3.
        - PIN4 to configure Px.4.
        - PIN5 to configure Px.5.
        - PIN6 to configure Px.6.
        - PIN7 to configure Px.7
        use combination of above pins
    @param Mode :{0, 1, 2, 3}
        - Set the mode accoring to the multiplex options required.
**/

void DioCfgPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Mode)
{
    uint32_t reg;
    uint32_t bitPos = 0;
    uint32_t checkMsk = 1;
    reg = pPort->CON;
    for (bitPos = 0; bitPos < 16; bitPos++) {
        if (PinMsk & checkMsk) {
            reg &= ~(3u << (bitPos << 1)); // two bits of CFG register for each pin
            reg |= (Mode << (bitPos << 1));
        }
        checkMsk = checkMsk << 1;
    }
    pPort->CON = (uint16_t)reg;
}

/**
    @brief void DioCfg(ADI_GPIO_TypeDef *pPort, uint16_t iMpx)
        ======== Sets Digital IO port multiplexer.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param iMpx :{0-0xFFFF}
        - Set iMpx accoring to the multiplex options required.
**/

inline void DioCfg(ADI_GPIO_TypeDef *pPort, uint16_t iMpx) { pPort->CON = iMpx; }

/**
    @brief void DioOen(ADI_GPIO_TypeDef *pPort, uint8_t Oen)
        ======== Enables the output drive of port pins.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param Oen :{0-0xFF}
        - Select combination of BIT0 to BIT7 outputs to connect to pin e.g.
        - 0, none of the pins are configured as outputs.
        - BITX|BITY, only Pin X and Pin Y are configured as outputs on the specified port.
**/

inline void DioOen(ADI_GPIO_TypeDef *pPort, uint8_t Oen) { pPort->OE = Oen; }

/**
    @brief void DioOenPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Oen)
        ======== Enables the output drive of 1 GPIO of the specified port.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param PinMsk :{PIN0|PIN1|PIN2|PIN3|PIN4|PIN5|PIN6|PIN7}
    @param Oen :{0, 1}
        - 0 to disable the output drive
        - 1 to enable the output drive
**/
void DioOenPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Oen)
{
    uint8_t reg = pPort->OE;
    reg &= ~PinMsk;
    if (Oen)
        reg |= PinMsk;
    pPort->OE = reg;
}

/**
    @brief void DioPul(ADI_GPIO_TypeDef *pPort, uint8_t Pul)
        ======== Sets the pull-up/ pull-down resistor of port pins.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param Pul :{0-0xFF}
        - Select combination of BIT0 to BIT7 to enable the pull ups of pins e.g.
        - 0, all pull-ups/ pull-downs are disabled.
        - BITX|BITY, all pull ups are disabled except on Pin X and Pin Y of the specified port.
    @note GP0, GP1, GP2 and GP3 have pull-ups, GP4 and GP5 have pull-downs
**/

inline void DioPul(ADI_GPIO_TypeDef *pPort, uint8_t Pul) { pPort->PE = Pul; }

/**
    @brief void DioPulPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Pul)
        ======== Configures the pull-up of 1 GPIO of the specified port.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param PinMsk :{PIN0|PIN1|PIN2|PIN3|PIN4|PIN5|PIN6|PIN7}
    @param Pul :{0, 1}
        - 0 to disable the pull-up
        - 1 to enable the pull-up
**/
void DioPulPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Pul)
{
    uint8_t reg = pPort->PE;
    reg &= ~PinMsk;
    if (Pul)
        reg |= PinMsk;
    pPort->PE = reg;
}

/**
    @brief void DioIen(ADI_GPIO_TypeDef *pPort, uint8_t Ien)
        ======== Enables the input path of port pins.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param Ien :{0-0xFF}
        - Select combination of BIT0 to BIT7 inputs to connect to pin e.g.
        - 0, none of the pins are configured as inputs.
        - BITX|BITY, only Pin X and Pin Y are configured as inputs on the specified port.
**/

void DioIen(ADI_GPIO_TypeDef *pPort, uint8_t Ien) { pPort->IE = Ien; }

/**
    @brief void DioIenPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Ien)
        ======== Enables the input path of 1 GPIO of the specified port.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param PinMsk :{PIN0|PIN1|PIN2|PIN3|PIN4|PIN5|PIN6|PIN7}
    @param Ien :{0, 1}
        - 0 to disable the input path
        - 1 to enable the input path
**/

void DioIenPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Ien)
{
    uint8_t reg = pPort->IE;
    reg &= ~PinMsk;
    if (Ien)
        reg |= PinMsk;
    pPort->IE = reg;
}

/**
    @brief uint8_t DioRd(ADI_GPIO_TypeDef *pPort)
        ======== Reads values of port pins.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @return pPort->IN
**/
inline uint8_t DioRd(ADI_GPIO_TypeDef *pPort) { return (pPort->IN); }

/**
    @brief void DioWr(ADI_GPIO_TypeDef *pPort, uint8_t Val)
        ======== Writes values to outputs.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param Val :{0-0xFF}
**/
inline void DioWr(ADI_GPIO_TypeDef *pPort, uint8_t Val) { pPort->OUT = Val; }

/**
    @brief void DioSet(ADI_GPIO_TypeDef *pPort, uint8_t PinMsk)
        ======== Sets individual outputs.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param PinMsk :{PIN0|PIN1|PIN2|PIN3|PIN4|PIN5|PIN6|PIN7}
**/
inline void DioSet(ADI_GPIO_TypeDef *pPort, uint8_t PinMsk) { pPort->SET = PinMsk; }

/**
    @brief void DioClr(ADI_GPIO_TypeDef *pPort, uint8_t PinMsk)
        ======== Clears individual outputs.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param PinMsk :{PIN0|PIN1|PIN2|PIN3|PIN4|PIN5|PIN6|PIN7}
**/

inline void DioClr(ADI_GPIO_TypeDef *pPort, uint8_t PinMsk) { pPort->CLR = PinMsk; }

/**
    @brief void DioTgl(ADI_GPIO_TypeDef *pPort, uint8_t PinMsk)
        ======== Toggles individual outputs.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param PinMsk :{PIN0|PIN1|PIN2|PIN3|PIN4|PIN5|PIN6|PIN7}
**/
inline void DioTgl(ADI_GPIO_TypeDef *pPort, uint8_t PinMsk) { pPort->TGL = PinMsk; }

/**
    @brief void DioDsPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Ds)
        ======== Controls the drive strength of 1 GPIO of the specified port.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param PinMsk :{PIN0|PIN1|PIN2|PIN3|PIN4|PIN5|PIN6|PIN7}
    @param Ds :{0,1,2,3}
        - 0, Drive Strength 1
        - 1, Drive Strength 2
        - 2, Drive Strength 3
        - 3, Drive Strength 4
**/
void DioDsPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Ds)
{
    uint32_t reg;
    uint32_t bitPos = 0;
    uint32_t checkMsk = 1;
    reg = pPort->DS;
    for (bitPos = 0; bitPos < 16; bitPos++) {
        if (PinMsk & checkMsk) {
            reg &= ~(3u << (bitPos << 1)); // two bits of CFG register for each pin
            reg |= (Ds << (bitPos << 1));
        }
        checkMsk = checkMsk << 1;
    }
    pPort->DS = (uint16_t)reg;
}

/**
    @brief void DioOpenDrainPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t En)
        ======== Enables/Disable open drain function of the specified Pin.
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param PinMsk :{PIN0|PIN1|PIN2|PIN3|PIN4|PIN5|PIN6|PIN7}
    @param En :{0, 1}
        - 0 to disable open drain
        - 1 to enable open drain
**/
void DioOpenDrainPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t En)
{
    uint8_t reg = pPort->ODE;
    reg &= ~PinMsk;
    if (En)
        reg |= PinMsk;
    pPort->ODE = reg;
}

/**
    @brief void DioPwrCfgPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Opt)
        ======== select CMOS or Schmitt input for specified pin
    @param pPort :{pADI_GPIO0,pADI_GPIO1,pADI_GPIO2,pADI_GPIO3,
                    pADI_GPIO4,pADI_GPIO5,pADI_GPIO6}
    @param PinMsk :{PIN0|PIN1|PIN2|PIN3|PIN4|PIN5|PIN6|PIN7}
    @param Opt :{GPIO_PWR_1V2, GPIO_PWR_1V8, GPIO_PWR_3V3}
        - 0 for GPIO_PWR_1V2
        - 1 for GPIO_PWR_1V8
        - 2,3 for GPIO_PWR_3V3
**/
void DioPwrCfgPin(ADI_GPIO_TypeDef *pPort, uint32_t PinMsk, uint32_t Opt)
{
    uint16_t reg;

    uint32_t bitPos = 0;
    uint32_t checkMsk = 1;

    reg = pPort->PWR;

    for (bitPos = 0; bitPos < 16; bitPos++) {
        if (PinMsk & checkMsk) {
            reg &= ~(3u << (bitPos << 1)); // two bits of CFG register for each pin
            reg |= (Opt << (bitPos << 1));
        }
        checkMsk = checkMsk << 1;
    }

    pPort->PWR = reg;
}
