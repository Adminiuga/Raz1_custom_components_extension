 /***************************************************************************//**
* @file battery-monitor-efr32.c
* @brief An example about sampling the battery voltage through IADC(Series 2) or ADC(Series 1).
* @version v0.01
*******************************************************************************
* # License
* <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
*
* SPDX-License-Identifier: Zlib
*
* The licensor of this software is Silicon Laboratories Inc.
*
* This software is provided \'as-is\', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*
*******************************************************************************
* # Experimental Quality
* This code has not been formally tested and is provided as-is. It is not
* suitable for production environments. In addition, this code will not be
* maintained and there may be no bug maintenance planned for these resources.
* Silicon Labs may update projects from time to time.
******************************************************************************/

#include "app/framework/include/af.h"

#include "gpiointerrupt.h"
#include "em_cmu.h"
#include "sl_power_manager.h"
#include "sl_sleeptimer.h"
#include "sl_battery_monitor.h"
#include "sl_battery_monitor_config.h"

#if defined(_SILICON_LABS_32B_SERIES_2)
#include "em_iadc.h"
#else
#include "em_adc.h"
#endif

#include "em_prs.h"

#define MOD(a, b) ((a) % (b))
#define IS_EVEN(a) (MOD((a), 2) == 0)

// Shorter macros for plugin options
#define FIFO_SIZE \
  SL_BATTERY_MONITOR_SAMPLE_FIFO_SIZE
#define MS_BETWEEN_BATTERY_CHECK \
 (SL_BATTERY_MONITOR_TIMEOUT_MINUTES * 60 * 1000)
#define BSP_BATTERYMON_TX_ACTIVE_CHANNEL \
    SL_BATTERY_MONITOR_TX_ACTIVE_CHANNEL
#define BSP_BATTERYMON_TX_ACTIVE_PORT \
    SL_BATTERY_MONITOR_TX_ACTIVE_PORT
#define BSP_BATTERYMON_TX_ACTIVE_PIN \
    SL_BATTERY_MONITOR_TX_ACTIVE_PIN
#ifdef SL_BATTERY_MONITOR_TX_ACTIVE_LOC
#define BSP_BATTERYMON_TX_ACTIVE_LOC \
    SL_BATTERY_MONITOR_TX_ACTIVE_LOC
#endif  // SL_BATTERY_MONITOR_TX_ACTIVE_LOC

#define MAX_INT_MINUS_DELTA              0xe0000000
#define _SL_BATTERY_MONITOR_ADC_MAX      0xfff

#if defined(_SILICON_LABS_32B_SERIES_2)

#if defined(_SILICON_LABS_32B_SERIES_2_CONFIG_1)
#define PRS_SOURCE                       PRS_ASYNC_CH_CTRL_SOURCESEL_RAC
#define PRS_SIGNAL                       PRS_ASYNC_CH_CTRL_SIGSEL_RACTX
#elif defined(_SILICON_LABS_32B_SERIES_2_CONFIG_2)
#define PRS_SOURCE                       PRS_ASYNC_CH_CTRL_SOURCESEL_RACL
#define PRS_SIGNAL                       PRS_ASYNC_CH_CTRL_SIGSEL_RACLTX
#else
error("please define the correct macros here!")
#endif

// Set CLK_ADC to 100kHz (this corresponds to a sample rate of 10ksps)
#define CLK_SRC_ADC_FREQ        1000000  // CLK_SRC_ADC
#define CLK_ADC_FREQ            100000   // CLK_ADC

/** Default config for IADC single input structure. */
#define IADC_SINGLEINPUT_BATTERY                                       \
  {                                                                    \
    iadcNegInputGnd,             /* Negative input GND */              \
    SL_BATTERY_MONITOR_IADC_POS, /* Positive input iadcPosInputAvdd */ \
    0,                           /* Config 0 */                        \
    false                        /* Do not compare results */          \
  }

