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
static uint16_t st_StatusF=0, st_StatusW=0, st_SRQ_MASK=0x1FBFu;
static uint16_t st_LstRsp=0;   /* 0x13: last response data field */
static uint8_t st_busy=0;

/* Status register bit positions per OIF-ITLA-MSA-01.3 (Table 10.2-x). */
#define ST_FPWRL   0x0001
#define ST_FTHERML 0x0002
#define ST_FFREQL  0x0004
#define ST_FVSFL   0x0008
#define ST_CRL     0x0010
#define ST_MRL     0x0020
#define ST_CEL     0x0040
#define ST_XEL     0x0080

#define ST_FPWR    0x0100
#define ST_FTHERM  0x0200
#define ST_FFREQ   0x0400
#define ST_FVSF    0x0800
#define ST_DIS     0x1000
#define ST_FATAL   0x2000
#define ST_ALM     0x4000
#define ST_SRQ     0x8000

/* Warning status uses the same bit positions as fatal status. */
#define ST_WPWRL   ST_FPWRL
#define ST_WTHERML ST_FTHERML
#define ST_WFREQL  ST_FFREQL
#define ST_WVSFL   ST_FVSFL
#define ST_WPWR    ST_FPWR
#define ST_WTHERM  ST_FTHERM
#define ST_WFREQ   ST_FFREQ
#define ST_WVSF    ST_FVSF

static uint16_t st_comm_latched=0;   /* shared bits 7:4 (XEL/CEL/MRL/CRL) */
static uint16_t st_fatal_latched=0;  /* StatusF latched bits 3:0 */
static uint16_t st_warn_latched=0;   /* StatusW latched bits 3:0 */
static uint16_t st_ResEna=0, st_MCB=0x0002u; /* MCB default: ADT=1 per spec */

/* ResEna / MCB bit definitions (OIF-ITLA-MSA-01.3 §9.6.3/9.6.4). */
#define RESENA_MR          0x0001u
#define RESENA_SR          0x0002u
#define RESENA_SENA        0x0008u
/* Legacy compatibility seen in existing scripts/firmware variants. */
#define RESENA_SENA_LEGACY 0x0004u
#define MCB_ADT            0x0002u
#define MCB_SDF            0x0004u

/* 9.6 channel / grid */
static uint16_t st_CHANNEL   = 0;      /* low word */
static uint16_t st_CHANNELH  = 0;      /* high word (new: 0x65) */
static uint16_t st_CHANNELH_staged = 0;/* staged high word, committed on 0x30 write */
static uint8_t  st_CHANNELH_pending = 0;
/*
 * GRID register (0x34) units: GHz×10 (i.e., 0.1 GHz steps).
 * For a 50 GHz grid, GRID must be 500 (500 × 0.1 GHz = 50.0 GHz).
 */
static int16_t  st_GRID      = 500;    /* default to 50 GHz grid */
static int16_t  st_GRID2     = 0;      /* fine grid part in MHz, signed (new: 0x66) */

/* General/EA/DL */

static uint16_t st_GenCfg=0, st_AEA_EAC=0, st_AEA_EA=0, st_IOCap=0, st_EAC=0, st_EA=0, st_DLConfig=0, st_DLStatus=0;
/* 9.4 download control/status state. */
static uint8_t st_dl_busy=0, st_dl_done=0, st_dl_fail=0, st_dl_abort=0, st_dl_backend_active=0;
static uint8_t st_dl_ticks=0;
/* Thresholds & masks */
static uint16_t st_FPowTh=0, st_WPowTh=0, st_FFreqTh=0, st_WFreqTh=0, st_FThermTh=0, st_WThermTh=0;
static uint16_t st_FatalT=0x000Fu, st_ALMT=0x0D0Du;
static uint16_t st_FFreqTh2=0, st_WFreqTh2=0; /* optional high-resolution frequency thresholds */
/* Optical */
static uint16_t st_PWR=0, st_OOP=0; static int16_t st_CTemp=2500;
/* ------------------------------------------------------------------
 * Manufacturer-specific R/W extensions (0x8C–0x91)
 *
 * Tuners, SOA, and Gain Bias are all centi-units (value * 100):
 *   e.g. 1.23 V -> 123
 *        9.99 V ->  999
 *        5.50 V ->  550
 *
 * TEC is a raw signed 16-bit value (no scaling) unless/until the real
 * MSA map specifies a scaling.
 * ------------------------------------------------------------------ */
static uint16_t st_TUNER_PHASE_centi = 0;  /* 0x8C */
static uint16_t st_TUNER_RING1_centi = 0;  /* 0x8D */
static uint16_t st_TUNER_RING2_centi = 0;  /* 0x8E */

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
static uint16_t st_FCF1_THz = 191;
static int16_t  st_FCF2_G10 = 5000;   /* 500.0 GHz×10 → FCF = 191.500 THz (ITU C-band ch1) */
static int16_t  st_FCF3_MHz = 0;
/* 9.7 Module capabilities (backend-wired, with safe defaults). */
typedef struct {
  uint16_t ftfr_mhz;   /* 0x4F */
  int16_t  opsl;       /* 0x50 */
  int16_t  opsh;       /* 0x51 */
  uint16_t lgrid10;    /* 0x56 */
  int16_t  lgrid2_mhz; /* 0x6B */
} ItlaCaps97;

