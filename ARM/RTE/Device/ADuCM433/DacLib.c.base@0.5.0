/*!
 *****************************************************************************
 * @file:   DacLib.c
 * @brief:  source file of Digital to Analog Voltage convertor
 * @version: V0.5
 * @date:    May 2022
 * @par:     Revision History:
 * - V0.1, March 2021: Initial version.
 * - V0.2, July 2021: Disable negative buffer while out sel = 0, which means
 *                      output positive votlage.
 * - V0.3, Dec 2021: Add support for other devices in product family
 * - V0.4, Feb 2022: Add userkey load and clear for below functions
 *                   - VDacCfg(),IDacCfg(),IDacWr(),IDacWrAutoSync(),IDacPdCh(),IDacEnCh()
 * - V0.5, May 2022: Cleanup Doxygen Docs. API change to IDacCfg(), removed shutdown
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2022 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#include "DacLib.h"

/**
    @brief uint32_t VDacWr(uint32_t index, uint32_t data)
        ======== Writes the DAC value.
    @param index: {0-8}
        - dac channel index.
    @param data : {0x000-0xFFF}
        - 12-bit digital data to output to DAC.
    @return DAC data.
**/
uint32_t VDacWr(uint32_t index, uint32_t data)
{
    switch (index) {
        case 0:
            pADI_VDAC->VDAC0DAT = 1 << BITP_VDAC_VDAC0DAT_DACPREV | (0xFFF & data);
            break;

        case 1:
            pADI_VDAC->VDAC1DAT = 1 << BITP_VDAC_VDAC1DAT_DACPREV | (0xFFF & data);
            break;

        case 2:
            pADI_VDAC->VDAC2DAT = 1 << BITP_VDAC_VDAC2DAT_DACPREV | (0xFFF & data);
            break;

        case 3:
            pADI_VDAC->VDAC3DAT = 1 << BITP_VDAC_VDAC3DAT_DACPREV | (0xFFF & data);
            break;

        case 4:
            pADI_VDAC->VDAC4DAT = 1 << BITP_VDAC_VDAC4DAT_DACPREV | (0xFFF & data);
            break;

        case 5:
            pADI_VDAC->VDAC5DAT = 1 << BITP_VDAC_VDAC5DAT_DACPREV | (0xFFF & data);
            break;

        case 6:
            pADI_VDAC->VDAC6DAT = 1 << BITP_VDAC_VDAC6DAT_DACPREV | (0xFFF & data);
            break;

        case 7:
            pADI_VDAC->VDAC7DAT = 1 << BITP_VDAC_VDAC7DAT_DACPREV | (0xFFF & data);
            break;

        case 8:
            pADI_VDAC->VDAC8DAT = 1 << BITP_VDAC_VDAC8DAT_DACPREV | (0xFFF & data);
            break;

        default:
            break;
    }

    return data;
}

/**
    @brief uint32_t VDacWrAutoSync(uint32_t index, uint32_t data)
        ======== Writes the DAC value.
    @param index: {0~8}
            - dac channel index
    @param data : {0x00 ~ 0xFFF}
            - 12-bit digital data to output to DAC.
    @return DAC data.
**/
uint32_t VDacWrAutoSync(uint32_t index, uint32_t data)
{
    switch (index) {
        case 0:
            pADI_VDAC->VDAC0DAT = 0xFFF & data;
            break;

        case 1:
            pADI_VDAC->VDAC1DAT = 0xFFF & data;
            break;

        case 2:
            pADI_VDAC->VDAC2DAT = 0xFFF & data;
            break;

        case 3:
            pADI_VDAC->VDAC3DAT = 0xFFF & data;
            break;

        case 4:
            pADI_VDAC->VDAC4DAT = 0xFFF & data;
            break;

        case 5:
            pADI_VDAC->VDAC5DAT = 0xFFF & data;
            break;

        case 6:
            pADI_VDAC->VDAC6DAT = 0xFFF & data;
            break;

        case 7:
            pADI_VDAC->VDAC7DAT = 0xFFF & data;
            break;

        case 8:
            pADI_VDAC->VDAC8DAT = 0xFFF & data;
            break;

        default:
            break;
    }

    return data;
}

