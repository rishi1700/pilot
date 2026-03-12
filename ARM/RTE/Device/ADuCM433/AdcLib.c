/*!
 *****************************************************************************
 * @file:    AdcLib.c
 * @brief:   source file of Adc
 * @version: V0.2
 * @date:    April 2022
 * @par:     Revision History:
 * - V0.1, March 2021: Initial version.
 * - V0.2, April 2022: Cleanup Doxygen Docs
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#include "AdcLib.h"

/**
    @brief void AdcPowerDown(bool flag)
        ======== power down/up adc
    @param flag{0,1}
        - 0 PowerOn the ADC
        - 1 PowerDown ADC
**/

void AdcPowerDown(bool flag)
{
    if (flag) {
        pADI_ADC->ADCCON1 |= BITM_ADC_ADCCON1_RESTARTADC; // power down the ADC
        pADI_ADC->ADCCON0 |= BITM_ADC_ADCCON0_PDREFBUF    // power down the ADC & reference buffer
                             | BITM_ADC_ADCCON0_PDADC;
    }
    else {
        pADI_ADC->ADCCON1 &= ~BITM_ADC_ADCCON1_RESTARTADC; // Power up the ADC
        pADI_ADC->ADCCON0 &=
            ~(BITM_ADC_ADCCON0_PDADC | BITM_ADC_ADCCON0_PDREFBUF); // Power up the ADC & reference buffer
    }
}

/**
    @brief uint16_t AdcSpeed(uint16_t uiDiv, uint8_t uiFastHSlowL)
        ======== ADC conversion = PCLK0/uiDiv. PCLK0 assumed to be 40MHz.
    @param uiDiv :{20-127}
        - Set uiDiv to desired 40MHz division factor for FAST conversions is uiFastHSlowL is 1
        - Set uiDiv to desired 40MHz division factor for SLOW conversions is uiFastHSlowL is 0
    @param uiFastHSlowL :{0,1}
        - 0 to Configure Internal channels sampling rate.
        - 1 to Configure External channels sampling rate.
    @return pADI_ADC->ADCCNVCEXT or pADI_ADC->ADCCNVCINT depending on uiFastHSlowL

**/

uint16_t AdcSpeed(uint16_t uiDiv, uint8_t uiFastHSlowL)
{
    if (uiFastHSlowL == 1) {
        pADI_ADC->ADCCNVCEXT = uiDiv;
        return pADI_ADC->ADCCNVCEXT;
    }
    else {
        pADI_ADC->ADCCNVCINT = uiDiv;
        return pADI_ADC->ADCCNVCINT;
    }
}
/**
    @brief int AdcOverSampling(uint16_t uiOSR)
        ======== ADC oversampling Setup - ensure uiOSR x Sampling rate < max ADC speed .
    @param uiOSR :{0,1,2,3,4,5}
        - 0 or ENUM_ADC_OSR_NS1 for no oversampling
        - 1 or ENUM_ADC_OSR2 for oversampling rate of x2
        - 2 or ENUM_ADC_OSR4 for oversampling rate of x4
        - 3 or ENUM_ADC_OSR8 for oversampling rate of x8
        - 4 or ENUM_ADC_OSR16 for oversampling rate of x16
        - 5 or ENUM_ADC_OSR32 for oversampling rate of x32
    @return pADI_ADC->ADCCON0
**/

