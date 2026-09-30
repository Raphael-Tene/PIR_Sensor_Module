#ifndef __pir_app_H
#define __pir_app_H

#include "stm32f0xx_hal.h"
#include <stdbool.h>

#define WARM_UP_TIME_MS 3000U
#define DISARM_TIME_MS 3000U
#define DEBOUNCE_TIME_MS 30U
#define SLOW_BLINK_HALF_MS 500U
#define FAST_BLINK_HALF_MS 100U
#define BEEP_ON_MS 200U
#define BEEP_OFF_MS 200U

#define PIR_PORT GPIOA
#define PIR_PIN GPIO_PIN_1
#define BUTTON_PORT GPIOC
#define BUTTON_PIN GPIO_PIN_8
#define BUZZER_PORT GPIOA
#define BUZZER_PIN GPIO_PIN_6
#define INDICATOR_PORT GPIOB
#define ON_BOARD_LED GPIO_PIN_8

typedef enum { OFF, SOLID, SLOW_BLINK, FAST_BLINK } indicator_patterns_t;
typedef enum { BEEP_OFF, BEEP_ALARM } buzzer_patterns_t;
typedef enum { WARMUP, ARMED, ALARM, DISARMED } app_state_t;

/* PIR sensor: true while the sensor output is high. */
void pir_init(void);
bool pir_is_motion(void);

/* Button: call button_update() every loop pass; button_was_pressed()
 * returns true once per debounced press (reading it consumes it). */
void button_init(void);
void button_update(void);
bool button_was_pressed(void);

/* Status LED: indicator_set() changes pattern (no-op if unchanged),
 * indicator_update() drives the blinking. */
void indicator_init(void);
void indicator_set(indicator_patterns_t indicator_pattern);
void indicator_update(void);

/* Active buzzer: buzzer_set() changes pattern (no-op if unchanged),
 * buzzer_update() drives the beeping. */
void buzzer_init(void);
void buzzer_set(buzzer_patterns_t buzzer_pattern);
void buzzer_update(void);

/* Alarm state machine: WARMUP -> ARMED -> ALARM -> DISARMED. */
void app_init(void);
void app_update(void);

#endif
