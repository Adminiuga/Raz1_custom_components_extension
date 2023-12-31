/*******************************************************************************
 * @file
 * @brief API to blink simple led instances
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Alexei Chetroi</b>
 *******************************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include "sl_component_catalog.h"

#ifdef SL_CATALOG_SIMPLE_LED_PRESENT
#include "zigbee_app_framework_event.h"
#include "sl_simple_led_instances.h"
#include "sl_led.h"
#include "rz_led_blink_config.h"

// ------------------------------------------------------------------------------
// Private macros
#define led_turn_on(led) sl_led_turn_on(sl_simple_led_array[led])
#define led_turn_off(led) sl_led_turn_off(sl_simple_led_array[led])
#define led_toggle(led) sl_led_toggle(sl_simple_led_array[led])
#define Event(idx) (&blinkEvent[idx])

// ------------------------------------------------------------------------------
// private types and enums
typedef enum {
    LED_ON = 0x00,
    LED_OFF = 0x01,
    LED_BLINKING_ON = 0x02,
    LED_BLINKING_OFF = 0x03,
    LED_BLINK_PATTERN = 0x04,
} ledState_t;

typedef struct {
    uint16_t pattern[RZ_LED_BLINK_MAX_PATTERN_LEN];
    uint8_t length;
    uint8_t index;
} blinkPatternState_t;

typedef struct {
    blinkPatternState_t patternState;
    ledState_t ledState;
    uint16_t timeMs;
    uint8_t count;
} blinkState_t;

// ------------------------------------------------------------------------------
// private global variables
static blinkState_t blinkState[SL_SIMPLE_LED_COUNT];
static sl_zigbee_event_t blinkEvent[SL_SIMPLE_LED_COUNT];

// ------------------------------------------------------------------------------
// Forward declaration of private functions
static void handlerLedBlinkLedEventHandler(uint8_t ledIndex);
#if SL_SIMPLE_LED_COUNT >= 1
static void handlerLedBlinkLed0EventHandler(sl_zigbee_event_t *event);
#endif
#if SL_SIMPLE_LED_COUNT >= 2
static void handlerLedBlinkLed1EventHandler(sl_zigbee_event_t *event);
#endif
#if SL_SIMPLE_LED_COUNT >= 3
static void handlerLedBlinkLed2EventHandler(sl_zigbee_event_t *event);
#endif
#if SL_SIMPLE_LED_COUNT >= 4
static void handlerLedBlinkLed3EventHandler(sl_zigbee_event_t *event);
#endif
#if SL_SIMPLE_LED_COUNT >= 5
static void handlerLedBlinkLed4EventHandler(sl_zigbee_event_t *event);
#endif
static void _blinkLedOnOrOff(uint32_t timeMs, uint8_t ledIndex, ledState_t ledState);
static void _blinkPatternHandler(uint8_t ledIndex);

// ------------------------------------------------------------------------------
// public functions implementation
/* @brief Initialize handler
 * @note Initialize Events to handle blinker events
 *******************************************************************************/
void rz_led_blink_init(void) {
    for (uint8_t i = 0; i < SL_SIMPLE_LED_COUNT; i++) {
        blinkState[i].ledState = LED_OFF;
        blinkState[i].count = 0;
    }

#if SL_SIMPLE_LED_COUNT >= 1
    sl_zigbee_event_init(Event(0), handlerLedBlinkLed0EventHandler);
#endif
#if SL_SIMPLE_LED_COUNT >= 2
  sl_zigbee_event_init(Event(1), handlerLedBlinkLed1EventHandler);
#endif
#if SL_SIMPLE_LED_COUNT >= 3
  sl_zigbee_event_init(Event(2), handlerLedBlinkLed2EventHandler);
#endif
#if SL_SIMPLE_LED_COUNT >= 4
  sl_zigbee_event_init(Event(3), handlerLedBlinkLed3EventHandler);
#endif
#if SL_SIMPLE_LED_COUNT >= 5
  sl_zigbee_event_init(Event(4), handlerLedBlinkLed4EventHandler);
#endif
}

void rz_led_blink_blink_led_off(uint32_t timeMs, uint8_t ledIndex) {
    _blinkLedOnOrOff(timeMs, ledIndex, LED_OFF);
}

void rz_led_blink_blink_led_on(uint32_t timeMs, uint8_t ledIndex) {
    _blinkLedOnOrOff(timeMs, ledIndex, LED_ON);
}

void rz_led_blink_counted(uint8_t count, uint16_t blinkTimeMs, uint8_t ledIndex) {
    if (ledIndex >= RZ_LED_BLINK_MAX_LED_INSTANCES)
        return;

    blinkState[ledIndex].timeMs = blinkTimeMs;
    led_turn_off(ledIndex);
    blinkState[ledIndex].ledState = LED_BLINKING_OFF;
    sl_zigbee_event_set_delay_ms(Event(ledIndex), blinkState[ledIndex].timeMs);
    blinkState[ledIndex].count = count;
}

void rz_led_blink_pattern(uint8_t count, uint8_t length, uint16_t *pattern, uint8_t ledIndex) {
    if (ledIndex >= RZ_LED_BLINK_MAX_LED_INSTANCES || length < 2)
        return;

    led_turn_on(ledIndex);

    blinkState[ledIndex].ledState = LED_BLINK_PATTERN;

    if (length > RZ_LED_BLINK_MAX_PATTERN_LEN) {
        length = RZ_LED_BLINK_MAX_PATTERN_LEN;
    }

    blinkState[ledIndex].patternState.length = length;
    blinkState[ledIndex].count = count;

    for (uint8_t i = 0; i < blinkState[ledIndex].patternState.length; i++) {
        blinkState[ledIndex].patternState.pattern[i] = pattern[i];
    }

    sl_zigbee_event_set_delay_ms(Event(ledIndex),
            blinkState[ledIndex].patternState.pattern[0]);

    blinkState[ledIndex].patternState.index = 1;
}

