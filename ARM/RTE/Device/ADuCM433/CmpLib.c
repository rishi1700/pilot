/*!
 *****************************************************************************
 * @file:   CmpLib.c
 * @brief:  source file of Analog & Digital  Comparator
 * @version: V0.3
 * @date:    April 2022
 * @par:     Revision History:
 * - V0.1, March 2021: Initial version.
 * - V0.2, August 2021: Added Digital Comparator Functions
 * - V0.3, April 2022: Cleanup Doxygen Docs
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2021 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#include "CmpLib.h"

/**
    @brief uint32_t CmpHysCfg(uint32_t CmpNum, uint32_t iHysVoltage)
        ======== Sets hysteresis type and hysteresis.
    @param CmpNum :0~3
        The number of each comparator part
    @param iHysVoltage :{0~0x1F}
        -0x00 : hysteresis disabled
        -0x01 : 10mv hysteresis enabled
        -0x02 : 20mv hysteresis
        -0x03 : 30mv hysteresis
            ..  : ...
        -0x14 : 200mv hysteresis
    @return 0.
**/

uint32_t CmpHysCfg(uint32_t CmpNum, uint32_t iHysVoltage)
{
    uint32_t val;
    switch (CmpNum) {
        case 0:
            val = pADI_ANAPLT->PLATCMP_CMP0CON;
            val &= ~BITM_ANAPLT_PLATCMP_CMP0CON_HYS;
            val |= iHysVoltage & BITM_ANAPLT_PLATCMP_CMP0CON_HYS;
            pADI_ANAPLT->PLATCMP_CMP0CON = val;
            break;

        case 1:
            val = pADI_ANAPLT->PLATCMP_CMP1CON;
            val &= ~BITM_ANAPLT_PLATCMP_CMP1CON_HYS;
            val |= iHysVoltage & BITM_ANAPLT_PLATCMP_CMP1CON_HYS;
            pADI_ANAPLT->PLATCMP_CMP1CON = val;
            break;

        case 2:
            val = pADI_ANAPLT->PLATCMP_CMP2CON;
            val &= ~BITM_ANAPLT_PLATCMP_CMP2CON_HYS;
            val |= iHysVoltage & BITM_ANAPLT_PLATCMP_CMP2CON_HYS;
            pADI_ANAPLT->PLATCMP_CMP2CON = val;
            break;

        case 3:
            val = pADI_ANAPLT->PLATCMP_CMP3CON;
            val &= ~BITM_ANAPLT_PLATCMP_CMP3CON_HYS;
            val |= iHysVoltage & BITM_ANAPLT_PLATCMP_CMP3CON_HYS;
            pADI_ANAPLT->PLATCMP_CMP3CON = val;
            break;

        default:
            break;
    }

    return 0;
}

/**
    @brief uint32_t CmpEnable(uint32_t CmpNum, uint32_t iEn)
        ======== Powers up and enables comparator
    @param CmpNum :0~3
        The number of each comparator part
    @param iEn :{0, 1}
        -0 to power off comparator.
        -1 to power up and enable comparator.
    @return 0.
**/

uint32_t CmpEnable(uint32_t CmpNum, uint32_t iEn)
{
    switch (CmpNum) {
        case 0:
            if (iEn)
                pADI_ANAPLT->PLATCMP_CMP0CON |= BITM_ANAPLT_PLATCMP_CMP0CON_ENCMP;
            else
                pADI_ANAPLT->PLATCMP_CMP0CON &= ~BITM_ANAPLT_PLATCMP_CMP0CON_ENCMP;
            break;

        case 1:
            if (iEn)
                pADI_ANAPLT->PLATCMP_CMP1CON |= BITM_ANAPLT_PLATCMP_CMP1CON_ENCMP;
            else
                pADI_ANAPLT->PLATCMP_CMP1CON &= ~BITM_ANAPLT_PLATCMP_CMP1CON_ENCMP;
            break;

        case 2:
            if (iEn)
                pADI_ANAPLT->PLATCMP_CMP2CON |= BITM_ANAPLT_PLATCMP_CMP2CON_ENCMP;
            else
                pADI_ANAPLT->PLATCMP_CMP2CON &= BITM_ANAPLT_PLATCMP_CMP2CON_ENCMP;
            break;

        case 3:
            if (iEn)
                pADI_ANAPLT->PLATCMP_CMP3CON |= BITM_ANAPLT_PLATCMP_CMP3CON_ENCMP;
            else
                pADI_ANAPLT->PLATCMP_CMP3CON &= ~BITM_ANAPLT_PLATCMP_CMP3CON_ENCMP;
            break;

        default:
            break;
    }

    return 0;
}

