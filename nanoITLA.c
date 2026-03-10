// nanoITLA.c — Python-first or C-only register handling, runtime switchable

#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nano_lut_50ghz.h"
#include "nano_globals.h"

/* Active LUT row (filled by nano_apply_channel_from_lut in nano_lut_50ghz.c) */
extern ChannelLutEntry g_active_channel_50GHz;

/* ---------- Build switches ---------- */
#ifndef ITLA_EMBED_PY
#  define ITLA_EMBED_PY 0        /* default: no Python */
#endif

#ifdef __ARMCC_VERSION           /* Keil on MCU => force no Python */
#  undef ITLA_EMBED_PY
#  define ITLA_EMBED_PY 0
#  undef  DEFAULT_PI_MODE
#  define DEFAULT_PI_MODE 0
#endif

#ifndef DEFAULT_PI_MODE
#  define DEFAULT_PI_MODE 0      /* 0=C handler default, 1=Python default */
#endif

#ifdef _WIN32
#  define ITLA_API __declspec(dllexport)
#  define _CRT_SECURE_NO_WARNINGS
#else
#  define ITLA_API
#endif

/* ---------- Status/Error defs ---------- */
typedef enum { STAT_OK=0, STAT_XE=1, STAT_AEA=2, STAT_CP=3 } ItlaStatus;

enum {
  LERR_NONE=0x00, LERR_RNI=0x01, LERR_RNW=0x02, LERR_RVE=0x03, LERR_CIP=0x04,
  LERR_CII=0x05, LERR_ERE=0x06, LERR_ERO=0x07, LERR_CIE=0x09, LERR_PYFAIL=0x0A
};

static uint8_t g_last_error = 0x00;

/* ---------- Mode switch ---------- */
static int use_python = DEFAULT_PI_MODE;
static int py_ready = 0;
static uint8_t g_direct_ctrl_mode = 0; /* 0 = MSA default, 1 = direct/UART mode */


#if ITLA_EMBED_PY
  #include <Python.h>
  static PyObject *py_module=NULL, *py_func=NULL;
#else
  /* Dummy types so file compiles without Python.h */
  typedef void PyObject;
  static PyObject *py_module=NULL, *py_func=NULL;
#endif

static inline void itla_set_use_python(int on){ use_python = on?1:0; }
static inline int  itla_get_use_python(void){ return use_python?1:0; }

/* ---------- Wire helpers ---------- */
static uint8_t bip4(uint8_t b0,uint8_t b1,uint8_t b2,uint8_t b3){
  uint8_t bip8 = (b0 & 0x0F) ^ b1 ^ b2 ^ b3;
  return ((bip8>>4)&0x0F) ^ (bip8&0x0F);
}
static inline uint32_t build_inbound_app_packet(uint8_t isw,uint8_t reg,uint16_t data){
  return (((uint32_t)(isw&1))<<26) | (((uint32_t)reg&0xFF)<<18) | ((uint32_t)data&0xFFFF);
}
uint32_t build_inbound_frame(uint8_t lstRsp,uint8_t is_write,uint8_t reg,uint16_t data){
  uint32_t app = build_inbound_app_packet(is_write,reg,data);
  uint32_t tmp = (((uint32_t)(lstRsp&1))<<27) | (app & 0x07FFFFFF);
  uint8_t b0=(tmp>>24)&0xFF,b1=(tmp>>16)&0xFF,b2=(tmp>>8)&0xFF,b3=tmp&0xFF;
  uint8_t csum = bip4(b0&0x0F,b1,b2,b3)&0x0F;
  return tmp | ((uint32_t)csum<<28);
}
typedef struct { uint8_t checksum,lstRsp,is_write,reg; uint16_t data; } InboundFields;
int parse_inbound_frame(uint32_t frame, InboundFields *out, uint8_t *ce_out){
  if(!out||!ce_out) return -1;
  memset(out,0,sizeof(*out)); *ce_out=0;
  uint8_t csum=(frame>>28)&0x0F;
  uint8_t b0=(frame>>24)&0xFF,b1=(frame>>16)&0xFF,b2=(frame>>8)&0xFF,b3=frame&0xFF;
  uint8_t rc = bip4(b0&0x0F,b1,b2,b3)&0x0F;
  if(rc!=csum) *ce_out=1;
  out->checksum=csum; out->lstRsp=(frame>>27)&1;
  uint32_t app=frame & 0x07FFFFFF;
  out->is_write=(app>>26)&1; out->reg=(app>>18)&0xFF; out->data=app & 0xFFFF;
  return 0;
}
uint32_t build_outbound_frame_spec(uint8_t ce,uint8_t xe,uint8_t reg,uint16_t data){
  uint32_t resp = (((uint32_t)(xe&1))<<25) | (((uint32_t)reg&0xFF)<<17) | (((uint32_t)data&0xFFFF)<<1);
  uint32_t tmp = resp | ((uint32_t)1<<26) | (((uint32_t)(ce&1))<<27);
  uint8_t b0=(tmp>>24)&0xFF,b1=(tmp>>16)&0xFF,b2=(tmp>>8)&0xFF,b3=tmp&0xFF;
  uint8_t csum=bip4(b0&0x0F,b1,b2,b3)&0x0F;
  return tmp | ((uint32_t)csum<<28);
}

