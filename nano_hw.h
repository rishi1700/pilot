#ifndef NANO_HW_H
#define NANO_HW_H

#include <stdint.h>

/*
 * Hardware abstraction for Nano-ITLA biasing & control.
 * These are implemented on the MCU side (DACs, current drivers, TEC, etc.).
 * For now they can be stubs; later you wire them to the real registers.
 */

void nano_hw_set_v1_voltage(float volts);
void nano_hw_set_v2_voltage(float volts);
void nano_hw_set_v3_voltage(float volts);

void nano_hw_set_gain_current_mA(float mA);
void nano_hw_set_soa_current_mA(float mA);

void nano_hw_set_temperature_C(float degC);

/* PD related – interpretation TBD, but keep the hooks here */
void nano_hw_set_wmpd_target(float val);
void nano_hw_set_wlpd_target(float val);
void nano_hw_set_mpd_target(float val);

/* Optional: debug print hook */
void nano_hw_debug_printf(const char *fmt, ...);

#endif /* NANO_HW_H */