/**
    @brief uint32_t CmpOutputCfg(uint32_t CmpNum, uint32_t iInvert)
        ======== chooses direction and set output.
    @param CmpNum :0~3
        The number of each comparator part
    @param iInvert :{0, 1}
        - 0 for output high if +ve input above -ve input.
        - 1 for output high if +ve input below -ve input.
    @return 0.
**/
uint32_t CmpOutputCfg(uint32_t CmpNum, uint32_t iInvert)
{
    switch (CmpNum) {
        case 0:
            if (iInvert)
                pADI_ANAPLT->PLATCMP_CMP0CON |= BITM_ANAPLT_PLATCMP_CMP0CON_INV;
            else
                pADI_ANAPLT->PLATCMP_CMP0CON &= ~BITM_ANAPLT_PLATCMP_CMP0CON_INV;
            break;

        case 1:
            if (iInvert)
                pADI_ANAPLT->PLATCMP_CMP1CON |= BITM_ANAPLT_PLATCMP_CMP1CON_INV;
            else
                pADI_ANAPLT->PLATCMP_CMP1CON &= ~BITM_ANAPLT_PLATCMP_CMP1CON_INV;
            break;

        case 2:
            if (iInvert)
                pADI_ANAPLT->PLATCMP_CMP2CON |= BITM_ANAPLT_PLATCMP_CMP2CON_INV;
            else
                pADI_ANAPLT->PLATCMP_CMP2CON &= ~BITM_ANAPLT_PLATCMP_CMP2CON_INV;
            break;

        case 3:
            if (iInvert)
                pADI_ANAPLT->PLATCMP_CMP3CON |= BITM_ANAPLT_PLATCMP_CMP3CON_INV;
            else
                pADI_ANAPLT->PLATCMP_CMP3CON &= ~BITM_ANAPLT_PLATCMP_CMP3CON_INV;
            break;

        default:
            break;
    }

    return 0;
}

/**
    @brief uint32_t CmpInputCfg(uint32_t CmpNum, uint32_t iInPos, uint32_t iInNeg)
        ======== Sets Comparator Positive input and Negtive input.
    @param CmpNum :0~3
        The number of each comparator part
    @param iInPos
        -Select Comparator Positive Input Source
        -0x00 : Enable COMPxP Input
        -0x01 : Enable VREF1P25 from Compx
    @param iInNeg
      -Select Comparator Positive Input Source
        -0x00 : Enable VDAC0 Input
        -0x01 : Enable VDAC1 Input
        -0x02 : Enable VDAC2 Input
        -0x03 : Enable VDAC3 Input
        -0x04 : Enable VDAC4 Input
        -0x05 : Enable VDAC5 Input
        -0x06 : Enable COMPxN Input
        -0x07 : Enable COMPDINxN Input
    @return 0.
**/
uint32_t CmpInputCfg(uint32_t CmpNum, uint32_t iInPos, uint32_t iInNeg)
{
    uint32_t status;
    switch (CmpNum) {
        case 0:
            status = pADI_ANAPLT->PLATCMP_CMP0CON;
            status &= ~BITM_ANAPLT_PLATCMP_CMP0CON_INPOS;
            status |= (BITM_ANAPLT_PLATCMP_CMP0CON_INPOS & (iInPos << BITP_ANAPLT_PLATCMP_CMP0CON_INPOS));
            status &= ~BITM_ANAPLT_PLATCMP_CMP0CON_INNEG;
            status |= (BITM_ANAPLT_PLATCMP_CMP0CON_INNEG & (iInNeg << BITP_ANAPLT_PLATCMP_CMP0CON_INNEG));
            pADI_ANAPLT->PLATCMP_CMP0CON = status;
            break;

        case 1:
            status = pADI_ANAPLT->PLATCMP_CMP1CON;
            status &= ~BITM_ANAPLT_PLATCMP_CMP1CON_INPOS;
            status |= (BITM_ANAPLT_PLATCMP_CMP1CON_INPOS & (iInPos << BITP_ANAPLT_PLATCMP_CMP1CON_INPOS));
            status &= ~BITM_ANAPLT_PLATCMP_CMP1CON_INNEG;
            status |= (BITM_ANAPLT_PLATCMP_CMP1CON_INNEG & (iInNeg << BITP_ANAPLT_PLATCMP_CMP1CON_INNEG));
            pADI_ANAPLT->PLATCMP_CMP1CON = status;
            break;

        case 2:
            status = pADI_ANAPLT->PLATCMP_CMP2CON;
            status &= ~BITM_ANAPLT_PLATCMP_CMP2CON_INPOS;
            status |= (BITM_ANAPLT_PLATCMP_CMP2CON_INPOS & (iInPos << BITP_ANAPLT_PLATCMP_CMP2CON_INPOS));
            status &= ~BITM_ANAPLT_PLATCMP_CMP2CON_INNEG;
            status |= (BITM_ANAPLT_PLATCMP_CMP2CON_INNEG & (iInNeg << BITP_ANAPLT_PLATCMP_CMP2CON_INNEG));
            pADI_ANAPLT->PLATCMP_CMP2CON = status;
            break;

        case 3:
            status = pADI_ANAPLT->PLATCMP_CMP3CON;
            status &= ~BITM_ANAPLT_PLATCMP_CMP3CON_INPOS;
            status |= (BITM_ANAPLT_PLATCMP_CMP3CON_INPOS & (iInPos << BITP_ANAPLT_PLATCMP_CMP3CON_INPOS));
            status &= ~BITM_ANAPLT_PLATCMP_CMP3CON_INNEG;
            status |= (BITM_ANAPLT_PLATCMP_CMP3CON_INNEG & (iInNeg << BITP_ANAPLT_PLATCMP_CMP3CON_INNEG));
            pADI_ANAPLT->PLATCMP_CMP3CON = status;
            break;

        default:
            break;
    }

    return 0;
}