/* ======================================================================= */
/* =======================  PYTHON EMBEDDING PART  ======================== */
/* ======================================================================= */
#if ITLA_EMBED_PY
static int py_init(void){
  if(!use_python) return 0;
  if(py_ready) return 0;
  Py_Initialize();
  if(!Py_IsInitialized()){ use_python=0; return -1; }
  PyRun_SimpleString(
    "import os,sys\n"
    "moddir=os.getenv('ITLA_MODULE_DIR')\n"
    "sys.path[:0]=[moddir] if moddir else ['.']\n"
  );
  PyObject *pName = PyUnicode_DecodeFSDefault("mymodule");
  if(!pName){ use_python=0; return 0; }
  py_module = PyImport_Import(pName); Py_DECREF(pName);
  if(!py_module){ PyErr_Clear(); use_python=0; return 0; }
  py_func = PyObject_GetAttrString(py_module,"handle_register");
  if(!py_func || !PyCallable_Check(py_func)){ Py_XDECREF(py_func); Py_DECREF(py_module); py_module=NULL; use_python=0; return 0; }
  py_ready=1; return 0;
}
static void py_fini(void){
  if(!py_ready) return;
  Py_XDECREF(py_func); Py_XDECREF(py_module); Py_Finalize();
  py_func=NULL; py_module=NULL; py_ready=0;
}
static int py_handle_register(uint8_t reg,uint8_t isw,uint16_t data,uint8_t *xe_out,uint16_t *d_out){
  if(!use_python) return -10;
  if(!py_ready && py_init()!=0) return -11;
  if(!py_func) return -12;
  PyObject *args=Py_BuildValue("(iii)",(int)reg,(int)isw,(int)data);
  if(!args) return -13;
  PyObject *ret=PyObject_CallObject(py_func,args); Py_DECREF(args);
  if(!ret){ PyErr_Clear(); return -14; }
  int xe=1,d=0; int ok=PyArg_ParseTuple(ret,"ii",&xe,&d); Py_DECREF(ret);
  if(!ok) return -15;
  if(xe_out) *xe_out=(uint8_t)(xe&1);
  if(d_out)  *d_out =(uint16_t)(d&0xFFFF);
  return 0;
}
#else
/* No-Python stubs for MCU */
static int  py_init(void){ return 0; }
static void py_fini(void){}
static int  py_handle_register(uint8_t r,uint8_t w,uint16_t d,uint8_t *xe,uint16_t *do_){
  (void)r;(void)w;(void)d;(void)xe;(void)do_; return -10;
}
#endif

/* ======================================================================= */
/* =========================  PURE-C IMPLEMENTATION  ====================== */
/* ======================================================================= */
/* Always compiled (needed on MCU and as fallback on desktop) */
#define BUSY_BIT 0x0002
static uint16_t st_StatusF=0, st_StatusW=0, st_SRQ_MASK=0;
static uint16_t st_ResEna=0, st_MCB=0;

/* 9.6 channel / grid */
static uint16_t st_CHANNEL   = 0;      /* low word */
static uint16_t st_CHANNELH  = 0;      /* high word (new: 0x65) */
/*
 * GRID register (0x34) units: GHz×10 (i.e., 0.1 GHz steps).
 * For a 50 GHz grid, GRID must be 500 (500 × 0.1 GHz = 50.0 GHz).
 */
static int16_t  st_GRID      = 500;    /* default to 50 GHz grid */
static int16_t  st_GRID2     = 0;      /* fine grid part in MHz, signed (new: 0x66) */

/* General/EA/DL */

static uint16_t st_GenCfg=0, st_AEA_EAC=0, st_AEA_EA=0, st_IOCap=0, st_EAC=0, st_EA=0, st_DLConfig=0, st_DLStatus=0;
/* Thresholds & masks */
static uint16_t st_FPowTh=0, st_WPowTh=0, st_FFreqTh=0, st_WFreqTh=0, st_FThermTh=0, st_WThermTh=0, st_FatalT=0, st_ALMT=0;
/* Optical */
static uint16_t st_PWR=0, st_OOP=0; static int16_t st_CTemp=2500;
/* ------------------------------------------------------------------
 * Manufacturer-specific R/W extensions (0x8C–0x91)
 *
 * Tuners are represented as fixed-point milli-units (value * 1000):
 *   e.g. 12.12  -> 12120
 *        9.99   ->  9990
 *        5.550  ->  5550
 *
 * SOA is centi-units (value * 100) to match existing debug scaling.
 * Bias/TEC are raw signed 16-bit values unless/until the real MSA map
 * specifies a scaling.
 * ------------------------------------------------------------------ */
static uint16_t st_TUNER_PHASE_milli = 0;  /* 0x8C */
static uint16_t st_TUNER_RING1_milli = 0;  /* 0x8D */
static uint16_t st_TUNER_RING2_milli = 0;  /* 0x8E */