#define ITLA_CAP97_DEFAULT_FTFR_MHZ 5000u
#define ITLA_CAP97_DEFAULT_OPSL     0
#define ITLA_CAP97_DEFAULT_OPSH     2000
#define ITLA_CAP97_DEFAULT_LGRID10  500u   /* 50.0 GHz grid */
#define ITLA_CAP97_DEFAULT_LGRID2   0
/* §9.7 min/max lasing frequency (C-band ITU grid) */
#define ITLA_CAP97_DEFAULT_MINFREQ_THZ  191u
#define ITLA_CAP97_DEFAULT_MINFREQ_G10  7000u  /* 191.700 THz */
#define ITLA_CAP97_DEFAULT_MAXFREQ_THZ  196u
#define ITLA_CAP97_DEFAULT_MAXFREQ_G10  7000u  /* 196.700 THz */
#define ITLA_CAP97_DEFAULT_MINPOWER     0      /* 0.00 dBm */
#define ITLA_CAP97_DEFAULT_MAXPOWER     1300   /* 13.00 dBm */

static uint16_t st_FTFR_MHz = ITLA_CAP97_DEFAULT_FTFR_MHZ;
static int16_t  st_OPSL = ITLA_CAP97_DEFAULT_OPSL;
static int16_t  st_OPSH = ITLA_CAP97_DEFAULT_OPSH;
static uint16_t st_LGrid10 = ITLA_CAP97_DEFAULT_LGRID10;
static int16_t  st_LGrid2_MHz = ITLA_CAP97_DEFAULT_LGRID2;
/* §9.7 min/max lasing frequency capability registers */
static uint16_t st_MinFreq_THz = ITLA_CAP97_DEFAULT_MINFREQ_THZ;
static uint16_t st_MinFreq_G10 = ITLA_CAP97_DEFAULT_MINFREQ_G10;
static uint16_t st_MaxFreq_THz = ITLA_CAP97_DEFAULT_MAXFREQ_THZ;
static uint16_t st_MaxFreq_G10 = ITLA_CAP97_DEFAULT_MAXFREQ_G10;
static int16_t  st_MinPower    = ITLA_CAP97_DEFAULT_MINPOWER;
static int16_t  st_MaxPower    = ITLA_CAP97_DEFAULT_MAXPOWER;
static uint8_t  st_caps97_loaded = 0;

static void apply_caps97(const ItlaCaps97 *caps){
  if(!caps) return;
  st_FTFR_MHz = caps->ftfr_mhz;
  st_OPSL = caps->opsl;
  st_OPSH = caps->opsh;
  st_LGrid10 = caps->lgrid10;
  st_LGrid2_MHz = caps->lgrid2_mhz;
}

#if defined(__CC_ARM)
#define ITLA_WEAK __weak
#elif defined(__GNUC__) || defined(__clang__) || defined(__ARMCC_VERSION)
#define ITLA_WEAK __attribute__((weak))
#else
#define ITLA_WEAK
#endif

#define ITLA_DL_HOOK_UNAVAIL (-32768)

/* Override these hooks in board code for real firmware download execution. */
ITLA_WEAK int itla_backend_dl_start(uint16_t dl_config){
  (void)dl_config;
  return ITLA_DL_HOOK_UNAVAIL;
}
ITLA_WEAK int itla_backend_dl_poll(uint16_t *dl_status){
  (void)dl_status;
  return ITLA_DL_HOOK_UNAVAIL;
}
ITLA_WEAK int itla_backend_dl_abort(void){
  return ITLA_DL_HOOK_UNAVAIL;
}
ITLA_WEAK int itla_backend_dl_reset(void){
  return ITLA_DL_HOOK_UNAVAIL;
}
ITLA_WEAK int itla_backend_dl_write_word(uint16_t word){
  (void)word;
  return ITLA_DL_HOOK_UNAVAIL;
}

/* Override this in board code to feed real module capability values from NVM/HW. */
ITLA_WEAK int itla_backend_get_caps97(uint16_t *ftfr_mhz,
                                      int16_t *opsl,
                                      int16_t *opsh,
                                      uint16_t *lgrid10,
                                      int16_t *lgrid2_mhz){
  (void)ftfr_mhz;
  (void)opsl;
  (void)opsh;
  (void)lgrid10;
  (void)lgrid2_mhz;
  return -1;
}

static void load_caps97_once(void){
  ItlaCaps97 caps;
  int rc;
  if(st_caps97_loaded) return;
  st_caps97_loaded = 1;
  caps.ftfr_mhz = ITLA_CAP97_DEFAULT_FTFR_MHZ;
  caps.opsl = ITLA_CAP97_DEFAULT_OPSL;
  caps.opsh = ITLA_CAP97_DEFAULT_OPSH;
  caps.lgrid10 = ITLA_CAP97_DEFAULT_LGRID10;
  caps.lgrid2_mhz = ITLA_CAP97_DEFAULT_LGRID2;
  rc = itla_backend_get_caps97(&caps.ftfr_mhz,
                               &caps.opsl,
                               &caps.opsh,
                               &caps.lgrid10,
                               &caps.lgrid2_mhz);
  (void)rc;
  apply_caps97(&caps);
}
static int16_t st_FTF_MHz = 0;   /* Fine Tune Frequency (signed), units: MHz (±12.5 GHz => ±12500 MHz) */
/* 9.8 command block (0x57-0x61). */
static uint16_t st_DitherE=0, st_DitherR=100, st_DitherA=0, st_DitherF=0;
static int16_t  st_TBTFL=-500, st_TBTFH=7000; /* default -5.00 C / +70.00 C */
static uint16_t st_FAgeTh=0, st_WAgeTh=0;
static uint16_t st_Age=0;
static uint8_t  st_tbtf_violate_count=0;

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
static void set_busy(int on);
static int pending_cnt;

