/*!
 *****************************************************************************
 * @file: main.c
 * @brief: ADC Example Code. Measures AIN0.
 *         Sets VDAC0 as an output. VDAC0 can externally be connected to AIN0
 * @version    V0.4
 * @date       March 2022
 * @par Revision History:
 * -V0.1, March 2021: initial version.
 * -V0.2, June 2021: added PLL interrupt code and removed unnecessary ADC calibration
 * -V0.3, Aug 2021: ADC trimmed reference voltage value is 2.52V.
 * -V0.4, March 2022: Replace floats by integers in outputs
 *-----------------------------------------------------------------------------
 *
Copyright (c) 2013-2018 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.
 ******************************************************************************/

#include "adi_processor.h"

#include "AdcLib.h"
#include "DacLib.h"
#include "DioLib.h"
#include "UrtLib.h"
#include "iramCode.h"
#include <string.h>

#include "WdtLib.h"
#include "nano_globals.h"
// --- LUT Global Variables (used by hardware) ---
float g_v1   = 0.0f;
float g_v2   = 0.0f;
float g_v3   = 0.0f;
float g_gain = 0.0f;
float g_soa  = 0.0f;
float g_temp = 0.0f;
float g_mpd  = 0.0f;
float g_wlpd = 0.0f;
float g_wmpd = 0.0f;

/* -------- Fallback macro defs (in case the pack doesn't define them) ----- */
#ifndef COMLCR_WLS_8BITS
#  define COMLCR_WLS_8BITS    (0x03u)        /* 8 data bits */
#endif

#ifndef BITM_UART_FCR_FIFOEN
#  define BITM_UART_FCR_FIFOEN    (1u << 0)   /* FIFO enable */
#endif
#ifndef BITM_UART_FCR_RXFIFOCLR
#  define BITM_UART_FCR_RXFIFOCLR (1u << 1)   /* Clear RX FIFO */
#endif
#ifndef BITM_UART_FCR_TXFIFOCLR
#  define BITM_UART_FCR_TXFIFOCLR (1u << 2)   /* Clear TX FIFO */
#endif

/* THR Empty bit: use whichever your pack provides, or fallback to bit 5 */
#if !defined(BITM_UART_LSR_THRE) && defined(COMLSR_THRE)
#  define BITM_UART_LSR_THRE  COMLSR_THRE
#endif
#ifndef BITM_UART_LSR_THRE
#  define BITM_UART_LSR_THRE  (1u << 5)      /* THR Empty */
#endif

// From nanoITLA.c
extern uint32_t itla_handle_frame(uint32_t in_frame,
                                  uint8_t *out_ce,
                                  uint8_t *out_status,
                                  uint8_t *out_reg,
                                  uint16_t *out_data);


static void uart1_init(void) {
    DioCfgPin(pADI_GPIO1, PIN0, 1); // TX
    DioCfgPin(pADI_GPIO1, PIN1, 1); // RX
    UrtCfg(pADI_UART, 115200u, COMLCR_WLS_8BITS, 0u);
    pADI_UART->FCR = BITM_UART_FCR_FIFOEN | BITM_UART_FCR_RXFIFOCLR | BITM_UART_FCR_TXFIFOCLR;
}


#define ADC_DATA_LENGTH 20
uint32_t AdcData[ADC_DATA_LENGTH];
uint16_t AdcData16b[ADC_DATA_LENGTH];

#define MAX_STRING_LENGTH 128
#define RX_CLR 0x02
#define TX_CLR 0x04

static char printBuffer[MAX_STRING_LENGTH] = "";
extern volatile uint8_t tmr0_trig;
volatile uint8_t comRx1 = 0; // Variable used to read UART Rx buffer contents into
volatile uint32_t TxDone = 0;
char urt_data[50]={0};
int counts=0;
#define MAX_STRING_LENGTH 128
#define RX_CLR 0x02
#define TX_CLR 0x04


volatile uint32_t dataCnt;
volatile uint32_t sta;
uint32_t conversionDone;
uint8_t ucFirstIRQ = 0;
uint8_t ucPLLLoss = 0; // flag to indicate loss of PLL Lock error