static uint16_t st_SOA_centi  = 0;         /* 0x8F */
static int16_t  st_BIAS_raw   = 0;         /* 0x90 */
static int16_t  st_TEC_raw    = 0;         /* 0x91 */
/* FCF (First Channel Frequency)
 * Default base frequency must be 192.950000 THz (i.e., 192950.000 GHz)
 * Mapping per spec:
 *   FCF1 (0x35) = THz
 *   FCF2 (0x36) = GHz * 10  (0.1 GHz steps)
 *   FCF3 (0x67) = MHz (fine part)
 */
static uint16_t st_FCF1_THz = 193;
static int16_t  st_FCF2_G10 = 4500;   /* 450.0 GHz * 10 */
static int16_t  st_FCF3_MHz = 0;
/* Capabilities */
static uint16_t st_FTFR_MHz=5000; static int16_t st_OPSL=0, st_OPSH=2000; static uint16_t st_LGrid10=250;
static int16_t st_FTF_MHz = 0;   /* Fine Tune Frequency (signed), units: MHz (±12.5 GHz => ±12500 MHz) */

#define CAP_NUM_CHANNELS LUT_NUM_CHANNELS_50GHZ
/* Identity strings */
static const char s_devtyp[] ="CW ITLA";
static const char s_mfgr[]  ="Pilot Photonics";
static const char s_model[] ="NYITLA-01";
static const char s_serno[] ="PP-000123";
static const char s_mfgdate[]="2025-08-27";
static const char s_release[]="1.0.0";
static const char s_relback[]="1.0.0";

/* EA buffer */
static const uint8_t *ea_buf=NULL; static size_t ea_len=0; static size_t aea_idx=0, man_idx=0;
#define EAC_INC_ON_READ  0x0001
#define EAC_INC_ON_WRITE 0x0002

static void ea_set(const char *s){ ea_buf=(const uint8_t*)s; ea_len=strlen(s)+1; aea_idx=man_idx=0; st_AEA_EA=st_EA=0; }
static void set_busy(int on){ if(on) st_StatusW|=BUSY_BIT; else st_StatusW&=(uint16_t)~BUSY_BIT; }
static int pending_cnt=0;
static void tick_pending(void){ if(pending_cnt>0){ if(--pending_cnt==0) set_busy(0); } }

static int c_get_pending(void){ return (st_StatusW & BUSY_BIT)?1:0; }
static int c_get_srq(void){ return ((st_StatusF|st_StatusW) & st_SRQ_MASK)!=0; }

/* ------------------------------------------------------------------
 * Compute optical frequency as fixed-point GHz×1e4 (0.0001 GHz = 100 kHz)
 *
 * Freq = FCF + (Laser_Channel-1) * GRID + FTF
 *
 * Where:
 *   Laser_Channel = ChannelH:Channel (32-bit)
 *   FCF1_THz   : THz
 *   FCF2_G10   : GHz×10
 *   FCF3_MHz   : MHz (signed)
 *   GRID       : GHz×10
 *   GRID2      : MHz (signed)
 *   FTF_MHz    : MHz (signed)
 *
 * All computation is performed in units of GHz×1e4.
 * ------------------------------------------------------------------ */
static uint32_t freq_gx1e4(void)
{
    /* Channel index (spec: ChannelH * 65536 + Channel) */
    uint32_t laser_channel =
        ((uint32_t)st_CHANNELH << 16) | (uint32_t)st_CHANNEL;

    /* GRID spacing in GHz×1e4:
     *   GRID  is GHz×10  -> multiply by 1000
     *   GRID2 is MHz     -> 1 MHz = 0.001 GHz -> ×1e4 => 10
     */
    int32_t grid_gx1e4 =
        (int32_t)st_GRID * 1000 +
        (int32_t)st_GRID2 * 10;

    /* Base FCF in GHz×1e4:
     *   FCF1_THz  THz -> GHz×1e4 = THz * 1000 * 10000
     *   FCF2_G10  GHz×10 -> GHz×1e4 = *1000
     *   FCF3_MHz  MHz -> GHz×1e4 = *10
     */
    int64_t base_gx1e4 =
        (int64_t)st_FCF1_THz * 1000LL * 10000LL +
        (int64_t)st_FCF2_G10 * 1000LL +
        (int64_t)st_FCF3_MHz * 10LL;

    /* Fine tune in GHz×1e4 (MHz -> *10) */
    int64_t ftf_gx1e4 = (int64_t)st_FTF_MHz * 10LL;

    /* Channel offset (Laser_Channel is 1-based) */
    int64_t chan_offset_gx1e4 = 0;
    if (laser_channel >= 1) {
        chan_offset_gx1e4 = (int64_t)((int64_t)laser_channel - 1LL) * (int64_t)grid_gx1e4;
    }

    int64_t total_gx1e4 = base_gx1e4 + chan_offset_gx1e4 + ftf_gx1e4;
    if (total_gx1e4 < 0) total_gx1e4 = 0;

    return (uint32_t)total_gx1e4;
}


static uint16_t lf1_thz(void){
    uint32_t f = freq_gx1e4();
    /* THz = GHz / 1000, and f is GHz×1e4 */
    return (uint16_t)(f / (1000u * 10000u));
}