int AdcOverSampling(uint16_t uiOSR)
{
    uint32_t uiTemp = 0;
    uint16_t Osrval;

    Osrval = uiOSR;
    uiTemp = pADI_ADC->ADCCON0;
    switch (Osrval) {
        case ENUM_ADC_OSR_NS1:
            uiTemp &= 0xFFC7; // ADCCON0[5:3] = 000;
            pADI_ADC->ADCCON0 = uiTemp;
            break;

        case ENUM_ADC_OSR2:
            uiTemp &= 0xFFC7; // ADCCON0[5:3] = 001;
            uiTemp |= 0x8;
            pADI_ADC->ADCCON0 = uiTemp;
            break;

        case ENUM_ADC_OSR4:
            uiTemp &= 0xFFC7; // ADCCON0[5:3] = 010;
            uiTemp |= 0x10;
            pADI_ADC->ADCCON0 = uiTemp;
            break;

        case ENUM_ADC_OSR8:
            uiTemp &= 0xFFC7; // ADCCON0[5:3] = 011;
            uiTemp |= 0x18;
            pADI_ADC->ADCCON0 = uiTemp;
            break;

        case ENUM_ADC_OSR16:
            uiTemp &= 0xFFC7; // ADCCON0[5:3] = 100;
            uiTemp |= 0x20;
            pADI_ADC->ADCCON0 = uiTemp;
            break;

        case ENUM_ADC_OSR32:
            uiTemp &= 0xFFC7; // ADCCON0[5:3] = 101;
            uiTemp |= 0x28;
            pADI_ADC->ADCCON0 = uiTemp;
            break;
        default:
            uiTemp &= 0xFFC7; // ADCCON0[5:3] = 000;
            pADI_ADC->ADCCON0 = uiTemp;
            break;
    }
    return pADI_ADC->ADCCON0;
}

/**
    @brief void AdcGo(uint32_t flag, uint8_t PinMode)
        ======== start/stop ADC conversion
    @param flag: {ENUM_ADC_IDLE ,ENUM_ADC_GPIO,
               ENUM_ADC_SINGL,ENUM_ADC_CONT,
               ENUM_ADC_PLA  ,ENUM_ADC_GPT}
        - ENUM_ADC_IDLE  No Conversion
        - ENUM_ADC_GPIO  ADC Controlled by GPIO Pin
        - ENUM_ADC_SINGL Software Single Conversion
        - ENUM_ADC_CONT  Software Continue Conversion
        - ENUM_ADC_PLA   PLA Conversion
        - ENUM_ADC_GPT   GPT Triggered Conversion
    @param PinMode: {ENUM_ADC_PIN_LVL,ENUM_ADC_PIN_EDGE }
**/
void AdcGo(uint32_t flag, uint8_t PinMode)
{
    uint32_t uiTemp = 0;

    uiTemp = pADI_ADC->ADCCON0;
    uiTemp &= ~BITM_ADC_ADCCON0_CONVTYPE;
    uiTemp |= flag | (PinMode << BITP_ADC_ADCCON0_PINMOD);
    pADI_ADC->ADCCON0 = uiTemp;
}

/**
    @brief int AdcGptTrigSetup(uint16_t uiGptSelect, uint16_t uiTimeoutLCountH)
        ======== Configure ADCCON1 to setup General Purpose Timer triggered ADC Conversions
    @param uiGptSelect{0,ENUM_ADC_GPT0,ENUM_ADC_GPT1,ENUM_ADC_GPT2,ENUM_ADC_GPT3}
                   0
                   ENUM_ADC_GPT0 or 1      - Select the Gpt0 Trigger
                   ENUM_ADC_GPT1 or 2      - Select the Gpt1 Trigger
                   ENUM_ADC_GPT2 or 3      - Select the Gpt2 Trigger
                   ENUM_ADC_GPT3 or 4      - Select 32-bit GPT
    @param uiTimeoutLCountH{ENUM_ADC_MD0,ENUM_ADC_MD1}
                       0 or ENUM_ADC_MD0    - Timer timeout based triggers
                       1 or ENUM_ADC_MD1    - Timer count based triggers - GPT0/1/2 only

    @return pADI_ADC->ADCCON1
**/
int AdcGptTrigSetup(uint16_t uiGptSelect, uint16_t uiTimeoutLCountH)
{
    uint32_t uiTemp = 0;

    uiTemp = pADI_ADC->ADCCON1;

    uiTemp &= ~(BITM_ADC_ADCCON1_GPTTRIGEN | BITM_ADC_ADCCON1_GPTTRIGMD); // Mask off ADCCON1[5:1]
    uiTemp |= (uiGptSelect << BITP_ADC_ADCCON1_GPTTRIGEN); // Set GPTEVENTEN bits to select timer for trigger
    uiTemp |= (uiTimeoutLCountH << BITP_ADC_ADCCON1_GPTTRIGMD);
    pADI_ADC->ADCCON1 = uiTemp;

    return uiTemp;
}