extern void ADuCM430Setup(void);
void Setup(void);

int AdcPollingTest(void);
int AdcIntTest(void);
void AdcInit(void);
void VdacInit();
void PLL_Interrupt_Init(void);
void delay(uint32_t tick);
void VdacSetup(void);
void IdacSetup(void);
void I_dac_write(int i);
void V_dac_write(int j, int i);

void SetSOA(float Current);
void SetGain(float Current);
void SetR1(float Voltage);
void SetR2(float Voltage);
void SetPhase(float Voltage);

float last_time=0;
float now=0;
float setpoint=1272;
/*
kp =0.78;   
ki = 0.38;
kd = 0.0041;
*/

/* from software simulation
kp =0.78;   
ki = 0.49;
kd = 0.25;
*/

/* from software simulation
kp =0.7;   
ki = 0.49;
kd = 0.17;
*/

/* from software simulation*/
float kp =2.84;   
float ki = 1.3;
float kd = 0.8;

float dt=0.01f;
float prev_error = 0.0f;
float integral = 0.0f;
float error=0.0f;


void delay(uint32_t tick)
{
    for (volatile uint32_t i = 0; i < tick; i++) {
        for (volatile uint32_t j = 0; j < 100; j++)
            ;
    }
}

void UART_Setup(void)
{
#if defined(__ADUCM433__)
    DioCfgPin(pADI_GPIO0, PIN4, P0_4_UART0_RX); // This needs to be used for the ADuCM433
    DioCfgPin(pADI_GPIO0, PIN5, P0_5_UART0_TX); // This needs to be used for the ADuCM433
#else
    DioCfgPin(pADI_GPIO3, PIN3, P3_3_UART0_RX);
    DioCfgPin(pADI_GPIO3, PIN4, P3_4_UART0_TX);
#endif

    UrtCfg(pADI_UART, B115200, ENUM_UART_LCR_WLS_BITS8, 0);
    UrtIntCfg(pADI_UART, COMIEN_ERBFI|COMIEN_ETBEI);
		UrtFifoCfg(pADI_UART,0xc0,0x01);
    NVIC_EnableIRQ(UART0_IRQn);
}

/*
float pid(float error)
{
  float proportional = error;
  integral += error * dt;
  float derivative = (error - previous) / dt;
  previous = error;
  float output = (kp * proportional) + (ki * integral) + (kd * derivative);
  return output;
}
*/

typedef struct
{
    float Kp;          // Proportional gain
    float Ki;          // Integral gain
    float Kd;          // Derivative gain
    float prev_error;  // Previous error
    float integral;    // Integral term
    float dt;          // Time interval
    float out_min;     // Minimum output limit
    float out_max;     // Maximum output limit
} PID_Controller;

// Initialize PID controller
void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd, float dt, float out_min, float out_max)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->dt = dt;
    pid->prev_error = 0.0f;
    pid->integral = 0.0f;
    pid->out_min = out_min;
    pid->out_max = out_max;
}

// Compute PID output
float PID_Compute(PID_Controller *pid, float setpoint, float measurement)
{
    float error = setpoint - measurement;
    pid->integral += error * pid->dt;

    // Anti-windup: limit the integral term
    if (pid->integral > pid->out_max) pid->integral = pid->out_max;
    else if (pid->integral < pid->out_min) pid->integral = pid->out_min;

    float derivative = -(error - pid->prev_error) / pid->dt;
    float output = pid->Kp * error + pid->Ki * pid->integral + pid->Kd * derivative;
	  //float output = pid->Kp * error;
	  //float output = pid->Kp * error + pid->Ki * pid->integral;

    // Clamp output to limits
    //if (output > pid->out_max) output = pid->out_max;
    //else if (output < pid->out_min) output = pid->out_min;

    pid->prev_error = error;
    return output;
}