static uint16_t lf2_g10(void){
    uint32_t f = freq_gx1e4();
    /* remainder within the current THz, in GHz×1e4 */
    uint32_t rem = f % (1000u * 10000u);
    /* Convert to GHz×10 (0.1 GHz steps): 0.1 GHz = 1000 in GHz×1e4 */
    return (uint16_t)(rem / 1000u);
}
static uint16_t lf3_mhz(void){
    uint32_t f = freq_gx1e4();
    /* remainder within the current THz, in GHz×1e4 */
    uint32_t rem = f % (1000u * 10000u);
    /*
     * LF2 reports GHz×10 (0.1 GHz steps). 0.1 GHz = 100 MHz.
     * The remainder smaller than 0.1 GHz lives in (rem % 1000) units of (0.0001 GHz).
     * 0.0001 GHz = 100 kHz = 0.1 MHz, so divide by 10 to convert to integer MHz.
     */
    uint32_t sub_0p1ghz = rem % 1000u;      /* 0.0001 GHz units */
    return (uint16_t)(sub_0p1ghz / 10u);    /* MHz */
}
static uint16_t lfl1_thz(void){ return st_FCF1_THz; }
static uint16_t lfl2_g10(void){ return st_FCF2_G10; }
/* ------------------------------------------------------------------
 * LASTF = FCF + (N-1)*GRID + FTF   — full spec-accurate version
 * Units returned:
 *   0x54 : THz
 *   0x55 : GHz×10
 * ------------------------------------------------------------------ */
static void last_freq_split(uint16_t *t, uint16_t *g)
{
    /* LASTF is the frequency at the last supported channel.
     * For this emulator/LUT, that is CAP_NUM_CHANNELS (1..N).
     */
    uint32_t laser_ch_last = (CAP_NUM_CHANNELS > 0) ? (uint32_t)CAP_NUM_CHANNELS : 1u;

    /* Save current channel registers */
    uint16_t save_ch  = st_CHANNEL;
    uint16_t save_chh = st_CHANNELH;

    /* Temporarily set channel to last channel (32-bit ChannelH:Channel) */
    st_CHANNEL  = (uint16_t)(laser_ch_last & 0xFFFFu);
    st_CHANNELH = (uint16_t)((laser_ch_last >> 16) & 0xFFFFu);

    /* Compute LASTF using the same fixed-point path as LF */
    uint32_t last_gx1e4 = freq_gx1e4();

    /* Restore channel registers */
    st_CHANNEL  = save_ch;
    st_CHANNELH = save_chh;

    /* Split LASTF into THz + GHz×10 */
    if (t) *t = (uint16_t)(last_gx1e4 / (1000u * 10000u));
    if (g) {
        uint32_t rem = last_gx1e4 % (1000u * 10000u);
        *g = (uint16_t)(rem / 1000u);
    }
}