/**
    @brief uint32_t VDacCfg(uint32_t index, uint32_t fullScale, uint32_t outMux, uint32_t negGain)
        ======== Config VDAC register.
    @param index: {0-8}
        - dac channel index
    @param fullScale: {ENUM_VDAC_VDAC0CON_FSLVL_FS2P5,ENUM_VDAC_VDAC0CON_FSLVL_FS3P75}
        - 2.5V or 3.75V, vdac0~7,
        - vdac8 fullcale is 2.5V fixed
    @param outMux: {0,ENUM_VDAC_VDAC1CON_OUTSEL_PRIMARY,ENUM_VDAC_VDAC1CON_OUTSEL_SECONDARY}
        - ENUM_VDAC_VDAC1CON_OUTSEL_PRIMARY positive output only for vdac0~3
        - ENUM_VDAC_VDAC1CON_OUTSEL_SECONDARY negative output only for vdac0~3
        - 0 for all other VDACs
    @param negGain: {0, ENUM_VDAC_VDAC0CON_FSLVL_FS2P5, ENUM_VDAC_VDAC0CON_FSLVL_FS3P75}
        - ENUM_VDAC_VDAC0CON_FSLVL_FS2P5 negative output range -2.5V for vdac0~3
        - ENUM_VDAC_VDAC0CON_FSLVL_FS3P75 negative output range -3.75V for vdac0~3
        - 0 for all other VDACs
    @return 0.
**/
uint32_t VDacCfg(uint32_t index, uint32_t fullScale, uint32_t outMux, uint32_t negGain)
{
    uint16_t reg;
    pADI_MISC->USERKEY = 0x9FE5;
    switch (index) {
        case 0:
            reg = fullScale << BITP_VDAC_VDAC0CON_FSLVL | outMux << BITP_VDAC_VDAC0CON_OUTSEL | (outMux ? 0 : 1 << 4)
#if defined(__ADUCM430__)
                  | negGain << BITP_VDAC_VDAC0CON_NFSLVL
#endif
                ;
            pADI_VDAC->VDAC0CON = reg;
            break;

        case 1:
            reg = fullScale << BITP_VDAC_VDAC1CON_FSLVL | outMux << BITP_VDAC_VDAC1CON_OUTSEL | (outMux ? 0 : 1 << 4)
#if defined(__ADUCM430__)
                  | negGain << BITP_VDAC_VDAC1CON_NFSLVL
#endif
                ;
            pADI_VDAC->VDAC1CON = reg;
            break;

        case 2:
            reg = fullScale << BITP_VDAC_VDAC2CON_FSLVL | outMux << BITP_VDAC_VDAC2CON_OUTSEL | (outMux ? 0 : 1 << 4)
#if defined(__ADUCM430__)
                  | negGain << BITP_VDAC_VDAC2CON_NFSLVL
#endif
                ;
            pADI_VDAC->VDAC2CON = reg;
            break;

        case 3:
            reg = fullScale << BITP_VDAC_VDAC3CON_FSLVL | outMux << BITP_VDAC_VDAC3CON_OUTSEL | (outMux ? 0 : 1 << 4)
#if defined(__ADUCM430__)
                  | negGain << BITP_VDAC_VDAC3CON_NFSLVL
#endif
                ;
            pADI_VDAC->VDAC3CON = reg;
            break;

        case 4:
            reg = fullScale << BITP_VDAC_VDAC4CON_FSLVL;
            pADI_VDAC->VDAC4CON = reg;
            break;

        case 5:
            reg = fullScale << BITP_VDAC_VDAC5CON_FSLVL;
            pADI_VDAC->VDAC5CON = reg;
            break;

        case 6:
            reg = fullScale << BITP_VDAC_VDAC6CON_FSLVL;
            pADI_VDAC->VDAC6CON = reg;
            break;

        case 7:
            reg = fullScale << BITP_VDAC_VDAC7CON_FSLVL;
            pADI_VDAC->VDAC7CON = reg;
            break;

        case 8:
            pADI_VDAC->VDAC8CON = 0;
            break;

        default:
            break;
    }
    pADI_MISC->USERKEY = 0;
    return 0;
}