void time_0_init(void)
{

		pADI_GPT0->CON = ENUM_TMR_CON_RLD_EN << BITP_TMR_CON_RLD | ENUM_TMR_CON_MOD_PERIODIC << BITP_TMR_CON_MOD |
                     ENUM_TMR_CON_UP_EN << BITP_TMR_CON_UP | ENUM_TMR_CON_PRE_DIV1OR4  << BITP_TMR_CON_PRE;

    pADI_GPT0->LD = 0;
    //NVIC_EnableIRQ(GPT0_IRQn);
    pADI_GPT0->CON |= ENUM_TMR_CON_ENABLE_EN << BITP_TMR_CON_ENABLE;

}

void PLL_Interrupt_Init(void)
{
    pADI_CLK->CLKCON0 |= BITM_CLOCK_CLKCON0_SPLLIE; // Enable PLL interrupts
    NVIC_EnableIRQ(PLL_IRQn);                       // Enable PLL detection interrupt
}
/*
void VdacInit(void)
{
    // Enable VDAC Reference Buffer
    pADI_VDAC->VDACALLCON = 1;

    // Gpio mux Setting
    DioCfgPin(pADI_GPIO5, PIN0, P5_0_VDAC0);

    // Vdac config
    // Vdac0, output full scale 2V5, positive buffer,
    VDacCfg(0, ENUM_VDAC_VDAC0CON_FSLVL_FS2P5, ENUM_VDAC_VDAC0CON_OUTSEL_PRIMARY, 0);
    VDacWrAutoSync(0, 0x000);
}
*/

void AdcInit(void)
{
    // Power up the ADC & ADC reference buffer
    AdcPowerDown(0);

    // Use MMR calibrations
    //  AdcCalibrationSource(BITM_ADC_ADCCON0_OFGNDIFFEN, BITM_ADC_ADCCON0_OFGNSEEN);

    // Initialize ADC External channels conversion speed.
    //AdcSpeed(800, FAST_CONVERSIONS);
    //AdcOverSampling(ENUM_ADC_OSR4); // Hardware Oversampling factor of 4
	
	  AdcSpeed(127, FAST_CONVERSIONS);
	  AdcOverSampling(ENUM_ADC_OSR2);

    // Select ADC input channel in Single ended mode
    AdcPinExt(ENUM_ADC_AIN0);

    // Enable ADC interrupts - at ADC block level - nVIC call needed to fully enable
    AdcIntEn(1);
}

int Adc_read(void)
{
		AdcGo(ENUM_ADC_IDLE, 0);
    NVIC_DisableIRQ(ADC_IRQn);

    AdcIntEn(false);

    // AIN4, single ended
    AdcPinExt(ENUM_ADC_AIN0);

    // start conversion
    AdcGo(ENUM_ADC_CONT, 0);
		
		//int Data = AdcRd(ENUM_ADC_AIN4);
		int AdcData=0;
    for (uint32_t i = 0; i < 20;) {
        AdcData = AdcRd(ENUM_ADC_AIN0);
        if (AdcData & BITM_ADC_ADCDAT_N__RDY)
            i++;
    }
		

    AdcGo(ENUM_ADC_IDLE, 1);
    //UrtPrint("Adc Sample AIN0 Channel By polling status register: \r\n");
		/*
    for (uint32_t i = 0; i < ADC_DATA_LENGTH; i++) {
        UrtPrint("Hex:%x, %u mV \r\n", (AdcData[i] & 0xFFFF), ((AdcData[i] & 0xFFFF) * 2520 / 65536));
    }
		*/
		
		
		//UrtPrint("Hex:%x, %u mV \r\n", (AdcData & 0xFFFF), ((AdcData & 0xFFFF) * 2520 / 65536));
		return AdcData;
}

int AdcIntTest(void)
{
    dataCnt = 0;

    AdcIntEn(true);

    // AIN0, single ended
    AdcPinExt(ENUM_ADC_AIN0);
    conversionDone = 0;
    dataCnt = 0;
    NVIC_EnableIRQ(ADC_IRQn); // Enable ADC interrupt in Cortex nVIC
    ucFirstIRQ = 1;
    AdcGo(ENUM_ADC_CONT, 0); // Continuous S/W conversions

    while (conversionDone == 0)
        ; // wait for conversion finish

    UrtPrint("Adc Sample AIN0 Channel with Interrupt: \r\n");
    for (uint32_t i = 0; i < 20; i++) {
        AdcData16b[i] = (uint16_t)AdcData[i];
        //UrtPrint("Hex:%x, %u mV \r\n", (AdcData[i] & 0xFFFF), ((AdcData[i] & 0xFFFF) * 2520 / 65536));
    }
		
		return AdcData[10];
}

