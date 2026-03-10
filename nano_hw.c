#include "nano_hw.h"
#include <stdarg.h>
#include <stdio.h>

/*
 * NOTE:
 *  - Replace these bodies with the REAL register/DAC writes
 *  - For now they are just stubs so the project builds and you can
 *    confirm the LUT/channel logic is working.
 */

void nano_hw_set_v1_voltage(float volts)
{
    /* TODO: write 'volts' to V1 DAC / bias register */
    (void)volts;
}

void nano_hw_set_v2_voltage(float volts)
{
    /* TODO: write 'volts' to V2 DAC / bias register */
    (void)volts;
}

void nano_hw_set_v3_voltage(float volts)
{
    /* TODO: write 'volts' to V3 DAC / bias register */
    (void)volts;
}

void nano_hw_set_gain_current_mA(float mA)
{
    /* TODO: write 'mA' to gain current driver */
    (void)mA;
}

void nano_hw_set_soa_current_mA(float mA)
{
    /* TODO: write 'mA' to SOA current driver */
    (void)mA;
}

void nano_hw_set_temperature_C(float degC)
{
    /* TODO: set TEC or temperature controller set-point */
    (void)degC;
}

void nano_hw_set_wmpd_target(float val)
{
    /* TODO: map to WMPD set-point / threshold / gain */
    (void)val;
}

void nano_hw_set_wlpd_target(float val)
{
    /* TODO: map to WLPD set-point / threshold / gain */
    (void)val;
}

void nano_hw_set_mpd_target(float val)
{
    /* TODO: map to MPD set-point / threshold / gain */
    (void)val;
}

/*
 * Simple debug print hook.
 * If you already have a board-specific UART printf, you can
 * redirect to that instead of plain printf.
 */
void nano_hw_debug_printf(const char *fmt, ...)
{
#ifdef NANO_HW_DEBUG
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);   /* or your board's debug UART function */
    va_end(ap);
#else
    (void)fmt;
#endif
}