uint32_t CmpIntCfg(uint32_t CmpNum, uint32_t intMode)
{
    uint32_t status;
    switch (CmpNum) {
        case 0:
            status = pADI_ANAPLT->PLATCMP_COMPIRQMODE;
            status &= ~BITM_ANAPLT_PLATCMP_COMPIRQMODE_COMP0;
            status |= intMode << BITP_ANAPLT_PLATCMP_COMPIRQMODE_COMP0;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE = status;

        case 1:
            status = pADI_ANAPLT->PLATCMP_COMPIRQMODE;
            status &= ~BITM_ANAPLT_PLATCMP_COMPIRQMODE_COMP1;
            status |= intMode << BITP_ANAPLT_PLATCMP_COMPIRQMODE_COMP1;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE = status;

        case 2:
            status = pADI_ANAPLT->PLATCMP_COMPIRQMODE;
            status &= ~BITM_ANAPLT_PLATCMP_COMPIRQMODE_COMP2;
            status |= intMode << BITP_ANAPLT_PLATCMP_COMPIRQMODE_COMP2;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE = status;

        case 3:
            status = pADI_ANAPLT->PLATCMP_COMPIRQMODE;
            status &= ~BITM_ANAPLT_PLATCMP_COMPIRQMODE_COMP3;
            status |= intMode << BITP_ANAPLT_PLATCMP_COMPIRQMODE_COMP3;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE = status;

        default:
            break;
    }

    return 0;
}