static void ea_set_buf(const uint8_t *buf, size_t len){
  ea_buf=buf; ea_len=len; aea_idx=man_idx=0; st_AEA_EA=st_EA=0;
}
static void ea_set(const char *s){ ea_set_buf((const uint8_t*)s, strlen(s)+1); }

static void resena_apply_write(uint16_t data)
{
  uint8_t req_mr = (uint8_t)((data & RESENA_MR) != 0u);
  uint8_t req_sr = (uint8_t)((data & RESENA_SR) != 0u);

  /* MR/SR are self-clearing bits; preserve all other fields as written. */
  st_ResEna = (uint16_t)(data & (uint16_t)~(RESENA_MR | RESENA_SR));

  if (req_mr || req_sr) {
    /* Communication reset latch is set on both SR and MR, while
     * module restart latch is set on MR.
     */
    st_comm_latched |= ST_CRL;
    if (req_mr) st_comm_latched |= ST_MRL;

    /* Soft reset behavior: clear EA/AEA pointers and configuration. */
    st_AEA_EAC = 0;
    st_AEA_EA = 0;
    st_EAC = 0;
    st_EA = 0;
    aea_idx = 0;
    man_idx = 0;

    /* Any in-progress tuning operation is aborted by reset. */
    set_busy(0);
    pending_cnt = 0;
  }
}

static void set_busy(int on){ st_busy=(uint8_t)(on?1:0); }
static int pending_cnt=0;
static void tick_pending(void){ if(pending_cnt>0){ if(--pending_cnt==0) set_busy(0); } }

/* 9.4 helper: composed status bits (implementation-defined until backend wiring). */
#define DLSTAT_BUSY  0x0001u
#define DLSTAT_DONE  0x0002u
#define DLSTAT_FAIL  0x0004u
#define DLSTAT_ABORT 0x0008u

static void dl_set_flags_from_status(uint16_t st){
  st_dl_busy  = (uint8_t)(((st & DLSTAT_BUSY)  != 0u) ? 1u : 0u);
  st_dl_done  = (uint8_t)(((st & DLSTAT_DONE)  != 0u) ? 1u : 0u);
  st_dl_fail  = (uint8_t)(((st & DLSTAT_FAIL)  != 0u) ? 1u : 0u);
  st_dl_abort = (uint8_t)(((st & DLSTAT_ABORT) != 0u) ? 1u : 0u);
}

static void dl_refresh_status(void){
  uint16_t v = 0;
  if(st_dl_busy)  v |= DLSTAT_BUSY;
  if(st_dl_done)  v |= DLSTAT_DONE;
  if(st_dl_fail)  v |= DLSTAT_FAIL;
  if(st_dl_abort) v |= DLSTAT_ABORT;
  st_DLStatus = v;
}

static void dl_tick(void){
  if(st_dl_backend_active){
    uint16_t st = 0;
    int rc = itla_backend_dl_poll(&st);
    if(rc == 0){
      dl_set_flags_from_status(st);
      dl_refresh_status();
      return;
    }
    /* Backend unexpectedly unavailable: preserve a deterministic failure state. */
    st_dl_backend_active = 0;
    st_dl_busy = 0;
    st_dl_done = 0;
    st_dl_fail = 1;
    st_dl_abort = 0;
    dl_refresh_status();
    return;
  }

  if(st_dl_busy && st_dl_ticks>0){
    st_dl_ticks--;
    if(st_dl_ticks==0){
      st_dl_busy = 0;
      st_dl_done = 1;
      st_dl_fail = 0;
      st_dl_abort = 0;
    }
  }
  dl_refresh_status();
}

static void dl_apply_config(uint16_t cfg){
  int rc;
  st_DLConfig = cfg;

  /* Reset request: clear staged download state. */
  if(cfg == 0u){
    rc = itla_backend_dl_reset();
    (void)rc;
    st_dl_backend_active = 0;
    st_dl_busy = 0;
    st_dl_done = 0;
    st_dl_fail = 0;
    st_dl_abort = 0;
    st_dl_ticks = 0;
    dl_refresh_status();
    return;
  }

  /* Abort request (bit1). */
  if((cfg & 0x0002u) != 0u){
    rc = itla_backend_dl_abort();
    if(rc == 0){
      st_dl_backend_active = 1;
      st_dl_busy = 0;
      st_dl_done = 0;
      st_dl_fail = 1;
      st_dl_abort = 1;
      st_dl_ticks = 0;
      dl_refresh_status();
      return;
    }
    st_dl_backend_active = 0;
    st_dl_busy = 0;
    st_dl_done = 0;
    st_dl_fail = 1;
    st_dl_abort = 1;
    st_dl_ticks = 0;
    dl_refresh_status();
    return;
  }

  /* Start request (bit0): use backend if available, otherwise simulate. */
  if((cfg & 0x0001u) != 0u){
    rc = itla_backend_dl_start(cfg);
    if(rc == 0){
      st_dl_backend_active = 1;
      st_dl_busy = 1;
      st_dl_done = 0;
      st_dl_fail = 0;
      st_dl_abort = 0;
      st_dl_ticks = 0;
      dl_refresh_status();
      return;
    }
    if(rc != ITLA_DL_HOOK_UNAVAIL){
      st_dl_backend_active = 0;
      st_dl_busy = 0;
      st_dl_done = 0;
      st_dl_fail = 1;
      st_dl_abort = 0;
      st_dl_ticks = 0;
      dl_refresh_status();
      return;
    }

    st_dl_backend_active = 0;
    st_dl_busy = 1;
    st_dl_done = 0;
    st_dl_fail = 0;
    st_dl_abort = 0;
    st_dl_ticks = 3;
    dl_refresh_status();
  }
}

