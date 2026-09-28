#ifndef __pir_app_H

#include "stm32f0xx_hal.h"
#include <stdbool.h>
#define __pir_app_H
#define WARM_UP_TIME_MS 3000U
#define DISARM_TIME_MS 3000U
#define DEBOUNCE_TIME_MS 30U
#define SLOW_BLINK_HALF_MS 500U
#define FAST_BLINK_HALF_MS 100U
#define BEEP_ON_MS 200U
#define BEEP_OFF_MS 200U
#define PIR_PIN GPIO_PIN_5    // port C
#define BUTTON_PIN GPIO_PIN_8 // port C
#define BUZZER_PIN GPIO_PIN_6
#define ON_BOARD_LED GPIO_PIN_8 // port B

typedef enum { OFF, SOLID, SLOW_BLINK, FAST_BLINK } indicator_patterns_t;
typedef enum { BEEP_OFF, BEEP_ALARM } buzzer_patterns_t;
typedef enum {
  indicator_current_pattern,
  indicator_last_toggle
} remember_indicator_state_t;
typedef enum {
  buzzer_current_pattern,
  buzzer_last_change,
  buzzer_is_on
} remember_buzzer_state_t;
typedef enum { WARMUP, ARMED, ALARM, DISARMED } app_state_t;
typedef enum { state, entered_at } remember_app_state_t;
void pir_init(void);
bool pir_is_motion(void);
void button_init(void);
void button_update(void);
bool button_was_pressed(void);
void indicator_init(void);
void indicator_set(indicator_patterns_t indicator_pattern);
void indicator_update(void);
void buzzer_init(void);
void buzzer_set(buzzer_patterns_t buzzer_pattern);
void buzzer_update(void);

void enter_app_state(app_state_t app_state);

void app_init(void);
void app_update(app_state_t app_state);

#endif