/**
    @brief void CmpSetup(const CMP_SETUP_t *pSetup, uint32_t CmpNum)
        ======== = write value of Comparator setup structure to device
    @param pSetup: pointer to Comparator setup structure
    @param CmpNum{1-3}
        specify which comparator to enable
**/
void CmpSetup(const CMP_SETUP_t *pSetup, uint32_t CmpNum)
{
    uint32_t val;
    uint32_t mode;

    switch (CmpNum) {
        case 0:
            val = pSetup->cmp_en << BITP_ANAPLT_PLATCMP_CMP0CON_ENCMP |
                  pSetup->cmp_input_pos << BITP_ANAPLT_PLATCMP_CMP0CON_INPOS |
                  pSetup->cmp_input_neg << BITP_ANAPLT_PLATCMP_CMP0CON_INNEG |
                  pSetup->cmp_invert << BITP_ANAPLT_PLATCMP_CMP0CON_INV |
                  pSetup->cmp_hys << BITP_ANAPLT_PLATCMP_CMP0CON_HYS | 1 << 14;
            pADI_ANAPLT->PLATCMP_CMP0CON = val;

            mode = pSetup->int_mode << BITP_ANAPLT_PLATCMP_COMPIRQMODE_COMP0;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE &= BITM_ANAPLT_PLATCMP_COMPIRQMODE_COMP0;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE |= mode;
            break;

        case 1:
            val = pSetup->cmp_en << BITP_ANAPLT_PLATCMP_CMP1CON_ENCMP |
                  pSetup->cmp_input_pos << BITP_ANAPLT_PLATCMP_CMP1CON_INPOS |
                  pSetup->cmp_input_neg << BITP_ANAPLT_PLATCMP_CMP1CON_INNEG |
                  pSetup->cmp_invert << BITP_ANAPLT_PLATCMP_CMP1CON_INV |
                  pSetup->cmp_hys << BITP_ANAPLT_PLATCMP_CMP1CON_HYS;
            pADI_ANAPLT->PLATCMP_CMP1CON = val;

            mode = pSetup->int_mode << BITP_ANAPLT_PLATCMP_COMPIRQMODE_COMP1;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE &= BITM_ANAPLT_PLATCMP_COMPIRQMODE_COMP1;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE |= mode;
            break;

        case 2:
            val = pSetup->cmp_en << BITP_ANAPLT_PLATCMP_CMP2CON_ENCMP |
                  pSetup->cmp_input_pos << BITP_ANAPLT_PLATCMP_CMP2CON_INPOS |
                  pSetup->cmp_input_neg << BITP_ANAPLT_PLATCMP_CMP2CON_INNEG |
                  pSetup->cmp_invert << BITP_ANAPLT_PLATCMP_CMP2CON_INV |
                  pSetup->cmp_hys << BITP_ANAPLT_PLATCMP_CMP2CON_HYS;
            pADI_ANAPLT->PLATCMP_CMP2CON = val;

            mode = pSetup->int_mode << BITP_ANAPLT_PLATCMP_COMPIRQMODE_COMP2;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE &= BITM_ANAPLT_PLATCMP_COMPIRQMODE_COMP2;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE |= mode;
            break;

        case 3:
            val = pSetup->cmp_en << BITP_ANAPLT_PLATCMP_CMP3CON_ENCMP |
                  pSetup->cmp_input_pos << BITP_ANAPLT_PLATCMP_CMP3CON_INPOS |
                  pSetup->cmp_input_neg << BITP_ANAPLT_PLATCMP_CMP3CON_INNEG |
                  pSetup->cmp_invert << BITP_ANAPLT_PLATCMP_CMP3CON_INV |
                  pSetup->cmp_hys << BITP_ANAPLT_PLATCMP_CMP3CON_HYS;
            pADI_ANAPLT->PLATCMP_CMP3CON = val;

            mode = pSetup->int_mode << BITP_ANAPLT_PLATCMP_COMPIRQMODE_COMP3;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE &= BITM_ANAPLT_PLATCMP_COMPIRQMODE_COMP3;
            pADI_ANAPLT->PLATCMP_COMPIRQMODE |= mode;
            break;

        default:
            break;
    }
}

/**
    @brief uint32_t OscPd(uint32_t enable)
        ========  Power down Osc on chip  need user key protect
    @param enable
        - 0 for power down.
        - 1 for disable power down.
    @return 0
**/
uint32_t OscPd(uint32_t enable)
{
    pADI_MISC->USERKEY = 0x9FE5;

    if (enable)
        pADI_ANAPLT->PLATCMP_PLATPDCON &= ~BITM_ANAPLT_PLATCMP_PLATPDCON_HPOSCPD;
    else
        pADI_ANAPLT->PLATCMP_PLATPDCON |= BITM_ANAPLT_PLATCMP_PLATPDCON_HPOSCPD;
    return 0;
}

/**
    @brief uint32_t XtalEn(uint32_t enable)
        ========  Enable XTAL need user key protect
    @param enable
        - 0 to disable XTAL
        - 1 to enable XTAL.
    @return 0
**/
uint32_t XtalEn(uint32_t enable)
{
    pADI_MISC->USERKEY = 0x9FE5;

    if (enable)
        pADI_ANAPLT->PLATCMP_PLATPDCON |= BITM_ANAPLT_PLATCMP_PLATPDCON_XTALEN;
    else
        pADI_ANAPLT->PLATCMP_PLATPDCON &= ~(BITM_ANAPLT_PLATCMP_PLATPDCON_XTALEN);

    return 0;
}