/**
    @brief void AdcPinExt(uint32_t config)
        ======== select ADC channel pin when measuring AIN channel in single-ended mode /differenal mode
    @param config
      ENUM_ADC_AIN0
      ENUM_ADC_AIN1
      ENUM_ADC_AIN2
      ENUM_ADC_AIN3
      ENUM_ADC_AIN4
      ENUM_ADC_AIN5
      ENUM_ADC_AIN6
      ENUM_ADC_AIN7
      ENUM_ADC_AIN8
      ENUM_ADC_AIN9
      ENUM_ADC_AIN10
      ENUM_ADC_AIN11
      ENUM_ADC_AIN12
      ENUM_ADC_AIN13
      ENUM_ADC_AIN14
      ENUM_ADC_AIN15
      ENUM_ADC_VTEMP
      ENUM_ADC_AVDD0K
      ENUM_ADC_IOVDDK
      ENUM_ADC_PVDD0K
      ENUM_ADC_PVDD1K
      ENUM_ADC_AIN0_AIN1_DIFF  AIN0 positive AIN1 negative
      ENUM_ADC_AIN2_AIN3_DIFF  AIN2 positive AIN3 negative
      ENUM_ADC_AIN4_AIN5_DIFF  Ain4 positive AIN5 negative
**/
void AdcPinExt(uint32_t config) { pADI_ADC->ADCCH = config; }

/**
    @brief void AdcPinInt(uint16_t uiAdcCh, uint16_t uiVmonMux, uint16_t uiImonMux)
        ======== config ADC Internal channle and monitor mux
    @param uiAdcCh :{ENUM_ADC_MUXOUT,ENUM_ADC_IMONVOUT}
    @param uiVmonMux :{0}
            - ENUM_ADC_VDACV0 << 5
            - ENUM_ADC_VDACV1 << 5
            - ENUM_ADC_VDACV2 << 5
            - ENUM_ADC_VDACV3 << 5
            - ENUM_ADC_VDACV4 << 5
            - ENUM_ADC_VDACV5 << 5
            - ENUM_ADC_VDACV6 << 5
            - ENUM_ADC_VDACV7 << 5
            - ENUM_ADC_VDACV8 << 5
    @param uiImonMux :{ENUM_ADC_IDACI0,ENUM_ADC_IDACI1,ENUM_ADC_IDACI2,ENUM_ADC_IDACI3}
**/
void AdcPinInt(uint16_t uiAdcCh, uint16_t uiVmonMux, uint16_t uiImonMux)
{
    pADI_ADC->ADCCH = uiAdcCh;
    pADI_ADC->ADCMONCHMUX = uiVmonMux | uiImonMux;
    pADI_ADC->ADCCON0 |= BITM_ADC_ADCCON0_MUXBUFEN; // Enable Internal Mux
}

/**
    @brief int AdcCalibrationSource(uint16_t uiDiffSource, uint16_t uSeSource)
        ======== Select Offset/gain calibration values - factory is OTP, self calibration is MMR
    @param uiDiffSource{0,BITM_ADC_ADCCON0_OFGNDIFFEN}
    @param uSeSource{0,BITM_ADC_ADCCON0_OFGNSEEN}
    @return pADI_ADC->ADCCON0
**/
int AdcCalibrationSource(uint16_t uiDiffSource, uint16_t uSeSource)
{
    uint32_t uiTemp = 0;

    uiTemp = pADI_ADC->ADCCON0;

    if ((uiDiffSource & BITM_ADC_ADCCON0_OFGNDIFFEN) == BITM_ADC_ADCCON0_OFGNDIFFEN)
        uiTemp |= BITM_ADC_ADCCON0_OFGNDIFFEN;
    else
        uiTemp &= ~(BITM_ADC_ADCCON0_OFGNDIFFEN);

    if ((uSeSource & BITM_ADC_ADCCON0_OFGNSEEN) == BITM_ADC_ADCCON0_OFGNSEEN)
        uiTemp |= BITM_ADC_ADCCON0_OFGNSEEN;
    else
        uiTemp &= ~(BITM_ADC_ADCCON0_OFGNSEEN);

    pADI_ADC->ADCCON0 = uiTemp;
    return uiTemp;
}

