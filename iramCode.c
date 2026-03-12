/*!
 *****************************************************************************
 * @file:
 * @brief:
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#include "iramCode.h"

uint32_t ADCIRQSTA = 0;
extern uint8_t ucFirstIRQ;
extern volatile uint32_t dataCnt;
extern uint32_t AdcData[20];
extern uint8_t ucPLLLoss; // flag to indicate loss of PLL Lock error
uint32_t data;

extern uint32_t conversionDone;

void toggleLedIRam() { pADI_GPIO2->TGL = 0x1; }

void ADC_Int_Handler()
{
    ADCIRQSTA = pADI_ADC->ADCIRQSTAT;               // must read to clear status
    if (ADCIRQSTA & BITM_ADC_ADCIRQSTAT_CNVIRQSTAT) // conversion triggered interrupt
    {
        ucFirstIRQ = 0;
        data = pADI_ADC->ADCDAT0;
        if (!conversionDone) {
            AdcData[dataCnt] = data;
            dataCnt++;
        }
        if (dataCnt >= 20) {
            conversionDone = 1;
            dataCnt = 0;
            pADI_ADC->ADCCON0 &= ~BITM_ADC_ADCCON0_CONVTYPE; // stop conversions - clear bits 2:0
        }
    }
}

void PLL_Int_Handler()
{
    uint32_t ulPLLSTA = 0;

    ulPLLSTA = pADI_CLK->CLKSTAT0;
    if ((ulPLLSTA & BITM_CLOCK_CLKSTAT0_SPLLUNLOCK) ==
        BITM_CLOCK_CLKSTAT0_SPLLUNLOCK) // PLL loss of lock error detected
    {
        // Change CPU clock source to Internal Oscillator
        pADI_CLK->CLKCON0 &= 0xFFFC; // Return to internal oscillator - PLL unstable
        ucPLLLoss = 1;               // Set flag to indicate loss of PLL Lock error
    }
    if ((ulPLLSTA & BITM_CLOCK_CLKSTAT0_SPLLLOCK) == BITM_CLOCK_CLKSTAT0_SPLLLOCK) // PLL  lock detected
    {
        // Change CPU clock source to PLL – PLL is stable
        pADI_CLK->CLKCON0 &= 0xFFFC;
        pADI_CLK->CLKCON0 |= 0x1; // PLL is stable
        ucPLLLoss = 0;            // Set flag to indicate loss of PLL Lock error
    }
    pADI_CLK->CLKSTAT0 |= // Clear PLL Lock/Unlock detection flags
        BITM_CLOCK_CLKSTAT0_SPLLLOCKCLR | BITM_CLOCK_CLKSTAT0_SPLLUNLOCKCLR;
}