int MPD_read(void)
{
	
		//AdcPowerDown(0);
 
		AdcGo(ENUM_ADC_IDLE, 0);
    NVIC_DisableIRQ(ADC_IRQn);

    AdcIntEn(false);

    // AIN4, single ended
    AdcPinExt(ENUM_ADC_AIN2);

    // start conversion
    AdcGo(ENUM_ADC_CONT, 0);

		//int Data = AdcRd(ENUM_ADC_AIN4);
		int AdcData=0;
    for (uint32_t i = 0; i < 5;)
		{
        AdcData = AdcRd(ENUM_ADC_AIN2);
        if (AdcData & BITM_ADC_ADCDAT_N__RDY)
            i++;
    }
    AdcGo(ENUM_ADC_IDLE, 1);
		//UrtPrint("%u\r\n", ((AdcData & 0xFFFF) * 2520 / 65536));
		return (AdcData & 0xFFFF) * 2520 / 65536;
}

int WLPD_read(void)
{
	  //AdcPowerDown(0);
		AdcGo(ENUM_ADC_IDLE, 0);
    NVIC_DisableIRQ(ADC_IRQn);

    AdcIntEn(false);

    // AIN4, single ended
    AdcPinExt(ENUM_ADC_AIN1);

    // start conversion
    AdcGo(ENUM_ADC_CONT, 0);

		//int Data = AdcRd(ENUM_ADC_AIN4);
		int AdcData=0;
    for (uint32_t i = 0; i < 5;)
		{
        AdcData = AdcRd(ENUM_ADC_AIN1);
        if (AdcData & BITM_ADC_ADCDAT_N__RDY)
            i++;
    }
    AdcGo(ENUM_ADC_IDLE, 1);
		//UrtPrint("%u\r\n", ((AdcData & 0xFFFF) * 2520 / 65536));
		return (AdcData & 0xFFFF) * 2520 / 65536;
}

int WMPD_read(void)
{
	
		//DioCfgPin(pADI_GPIO4, PIN3, 1);
	  
	  //DioOenPin(pADI_GPIO4, PIN3, 0);
	
		NVIC_DisableIRQ(EINT7_IRQn);
	  //AdcPowerDown(0);
		AdcGo(ENUM_ADC_IDLE, 0);
    NVIC_DisableIRQ(ADC_IRQn);

    AdcIntEn(false);

    // AIN4, single ended
    AdcPinExt(ENUM_ADC_AIN7);

    // start conversion
    AdcGo(ENUM_ADC_CONT, 0);

		//int Data = AdcRd(ENUM_ADC_AIN4);
		int AdcData=0;
    for (uint32_t i = 0; i < 5;)
		{
        AdcData = AdcRd(ENUM_ADC_AIN7);
        if (AdcData & BITM_ADC_ADCDAT_N__RDY)
            i++;
    }
    AdcGo(ENUM_ADC_IDLE, 1);

		//UrtPrint("%u\r\n", ((AdcData & 0xFFFF) * 2520 / 65536));
		return (AdcData & 0xFFFF) * 2520 / 65536;
}

void UART_Int_Handler(void)
{
    uint32_t sta;
    uint32_t iir = UrtIntSta(pADI_UART);
    sta = (iir & BITM_UART_IIR_STAT) >> BITP_UART_IIR_STAT;
	
		if(pADI_UART->RFC>0)
			{
			delay(200);
			counts=pADI_UART->RFC;
			delay(200);
			for(int i=0;i<counts;i++)
				{
					urt_data[i]=pADI_UART->RXTX;
				}
				UrtFifoClr(pADI_UART, RX_CLR);
			}
			UrtFifoClr(pADI_UART, RX_CLR);
			UrtFifoClr(pADI_UART, TX_CLR);
}



