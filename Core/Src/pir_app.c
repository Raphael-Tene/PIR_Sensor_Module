#include "pir_app.h"
#include "stm32f0xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

/* ---- button state ---- */
static bool last_raw;       // what the pin read on the previous pass
static bool stable;         // the debounced, trusted level
static uint32_t changed_at; // HAL_GetTick() when last_raw last changed
static bool press_pending;  // a press waiting to be collected

/* ---- indicator state ---- */
static indicator_patterns_t indicator_current_pattern;
static uint32_t last_toggle;

/* ---- buzzer state ---- */
static buzzer_patterns_t buzzer_current_pattern;
static uint32_t buzzer_last_change;
static bool buzzer_is_on;

/* ---- app state ---- */
static app_state_t state;
static uint32_t entered_at;

void pir_init(void) {
  GPIO_InitTypeDef pir_config = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  pir_config.Pin = PIR_PIN;
  pir_config.Mode = GPIO_MODE_INPUT;
  pir_config.Pull = GPIO_PULLDOWN;

  HAL_GPIO_Init(PIR_PORT, &pir_config);
}

bool pir_is_motion(void) {
  return HAL_GPIO_ReadPin(PIR_PORT, PIR_PIN) == GPIO_PIN_SET;
}

static bool read_pressed(void) {
  return HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_SET;
}

void button_init(void) {
  GPIO_InitTypeDef button_config = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  button_config.Pin = BUTTON_PIN;
  button_config.Mode = GPIO_MODE_INPUT;
  button_config.Pull = GPIO_PULLDOWN;

  HAL_GPIO_Init(BUTTON_PORT, &button_config);

  last_raw = read_pressed();
  stable = last_raw;
  changed_at = HAL_GetTick();
  press_pending = false;
}

void button_update(void) {
  uint32_t now = HAL_GetTick();
  bool raw = read_pressed();

  if (raw != last_raw) {
    last_raw = raw;
    changed_at = now;
  }
  if ((now - changed_at) >= DEBOUNCE_TIME_MS && raw != stable) {
    stable = raw;
    if (stable) {
      press_pending = true;
    }
  }
}

bool button_was_pressed(void) {
  if (press_pending) {
    press_pending = false;
    return true;
  }
  return false;
}

void indicator_init(void) {
  GPIO_InitTypeDef indicator_config = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  indicator_config.Pin = ON_BOARD_LED;
  indicator_config.Mode = GPIO_MODE_OUTPUT_PP;
  indicator_config.Pull = GPIO_NOPULL;
  indicator_config.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(INDICATOR_PORT, &indicator_config);
  HAL_GPIO_WritePin(INDICATOR_PORT, ON_BOARD_LED, GPIO_PIN_RESET);
  indicator_current_pattern = OFF;
  last_toggle = HAL_GetTick();
}

void indicator_set(indicator_patterns_t indicator_pattern) {
  if (indicator_pattern == indicator_current_pattern) {
    return;
  }
  indicator_current_pattern = indicator_pattern;
  last_toggle = HAL_GetTick();

  // every pattern except OFF starts with the LED lit
  if (indicator_pattern == OFF) {
    HAL_GPIO_WritePin(INDICATOR_PORT, ON_BOARD_LED, GPIO_PIN_RESET);
  } else {
    HAL_GPIO_WritePin(INDICATOR_PORT, ON_BOARD_LED, GPIO_PIN_SET);
  }
}

void indicator_update(void) {
  uint32_t half;

  if (indicator_current_pattern == SLOW_BLINK) {
    half = SLOW_BLINK_HALF_MS;
  } else if (indicator_current_pattern == FAST_BLINK) {
    half = FAST_BLINK_HALF_MS;
  } else {
    return; // OFF and SOLID are static, set once in indicator_set
  }

  uint32_t now = HAL_GetTick();
  if ((now - last_toggle) >= half) {
    HAL_GPIO_TogglePin(INDICATOR_PORT, ON_BOARD_LED);
    last_toggle = now;
  }
}

void buzzer_init(void) {
  GPIO_InitTypeDef buzzer_config = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  buzzer_config.Pin = BUZZER_PIN;
  buzzer_config.Mode = GPIO_MODE_OUTPUT_PP;
  buzzer_config.Pull = GPIO_NOPULL;
  buzzer_config.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(BUZZER_PORT, &buzzer_config);
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
  buzzer_current_pattern = BEEP_OFF;
  buzzer_is_on = false;
  buzzer_last_change = HAL_GetTick();
}

void buzzer_set(buzzer_patterns_t buzzer_pattern) {
  if (buzzer_pattern == buzzer_current_pattern) {
    return;
  }
  buzzer_current_pattern = buzzer_pattern;
  buzzer_last_change = HAL_GetTick();

  if (buzzer_pattern == BEEP_OFF) {
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
    buzzer_is_on = false;
  } else {
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
    buzzer_is_on = true;
  }
}

void buzzer_update(void) {
  if (buzzer_current_pattern == BEEP_OFF) {
    return;
  }

  uint32_t now = HAL_GetTick();
  uint32_t wait_time = buzzer_is_on ? BEEP_ON_MS : BEEP_OFF_MS;

  if ((now - buzzer_last_change) >= wait_time) {
    buzzer_is_on = !buzzer_is_on;
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN,
                      buzzer_is_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    buzzer_last_change = now;
  }
}

/* The output table: every state's LED and buzzer behaviour lives here. */
static void enter_state(app_state_t new_state) {
  state = new_state;
  entered_at = HAL_GetTick();

  switch (new_state) {
  case WARMUP:
    indicator_set(SOLID);
    buzzer_set(BEEP_OFF);
    break;
  case ARMED:
    indicator_set(SLOW_BLINK);
    buzzer_set(BEEP_OFF);
    break;
  case ALARM:
    indicator_set(FAST_BLINK);
    buzzer_set(BEEP_ALARM);
    break;
  case DISARMED:
    indicator_set(OFF);
    buzzer_set(BEEP_OFF);
    break;
  }
}

void app_init(void) { enter_state(WARMUP); }

void app_update(void) {
  bool pressed = button_was_pressed(); // read once: it is consumed
  uint32_t in_state = HAL_GetTick() - entered_at;

  switch (state) {
  case WARMUP:
    if (in_state >= WARM_UP_TIME_MS) {
      enter_state(ARMED);
    }
    break;

  case ARMED:
    if (pir_is_motion()) {
      enter_state(ALARM);
    }
    break;

  case ALARM:
    if (pressed) {
      enter_state(DISARMED);
    } else if (!pir_is_motion()) {
      enter_state(ARMED);
    }
    break;

  case DISARMED:
    if (pressed || in_state >= DISARM_TIME_MS) {
      enter_state(ARMED);
    }
    break;
  }
}