// ************************* Digital Comparator Functions *************************************************
/**
    @brief uint32_t DigCompInputSelect(uint8_t ChanNum, uint32_t AinSelect)
        ========  Selects AINx ADC input for Digital Comparator
    @param ChanNum
                   DIGCHAN0 or 0,      - Setup Digital Comparator 0
                   DIGCHAN1 or 1,      - Setup Digital Comparator 1
                   DIGCHAN2 or 2,      - Setup Digital Comparator 2
                   DIGCHAN3 or 3,      - Setup Digital Comparator 3

    @param AinSelect
                  ENUM_ADC_AIN0,   (0X00000000U)   AIN0
                  ENUM_ADC_AIN1,   (0X00000001U)   AIN1
                  ENUM_ADC_AIN2,   (0X00000002U)   AIN2
                  ENUM_ADC_AIN3,   (0X00000003U)   AIN3
                  ENUM_ADC_AIN4,   (0X00000004U)   AIN4
                  ENUM_ADC_AIN5    (0X00000005U)   AIN5
                  ENUM_ADC_AIN6    (0X00000006U)   AIN6
                  ENUM_ADC_AIN7    (0X00000007U)   AIN7
                  ENUM_ADC_AIN8    (0X00000008U)   AIN8
                  ENUM_ADC_AIN9    (0X00000009U)   AIN9
                  ENUM_ADC_AIN10   (0X0000000AU)   AIN10
                  ENUM_ADC_AIN11   (0X0000000BU)   AIN11
                  ENUM_ADC_AIN12   (0X0000000CU)   AIN12
                  ENUM_ADC_AIN13   (0X0000000DU)   AIN13
                  ENUM_ADC_AIN14   (0X0000000EU)   AIN14
                  ENUM_ADC_AIN15   (0X0000000FU)   AIN15
                  ENUM_ADC_VTEMP   (0X00000010U)   VTEMP
                  ENUM_ADC_AVDD0K  (0X00000011U)   AVDD0K
                  ENUM_ADC_IOVDDK  (0X00000012U)   IOVDDK
                  ENUM_ADC_PVDD0K  (0X00000013U)   PVDD0K
                  ENUM_ADC_PVDD1K  (0X00000014U)   PVDD1K
                  ENUM_ADC_MUXOUT  (0X00000015U)   MUXOUT
                  ENUM_ADC_IMONVOUT (0X00000016U)   IMONVOUT

    @return u32AinTemp
**/
uint32_t DigCompInputSelect(uint8_t ChanNum, uint32_t AinSelect)
{
    uint8_t u8TempChan = 0;
    uint32_t u32AinTemp;

    u8TempChan = ChanNum;
    switch (u8TempChan) {
        case DIGCHAN0:
            u32AinTemp = (pADI_ADC->ADCCMPCH01 & 0xFFE0); // ADCCMPCH01[4:0] mask
            u32AinTemp |= AinSelect;
            pADI_ADC->ADCCMPCH01 = u32AinTemp;
            break;
        case DIGCHAN1:
            u32AinTemp = (pADI_ADC->ADCCMPCH01 & 0xFC1F); // ADCCMPCH01[9:5] mask
            u32AinTemp |= (AinSelect << 5);
            pADI_ADC->ADCCMPCH01 = u32AinTemp;
            break;
        case DIGCHAN2:
            u32AinTemp = (pADI_ADC->ADCCMPCH23 & 0xFFE0); // ADCCMPCH23[4:0] mask
            u32AinTemp |= AinSelect;
            pADI_ADC->ADCCMPCH23 = u32AinTemp;
            break;
        case DIGCHAN3:
            u32AinTemp = (pADI_ADC->ADCCMPCH23 & 0xFC1F); // ADCCMPCH23[9:5] mask
            u32AinTemp |= (AinSelect << 5);
            pADI_ADC->ADCCMPCH23 = u32AinTemp;
            break;
        default:
            break;
    }
    return u32AinTemp;
}