void TecInit(void)
{
    // Enable VDAC Reference Buffer
    pADI_VDAC->VDACALLCON = 1;
    // DioCfgPin(pADI_GPIO6, PIN0, P6_0_VDAC8);
    // Vdac8
    VDacCfg(8, 0, 0, 0);
    // update Vdac data value
    VDacWrAutoSync(8, 0x800);

    pADI_MISC->USERKEY = 0x9FE5;
    pADI_MISC->USERKEY1 = 0xC7FA;

    
    pADI_TEC->TECLIMH = 0x00 << BITP_TEC_TECLIMH_ITECLIMH | 0x7F << BITP_TEC_TECLIMH_VTECLIMH;
	
    
    pADI_TEC->TECLIMC = 0x00 << BITP_TEC_TECLIMC_ITECLIMC | 0x7F << BITP_TEC_TECLIMC_VTECLIMC;

  
    pADI_TEC->TECCON &= ~BITM_TEC_TECCON_MODE;
    pADI_TEC->TECCON |= ENUM_TEC_TECCON_MODE_TECMODE << BITP_TEC_TECCON_MODE; // TEC Mode
    

    // set soft-start time 1code
    pADI_TEC->TECCON &= ~BITM_TEC_TECCON_SSTIME;
    pADI_TEC->TECCON |= ENUM_TEC_TECCON_SSTIME_SST7 << BITP_TEC_TECCON_SSTIME;

    
    pADI_TEC->TECFLAGIRQEN = 1 << BITP_TEC_TECFLAGIRQEN_DCDCFAULTEN | 1 << BITP_TEC_TECFLAGIRQEN_TECFAULTEN;

    
    pADI_TEC->TECCON |= BITM_TEC_TECCON_ENTEC;

    pADI_MISC->USERKEY = 0x0;
    pADI_MISC->USERKEY1 = 0x0;
}

extern uint32_t itla_handle_frame(uint32_t in_frame,
                                  uint8_t *out_ce,
                                  uint8_t *out_status,
                                  uint8_t *out_reg,
                                  uint16_t *out_data);


static void uart1_write_bytes(const uint8_t *buf, int len) {
    for (int i=0;i<len;i++) {
        while ((pADI_UART->LSR & BITM_UART_LSR_THRE) == 0) { /* wait */ }
        pADI_UART->RXTX = buf[i];
    }
}

