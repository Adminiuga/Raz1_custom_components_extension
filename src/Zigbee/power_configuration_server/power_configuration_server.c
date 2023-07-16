/***************************************************************************//**
 * @file
 * @brief Power Configuration Server Cluster
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Alexei Chetroi</b>
 *******************************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include "sl_power_server_config.h"

#if SL_POWER_CONFIGURATION_BATTERY_TYPE == "CR2032"
#define SL_POWER_CONFIGURATION_BATTERY_VOLTAGE_CAPACITY \
   { { 3.0, 100 }, { 2.9, 80 }, { 2.8, 60 }, { 2.7, 40 }, { 2.6, 30 }, \
     { 2.5, 20 }, { 2.4, 10 }, { 2.0, 0 } };
#else if SL_POWER_CONFIGURATION_BATTERY_TYPE == "18650"
#define SL_POWER_CONFIGURATION_BATTERY_VOLTAGE_CAPACITY \
   { { 4.2, 100 }, { 4.06, 90 }, { 3.98, 80 }, { 3.92, 70 }, { 3,87, 60 }, 5
   { 3.82, 50 }, { 3.79, 40 }, { 3.77, 30 }, { 3.74, 20 }, { 3.68, 10 } \
   { 3.60, 5 }, { 3.20, 0 } }
#endif