static int c_handle_register(uint8_t reg,uint8_t isw,uint16_t data,uint8_t *xe_out,uint16_t *d_out){
  tick_pending();
  uint8_t xe=0; uint16_t d=0;

  if(isw && reg>=0x01 && reg<=0x07){ xe=1; d=0; g_last_error=LERR_RNW; goto done; }

  switch(reg){
    case 0x00: xe=0; d=0; break;
    case 0x01: if(!isw){ ea_set(s_devtyp);  d=(uint16_t)ea_len; } else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x02: if(!isw){ ea_set(s_mfgr);    d=(uint16_t)ea_len; } else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x03: if(!isw){ ea_set(s_model);   d=(uint16_t)ea_len; } else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x04: if(!isw){ ea_set(s_serno);   d=(uint16_t)ea_len; } else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x05: if(!isw){ ea_set(s_mfgdate); d=(uint16_t)ea_len; } else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x06: if(!isw){ ea_set(s_release); d=(uint16_t)ea_len; } else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x07: if(!isw){ ea_set(s_relback); d=(uint16_t)ea_len; } else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x08: if(isw){ st_GenCfg=data; } else d=st_GenCfg; break;
    case 0x09: if(isw){ st_AEA_EAC=data; } else d=st_AEA_EAC; break;
    case 0x0A: if(isw){ st_AEA_EA=data; aea_idx=(size_t)(st_AEA_EA&0xFFFF); if(aea_idx>ea_len) aea_idx=ea_len; } else d=st_AEA_EA; break;
    case 0x0B:
      if(isw){ xe=1; g_last_error=LERR_ERO; break; }
      if(!ea_buf || aea_idx>=ea_len){ xe=1; d=0; g_last_error=LERR_ERE; break; }
      { uint8_t hi=ea_buf[aea_idx], lo=(aea_idx+1<ea_len)?ea_buf[aea_idx+1]:0;
        d=(uint16_t)((hi<<8)|lo); if(st_AEA_EAC & EAC_INC_ON_READ){ aea_idx+=2; st_AEA_EA=(uint16_t)aea_idx; } }
      break;
    case 0x0C: goto not_impl;
    case 0x0D: if(isw){ st_IOCap=data; } else d=st_IOCap; break;
    case 0x0E: if(isw){ st_EAC=data; } else d=st_EAC; break;
    case 0x0F:
      if(isw){ st_EA=data; man_idx=(size_t)(st_EA&0xFFFF); if(man_idx>ea_len) man_idx=ea_len;
               if(st_EAC & EAC_INC_ON_WRITE){ man_idx+=2; st_EA=(uint16_t)man_idx; } }
      else d=st_EA; break;
    case 0x10:
      if(isw){ xe=1; g_last_error=LERR_ERO; break; }
      if(!ea_buf || man_idx>=ea_len){ xe=1; d=0; g_last_error=LERR_ERE; break; }
      { uint8_t hi=ea_buf[man_idx], lo=(man_idx+1<ea_len)?ea_buf[man_idx+1]:0;
        d=(uint16_t)((hi<<8)|lo); if(st_EAC & EAC_INC_ON_READ){ man_idx+=2; st_EA=(uint16_t)man_idx; } }
      break;
    case 0x11: case 0x12: case 0x13: goto not_impl;
    case 0x14: if(isw){ st_DLConfig=data; } else d=st_DLConfig; break;
    case 0x15: if(isw){ xe=1; g_last_error=LERR_RNW; } else d=st_DLStatus; break;
    case 0x20: if(isw){ st_StatusF=data; } else d=st_StatusF; break;
    case 0x21:
      if (isw) {
        /* Never allow host to set/clear BUSY; BUSY is controlled internally via set_busy()/tick_pending(). */
        uint16_t keep_busy = (uint16_t)(st_StatusW & BUSY_BIT);
        st_StatusW = (uint16_t)((data & (uint16_t)~BUSY_BIT) | keep_busy);
      } else {
        d = st_StatusW;
      }
      break;
    case 0x22: if(isw){ st_FPowTh=data; } else d=st_FPowTh; break;
    case 0x23: if(isw){ st_WPowTh=data; } else d=st_WPowTh; break;
    case 0x24: if(isw){ st_FFreqTh=data; } else d=st_FFreqTh; break;
    case 0x25: if(isw){ st_WFreqTh=data; } else d=st_WFreqTh; break;
    case 0x26: if(isw){ st_FThermTh=data; } else d=st_FThermTh; break;
    case 0x27: if(isw){ st_WThermTh=data; } else d=st_WThermTh; break;
    case 0x28: if(isw){ st_SRQ_MASK=data; } else d=st_SRQ_MASK; break;
    case 0x29: if(isw){ st_FatalT=data; } else d=st_FatalT; break;
    case 0x2A:
      if (isw) { st_ALMT = data; } else d = st_ALMT;
      break;
    case 0x30:
      if (isw) {
        /* Full 32-bit Laser_Channel = ChannelH:Channel */
        uint32_t full_ch = ((uint32_t)st_CHANNELH << 16) | (uint32_t)data;

        /* For this emulator/LUT we only support 1..LUT_NUM_CHANNELS_50GHZ */
        if (full_ch < 1 || full_ch > LUT_NUM_CHANNELS_50GHZ) {
          xe = 1;
          d = 0;
          g_last_error = LERR_RVE;
        } else {
          st_CHANNEL = data;
          nano_apply_channel_from_lut((uint16_t)full_ch);

          /* Simulate tuning time; host polls NOP (0x00) until pending clears */
          set_busy(1);
          pending_cnt = 3;
        }
      } else {
        d = st_CHANNEL;
      }
      break;


    case 0x31: if(isw){ st_PWR=data; } else d=st_PWR; break;
    case 0x32: if(isw){ st_ResEna=data; } else d=st_ResEna; break;
    case 0x33: if(isw){ st_MCB=data; } else d=st_MCB; break;
    case 0x34:
      /* GRID (coarse grid) — units: GHz×10 (0.1 GHz). Common values:
       *   25  =>  2.5 GHz
       *   500 => 50.0 GHz (expected for 50 GHz ITU grid)
       */
      if (isw) {
        if (data != 25 && data != 500) {
          xe = 1;
          g_last_error = LERR_RVE;
        } else {
          st_GRID = (int16_t)data;
        }
      } else {
        d = (uint16_t)st_GRID;
      }
      break;
    case 0x35: if(isw){ st_FCF1_THz=data; } else d=st_FCF1_THz; break;
    case 0x36: if(isw){ st_FCF2_G10=data; } else d=st_FCF2_G10; break;
    case 0x40: if(!isw) d=lf1_thz(); else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x41: if(!isw) d=lf2_g10(); else { xe=1; g_last_error=LERR_RNW; } break;
  /* ------------------------------------------------------------------
   * 0x68 — LF3 (fine frequency part)
   *
   * LF1 (0x40) returns THz.
   * LF2 (0x41) returns GHz×10 (0.1 GHz steps).
   * LF3 (0x68) returns the remaining fine part in MHz (0..99 for a 0.1 GHz bucket).
   *
   * Read-only: writes return RNW.
   * ------------------------------------------------------------------ */
    case 0x68:
      if (!isw) {
        d = lf3_mhz();
      } else {
        xe = 1;
        g_last_error = LERR_RNW;
      }
      break;
    case 0x42: if(!isw) d= (st_OOP?st_OOP:st_PWR); else st_OOP=data; break;
    case 0x43: if(!isw) d=(uint16_t)st_CTemp; else st_CTemp=(int16_t)data; break;
    case 0x4F: if(!isw) d=st_FTFR_MHz; else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x50: if(!isw) d=(uint16_t)st_OPSL; else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x51: if(!isw) d=(uint16_t)st_OPSH; else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x52: if(!isw) d=lfl1_thz(); else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x53: if(!isw) d=lfl2_g10(); else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x54: if(!isw){ uint16_t t,g; last_freq_split(&t,&g); d=t; } else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x55: if(!isw){ uint16_t t,g; last_freq_split(&t,&g); d=g; } else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x56: if(!isw) d=st_LGrid10; else { xe=1; g_last_error=LERR_RNW; } break;
      /* ChannelH (high word of Laser_Channel) – 0x65 */
    case 0x65:
      if (isw) {
        st_CHANNELH = data;
      } else {
        d = st_CHANNELH;
      }
      break;

    /* GRID2 – fine grid part (MHz, signed) – 0x66 */
    case 0x66:
      if (isw) {
        st_GRID2 = (int16_t)data;
      } else {
        d = (uint16_t)st_GRID2;
      }
      break;

    /* FCF3 – fine part of first channel frequency (MHz, signed) – 0x67 */
    case 0x67:
      if (isw) {
        st_FCF3_MHz = (int16_t)data;
      } else {
        d = (uint16_t)st_FCF3_MHz;
      }
      break;

    /* ------------------------------------------------------------------
     * 0x62 — Fine Tune Frequency (FTF)
     * Units: MHz (signed)
     * Accept ±12.5 GHz => ±12500 MHz
     * ------------------------------------------------------------------ */
    case 0x62:
    {
        if (isw)
        {
            int32_t val = (int32_t)(int16_t)data;

            const int32_t FTF_MIN = -12500;
            const int32_t FTF_MAX = +12500;

            if (val < FTF_MIN || val > FTF_MAX)
            {
                xe = 1;
                g_last_error = LERR_RVE;
            }
            else
            {
                st_FTF_MHz = (int16_t)val;
            }
        }
        else
        {
            d = (uint16_t)st_FTF_MHz;
        }
    }
    break;

    /* ------------------------------------------------------------------
     * Manufacturer-specific LUT / PD debug window (0x80–0x88)
     *
     * These are NOT standard MSA spec registers; they live in the
     * manufacturer area (0x80–0xFE) and provide a convenient way to:
     *   - Read back the current LUT row that nano_apply_channel_from_lut()
     *     loaded into g_active_channel_50GHz / globals in nano_globals.c.
     *   - Read the main PD / wavelength monitor PDs for Jorge's dev kit.
     *
     * Scaling (MSA-style, host will divide back):
     *   - Voltages (V1..V3, Gain, SOA):  V * 100   -> centi-volts
     *   - Temperature:                   °C * 10   -> deci-°C
     *   - PDs (Power_PD, Etalon_PD, WM_PD): value * 10 (deci-units)
     *
     * Read-only: writes return RNW error.
     * ------------------------------------------------------------------ */

    case 0x80:   /* V1 */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_v1 * 100.0f);
      }
      break;

    case 0x81:   /* V2 */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_v2 * 100.0f);
      }
      break;

    case 0x82:   /* V3 */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_v3 * 100.0f);
      }
      break;

    case 0x83:   /* Gain */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_gain * 100.0f);
      }
      break;

    case 0x84:   /* SOA */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_soa * 100.0f);
      }
      break;

    case 0x85:   /* Temp */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_temp * 10.0f);   /* more typical resolution */
      }
      break;

    case 0x86:   /* Power_PD (main power detector) */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_mpd * 10.0f);
      }
      break;

    case 0x87:   /* Etalon_PD */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_wlpd * 10.0f);
      }
      break;

    case 0x88:   /* WM_PD (wavelength monitor PD) */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_wmpd * 10.0f);
      }
      break;
    /* ------------------------------------------------------------------
     * Alias / convenience PD registers (0x89–0x8B)
     *
     * William requested explicit PD readbacks:
     *   - WM_PD
     *   - ETALON_PD
     *   - Power_PD
     *
     * We already expose the same underlying values at 0x86–0x88.
     * These aliases provide dedicated addresses after 0x88 without
     * changing the existing LUT debug layout.
     *
     * Scaling matches 0x86–0x88 (value * 10).
     * Read-only: writes return RNW.
     * ------------------------------------------------------------------ */

    case 0x89:   /* WM_PD (alias of 0x88 / g_wmpd) */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_wmpd_meas * 10.0f);
      }
      break;

    case 0x8A:   /* ETALON_PD (alias of 0x87 / g_wlpd) */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_wlpd_meas * 10.0f);
      }
      break;

    case 0x8B:   /* Power_PD (alias of 0x86 / g_mpd) */
      if (isw) {
        xe = 1;
        g_last_error = LERR_RNW;
      } else {
        d = (uint16_t)(g_mpd_meas * 10.0f);
      }
      break;

    /* ------------------------------------------------------------------
     * R/W extensions requested for MSA code (0x8C–0x91)
     * ------------------------------------------------------------------ */

    case 0x8C:   /* PHASE tuner (milli-units) */
      if (isw) { //3.3
        st_TUNER_PHASE_milli = data;
				g_v3 = data
      } else {
        d = st_TUNER_PHASE_milli;
      }
      break;

    case 0x8D:   /* RING1 tuner (milli-units) */
      if (isw) {
        st_TUNER_RING1_milli = data;
				g_v1
      } else {
        d = st_TUNER_RING1_milli;
      }
      break;

    case 0x8E:   /* RING2 tuner (milli-units) */
      if (isw) {
        st_TUNER_RING2_milli = data;
				g_v2
      } else {
        d = st_TUNER_RING2_milli;
      }
      break;

    case 0x8F:   /* SOA (centi-units, matches existing debug scaling) */
      if (isw) {
        st_SOA_centi = data;
				g_soa
      } else {
        d = st_SOA_centi;
      }
      break;

    case 0x90:   /*Gain Bias (raw signed 16-bit) */
      if (isw) {
        st_BIAS_raw = (int16_t)data;
				SetGain
      } else {
        d = (uint16_t)st_BIAS_raw;
      }
      break;

    case 0x91:   /* TEC (raw signed 16-bit) */
      if (isw) {
        st_TEC_raw = (int16_t)data;
				g_temp =data
      } else {
        d = (uint16_t)st_TEC_raw;
      }
      break;
    case 0x92:   /* Mode switch register: direct control enable */
      if (isw) {
        /* Enable only on ASCII "AB" (0x4142) or "CD" (0x4344); disable on 0 */
        if (data == 0x0000) {
          g_direct_ctrl_mode = 0;
        } else if (data == 0x4142 || data == 0x9036) {
          g_direct_ctrl_mode = 1;
        }
      } else {
        d = (uint16_t)g_direct_ctrl_mode;
      }
      break;
    default: goto not_impl;
  }
  goto done;