int main(void)
{
    uint32_t counter = 0;
    pADI_ALLON->RSTKEY = 0x2009;
    pADI_ALLON->RSTKEY = 0x0426;
    pADI_ALLON->RSTCFG = 0x7; // Ensure that GPIO, Analog and Clocks are reset during a soft/wdt reset
		
		//VDacWrAutoSync(0, 0x000);
	float actual=0;
	float error = 0;
	float output=0;
	 float Temperature = 0;
	 int PWM = 0;
	

	
  DioCfgPin(pADI_GPIO2, PIN0, 0);
  DioOenPin(pADI_GPIO2, PIN0, 1);
	DioClr(pADI_GPIO2, PIN0);

	DioCfgPin(pADI_GPIO2, PIN3, 0);
  DioOenPin(pADI_GPIO2, PIN3, 1);
	DioClr(pADI_GPIO2, PIN3);
	
	UART_Setup();
	time_0_init();
	
  ADuCM430Setup();
  Setup();

  sprintf(printBuffer, "....nano-ITLA....\n\r");
  UrtSendString(pADI_UART, printBuffer);

  sprintf(printBuffer, "Enter command \n\r");
	UrtSendString(pADI_UART, printBuffer);
	delay(100);
    /* connect Vdc0 <---------------> AIN0 on the EVB Board */
    //VdacInit();
	  uint32_t k=0;

		const char *s = "#";


		VDacWr(2, 0x0); // Ring supply enable/disable
		VDacWr(1, 0x0); // Ring 1
		VDacWr(3, 0x0); // Ring 2
		VDacWr(7, 0x0);	// Phase
		VDacWr(5, 0x0); // SOA current
		VDacSync(0x1FF); // Set the output at the initial values (cero).
		
		DioSet(pADI_GPIO2, PIN3);
		V_dac_write(2,0xfff);
		
    AdcInit();

	  TecInit();
    //AdcPollingTest();

    //AdcIntTest();

    PLL_Interrupt_Init();
		float x=0;
		
		
		
    PID_Controller pid;
    //PID_Init(&pid, 0.88f, .058f, 0.005f, 0.1f, -2000.0f, 2000.0f);  // dt = 10ms
		//PID_Init(&pid, 0.5f, 0.30f, 0.05f, 0.1f, -2000.0f, 2000.0f);  // dt = 10ms						/0.09 Ki good--- with multimeter
		//PID_Init(&pid, 0.05f, 0.40f, 0.005f, 0.1f, -500.0f, 500.0f);  // dt = 10ms						/0.09 Ki good--- reach 1700 and change to 400 without reset, but not reach 400
		//PID_Init(&pid, 0.01f, 0.40f, 0.005f, 0.1f, -850.0f, 250.0f);  // dt = 10ms						/0.09 Ki good--- Reach 1700, reach 400. Oscilaactions betewn. best part. I can change betwen 1700 and 400
		//PID_Init(&pid, 0.005f, 0.30f, 0.005f, 0.1f, -1150.0f, 450.0f);  // dt = 10ms			//the best without multimeter
		//PID_Init(&pid, 0.2f, 0.10f, 0.1f, 0.2f, -3300.0f, 2000.0f);  // dt = 10ms			//2 optionthe best without multimeter
		PID_Init(&pid, 0.2f, 0.10f, 0.3f, 0.2f, -3300.0f, 2000.0f);  // dt = 10ms			//the best without multimeter
		//PID_Init(&pid, 0.005f, 0.30f, 0.30f, 0.2f, -850.0f, 450.0f);  // dt = 10ms
		//PID_Init(&pid, 0.5f, 0.20f, 0.008f, 0.1f, -1150.0f, 450.0f);  // dt = 10ms
		//PID_Init(&pid, 0.008f, 0.15f, 0.1f, 0.2f, -1750.0f, 850.0f);  // dt = 10ms			//
		//PID_Init(&pid, 0.5f, 0.05f, 0.1f, 0.1f, -1000.0f, 1000.0f);  // dt = 10ms
		
		actual=0;
		volatile int adc_val=0;
		volatile int adc_val2=0;
		int MPD_data=0;
    int WLPD_data=0;
    int WMPD_data=0;
		
		WdtGo(false);
    SystemInit();
    uart1_init();
		
		
		
    while (1) {
			// Wait for 4-byte inbound frame
       if (pADI_UART->RFC >= 4) {
            uint8_t b[4];
            for (int i=0; i<4; i++) b[i] = pADI_UART->RXTX;

            uint32_t in_frame = (b[0]<<24)|(b[1]<<16)|(b[2]<<8)|b[3];
            uint8_t ce=0, status=0, reg=0; uint16_t data=0;

            uint32_t out_frame = itla_handle_frame(in_frame,&ce,&status,&reg,&data);
					
            uint8_t o[4] = {
                (out_frame>>24)&0xFF,
                (out_frame>>16)&0xFF,
                (out_frame>>8)&0xFF,
                out_frame&0xFF
            };
            uart1_write_bytes(o,4);
						
						//UrtPrint("%f,%f", g_v1,g_v2);	
						//V_dac_write(7,g_v1);
						SetSOA(g_soa);			//fine
						SetGain(g_gain);		//fine
						SetR1(g_v1);				//fine
						SetR2(g_v2);				//fine
						SetPhase(g_v3);			//fine
        }	
		}			
}


void IdacSetup(void)
{
    IDacCfg(0, ENUM_IDAC_IDAC0CON_RANGE_RANGE150M, 0, ENUM_IDAC_IDAC0CON_CLRBIT_CLRB);
}