/**
    @brief uint32_t DigCompThreshSetup(uint8_t ChanNum, uint32_t LowThresh, uint32_t HighThresh)
        ========  Sets Upper/lower comparison thresholds for digital comparator
    @param ChanNum
                    DIGCHAN0 or 0,      - Setup Digital Comparator 0
                    DIGCHAN1 or 1,      - Setup Digital Comparator 1
                    DIGCHAN2 or 2,      - Setup Digital Comparator 2
                    DIGCHAN3 or 3,      - Setup Digital Comparator 3

    @param LowThresh:{0-0xFFF}
        - Set Low Comparison threshold
    @param HighThresh:{0-0xFFF}
        - Set high Comparison threshold
    @return LowThresh
**/
uint32_t DigCompThreshSetup(uint8_t ChanNum, uint32_t LowThresh, uint32_t HighThresh)
{
    uint8_t u8TempChan = 0;
    uint32_t u32AinTemp;

    u8TempChan = ChanNum;
    switch (u8TempChan) {
        case DIGCHAN0:
            u32AinTemp = (pADI_ADC->ADCCMP0CON0 & 0x3003); // ADCCMP0CON0[13:2] mask
            u32AinTemp |= (LowThresh << 2);
            pADI_ADC->ADCCMP0CON0 = u32AinTemp;
            u32AinTemp = (pADI_ADC->ADCCMP0CON1 & 0xF000); // ADCCMP0CON1[11:0] mask
            u32AinTemp |= HighThresh;
            pADI_ADC->ADCCMP0CON1 = u32AinTemp;
            break;
        case DIGCHAN1:
            u32AinTemp = (pADI_ADC->ADCCMP1CON0 & 0x3003); // ADCCMP1CON0[13:2] mask
            u32AinTemp |= (LowThresh << 2);
            pADI_ADC->ADCCMP1CON0 = u32AinTemp;
            u32AinTemp = (pADI_ADC->ADCCMP1CON1 & 0xF000); // ADCCMP1CON1[11:0] mask
            u32AinTemp |= HighThresh;
            pADI_ADC->ADCCMP1CON1 = u32AinTemp;
            break;
        case DIGCHAN2:
            u32AinTemp = (pADI_ADC->ADCCMP2CON0 & 0x3003); // ADCCMP2CON0[13:2] mask
            u32AinTemp |= (LowThresh << 2);
            pADI_ADC->ADCCMP2CON0 = u32AinTemp;
            u32AinTemp = (pADI_ADC->ADCCMP2CON1 & 0xF000); // ADCCMP2CON1[11:0] mask
            u32AinTemp |= HighThresh;
            pADI_ADC->ADCCMP2CON1 = u32AinTemp;
            break;
        case DIGCHAN3:
            u32AinTemp = (pADI_ADC->ADCCMP3CON0 & 0x3003); // ADCCMP3CON0[13:2] mask
            u32AinTemp |= (LowThresh << 2);
            pADI_ADC->ADCCMP3CON0 = u32AinTemp;
            u32AinTemp = (pADI_ADC->ADCCMP3CON1 & 0xF000); // ADCCMP3CON1[11:0] mask
            u32AinTemp |= HighThresh;
            pADI_ADC->ADCCMP3CON1 = u32AinTemp;
            break;
        default:
            break;
    }
    return LowThresh;
}

