/***************************************************************************//**
 * @file
 * @brief sl_battery_monitor Config
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef SL_BATTERY_MONITOR_CONFIG_H
#define SL_BATTERY_MONITOR_CONFIG_H

// <<< Use Configuration Wizard in Context Menu

// <h> Battery Monitor settings
// <o SL_BATTERY_MONITOR_TIMEOUT_MINUTES> Monitor Timeout (Minutes)
// <1..1000:1>
// <i> Default: 30
// <i> The length of time between battery reads.
#define SL_BATTERY_MONITOR_TIMEOUT_MINUTES 30

// <o SL_BATTERY_MONITOR_SAMPLE_FIFO_SIZE> Sample Collection FIFO Size
// <1..20:1>
// <i> Default: 16
// <i> The number of entries in the fifo used to collect ADC reads of the battery
#define SL_BATTERY_MONITOR_SAMPLE_FIFO_SIZE 16

// </h> end Battery Monitor config
// <<< end of configuration section >>>


// <<< sl:start pin_tool >>>
// <prs gpio=true > SL_BATTERY_MONITOR_TX_ACTIVE
// $[PRS_SL_BATTERY_MONITOR_TX_ACTIVE]
#define SL_BATTERY_MONITOR_TX_ACTIVE_CHANNEL    0
#define SL_BATTERY_MONITOR_TX_ACTIVE_PORT       gpioPortA
#define SL_BATTERY_MONITOR_TX_ACTIVE_PIN        0
// [PRS_SL_BATTERY_MONITOR_TX_ACTIVE]$
// <<< sl:end pin_tool >>>

#endif // SL_BATTERY_MONITOR_CONFIG_H