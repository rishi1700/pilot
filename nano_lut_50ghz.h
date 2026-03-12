#ifndef NANO_LUT_50GHZ_H
#define NANO_LUT_50GHZ_H

#include <stdint.h>

typedef struct {
    float v1_v;          /* V1 (-V)        */
    float v2_v;          /* V2 (-V)        */
    float v3_v;          /* V3 (-V)        */
    float gain_mA;       /* Gain (mA)      */
    float soa_mA;        /* SOA (mA)       */
    float temp_C;        /* Temperature C  */
    float wmpd;          /* WMPD           */
    float wlpd;          /* WLPD           */
    float mpd;           /* MPD            */

    double freq_ghz;     /* NEW: calibrated optical frequency for this channel */
    /* (optional alternative: double wl_nm;) */
} ChannelLutEntry;

#define LUT_NUM_CHANNELS_50GHZ  101U

extern const ChannelLutEntry g_lut_50GHz[LUT_NUM_CHANNELS_50GHZ];

/* Active channel copy (for debug / later use) */
extern ChannelLutEntry g_active_channel_50GHz;

/* Apply LUT row for given channel (1..101) */
void nano_apply_channel_from_lut(uint16_t channel);

#endif /* NANO_LUT_50GHZ_H */