not_impl:
  xe=1; d=0; g_last_error=LERR_RNI;
done:
  if(!xe) g_last_error=LERR_NONE;
  if(xe_out) *xe_out=xe;
  if(d_out)  *d_out=d;
  return 0;
}

/* ======================================================================= */
/* ============================ DISPATCH LAYER ============================ */
/* ======================================================================= */
typedef struct { uint8_t ce,status,reg; uint16_t data; } ResponseFields;

static int py_get_srq(int *out){
#if ITLA_EMBED_PY
  if(!use_python){ if(out)*out=c_get_srq(); return 0; }
  if(!py_ready && py_init()!=0){ if(out)*out=c_get_srq(); return 0; }
  PyObject *fn=PyObject_GetAttrString(py_module,"srq_asserted");
  if(!fn || !PyCallable_Check(fn)){ Py_XDECREF(fn); if(out)*out=c_get_srq(); return 0; }
  PyObject *ret=PyObject_CallObject(fn,NULL); Py_DECREF(fn);
  if(!ret){ if(out)*out=c_get_srq(); return 0; }
  long v=PyLong_AsLong(ret); Py_DECREF(ret);
  if(out)*out=(int)(v!=0); return 0;
#else
  if(out)*out=c_get_srq(); return 0;
#endif
}
static int py_get_pending(int *out){
#if ITLA_EMBED_PY
  if(!use_python){ if(out)*out=c_get_pending(); return 0; }
  if(!py_ready && py_init()!=0){ if(out)*out=c_get_pending(); return 0; }
  PyObject *fn=PyObject_GetAttrString(py_module,"pending_count");
  if(!fn || !PyCallable_Check(fn)){ Py_XDECREF(fn); if(out)*out=c_get_pending(); return 0; }
  PyObject *ret=PyObject_CallObject(fn,NULL); Py_DECREF(fn);
  if(!ret){ if(out)*out=c_get_pending(); return 0; }
  long v=PyLong_AsLong(ret); Py_DECREF(ret);
  if(out)*out=(int)((v<0)?0:v); return 0;
#else
  if(out)*out=c_get_pending(); return 0;
#endif
}
static int py_get_last_error(int *out){
#if ITLA_EMBED_PY
  if(!use_python){ if(out)*out=g_last_error&0x0F; return 0; }
  if(!py_ready && py_init()!=0){ if(out)*out=g_last_error&0x0F; return 0; }
  PyObject *fn=PyObject_GetAttrString(py_module,"last_error_code");
  if(!fn || !PyCallable_Check(fn)){ Py_XDECREF(fn); if(out)*out=g_last_error&0x0F; return 0; }
  PyObject *ret=PyObject_CallObject(fn,NULL); Py_DECREF(fn);
  if(!ret){ if(out)*out=g_last_error&0x0F; return 0; }
  long v=PyLong_AsLong(ret); Py_DECREF(ret);
  if(out)*out=(int)(v&0x0F); return 0;
#else
  if(out)*out=g_last_error&0x0F; return 0;
#endif
}