#define IADC_REF_MV             1210
#if SL_BATTERY_MONITOR_IADC_ANALOG_GAIN == _IADC_CFG_ANALOGGAIN_ANAGAIN0P5
#define IADC_REFERENCE_VOLTAGE_MILLIVOLTS (IADC_REF_MV << 1)
#define IADC_ANALOG_GAIN                  iadcCfgAnalogGain0P5x
#elif SL_BATTERY_MONITOR_IADC_ANALOG_GAIN == _IADC_CFG_ANALOGGAIN_ANAGAIN2
#define IADC_REFERENCE_VOLTAGE_MILLIVOLTS (IADC_REF_MV >> 1)
#define IADC_ANALOG_GAIN                  iadcCfgAnalogGain2x
#elif SL_BATTERY_MONITOR_IADC_ANALOG_GAIN == _IADC_CFG_ANALOGGAIN_ANAGAIN4
#define IADC_REFERENCE_VOLTAGE_MILLIVOLTS (IADC_REF_MV >> 2)
#define IADC_ANALOG_GAIN                  iadcCfgAnalogGain4x
#else
#define IADC_REFERENCE_VOLTAGE_MILLIVOLTS IADC_REF_MV
#define IADC_ANALOG_GAIN                  iadcCfgAnalogGain1x
#endif // SL_BATTERY_MONITOR_ANALOG_GAIN

#else //series 1

#define PRS_CH_CTRL_SOURCESEL_RAC_TX \
  (PRS_RAC_TX & _PRS_CH_CTRL_SOURCESEL_MASK)
#define PRS_CH_CTRL_SIGSEL_RAC_TX \
  (PRS_RAC_TX & _PRS_CH_CTRL_SIGSEL_MASK)

// Default settings to be used when configured the PRS to cause an external pin
// to emulate TX_ACTIVE functionality
#define PRS_SOURCE                       PRS_CH_CTRL_SOURCESEL_RAC_TX
#define PRS_SIGNAL                       PRS_CH_CTRL_SIGSEL_RAC_TX
#define PRS_EDGE                         prsEdgeOff
#define PRS_PIN_SHIFT                    (8                                   \
                                          * (BSP_BATTERYMON_TX_ACTIVE_CHANNEL \
                                             % 4))
#define PRS_PIN_MASK                     (0x1F << PRS_PIN_SHIFT)

#if BSP_BATTERYMON_TX_ACTIVE_CHANNEL < 4
#define PRS_ROUTE_LOC                    ROUTELOC0
#elif BSP_BATTERYMON_TX_ACTIVE_CHANNEL < 8
#define PRS_ROUTE_LOC                    ROUTELOC1
#else
#define PRS_ROUTE_LOC                    ROUTELOC2
#endif

// Default settings used to configured the ADC to read the battery voltage
#define ADC_INITSINGLE_BATTERY_VOLTAGE                                \
  {                                                                   \
    adcPRSSELCh0, /* PRS ch0 (if enabled). */                         \
    adcAcqTime16, /* 1 ADC_CLK cycle acquisition time. */             \
    adcRef5VDIFF, /* V internal reference. */                         \
    adcRes12Bit, /* 12 bit resolution. */                             \
    SL_BATTERY_MONITOR_ADC_POS_SEL, /* Select Vdd as posSel */        \
    adcNegSelVSS, /* Select Vss as negSel */                          \
    false,       /* Single ended input. */                            \
    false,       /* PRS disabled. */                                  \
    false,       /* Right adjust. */                                  \
    false,       /* Deactivate conversion after one scan sequence. */ \
    false,       /* No EM2 DMA wakeup from single FIFO DVL */         \
    false        /* Discard new data on full FIFO. */                 \
  }

#define ADC_REFERENCE_VOLTAGE_MILLIVOLTS 5000

#endif  // defined(_SILICON_LABS_32B_SERIES_2)

// ------------------------------------------------------------------------------
// Forward Declaration
static uint16_t filterVoltageSample(uint16_t sample);
static void tx_channel_irq_handler(uint8_t int_id, void *ctx);
#if defined(_SILICON_LABS_32B_SERIES_2)
static void handle_em0_transition(sl_power_manager_em_t from,
                                  sl_power_manager_em_t to);
#endif // _SILICON_LABS_32B_SERIES_2

// ------------------------------------------------------------------------------
// Globals

// count used to track when the last measurement occurred
// Ticks start at 0.  We use this value to limit how frequently we make
// measurements in an effort to conserve battery power.  By setting this to an
// arbitrary value close to MAX_INT, we are going to make sure we make a
// battery measurement on the first transmission.
static uint32_t lastBatteryMeasureTick = MAX_INT_MINUS_DELTA;

// sample FIFO access variables
static uint8_t samplePtr = 0;
static uint16_t voltageFifo[FIFO_SIZE];
static bool fifoInitialized = false;
static uint8_t interrupt_id = INTERRUPT_UNAVAILABLE;