/**
    @brief uint32_t VDacSync(uint32_t index)
        ======== Sync VDACxData to output.
    @param index: {0-8}
        - dac channel index.
    @return 0
**/
uint32_t VDacSync(uint32_t index)
{
    pADI_VDAC->VDACSYNCLOAD |= index & 0x1FF;
    return 0;
}

uint32_t VDacImonEn(uint16_t PosImonEn, uint16_t NegImonEn)
{
    uint16_t uiReadVal = 0;

    uiReadVal = pADI_VDAC->VDACIMONCON;
    if (PosImonEn == 0) // Enable VDAC Positive I monitor
        uiReadVal &= 0xFFFE;
    else // Disable VDAC Positive I monitor
        uiReadVal |= 0x1;
    pADI_VDAC->VDACIMONCON = uiReadVal;

    uiReadVal = pADI_VDAC->VDACNBUFIMCN;
    if (NegImonEn == 0) // Enable VDAC negative I monitor
        uiReadVal &= 0xFFFE;
    else // Disable VDAC negative I monitor
        uiReadVal |= 0x1;
    pADI_VDAC->VDACNBUFIMCN = uiReadVal;

    return 0;
}

#if defined(__ADUCM430__) || defined(__ADUCM433__)
/**
    @brief uint32_t IDacCfg(uint32_t index, uint32_t fullScale, uint32_t res, uint32_t clrBit)
        ======== Config IDAC register.
    @param index: {0-3}
        - idac channel.
    @param fullScale: {ENUM_IDAC_IDAC0CON_RANGE_RANGE50M, ENUM_IDAC_IDAC0CON_RANGE_RANGE100M,
                        ENUM_IDAC_IDAC0CON_RANGE_RANGE100MO2, ENUM_IDAC_IDAC0CON_RANGE_RANGE150M}
        - 0 or ENUM_IDAC_IDAC0CON_RANGE_RANGE50M for 50mA.
        - 1 or ENUM_IDAC_IDAC0CON_RANGE_RANGE100M for 100mA.
        - 2 or ENUM_IDAC_IDAC0CON_RANGE_RANGE100MO2 for 100mA with option2.
        - 3 or ENUM_IDAC_IDAC0CON_RANGE_RANGE150M for 150mA.
    @param res: {0}
        - Reserved unused parameter
    @param clrBit: {ENUM_IDAC_IDAC0CON_CLRBIT_CLR, ENUM_IDAC_IDAC0CON_CLRBIT_CLRB}
        - 0 or ENUM_IDAC_IDAC0CON_CLRBIT_CLR to clear idac data reigster.
        - 1 or ENUM_IDAC_IDAC0CON_CLRBIT_CLRB to enable write to idac data register.
    @return 0.
**/
uint32_t IDacCfg(uint32_t index, uint32_t fullScale, uint32_t res, uint32_t clrBit)
{
    uint16_t reg;
    (void)res;
    pADI_MISC->USERKEY = 0x9FE5;
    switch (index) {
        case 0:
            reg = clrBit << BITP_IDAC_IDAC0CON_CLRBIT | fullScale << BITP_IDAC_IDAC0CON_RANGE;
            pADI_IDAC->IDAC0CON = reg;
            break;
#if defined(__ADUCM430__)
        case 1:
            reg = clrBit << BITP_IDAC_IDAC1CON_CLRBIT | fullScale << BITP_IDAC_IDAC1CON_RANGE;
            pADI_IDAC->IDAC1CON = reg;
            break;

        case 2:
            reg = clrBit << BITP_IDAC_IDAC2CON_CLRBIT | fullScale << BITP_IDAC_IDAC2CON_RANGE;
            pADI_IDAC->IDAC2CON = reg;
            break;

        case 3:
            reg = clrBit << BITP_IDAC_IDAC3CON_CLRBIT | fullScale << BITP_IDAC_IDAC3CON_RANGE;
            pADI_IDAC->IDAC3CON = reg;
            break;
#endif // defined(__ADUCM430__)
        default:
            break;
    }
    pADI_MISC->USERKEY = 0;
    return 0;
}