static inline uint16_t abs_i16(int16_t v){
  return (uint16_t)((v < 0) ? -(int32_t)v : (int32_t)v);
}

static inline int16_t clamp_i16(int32_t v){
  if(v > 32767) return 32767;
  if(v < -32768) return -32768;
  return (int16_t)v;
}

static inline uint16_t freq_th_mhz(uint16_t g10, uint16_t mhz){
  return (uint16_t)(g10 * 100u + mhz);
}

static inline int resena_output_enabled(void){
  return ((st_ResEna & (RESENA_SENA | RESENA_SENA_LEGACY)) != 0u);
}

static inline int mcb_adt_enabled(void){
  return ((st_MCB & MCB_ADT) != 0u);
}

static inline int mcb_sdf_enabled(void){
  return ((st_MCB & MCB_SDF) != 0u);
}

static void status_latch_exec_error(void){ st_comm_latched |= ST_XEL; }
static void status_latch_comm_error(void){ st_comm_latched |= ST_CEL; }

/* Build StatusF/StatusW from live conditions + latched bits + trigger masks. */
static void status_refresh(void)
{
  uint16_t f_live = 0;
  uint16_t w_live = 0;
  uint8_t fatal_assert = 0;
  uint8_t output_enabled = (uint8_t)(resena_output_enabled() ? 1u : 0u);
  uint8_t adt_enabled = (uint8_t)(mcb_adt_enabled() ? 1u : 0u);
  uint8_t tuning_active = st_busy ? 1u : 0u;
  uint8_t laser_locked = (uint8_t)((output_enabled && !tuning_active) ? 1u : 0u);

  /* DIS reflects output-disable condition; SDF can force disable on FATAL. */
  if (!output_enabled) {
    f_live |= ST_DIS;
    w_live |= ST_DIS;
  }

  {
    uint16_t power_meas = st_OOP ? st_OOP : st_PWR;
    uint16_t power_err = (power_meas > st_PWR) ? (power_meas - st_PWR) : (st_PWR - power_meas);
    if (st_FPowTh && power_err > st_FPowTh) f_live |= ST_FPWR;
    if (st_WPowTh && power_err > st_WPowTh) w_live |= ST_WPWR;
  }

  {
    uint16_t fth = freq_th_mhz(st_FFreqTh, st_FFreqTh2);
    uint16_t wth = freq_th_mhz(st_WFreqTh, st_WFreqTh2);
    uint16_t ferr = abs_i16(st_FTF_MHz);
    if (fth && ferr > fth) f_live |= ST_FFREQ;
    if (wth && ferr > wth) w_live |= ST_WFREQ;
  }

  {
    /* Nominal control temperature placeholder: 25.00C encoded as 2500 (C*100). */
    const int16_t temp_ref = 2500;
    uint16_t terr = abs_i16((int16_t)(st_CTemp - temp_ref));
    if (st_FThermTh && terr > st_FThermTh) f_live |= ST_FTHERM;
    if (st_WThermTh && terr > st_WThermTh) w_live |= ST_WTHERM;
  }

  /* ADT behavior: warning frequency/power act as lock indicators. */
  if (adt_enabled) {
    if (!output_enabled || tuning_active) {
      w_live |= (uint16_t)(ST_WPWR | ST_WFREQ);
    }
  } else {
    if (!output_enabled || tuning_active) {
      w_live &= (uint16_t)~(ST_WPWR | ST_WFREQ);
    }
  }

  /* TBTF warning limits (9.8.4): assert WTHERM after persistent violation. */
  if (st_CTemp > st_TBTFH || st_CTemp < st_TBTFL) {
    if (st_tbtf_violate_count < 5) st_tbtf_violate_count++;
  } else {
    st_tbtf_violate_count = 0;
  }
  if (st_tbtf_violate_count >= 5) w_live |= ST_WTHERM;

  /* Age model (9.8.6): derive %EOL from bias-current magnitude, monotonic. */
  {
    uint16_t bias_abs = abs_i16(st_BIAS_raw);
    uint16_t derived_age = (uint16_t)(bias_abs / 100u); /* 100 raw units -> 1% EOL */
    if (derived_age > 100u) derived_age = 100u;
    if (derived_age > st_Age) st_Age = derived_age;
  }

  if (st_FAgeTh && st_Age > st_FAgeTh) f_live |= ST_FVSF;
  if (st_WAgeTh && st_Age > st_WAgeTh) w_live |= ST_WVSF;

  /* Latch low-nibble status bits when corresponding live bits assert. */
  if (f_live & ST_FPWR)   st_fatal_latched |= ST_FPWRL;
  if (f_live & ST_FTHERM) st_fatal_latched |= ST_FTHERML;
  if (f_live & ST_FFREQ)  st_fatal_latched |= ST_FFREQL;
  if (f_live & ST_FVSF)   st_fatal_latched |= ST_FVSFL;

  if (w_live & ST_WPWR)   st_warn_latched |= ST_WPWRL;
  if (w_live & ST_WTHERM) st_warn_latched |= ST_WTHERML;
  if (w_live & ST_WFREQ)  st_warn_latched |= ST_WFREQL;
  if (w_live & ST_WVSF)   st_warn_latched |= ST_WVSFL;

  st_StatusF = (uint16_t)(f_live | st_fatal_latched | st_comm_latched);
  st_StatusW = (uint16_t)(w_live | st_warn_latched  | st_comm_latched);

  {
    uint16_t fatal_eval_f = st_StatusF;
    uint16_t fatal_eval_w = st_StatusW;
    uint16_t srq_eval_f = st_StatusF;
    uint16_t srq_eval_w = st_StatusW;

    /* Section 9.5.5/9.5.6 exception: do not trigger FATAL/SRQ from
     * power/frequency/thermal faults when laser is not locked.
     */
    if (!laser_locked) {
      uint16_t suppress_f = (uint16_t)(ST_FPWR | ST_FTHERM | ST_FFREQ);
      uint16_t suppress_w = (uint16_t)(ST_WPWR | ST_WTHERM | ST_WFREQ);
      fatal_eval_f &= (uint16_t)~suppress_f;
      fatal_eval_w &= (uint16_t)~suppress_w;
      srq_eval_f &= (uint16_t)~suppress_f;
      srq_eval_w &= (uint16_t)~suppress_w;
    }

    /* FATAL bit follows FatalT trigger combination rules (Table 10.3-1). */
    fatal_assert =
      (uint8_t)((((st_FatalT & 0x0F00u) & fatal_eval_w) != 0u) ||
                ((((st_FatalT & 0x000Fu) << 8) & fatal_eval_f) != 0u) ||
                (((st_FatalT & ST_MRL) != 0u) && ((st_comm_latched & ST_MRL) != 0u)));
    if (fatal_assert) {
      st_StatusF |= ST_FATAL;
      st_StatusW |= ST_FATAL;
    }

    /* SDF forces output disable when FATAL asserts. */
    if (mcb_sdf_enabled() && fatal_assert) {
      st_StatusF |= ST_DIS;
      st_StatusW |= ST_DIS;
    }

    /* ALM bit follows ALMT trigger combination rules (Table 10.3-1). */
    if ((((st_ALMT & 0x0F00u) & st_StatusW) != 0u) ||
        ((((st_ALMT & 0x000Fu) << 8) & st_StatusF) != 0u)) {
      st_StatusF |= ST_ALM;
      st_StatusW |= ST_ALM;
    }

    /* SRQ bit follows SRQT trigger combination rules (Table 10.3-1). */
    {
      uint16_t direct_mask = (uint16_t)(ST_DIS | ST_XEL | ST_CEL | ST_MRL | ST_CRL);
      uint16_t direct_state = (uint16_t)((st_StatusF | st_StatusW) & direct_mask);
      if ((((st_SRQ_MASK & 0x0F00u) & srq_eval_w) != 0u) ||
          ((((st_SRQ_MASK & 0x000Fu) << 8) & srq_eval_f) != 0u) ||
          (((st_SRQ_MASK & direct_mask) & direct_state) != 0u)) {
        st_StatusF |= ST_SRQ;
        st_StatusW |= ST_SRQ;
      }
    }
  }
}