// Remember the last reported voltage value from callback, which will be the
// return value if anyone needs to manually poll for data
static uint16_t lastReportedVoltageMilliV;

#if defined(_SILICON_LABS_32B_SERIES_2)
static sl_power_manager_em_transition_event_handle_t em0_transition_event;
#endif // _SILICON_LABS32B_SERIES_2

// ------------------------------------------------------------------------------
// forward declarations
static void _activate_prs(void);
static void _deactivate_prs(void);

// ------------------------------------------------------------------------------
// Implementation of public functions

void sl_battery_monitor_init(void)
{
  #if defined(_SILICON_LABS_32B_SERIES_2)
  IADC_Init_t        init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t  initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t  initSingle = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t initSingleInput = IADC_SINGLEINPUT_BATTERY;
  
  
  CMU_ClockEnable(cmuClock_IADC0, true);

  IADC_reset(IADC0);  

  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO); // 20MHz

  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0); 

  initAllConfigs.configs[0].reference = iadcCfgReferenceInt1V2;
  initAllConfigs.configs[0].analogGain = IADC_ANALOG_GAIN;
  
  // Divides CLK_SRC_ADC to set the CLK_ADC frequency
  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                                     CLK_ADC_FREQ,
                                                                     0,
                                                                     iadcCfgModeNormal,
                                                                     init.srcClkPrescale);
  IADC_init(IADC0, &init, &initAllConfigs);

  initSingle.dataValidLevel = IADC_SCANFIFOCFG_DVL_VALID4;
  IADC_initSingle(IADC0, &initSingle, &initSingleInput);
  // Allocate the analog bus for ADC0 inputs
  if ( iadcPosInputPortAPin0 <= SL_BATTERY_MONITOR_IADC_POS
       && SL_BATTERY_MONITOR_IADC_POS <= iadcPosInputPortAPin15 ) {
#if IS_EVEN(SL_BATTERY_MONITOR_IADC_POS)
    GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AEVEN0_ADC0;
#else
    GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AODD0_ADC0;
#endif
  } else if ( iadcPosInputPortBPin0 <= SL_BATTERY_MONITOR_IADC_POS
              && SL_BATTERY_MONITOR_IADC_POS <= iadcPosInputPortBPin15 ) {
#if IS_EVEN(SL_BATTERY_MONITOR_IADC_POS)
    GPIO->BBUSALLOC |= GPIO_BBUSALLOC_BEVEN0_ADC0;
#else
    GPIO->BBUSALLOC |= GPIO_BBUSALLOC_BODD0_ADC0;
#endif
  } else if ( iadcPosInputPortCPin0 <= SL_BATTERY_MONITOR_IADC_POS
              && SL_BATTERY_MONITOR_IADC_POS <= iadcPosInputPortDPin15 ) {
#if IS_EVEN(SL_BATTERY_MONITOR_IADC_POS)
    GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDEVEN0_ADC0;
#else
    GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDODD0_ADC0;
#endif
  }
  #else //series 1
  uint32_t flags;
  
  ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef initAdc = ADC_INITSINGLE_BATTERY_VOLTAGE;

  // Enable ADC clock
  CMU_ClockEnable(cmuClock_ADC0, true);

  // Initialize the ADC peripheral
  ADC_Init(ADC0, &init);

  // Setup ADC for single conversions for reading AVDD with a 5V reference
  ADC_InitSingle(ADC0, &initAdc);

  flags = ADC_IntGet(ADC0);
  ADC_IntClear(ADC0, flags);
  ADC_Start(ADC0, adcStartSingle);

  #endif

  _activate_prs();
  // Set up the generic interrupt controller to activate the readADC event when
  // TX_ACTIVE goes high
  interrupt_id = GPIOINT_CallbackRegisterExt(
      BSP_BATTERYMON_TX_ACTIVE_PIN,
      tx_channel_irq_handler,
      NULL);
  if (interrupt_id != INTERRUPT_UNAVAILABLE) {
    GPIO_ExtIntConfig(
        BSP_BATTERYMON_TX_ACTIVE_PORT,
        BSP_BATTERYMON_TX_ACTIVE_PIN,
        int_id,
        true,
        false,
        true);
    sl_zigbee_app_debug_println("sl_battery_monitor: Initialization complete");
  } else {
    sl_zigbee_app_debug_println("sl_battery_monitor: couldn't initialize PRS interrupt");
  }

  #if defined(_SILICON_LABS_32B_SERIES_2)
  sl_power_manager_em_transition_event_info_t event_info = {
    .event_mask = ( SL_POWER_MANAGER_EVENT_TRANSITION_ENTERING_EM0 \
                    | SL_POWER_MANAGER_EVENT_TRANSITION_LEAVING_EM0),
    .on_event   = handle_em0_transition,
  };
  sl_power_manager_subscribe_em_transition_event(&em0_transition_event,
                                                 &event_info);
  #endif
}