/**
    @brief uint32_t IDacWr(uint32_t index, uint32_t data)
        ======== Writes the IDAC value.
    @param index: {0-3}
        - idac channel.
    @param data : {0x000-0xFFF}
        - 12-bit digital data to output to DAC.
    @return DAC data.
**/
uint32_t IDacWr(uint32_t index, uint32_t data)
{
    pADI_MISC->USERKEY = 0x9FE5;
    switch (index) {
        case 0:
            pADI_IDAC->IDAC0DAT = (0xFFF & data) | (1 << BITP_IDAC_IDAC0DAT_IDACPREV);
            break;

#if defined(__ADUCM430__)
        case 1:
            pADI_IDAC->IDAC1DAT = (0xFFF & data) | (1 << BITP_IDAC_IDAC1DAT_IDACPREV);
            break;

        case 2:
            pADI_IDAC->IDAC2DAT = (0xFFF & data) | (1 << BITP_IDAC_IDAC2DAT_IDACPREV);
            break;

        case 3:
            pADI_IDAC->IDAC3DAT = (0xFFF & data) | (1 << BITP_IDAC_IDAC3DAT_IDACPREV);
            break;
#endif // defined(__ADUCM430__)
        default:
            break;
    }
    pADI_MISC->USERKEY = 0;
    return data;
}

/**
    @brief uint32_t IDacWrAutoSync(uint32_t index, uint32_t data)
        ======== Writes the IDAC value.
    @param index: {0-3}
        - idac channel.
    @param data : {0x000-0xFFF}
        - 12-bit digital data to output to DAC.
    @return DAC data.
**/
uint32_t IDacWrAutoSync(uint32_t index, uint32_t data)
{
    pADI_MISC->USERKEY = 0x9FE5;
    switch (index) {
        case 0:
            pADI_IDAC->IDAC0DAT = (0xFFF & data);
            break;
#if defined(__ADUCM430__)
        case 1:
            pADI_IDAC->IDAC1DAT = (0xFFF & data);
            break;

        case 2:
            pADI_IDAC->IDAC2DAT = (0xFFF & data);
            break;

        case 3:
            pADI_IDAC->IDAC3DAT = (0xFFF & data);
            break;
#endif // defined(__ADUCM430__)
        default:
            break;
    }
    pADI_MISC->USERKEY = 0;
    return data;
}

/**
    @brief uint32_t IDacSync(uint32_t index)
        ======== Sync IDACxData to output.
    @param index :{0-3}
        - idac channel.
    @return 0.
**/
uint32_t IDacSync(uint32_t index)
{
    pADI_MISC->USERKEY = 0x9FE5;
    pADI_IDAC->IDACLOAD = index & 0xF;
    return 0;
    pADI_MISC->USERKEY = 0x0;
}

/**
    @brief uint32_t IDacPdCh(uint32_t index)
        ======== Power down IDAC channel.
    @param index :{0-3}
        - idac channel.
    @return 0.
**/
uint32_t IDacPdCh(uint32_t index)
{
    pADI_MISC->USERKEY = 0x9FE5;
    pADI_IDAC->IDACTOUTSD |= 1 << (index & 0xF);
    pADI_MISC->USERKEY = 0;
    return 0;
}

/**
    @brief uint32_t IDacEnCh(uint32_t index)
        ======== Enable IDAC channel.
    @param index :{0-3}
        - idac channel.
    @return 0.
**/
uint32_t IDacEnCh(uint32_t index)
{
    pADI_MISC->USERKEY = 0x9FE5;
    pADI_IDAC->IDACTOUTSD &= ~(1 << (index & 0xF));
    pADI_MISC->USERKEY = 0;
    return 0;
}

#endif // defined(__ADUCM430__) || defined(__ADUCM433__)
