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
static int16_t  st_GRID      = 25;     /* coarse grid, signed (e.g. 25 / 50) */
static int16_t  st_GRID2     = 0;      /* fine grid, signed (new: 0x66) */

/* General/EA/DL */

static uint16_t st_GenCfg=0, st_AEA_EAC=0, st_AEA_EA=0, st_IOCap=0, st_EAC=0, st_EA=0, st_DLConfig=0, st_DLStatus=0;
/* Thresholds & masks */
static uint16_t st_FPowTh=0, st_WPowTh=0, st_FFreqTh=0, st_WFreqTh=0, st_FThermTh=0, st_WThermTh=0, st_FatalT=0, st_ALMT=0;
/* Optical */
static uint16_t st_PWR=0, st_OOP=0; static int16_t st_CTemp=2500;
/* FCF */
static uint16_t st_FCF1_THz=193;
static int16_t  st_FCF2_G10=1000;
static int16_t  st_FCF3_MHz=0;   /* new: 0x67, signed MHz */
/* Capabilities */
static uint16_t st_FTFR_MHz=5000; static int16_t st_OPSL=0, st_OPSH=2000; static uint16_t st_LGrid10=250;
static int16_t st_FTF = 0;   /* Fine Tune Frequency (signed), units: MHz × 10^-3 */

#define CAP_NUM_CHANNELS 200
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
 * Compute optical frequency in GHz (integer floor)
 *
 * Freq = FCF + (Laser_Channel-1) * GRID + FTF
 *
 * Where:
 *   - FCF1_THz   : THz
 *   - FCF2_G10   : GHz×10
 *   - FCF3_MHz   : MHz
 *   - GRID       : GHz×10
 *   - GRID2      : MHz
 *   - FTF        : milli-MHz (0.001 MHz)
 *
 * All computation is performed in units of GHz×1e4 (0.0001 GHz)
 * ------------------------------------------------------------------ */
static uint32_t freq_ghz(void)
{
    /* Channel index */
    uint32_t laser_channel =
        ((uint32_t)st_CHANNELH << 16) | (uint32_t)st_CHANNEL;

    /* Convert GRID to GHz×1e4 */
    int32_t grid_g10 = (int32_t)st_GRID;       /* GHz×10 */
    int32_t grid_mhz = (int32_t)st_GRID2;      /* MHz    */
    int32_t grid_gx1e4 =
        grid_g10 * 1000 +          /* GHz×10 → ×1000 → GHz×1e4 */
        grid_mhz * 10;             /* MHz → ×10 → GHz×1e4 */

    /* Convert FCF to GHz×1e4 */
    int32_t base_gx1e4 =
        (int32_t)st_FCF1_THz * 1000 * 10000 +   /* THz → GHz×1e4 */
        (int32_t)st_FCF2_G10 * 1000 +           /* GHz×10 → GHz×1e4 */
        (int32_t)st_FCF3_MHz * 10;              /* MHz → GHz×1e4 */

    /* Fine tune FTF (milli-MHz → GHz×1e4) */
    int32_t ftf_gx1e4 = (int32_t)st_FTF * 10 / 1000;

    /* Channel offset */
    int32_t chan_offset_gx1e4 =
        ((int32_t)laser_channel - 1) * grid_gx1e4;

    int32_t total_gx1e4 =
        base_gx1e4 + chan_offset_gx1e4 + ftf_gx1e4;

    if (total_gx1e4 < 0)
        total_gx1e4 = 0;

    return (uint32_t)(total_gx1e4 / 10000);   /* return GHz (floor) */
}



static uint16_t lf1_thz(void){ return (uint16_t)(freq_ghz()/1000u); }
static uint16_t lf2_g10(void){ uint32_t ghz=freq_ghz(); return (uint16_t)((ghz%1000u)*10u); }
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
    /* Number of channels in laser */
    uint32_t N = (CAP_NUM_CHANNELS > 0) ? CAP_NUM_CHANNELS : 1;

    /* Compute LASTF in GHz using full freq model */
    uint32_t last_ghz;

    {
        uint32_t laser_ch_last = N;  /* last channel index */

        /* Compute full frequency model for last channel */
        uint32_t last_save_ch     = st_CHANNEL;
        uint32_t last_save_chh    = st_CHANNELH;

        /* Temporarily pretend we are on last channel */
        st_CHANNEL  = (laser_ch_last & 0xFFFF);
        st_CHANNELH = ((laser_ch_last >> 16) & 0xFFFF);

        last_ghz = freq_ghz();

        /* Restore */
        st_CHANNEL  = last_save_ch;
        st_CHANNELH = last_save_chh;
    }

    /* Split LASTF into THz + GHz×10 */
    if (t) *t = (uint16_t)(last_ghz / 1000);       /* THz        */
    if (g) *g = (uint16_t)((last_ghz % 1000) * 10); /* GHz × 10   */
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
    case 0x21: if(isw){ st_StatusW=data; } else d=st_StatusW; break;
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
         if (data > 9999) {
           xe = 1;
           g_last_error = LERR_RVE;
         } else {
           st_CHANNEL = data;
					 nano_apply_channel_from_lut(st_CHANNEL);
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
      if(isw){ if(data!=25 && data!=50){ xe=1; g_last_error=LERR_RVE; } else st_GRID=data; }
      else d=st_GRID; break;
    case 0x35: if(isw){ st_FCF1_THz=data; } else d=st_FCF1_THz; break;
    case 0x36: if(isw){ st_FCF2_G10=data; } else d=st_FCF2_G10; break;
    case 0x40: if(!isw) d=lf1_thz(); else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x41: if(!isw) d=lf2_g10(); else { xe=1; g_last_error=LERR_RNW; } break;
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
  * Units: milli-MHz (0.001 MHz = 1 kHz)
  * Spec allows very large range (typically ±12.5 GHz)
  * We accept ±12500000 milli-MHz = ±12.5 GHz
  * ------------------------------------------------------------------ */
    case 0x62:
    {
        if (isw)
        {
            int32_t val = (int32_t)(int16_t)data;

            /* ±12.5 GHz in milli-MHz units */
            const int32_t FTF_MIN = -12500000;
            const int32_t FTF_MAX = +12500000;

            if (val < FTF_MIN || val > FTF_MAX)
            {
                xe = 1;
                g_last_error = LERR_RVE;
            }
            else
            {
                st_FTF = val;   /* store raw milli-MHz */
            }
        }
        else
        {
            d = (uint16_t)st_FTF;
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