uint16_t sl_battery_monitor_get_voltage_in_mv(void)
{
  return lastReportedVoltageMilliV;
}

static uint32_t halBatteryMonitorReadVoltage()
{
  uint32_t milliV = 0;

  #if defined(_SILICON_LABS_32B_SERIES_2)
  float milliVPerBit = (float)IADC_REFERENCE_VOLTAGE_MILLIVOLTS
                     / (float)_SL_BATTERY_MONITOR_ADC_MAX;
#if SL_BATTERY_MONITOR_R_DIVIDER_ENABLED == 1
  milliVPerBit *= (float)SL_BATTERY_MONITOR_R_DIVIDER_COEF;
#endif  // SL_BATTERY_MONITOR_R_DIVIDER_ENABLED

  IADC_InitSingle_t  initSingle = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t initSingleInput = IADC_SINGLEINPUT_BATTERY;

  initSingle.dataValidLevel = IADC_SCANFIFOCFG_DVL_VALID4;
  IADC_initSingle(IADC0, &initSingle, &initSingleInput);

  // Start IADC conversion
  IADC_command(IADC0, iadcCmdStartSingle);

  // Wait for conversion to be complete
  while((IADC0->STATUS & (_IADC_STATUS_CONVERTING_MASK
              | _IADC_STATUS_SINGLEFIFODV_MASK)) != IADC_STATUS_SINGLEFIFODV); //while combined status bits 8 & 6 don't equal 1 and 0 respectively

  // Get IADC result
  IADC_Result_t sample = IADC_readSingleResult(IADC0);

  milliV = (uint32_t)(milliVPerBit * sample.data);
  emberAfAppPrintln("IADC sample: %d, milliV=%lu", sample.data, milliV);

  #else //series 1

  uint32_t flags;
  uint32_t vData;
  float milliVPerBit = (float)ADC_REFERENCE_VOLTAGE_MILLIVOLTS
                     / (float)_SL_BATTERY_MONITOR_ADC_MAX;
#if SL_BATTERY_MONITOR_R_DIVIDER_ENABLED == 1
  milliVPerBit *= (float)SL_BATTERY_MONITOR_R_DIVIDER_COEF;
#endif // SL_BATTERY_MONITOR_R_DIVIDER_ENABLED
  ADC_InitSingle_TypeDef initAdc = ADC_INITSINGLE_BATTERY_VOLTAGE;

  // In case something else in the system was using the ADC, reconfigure it to
  // properly sample the battery voltage
  ADC_InitSingle(ADC0, &initAdc);

  // The most common and shortest (other than the ACK) transmission is the
  // data poll.  It takes 512 uS for a data poll, which is plenty of time for
  // a 16 cycle conversion
  flags = ADC_IntGet(ADC0);
  ADC_IntClear(ADC0, flags);
  ADC_Start(ADC0, adcStartSingle);

  // wait for the ADC to finish sampling
  while ((ADC_IntGet(ADC0) & ADC_IF_SINGLE) != ADC_IF_SINGLE) {
  }
  vData = ADC_DataSingleGet(ADC0); 

  milliV = (uint32_t)(milliVPerBit * vData);
  emberAfAppPrintln("ADC sample: %d, milliV=%lu", vData, milliV);
  #endif

  return milliV;
}

SL_WEAK void sl_battery_monitor_measurement_ready_cb(uint16_t batteryVoltageMilliV)
{
}