/**
    @brief void AdcIntEn(bool flag)
        ======== enable/disable Adc Conversion Interrupt
    @param flag{0,1}
        - 0 Disable conversion interrupt
        - 1 Enabled converstion interrupt
**/
void AdcIntEn(bool flag)
{
    if (flag)
        pADI_ADC->ADCCON1 |= BITM_ADC_ADCCON1_CNVIRQEN;
    else
        pADI_ADC->ADCCON1 &= ~BITM_ADC_ADCCON1_CNVIRQEN;
}

/**
    @brief void AdcDmaEn(bool flag)
        ======== enable/disable Adc DMA mode
    @param flag{0,1}
        - 0 Disable DMA mode
        - 1 Enabled DMA mode
**/
void AdcDmaEn(bool flag)
{
    if (flag)
        pADI_ADC->ADCCON0 |= BITM_ADC_ADCCON0_CNVDMA;
    else
        pADI_ADC->ADCCON0 &= ~BITM_ADC_ADCCON0_CNVDMA;
}

/**
    @brief uint32_t AdcRd(uint32_t index)
        ======== Adc channel index
    @param index, channel is 0~22
    @return none
**/
uint32_t AdcRd(uint32_t index) { return *(&pADI_ADC->ADCDAT0 + (index % 23)); }

/**
    @brief void AdcSeqChan(uint32_t *const pSeqChx, uint32_t num)
        ======== confiuring sequence channel
    @param pSeqChx: pointer to channel table
    @param num: number of channels to be configured
**/
void AdcSeqChan(uint32_t *const pSeqChx, uint32_t num)
{
    uint32_t seqChMux0, seqChMux1;

    uint32_t *pChx = pSeqChx;
    uint32_t chanVal;

    seqChMux0 = 0;
    seqChMux1 = 0;

    for (int i = 0; i < num; i++) {
        chanVal = *pChx;
        if (chanVal < 16)
            seqChMux0 |= 1 << chanVal;
        else
            seqChMux1 |= 1 << (chanVal - 16);
        pChx++;
    }

    pADI_ADC->ADCSEQCH0 = seqChMux0;
    pADI_ADC->ADCSEQCH1 = seqChMux1;
}

/**
    @brief void AdcSeqSetup(ADC_SEQ_SETUP_t *pAdcSeqSetup)
        ======== Setup ADC
    @param pAdcSeqSetup: pointer to gAdcSeqSetup structure
**/
void AdcSeqSetup(ADC_SEQ_SETUP_t *pAdcSeqSetup)
{
    pADI_ADC->ADCSEQ |= pAdcSeqSetup->seqIntEn << BITP_ADC_ADCSEQ_SEQIRQEN;

    pADI_ADC->ADCSEQC = pAdcSeqSetup->repeatInterval;
}

/**
    @brief void AdcSeqGo(void)
        ======== start/stop ADC Sequence conversion
**/
void AdcSeqGo(void) { pADI_ADC->ADCSEQ |= BITM_ADC_ADCSEQ_SEQEN; }

/**
    @brief void AdcSeqStall(bool flag)
        ======== run/stall ADC Sequence conversion
    @param flag{0,1}
        - 1 Enable Sequencer Stall
        - 0 Disable Sequencer Stall
**/
void AdcSeqStall(bool flag)
{
    if (flag)
        pADI_ADC->ADCSEQ |= BITM_ADC_ADCSEQ_SEQSTL;
    else
        pADI_ADC->ADCSEQ &= ~BITM_ADC_ADCSEQ_SEQSTL;
}

/**
    @brief void AdcSeqRestart(void)
        ======== restart ADC Sequence conversion
**/
void AdcSeqRestart(void) { pADI_ADC->ADCSEQ |= BITM_ADC_ADCSEQ_SEQREN; }