static void status_clear_by_host(uint8_t reg, uint16_t data)
{
  /* COW semantics: host clears latched bits by writing 1 to target bit(s). */
  st_comm_latched &= (uint16_t)~(data & (ST_XEL | ST_CEL | ST_MRL | ST_CRL));
  if (reg == 0x20) {
    st_fatal_latched &= (uint16_t)~(data & (ST_FPWRL | ST_FTHERML | ST_FFREQL | ST_FVSFL));
  } else if (reg == 0x21) {
    st_warn_latched &= (uint16_t)~(data & (ST_WPWRL | ST_WTHERML | ST_WFREQL | ST_WVSFL));
  }
}

static int c_get_pending(void){ return st_busy ? 1 : 0; }
static int c_get_srq(void){ status_refresh(); return ((st_StatusF|st_StatusW) & ST_SRQ)!=0; }

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
static uint16_t lfl3_mhz(void){
    return (uint16_t)st_FCF3_MHz;
}
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

static uint16_t lfh3_mhz(void)
{
    uint32_t laser_ch_last = (CAP_NUM_CHANNELS > 0) ? (uint32_t)CAP_NUM_CHANNELS : 1u;
    uint16_t save_ch  = st_CHANNEL;
    uint16_t save_chh = st_CHANNELH;

    st_CHANNEL  = (uint16_t)(laser_ch_last & 0xFFFFu);
    st_CHANNELH = (uint16_t)((laser_ch_last >> 16) & 0xFFFFu);

    {
        uint32_t last_gx1e4 = freq_gx1e4();
        uint32_t rem = last_gx1e4 % (1000u * 10000u);
        uint32_t sub_0p1ghz = rem % 1000u;
        uint16_t out = (uint16_t)(sub_0p1ghz / 10u);

        st_CHANNEL  = save_ch;
        st_CHANNELH = save_chh;
        return out;
    }
}

static inline void put_be16(uint8_t *dst, int16_t v){
  uint16_t u=(uint16_t)v;
  dst[0]=(uint8_t)((u>>8)&0xFF);
  dst[1]=(uint8_t)(u&0xFF);
}

/* AEA payload backing for 0x57 (Currents) and 0x58 (Temps). */
static uint8_t aea_currents_payload[8];
static uint8_t aea_temps_payload[4];

static void prep_currents_aea(void){
  /*
   * 9.8.1: return module-specific current array in mA*10.
   * We provide four signed currents (8 bytes): TEC, diode, monitor, SOA.
   */
  int16_t tec_ma10    = st_TEC_raw;
  int16_t diode_ma10  = st_BIAS_raw;
  int16_t monitor_ma10= clamp_i16((int32_t)(g_mpd_meas * 10.0f));
  int16_t soa_ma10    = clamp_i16((int32_t)(st_SOA_centi * 10) / 100);

  put_be16(&aea_currents_payload[0], tec_ma10);
  put_be16(&aea_currents_payload[2], diode_ma10);
  put_be16(&aea_currents_payload[4], monitor_ma10);
  put_be16(&aea_currents_payload[6], soa_ma10);
  ea_set_buf(aea_currents_payload, sizeof(aea_currents_payload));
}