// This event will sample the ADC during a radio transmission and notify any
// interested parties of a new valid battery voltage level via the
// sl_battery_monitor_measurement_ready_cb
static void tx_channel_irq_handler(uint8_t int_id, void *ctx)
{
  uint16_t voltageMilliV;
  uint32_t currentMsTick = halCommonGetInt32uMillisecondTick();
  uint32_t timeSinceLastMeasureMS = currentMsTick - lastBatteryMeasureTick;

  if (timeSinceLastMeasureMS >= MS_BETWEEN_BATTERY_CHECK) {
    voltageMilliV = halBatteryMonitorReadVoltage();

    // filter the voltage to prevent spikes from overly influencing data
    voltageMilliV = filterVoltageSample(voltageMilliV);

    sl_battery_monitor_measurement_ready_cb(voltageMilliV);
    lastReportedVoltageMilliV = voltageMilliV;
    lastBatteryMeasureTick = currentMsTick;
  }
}

// Provide smoothing of the voltage readings by reporting an average over the
// last few values
static uint16_t filterVoltageSample(uint16_t sample)
{
  uint32_t voltageSum;
  uint8_t i;

  if (fifoInitialized) {
    voltageFifo[samplePtr++] = sample;

    if (samplePtr >= FIFO_SIZE) {
      samplePtr = 0;
    }
    voltageSum = 0;
    for (i = 0; i < FIFO_SIZE; i++) {
      voltageSum += voltageFifo[i];
    }
    sample = voltageSum / FIFO_SIZE;
  } else {
    for (i = 0; i < FIFO_SIZE; i++) {
      voltageFifo[i] = sample;
    }
    fifoInitialized = true;
  }

  return sample;
}

#if defined(_SILICON_LABS_32B_SERIES_2)
static void handle_em0_transition(sl_power_manager_em_t from,
                                  sl_power_manager_em_t to)
{
  bool enter = ( SL_POWER_MANAGER_EM0 == from );

  if (enter) {
    IADC0->EN_CLR = IADC_EN_EN;
  } else {
    IADC0->EN_SET = IADC_EN_EN;
  }
}
#endif

/**
 * @brief Activate the PRS signal to drive the TX_ACTIVE pin high
 */
static void _activate_prs(void)
{
#if defined(_SILICON_LABS_32B_SERIES_2)
  CMU_ClockEnable(cmuClock_PRS, true);

  // Initialize the PRS system to drive a GPIO high when the preamble is in the
  // air, effectively becoming a TX_ACT pin
  PRS_SourceAsyncSignalSet(BSP_BATTERYMON_TX_ACTIVE_CHANNEL,
                           PRS_SOURCE,
                           PRS_SIGNAL);
  PRS_PinOutput(BSP_BATTERYMON_TX_ACTIVE_CHANNEL, 
                prsTypeAsync, 
                BSP_BATTERYMON_TX_ACTIVE_PORT, 
                BSP_BATTERYMON_TX_ACTIVE_PIN);
  GPIO_PinModeSet(BSP_BATTERYMON_TX_ACTIVE_PORT,
                  BSP_BATTERYMON_TX_ACTIVE_PIN,
                  gpioModePushPull,
                  0);  
#else //series 1
  CMU_ClockEnable(cmuClock_PRS, true);

  // Initialize the PRS system to drive a GPIO high when the preamble is in the
  // air, effectively becoming a TX_ACT pin
  PRS_SourceSignalSet(BSP_BATTERYMON_TX_ACTIVE_CHANNEL,
                      PRS_SOURCE,
                      PRS_SIGNAL,
                      PRS_EDGE);

  // Enable the PRS channel and set the pin routing per the settings in the
  // board configuration header
  PRS_GpioOutputLocationSet(BSP_BATTERYMON_TX_ACTIVE_CHANNEL,
                            BSP_BATTERYMON_TX_ACTIVE_LOC);
  GPIO_PinModeSet(BSP_BATTERYMON_TX_ACTIVE_PORT,
                  BSP_BATTERYMON_TX_ACTIVE_PIN,
                  gpioModePushPull,
                  0);
#endif
}

/**
 * @brief Deactivate the PRS signal, when need to read the battery voltage ondemand`
 */
static void _deactivate_prs(void)
{
#if defined(_SILICON_LABS_32B_SERIES_2)
  GPIO->PRSROUTE[0].ROUTEEN &= ~(0x1 << (BSP_BATTERYMON_TX_ACTIVE_CHANNEL + _GPIO_PRS_ROUTEEN_ASYNCH0PEN_SHIFT));
#else //series 1
  PRS->ROUTEPEN &= ~(1 << BSP_BATTERYMON_TX_ACTIVE_CHANNEL); 
#endif
}