static void handle_register_and_build_response(const InboundFields *in, ResponseFields *out){
  memset(out,0,sizeof(*out)); out->reg=in->reg;

  if (!in->is_write && in->reg == 0x00) {
    /* IMPORTANT:
     * Hosts poll NOP while waiting for tuning to complete.
     * We must advance the pending/busy state even in this fast-path.
     */
    tick_pending();

    int srq = 0, pending = 0;

    py_get_srq(&srq);
    py_get_pending(&pending);

    uint16_t data = 0;
    data |= (uint16_t)(g_last_error & 0x0F);     /* low nibble = last error */
    if (pending > 0) data |= 0x0100;            /* bit 8 = pending operation */
    if (srq)          data |= 0x8000;           /* bit 15 = SRQ */

    out->ce    = 0;
    out->status= STAT_OK;
    out->data  = data;
    g_last_error = LERR_NONE;
    return;
  }


  uint8_t status=STAT_OK; uint16_t data=0; uint8_t xe=0;
  int pyrc=-999;

#if ITLA_EMBED_PY
  if(use_python){
    pyrc = py_handle_register(in->reg,in->is_write,in->data,&xe,&data);
    if(pyrc==0){
      status = xe?STAT_XE:STAT_OK;
      if(xe){ int lec=LERR_RNI; if(py_get_last_error(&lec)==0) g_last_error=(uint8_t)(lec&0x0F); else g_last_error=LERR_RNI; }
      else g_last_error=LERR_NONE;
    } else {
      g_last_error=LERR_PYFAIL;
      c_handle_register(in->reg,in->is_write,in->data,&xe,&data);
      status = xe?STAT_XE:STAT_OK;
    }
  } else
#endif
  {
    c_handle_register(in->reg,in->is_write,in->data,&xe,&data);
    status = xe?STAT_XE:STAT_OK;
  }

  if(!in->is_write && (in->reg>=0x01 && in->reg<=0x07) && status==STAT_OK) status=STAT_AEA;
  if((st_StatusW & BUSY_BIT) && status==STAT_OK) status=STAT_CP;

  out->ce=0; out->status=status; out->data=data;
}