static void prep_temps_aea(void){
  /* 9.8.2: return diode and case temperatures as signed values in C*100. */
  int16_t diode_c100 = st_CTemp;
  int16_t case_c100  = clamp_i16((int32_t)(g_temp * 100.0f));
  put_be16(&aea_temps_payload[0], diode_c100);
  put_be16(&aea_temps_payload[2], case_c100);
  ea_set_buf(aea_temps_payload, sizeof(aea_temps_payload));
}


static int c_handle_register(uint8_t reg,uint8_t isw,uint16_t data,uint8_t *xe_out,uint16_t *d_out){
  tick_pending();
  dl_tick();
  load_caps97_once();
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
    case 0x11: case 0x12: goto not_impl;
    case 0x13: if(isw){ xe=1; g_last_error=LERR_RNW; } else d=st_LstRsp; break;
    case 0x14:
      if(isw){
        dl_apply_config(data);
      } else {
        d=st_DLConfig;
      }
      break;
    case 0x15:
      if(isw){
        xe=1; g_last_error=LERR_RNW;
      } else {
        dl_refresh_status();
        d=st_DLStatus;
      }
      break;
    case 0x20:
      if(isw){
        status_clear_by_host(0x20, data);
      } else {
        status_refresh();
        d = st_StatusF;
      }
      break;
    case 0x21:
      if (isw) {
        status_clear_by_host(0x21, data);
      } else {
        status_refresh();
        d = st_StatusW;
      }
      break;
    case 0x22: if(isw){ st_FPowTh=data; } else d=st_FPowTh; break;
    case 0x23: if(isw){ st_WPowTh=data; } else d=st_WPowTh; break;
    case 0x24: if(isw){ st_FFreqTh=data; } else d=st_FFreqTh; break;
    case 0x25: if(isw){ st_WFreqTh=data; } else d=st_WFreqTh; break;
    case 0x63: if(isw){ st_FFreqTh2=data; } else d=st_FFreqTh2; break;
    case 0x64: if(isw){ st_WFreqTh2=data; } else d=st_WFreqTh2; break;
    case 0x26: if(isw){ st_FThermTh=data; } else d=st_FThermTh; break;
    case 0x27: if(isw){ st_WThermTh=data; } else d=st_WThermTh; break;
    case 0x28: if(isw){ st_SRQ_MASK=data; } else d=st_SRQ_MASK; break;
    case 0x29: if(isw){ st_FatalT=data; } else d=st_FatalT; break;
    case 0x2A:
      if (isw) { st_ALMT = data; } else d = st_ALMT;
      break;
    case 0x30:
      if (isw) {
        /* Commit ChannelH only when Channel is written (9.6.1 behavior). */
        uint16_t next_chh = st_CHANNELH_pending ? st_CHANNELH_staged : st_CHANNELH;
        uint32_t full_ch = ((uint32_t)next_chh << 16) | (uint32_t)data;

        /* For this emulator/LUT we only support 1..LUT_NUM_CHANNELS_50GHZ */
        if (full_ch < 1 || full_ch > LUT_NUM_CHANNELS_50GHZ) {
          xe = 1;
          d = 0;
          g_last_error = LERR_RVE;
        } else {
          st_CHANNELH = next_chh;
          st_CHANNEL = data;
          st_CHANNELH_pending = 0;
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
    case 0x32:
      if(isw){
        resena_apply_write(data);
      } else d=st_ResEna;
      break;
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
    case 0x69:
      if(!isw){
        d = lfl3_mhz();
      } else {
        xe = 1;
        g_last_error = LERR_RNW;
      }
      break;
    case 0x6A:
      if(!isw){
        d = lfh3_mhz();
      } else {
        xe = 1;
        g_last_error = LERR_RNW;
      }
      break;
    case 0x6B:
      if(!isw){
        d = (uint16_t)st_LGrid2_MHz;
      } else {
        xe = 1;
        g_last_error = LERR_RNW;
      }
      break;
    case 0x42: if(!isw) d=st_MinFreq_THz; else { xe=1; g_last_error=LERR_RNW; } break; /* LF1Min – min lasing freq THz */
    case 0x43: if(!isw) d=st_MaxFreq_THz; else { xe=1; g_last_error=LERR_RNW; } break; /* LF1Max – max lasing freq THz */
    case 0x4F: if(!isw) d=st_FTFR_MHz;    else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x50: if(!isw) d=st_MinFreq_THz; else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x51: if(!isw) d=st_MinFreq_G10; else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x52: if(!isw) d=st_MaxFreq_THz; else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x53: if(!isw) d=st_MaxFreq_G10; else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x54: if(!isw){ uint16_t t,g; last_freq_split(&t,&g); d=t; } else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x55: if(!isw){ uint16_t t,g; last_freq_split(&t,&g); d=g; } else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x56: if(!isw) d=st_LGrid10; else { xe=1; g_last_error=LERR_RNW; } break;
    case 0x57:
      if(!isw){
        prep_currents_aea();
        d=(uint16_t)ea_len;
      } else {
        xe=1; g_last_error=LERR_RNW;
      }
      break;
    case 0x58:
      if(!isw){
        prep_temps_aea();
        d=(uint16_t)ea_len;
      } else {
        xe=1; g_last_error=LERR_RNW;
      }
      break;
    case 0x59:
      if(isw){
        uint16_t wf = (uint16_t)(data & 0x0030u); /* waveform field */
        uint16_t en = (uint16_t)(data & 0x0002u); /* dither enable */
        if((data & (uint16_t)~0x0032u) != 0u){
          xe=1; g_last_error=LERR_RVE;
        } else if(wf != 0x0000u && wf != 0x0010u){
          xe=1; g_last_error=LERR_RVE;
        } else {
          st_DitherE = (uint16_t)(wf | en);
        }
      } else d=st_DitherE;
      break;
    case 0x5A:
      if(isw){
        if(st_DitherE & 0x0002u){
          xe=1; g_last_error=LERR_CIE;
        } else if(data < 10u || data > 200u){
          xe=1; g_last_error=LERR_RVE;
        } else {
          st_DitherR = data;
        }
      } else d=st_DitherR;
      break;
    case 0x5B:
      if(isw){
        if(st_DitherE & 0x0002u){
          xe=1; g_last_error=LERR_CIE;
        } else {
          st_DitherF = data;
        }
      } else d=st_DitherF;
      break;
    case 0x5C:
      if(isw){
        if(st_DitherE & 0x0002u){
          xe=1; g_last_error=LERR_CIE;
        } else if(data > 1000u){
          xe=1; g_last_error=LERR_RVE;
        } else {
          st_DitherA = data;
        }
      } else d=st_DitherA;
      break;
    case 0x5D: if(isw){ st_TBTFL=(int16_t)data; } else d=(uint16_t)st_TBTFL; break;
    case 0x5E: if(isw){ st_TBTFH=(int16_t)data; } else d=(uint16_t)st_TBTFH; break;
    case 0x5F:
      if(isw){
        if(data > 100u || (st_WAgeTh > 0u && data <= st_WAgeTh)){
          xe=1; g_last_error=LERR_RVE;
        } else {
          st_FAgeTh=data;
        }
      } else d=st_FAgeTh;
      break;
    case 0x60:
      if(isw){
        if(data > 100u || (st_FAgeTh > 0u && data >= st_FAgeTh)){
          xe=1; g_last_error=LERR_RVE;
        } else {
          st_WAgeTh=data;
        }
      } else d=st_WAgeTh;
      break;
    case 0x61: if(!isw) d=(uint16_t)(int16_t)(-12500); else { xe=1; g_last_error=LERR_RNW; } break; /* FTFMin -12500 MHz */
      /* ChannelH (high word of Laser_Channel) – 0x65 */
    case 0x65:
      if (isw) {
        st_CHANNELH_staged = data;
        st_CHANNELH_pending = 1;
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
                if(resena_output_enabled()){
                  set_busy(1);
                  pending_cnt = 1;
                }
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

    case 0x8C:   /* PHASE tuner (centi-units → V) */
      if (isw) {
        st_TUNER_PHASE_centi = data;
        g_v3 = data / 100.0f;
      } else {
        d = st_TUNER_PHASE_centi;
      }
      break;

    case 0x8D:   /* RING1 tuner (centi-units → V) */
      if (isw) {
        st_TUNER_RING1_centi = data;
        g_v1 = data / 100.0f;
      } else {
        d = st_TUNER_RING1_centi;
      }
      break;

    case 0x8E:   /* RING2 tuner (centi-units → V) */
      if (isw) {
        st_TUNER_RING2_centi = data;
        g_v2 = data / 100.0f;
      } else {
        d = st_TUNER_RING2_centi;
      }
      break;

    case 0x8F:   /* SOA (centi-units → V) */
      if (isw) {
        st_SOA_centi = data;
        g_soa = data / 100.0f;
      } else {
        d = st_SOA_centi;
      }
      break;

    case 0x90:   /* Gain Bias (centi-units → V) */
      if (isw) {
        st_BIAS_raw = data;
        g_gain = data / 100.0f;
      } else {
        d = (uint16_t)st_BIAS_raw;
      }
      break;

    case 0x91:   /* TEC (raw signed 16-bit) */
      if (isw) {
        st_TEC_raw = (int16_t)data;
        g_temp = (float)(int16_t)data;
      } else {
        d = (uint16_t)st_TEC_raw;
      }
      break;
    case 0x92:   /* Mode switch register: direct control enable */
      if (isw) {
        /* Enable on supported magic values; disable on 0. */
        if (data == 0x0000) {
          g_direct_ctrl_mode = 0;
        } else if (data == 0x4142 || data == 0x4344 || data == 0x9036) {
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

/* Last outbound response, for LstResp (0x13) replay. */
static ResponseFields g_last_resp = {0};

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

  /* 9.4.12 LstResp (0x13) [R, deprecated]: replay the last outbound response. */
  if (in->reg == 0x13) {
    if (in->is_write) {
      out->status = STAT_XE; g_last_error = LERR_RNW; status_latch_exec_error();
    } else {
      out->ce     = g_last_resp.ce;
      out->status = g_last_resp.status;
      out->reg    = g_last_resp.reg;
      out->data   = g_last_resp.data;
    }
    return;
  }

  if (!in->is_write && in->reg == 0x00) {
    /* IMPORTANT:
     * Hosts poll NOP while waiting for tuning to complete.
     * We must advance the pending/busy state even in this fast-path.
     */
    tick_pending();
    dl_tick();

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
#if ITLA_EMBED_PY
  int pyrc=-999;
#endif

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

  if(!in->is_write &&
     (((in->reg>=0x01 && in->reg<=0x07) || in->reg==0x57 || in->reg==0x58) &&
      status==STAT_OK)) {
    status=STAT_AEA;
  }
  if(status==STAT_XE) status_latch_exec_error();
  if(st_busy && status==STAT_OK) status=STAT_CP;

  out->ce=0; out->status=status; out->data=data;

  /* Save for LstResp (0x13) – do not overwrite with the LstResp response itself. */
  g_last_resp = *out;
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
ITLA_API int itla_dl_backend_is_active(void){ return st_dl_backend_active ? 1 : 0; }
ITLA_API uint16_t itla_dl_get_status(void){ dl_tick(); return st_DLStatus; }
ITLA_API void itla_dl_reset(void){ dl_apply_config(0u); }
ITLA_API void itla_dl_start(void){ dl_apply_config(0x0001u); }
ITLA_API void itla_dl_abort(void){ dl_apply_config(0x0002u); }
ITLA_API int itla_dl_write_word(uint16_t word){
  int rc;
  if(!st_dl_busy) return -2; /* no active download session */
  rc = itla_backend_dl_write_word(word);
  if(rc == ITLA_DL_HOOK_UNAVAIL){
    return -3; /* backend not connected */
  }
  if(rc < 0){
    st_dl_busy = 0;
    st_dl_done = 0;
    st_dl_fail = 1;
    st_dl_abort = 0;
    dl_refresh_status();
  }
  return rc;
}
ITLA_API void itla_set_capabilities_97(uint16_t ftfr_mhz,int16_t opsl,int16_t opsh,uint16_t lgrid10,int16_t lgrid2_mhz){
  ItlaCaps97 caps;
  caps.ftfr_mhz = ftfr_mhz;
  caps.opsl = opsl;
  caps.opsh = opsh;
  caps.lgrid10 = lgrid10;
  caps.lgrid2_mhz = lgrid2_mhz;
  apply_caps97(&caps);
  st_caps97_loaded = 1;
}
ITLA_API int itla_reload_capabilities_97(void){
  ItlaCaps97 caps;
  int rc;
  caps.ftfr_mhz = ITLA_CAP97_DEFAULT_FTFR_MHZ;
  caps.opsl = ITLA_CAP97_DEFAULT_OPSL;
  caps.opsh = ITLA_CAP97_DEFAULT_OPSH;
  caps.lgrid10 = ITLA_CAP97_DEFAULT_LGRID10;
  caps.lgrid2_mhz = ITLA_CAP97_DEFAULT_LGRID2;
  rc = itla_backend_get_caps97(&caps.ftfr_mhz,
                               &caps.opsl,
                               &caps.opsh,
                               &caps.lgrid10,
                               &caps.lgrid2_mhz);
  apply_caps97(&caps);
  st_caps97_loaded = 1;
  return rc;
}

ITLA_API uint32_t itla_process(uint8_t lstRsp,uint8_t isw,uint8_t reg,uint16_t data,
  uint8_t *out_ce,uint8_t *out_status,uint8_t *out_reg,uint16_t *out_data,uint32_t *out_in_frame)
{
  uint32_t in_frame = build_inbound_frame(lstRsp&1,isw&1,reg,data);
  InboundFields in_parsed; uint8_t ce_in=0;
  if(parse_inbound_frame(in_frame,&in_parsed,&ce_in)!=0){
    if(out_ce)*out_ce=1; if(out_status)*out_status=STAT_OK; if(out_reg)*out_reg=reg; if(out_data)*out_data=0; if(out_in_frame)*out_in_frame=in_frame;
    return 0;
  }
  if(ce_in) status_latch_comm_error();
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
  if(ce_in) status_latch_comm_error();
  ResponseFields resp={0}; resp.ce=ce_in; handle_register_and_build_response(&in_parsed,&resp);
  uint8_t xe_bit=(resp.status==STAT_XE)?1:0;
  uint32_t out_frame = build_outbound_frame_spec(resp.ce,xe_bit,resp.reg,resp.data);
  if(out_ce)*out_ce=resp.ce; if(out_status)*out_status=resp.status; if(out_reg)*out_reg=resp.reg; if(out_data)*out_data=resp.data;
  if(in_parsed.reg != 0x13) st_LstRsp = resp.data;  /* capture last response, skip when reading LstRsp itself */
  return out_frame;
}

ITLA_API uint32_t itla_process_frame(uint32_t in_frame,
  uint8_t *out_ce,uint8_t *out_status,uint8_t *out_reg,uint16_t *out_data)
{
  return itla_handle_frame(in_frame,out_ce,out_status,out_reg,out_data);
}

/* Feed real hardware measurements into ITLA telemetry state.
 * Call from main.c after every ADC sample cycle.
 *   ctemp_c100  : die temperature in degC x100  (e.g. 23.4 degC -> 2340)
 *   tec_ma10    : TEC current in mA x10          (e.g. 1.2 A    ->   12)
 *   oop_mv      : optical output power from MPD ADC in mV
 *   case_c100   : case/PCB temperature in degC x100
 */
ITLA_API void itla_update_hw_telemetry(int16_t ctemp_c100,
                                        int16_t tec_ma10,
                                        uint16_t oop_mv,
                                        int16_t case_c100)
{
  st_CTemp   = ctemp_c100;
  st_TEC_raw = tec_ma10;
  st_OOP     = oop_mv;
  (void)case_c100;  /* reserved for future case-temp use in prep_temps_aea */
}

#ifdef __cplusplus
}
#endif