// ------------------------------------------------------------------------------
// private functions implementation

#if SL_SIMPLE_LED_COUNT >= 1
static void handlerLedBlinkLed0EventHandler(sl_zigbee_event_t *event) {
    handlerLedBlinkLedEventHandler(0);
}
#endif

#if SL_SIMPLE_LED_COUNT >= 2
static void handlerLedBlinkLed1EventHandler(sl_zigbee_event_t *event)
{
  handlerLedBlinkLedEventHandler(1);
}
#endif

#if SL_SIMPLE_LED_COUNT >= 3
static void handlerLedBlinkLed2EventHandler(sl_zigbee_event_t *event)
{
  handlerLedBlinkLedEventHandler(2);
}
#endif

#if SL_SIMPLE_LED_COUNT >= 4
static void handlerLedBlinkLed3EventHandler(sl_zigbee_event_t *event)
{
  handlerLedBlinkLedEventHandler(3);
}
#endif

#if SL_SIMPLE_LED_COUNT >= 5
static void handlerLedBlinkLed4EventHandler(sl_zigbee_event_t *event)
{
  handlerLedBlinkLedEventHandler(4);
}
#endif

/** @brief handle blinking events
 * @note  The API to turn the light on can be used to either change the LED state to
 * a permanent on state, or to flash the LED on temporarily after which time
 * the LED will automatically turn off. This is achieved by specifying a
 * timeout for the on or off state. If you specify 0 for the timeout, the API
 * will permanently turn the LED on. If you specify a non-zero timeout, the
 * LED is immediately turned on, and the event is scheduled in the future
 * based on the timeout. Hence, if we hit this event and the current LED state
 * is ON, we need to turn it off.
 * The same is true for the LED_OFF state, only in the timeout event we need
 * to turn the LED on.
 *
 * @param ledIndex which led to handle event for
 */
static void handlerLedBlinkLedEventHandler(uint8_t ledIndex) {
    // currently only 5 leds are supported
    if (ledIndex >= RZ_LED_BLINK_MAX_LED_INSTANCES)
        return;

    blinkState_t *state = &blinkState[ledIndex];

    switch (state->ledState) {
    case LED_ON:
        // Led was on, it's time to turn it off
        led_turn_off(ledIndex);
        sl_zigbee_event_set_inactive(Event(ledIndex));
        break;

    case LED_OFF:
        // Led was on, it's time to turn it off.
        led_turn_on(ledIndex);
        sl_zigbee_event_set_inactive(Event(ledIndex));
        break;

    case LED_BLINKING_ON:
        led_turn_off(ledIndex);
        if (state->count > 0) {
            if (state->count != 255) { // blink forever if count is 255
                state->count--;
            }
            if (state->count > 0) {
                state->ledState = LED_BLINKING_OFF;
                sl_zigbee_event_set_delay_ms(Event(ledIndex), state->timeMs);
            } else {
                state->ledState = LED_OFF;
                sl_zigbee_event_set_inactive(Event(ledIndex));
            }
        } else {
            state->ledState = LED_BLINKING_OFF;
            sl_zigbee_event_set_delay_ms(Event(ledIndex), state->timeMs);
        }
        break;

    case LED_BLINKING_OFF:
        led_turn_on(ledIndex);
        state->ledState = LED_BLINKING_ON;
        sl_zigbee_event_set_delay_ms(Event(ledIndex), state->timeMs);
        break;

    case LED_BLINK_PATTERN:
        _blinkPatternHandler(ledIndex);
        break;

    default:
        break;
    }
}

static void _blinkPatternHandler(uint8_t ledIndex) {
    blinkState_t *state = &blinkState[ledIndex];
    blinkPatternState_t *blinkState = &(state->patternState);

    if (state->count == 0) {
        led_turn_off(ledIndex);
        state->ledState = LED_OFF;
        sl_zigbee_event_set_inactive(Event(ledIndex));
        return;
    }

    if (blinkState->index % 2 == 1) {
        led_turn_off(ledIndex);
    } else {
        led_turn_on(ledIndex);
    }

    sl_zigbee_event_set_delay_ms(Event(ledIndex), blinkState->pattern[blinkState->index]);

    blinkState->index++;

    if (blinkState->index >= blinkState->length) {
        blinkState->index = 0;
        if (state->count != 255) { // blink forever if count is 255
            state->count--;
        }
    }
}

static void _blinkLedOnOrOff(uint32_t timeMs, uint8_t ledIndex, ledState_t ledState) {
    if (ledIndex >= RZ_LED_BLINK_MAX_LED_INSTANCES)
        return;

    if (LED_ON == ledState) {
        led_turn_on(ledIndex);
    } else {
        led_turn_off(ledIndex);
    }
    blinkState[ledIndex].ledState = ledState;

    if (timeMs > 0) {
        sl_zigbee_event_set_delay_ms(Event(ledIndex), timeMs);
    } else {
        sl_zigbee_event_set_inactive(Event(ledIndex));
    }
}
;

#else // !SL_CATALOG_SIMPLE_LED_PRESENT
void rz_led_blink_blink_led_off(uint32_t timeMs, uint8_t ledIndex) {}
void rz_led_blink_blink_led_on(uint32_t timeMs, uint8_t ledIndex) {}
void rz_led_blink_counted(uint8_t count, uint16_t blinkTimeMs, uint8_t ledIndex) {}
void rz_led_blink_pattern(uint8_t  count,
                                 uint8_t  length,
                                 uint16_t *pattern,
                                 uint8_t  ledIndex) {}

#endif