/* ---------- Public API (ctypes-friendly) ---------- */
#ifdef __cplusplus
extern "C" {
#endif

ITLA_API void itla_use_python(int on){ itla_set_use_python(on?1:0); }
ITLA_API int  itla_is_python(void){ return itla_get_use_python(); }
ITLA_API int  itla_py_init(void){ return py_init(); }
ITLA_API void itla_py_fini(void){ py_fini(); }
ITLA_API uint8_t itla_get_direct_ctrl_mode(void){ return g_direct_ctrl_mode; }

ITLA_API uint32_t itla_process(uint8_t lstRsp,uint8_t isw,uint8_t reg,uint16_t data,
  uint8_t *out_ce,uint8_t *out_status,uint8_t *out_reg,uint16_t *out_data,uint32_t *out_in_frame)
{
  uint32_t in_frame = build_inbound_frame(lstRsp&1,isw&1,reg,data);
  InboundFields in_parsed; uint8_t ce_in=0;
  if(parse_inbound_frame(in_frame,&in_parsed,&ce_in)!=0){
    if(out_ce)*out_ce=1; if(out_status)*out_status=STAT_OK; if(out_reg)*out_reg=reg; if(out_data)*out_data=0; if(out_in_frame)*out_in_frame=in_frame;
    return 0;
  }
  ResponseFields resp={0}; resp.ce=ce_in; handle_register_and_build_response(&in_parsed,&resp);
  uint8_t xe_bit=(resp.status==STAT_XE)?1:0;
  uint32_t out_frame = build_outbound_frame_spec(resp.ce,xe_bit,resp.reg,resp.data);
  if(out_ce)*out_ce=resp.ce; if(out_status)*out_status=resp.status; if(out_reg)*out_reg=resp.reg; if(out_data)*out_data=resp.data; if(out_in_frame)*out_in_frame=in_frame;
  return out_frame;
}

ITLA_API uint32_t itla_handle_frame(uint32_t in_frame,
  uint8_t *out_ce,uint8_t *out_status,uint8_t *out_reg,uint16_t *out_data)
{
  InboundFields in_parsed; uint8_t ce_in=0;
  if(parse_inbound_frame(in_frame,&in_parsed,&ce_in)!=0){
    if(out_ce)*out_ce=1; if(out_status)*out_status=STAT_OK; if(out_reg)*out_reg=0; if(out_data)*out_data=0;
    return build_outbound_frame_spec(1,0,0,0);
  }
  ResponseFields resp={0}; resp.ce=ce_in; handle_register_and_build_response(&in_parsed,&resp);
  uint8_t xe_bit=(resp.status==STAT_XE)?1:0;
  uint32_t out_frame = build_outbound_frame_spec(resp.ce,xe_bit,resp.reg,resp.data);
  if(out_ce)*out_ce=resp.ce; if(out_status)*out_status=resp.status; if(out_reg)*out_reg=resp.reg; if(out_data)*out_data=resp.data;
  return out_frame;
}

ITLA_API uint32_t itla_process_frame(uint32_t in_frame,
  uint8_t *out_ce,uint8_t *out_status,uint8_t *out_reg,uint16_t *out_data)
{
  return itla_handle_frame(in_frame,out_ce,out_status,out_reg,out_data);
}

#ifdef __cplusplus
}
#endif