void Setup(void)
{
#if defined(__ADUCM433__)
    DioCfgPin(pADI_GPIO0, PIN4, P0_4_UART0_RX); // This needs to be used for the ADuCM433
    DioCfgPin(pADI_GPIO0, PIN5, P0_5_UART0_TX); // This needs to be used for the ADuCM433
#else
    DioCfgPin(pADI_GPIO3, PIN3, P3_3_UART0_RX);
    DioCfgPin(pADI_GPIO3, PIN4, P3_4_UART0_TX);
#endif

    UrtCfg(pADI_UART, B115200, ENUM_UART_LCR_WLS_BITS8, 0);
    UrtIntCfg(pADI_UART, COMIEN_ERBFI|COMIEN_ETBEI);
		UrtFifoCfg(pADI_UART,0xc0,0x01);
    NVIC_EnableIRQ(UART0_IRQn);

		    // Enable VDAC Reference Buffer
    pADI_VDAC->VDACALLCON = 1;

    // Gpio mux Setting

    DioCfgPin(pADI_GPIO5, PIN1, P5_1_VDAC1); // VDAC Ring 1
		DioCfgPin(pADI_GPIO5, PIN2, P5_2_VDAC2); // Ring_V_En
		DioCfgPin(pADI_GPIO5, PIN5, P5_5_VDAC5); // VDAC Ring 1
    DioCfgPin(pADI_GPIO5, PIN3, P5_3_VDAC3 ); // VDAC Ring 2
    DioCfgPin(pADI_GPIO5, PIN7, P5_7_VDAC7); // VDAC phase
		
	  //DioCfgPin(pADI_GPIO4, PIN3, 1);
	  
	  //DioOenPin(pADI_GPIO4, PIN3, 0);
	//  DioPulPin(pADI_GPIO4, PIN3, 0);


    VDacCfg(1, ENUM_VDAC_VDAC0CON_FSLVL_FS2P5, ENUM_VDAC_VDAC1CON_OUTSEL_PRIMARY, 0);
		VDacCfg(2, ENUM_VDAC_VDAC0CON_FSLVL_FS3P75, ENUM_VDAC_VDAC1CON_OUTSEL_PRIMARY, 0);
		VDacCfg(5, ENUM_VDAC_VDAC0CON_FSLVL_FS2P5, ENUM_VDAC_VDAC1CON_OUTSEL_PRIMARY, 0);
    VDacCfg(3, ENUM_VDAC_VDAC0CON_FSLVL_FS2P5, ENUM_VDAC_VDAC0CON_OUTSEL_PRIMARY, 0);
    VDacCfg(7, ENUM_VDAC_VDAC7CON_FSLVL_FS2P5, ENUM_VDAC_VDAC1CON_OUTSEL_PRIMARY, 0);
		IDacCfg(0, ENUM_IDAC_IDAC0CON_RANGE_RANGE150M, 0, ENUM_IDAC_IDAC0CON_CLRBIT_CLRB);
    IDacWr(0, 0x000);
    IDacSync(0xF); // IGAIN current channel 0
}



void I_dac_write(int i)
{

			IDacWr(0, i);
			IDacSync(0xF);
}

void V_dac_write(int j, int i)
{

			VDacWr(j, i);
			VDacSync(0x1FF);
}

void SetSOA(float Current)
{
	    int Data;
	    Data =(3.0225 * Current) - 16.605;
			V_dac_write(5, Data);
}

void SetGain(float Current)
{
			int Data;
	    Data =(25.779 * Current) - 168.93;
			I_dac_write(Data);
}

void SetR1(float Voltage)
{			//Voltage 5.5
			Voltage = -Voltage;
			int Data;
	    Data =(-161.51 * Voltage) - 5.1112;
			V_dac_write(1, Data);
}

void SetR2(float Voltage)
{
			Voltage = -Voltage;		
			int Data;
	    Data =(-161.51 * Voltage) - 5.1112;
			V_dac_write(3, Data);
}

void SetPhase(float Voltage)
{
			Voltage = -Voltage;		
			int Data;
	    Data =(-161.51 * Voltage) - 5.1112;
			V_dac_write(7, Data);
}

		