/**
    @brief uint32_t DigCompEnable_Disable(uint8_t ChanNum, uint8_t EnableDisable)
        ========  Sets Upper/lower comparison thresholds for digital comparator
    @param ChanNum
                    DIGCHAN0 or 0,      - Setup Digital Comparator 0
                    DIGCHAN1 or 1,      - Setup Digital Comparator 1
                    DIGCHAN2 or 2,      - Setup Digital Comparator 2
                    DIGCHAN3 or 3,      - Setup Digital Comparator 3

    @param EnableDisable:{0,1 }
        - 0 to Disable the Comparator
        - 1 to enable the Comparator

    @return EnableDisable
**/
uint32_t DigCompEnable_Disable(uint8_t ChanNum, uint8_t EnableDisable)
{
    uint8_t u8TempChan = 0;

    u8TempChan = ChanNum;

    switch (u8TempChan) {
        case DIGCHAN0:
            if (EnableDisable == 1)
                pADI_ADC->ADCCMP0CON0 |= 0x1; // Enable digital Comparator 0.
            else
                pADI_ADC->ADCCMP0CON0 &= 0xFFFE; // Disable digital Comparator 0.
            break;
        case DIGCHAN1:
            if (EnableDisable == 1)
                pADI_ADC->ADCCMP1CON0 |= 0x1; // Enable digital Comparator 1.
            else
                pADI_ADC->ADCCMP1CON0 &= 0xFFFE; // Disable digital Comparator 1.
            break;
        case DIGCHAN2:
            if (EnableDisable == 1)
                pADI_ADC->ADCCMP2CON0 |= 0x1; // Enable digital Comparator 2.
            else
                pADI_ADC->ADCCMP2CON0 &= 0xFFFE; // Disable digital Comparator 2.
            break;
        case DIGCHAN3:
            if (EnableDisable == 1)
                pADI_ADC->ADCCMP3CON0 |= 0x1; // Enable digital Comparator 3.
            else
                pADI_ADC->ADCCMP3CON0 &= 0xFFFE; // Disable digital Comparator 3.
            break;
        default:
            break;
    }
    return EnableDisable;
}
/**
    @brief uint32_t DigCompIrq(uint8_t ChanNum, uint8_t EnableDisable, uint8_t RiseEdgeH_FallEdgeL)
        ========  Sets Upper/lower comparison thresholds for digital comparator
    @param ChanNum
                    DIGCHAN0 or 0,      - Setup Digital Comparator 0
                    DIGCHAN1 or 1,      - Setup Digital Comparator 1
                    DIGCHAN2 or 2,      - Setup Digital Comparator 2
                    DIGCHAN3 or 3,      - Setup Digital Comparator 3

    @param EnableDisable:{0,1 }
        - 0 to Disable the Comparator IRQ
        - 1 to enable the Comparator IRQ

    @param RiseEdgeH_FallEdgeL:{0,1 }
        - 0 to Setup Comparator IRQ for Falling edge
        - 1 to Setup Comparator IRQ for rising edge
    @return EnableDisable
**/
uint32_t DigCompIrq(uint8_t ChanNum, uint8_t EnableDisable, uint8_t RiseEdgeH_FallEdgeL)
{
    uint8_t u8TempChan = 0;

    u8TempChan = ChanNum;

    switch (u8TempChan) {
        case DIGCHAN0:
            if (EnableDisable == 1) {
                pADI_ADC->ADCCMPCH01 |= BITM_ADC_ADCCMPCH01_IRQEN0; // Enable digital Comparator 0 IRQ.
                if (RiseEdgeH_FallEdgeL == 0)                       // Falling edge
                {
                    pADI_ADC->ADCCMP0CON0 |= BITM_ADC_ADCCMP0CON0_LOWIRQEN;
                    pADI_ADC->ADCCMP0CON0 &= ~(BITM_ADC_ADCCMP0CON0_CMPDIR);
                }
                else // Rising edge
                {
                    pADI_ADC->ADCCMP0CON0 &= ~(BITM_ADC_ADCCMP0CON0_LOWIRQEN);
                    pADI_ADC->ADCCMP0CON0 |= BITM_ADC_ADCCMP0CON0_CMPDIR;
                }
            }
            else
                pADI_ADC->ADCCMPCH01 &= ~(BITM_ADC_ADCCMPCH01_IRQEN0); // Disable digital Comparator 0 IRQ.
            break;
        case DIGCHAN1:
            if (EnableDisable == 1) {
                pADI_ADC->ADCCMPCH01 |= BITM_ADC_ADCCMPCH01_IRQEN1; // Enable digital Comparator 1 IRQ.
                if (RiseEdgeH_FallEdgeL == 0)                       // Falling edge
                {
                    pADI_ADC->ADCCMP1CON0 |= BITM_ADC_ADCCMP1CON0_LOWIRQEN;
                    pADI_ADC->ADCCMP1CON0 &= ~(BITM_ADC_ADCCMP1CON0_CMPDIR);
                }
                else // Rising edge
                {
                    pADI_ADC->ADCCMP1CON0 &= ~(BITM_ADC_ADCCMP1CON0_LOWIRQEN);
                    pADI_ADC->ADCCMP1CON0 |= BITM_ADC_ADCCMP1CON0_CMPDIR;
                }
            }
            else
                pADI_ADC->ADCCMPCH01 &= ~(BITM_ADC_ADCCMPCH01_IRQEN1); // Disable digital Comparator 1 IRQ.
            break;
        case DIGCHAN2:
            if (EnableDisable == 1) {
                pADI_ADC->ADCCMPCH01 |= BITM_ADC_ADCCMPCH01_IRQEN2; // Enable digital Comparator 2 IRQ.
                if (RiseEdgeH_FallEdgeL == 0)                       // Falling edge
                {
                    pADI_ADC->ADCCMP2CON0 |= BITM_ADC_ADCCMP2CON0_LOWIRQEN;
                    pADI_ADC->ADCCMP2CON0 &= ~(BITM_ADC_ADCCMP1CON0_CMPDIR);
                }
                else // Rising edge
                {
                    pADI_ADC->ADCCMP2CON0 &= ~(BITM_ADC_ADCCMP2CON0_LOWIRQEN);
                    pADI_ADC->ADCCMP2CON0 |= BITM_ADC_ADCCMP2CON0_CMPDIR;
                }
            }
            else
                pADI_ADC->ADCCMPCH01 &= ~(BITM_ADC_ADCCMPCH01_IRQEN2); // Disable digital Comparator 2 IRQ.
            break;
        case DIGCHAN3:
            if (EnableDisable == 1) {
                pADI_ADC->ADCCMPCH01 |= BITM_ADC_ADCCMPCH01_IRQEN3; // Enable digital Comparator 3 IRQ.
                if (RiseEdgeH_FallEdgeL == 0)                       // Falling edge
                {
                    pADI_ADC->ADCCMP3CON0 |= BITM_ADC_ADCCMP3CON0_LOWIRQEN;
                    pADI_ADC->ADCCMP3CON0 &= ~(BITM_ADC_ADCCMP3CON0_CMPDIR);
                }
                else // Rising edge
                {
                    pADI_ADC->ADCCMP3CON0 &= ~(BITM_ADC_ADCCMP3CON0_LOWIRQEN);
                    pADI_ADC->ADCCMP3CON0 |= BITM_ADC_ADCCMP3CON0_CMPDIR;
                }
            }
            else
                pADI_ADC->ADCCMPCH01 &= ~(BITM_ADC_ADCCMPCH01_IRQEN3); // Disable digital Comparator 3 IRQ.
            break;
        default:
            break;
    }
    return EnableDisable;
}
/**
    @brief uint32_t DigCompToPlaSetup(uint8_t ChanNum, uint8_t IrqOrCompOut)
        ========  Sets Upper/lower comparison thresholds for digital comparator
    @param ChanNum
                    DIGCHAN0 or 0,      - Setup Digital Comparator 0
                    DIGCHAN1 or 1,      - Setup Digital Comparator 1
                    DIGCHAN2 or 2,      - Setup Digital Comparator 2
                    DIGCHAN3 or 3,      - Setup Digital Comparator 3
    @param IrqOrCompOut:{0,1 }
        - 0 to connect Comparator output directly to the PLA element.
        - 1 to connect Comparator Interrupt  to the PLA element.
    @return IrqOrCompOut
**/
uint32_t DigCompToPlaSetup(uint8_t ChanNum, uint8_t IrqOrCompOut)
{
    uint8_t u8TempChan = 0;

    u8TempChan = ChanNum;

    switch (u8TempChan) {
        case DIGCHAN0:
            if (IrqOrCompOut == 1) // Comparator interrupt to PLA24
                pADI_ADC->ADCCMP0CON0 |= BITM_ADC_ADCCMP0CON0_PLAMUX0;
            else
                pADI_ADC->ADCCMP0CON0 &= ~(BITM_ADC_ADCCMP0CON0_PLAMUX0); // Comparator output directly to PLA24
            break;
        case DIGCHAN1:
            if (IrqOrCompOut == 1) // Comparator interrupt to PLA25
                pADI_ADC->ADCCMP1CON0 |= BITM_ADC_ADCCMP1CON0_PLAMUX1;
            else
                pADI_ADC->ADCCMP1CON0 &= ~(BITM_ADC_ADCCMP1CON0_PLAMUX1); // Comparator output directly to PLA25
            break;
        case DIGCHAN2:
            if (IrqOrCompOut == 1) // Comparator interrupt to PLA26
                pADI_ADC->ADCCMP2CON0 |= BITM_ADC_ADCCMP2CON0_PLAMUX2;
            else
                pADI_ADC->ADCCMP2CON0 &= ~(BITM_ADC_ADCCMP2CON0_PLAMUX2); // Comparator output directly to PLA26
            break;
        case DIGCHAN3:
            if (IrqOrCompOut == 1) // Comparator interrupt to PLA27
                pADI_ADC->ADCCMP3CON0 |= BITM_ADC_ADCCMP3CON0_PLAMUX3;
            else
                pADI_ADC->ADCCMP3CON0 &= ~(BITM_ADC_ADCCMP3CON0_PLAMUX3); // Comparator output directly to PLA27
            break;
        default:
            break;
    }
    return IrqOrCompOut;
}
