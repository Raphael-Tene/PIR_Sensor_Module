#include "pir_app.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include <stdbool.h>

void pir_init(void) {
  pir_config.Pin = PIR_PIN;
  pir_config.Mode = GPIO_MODE_INPUT;
  pir_config.Pull = GPIO_PULLDOWN;

  HAL_GPIO_Init(GPIOA, &pir_config);
};

bool pir_is_motion() {
  if (HAL_GPIO_ReadPin(GPIOA, PIR_PIN) != GPIO_PIN_SET) {
    return false;
  }
  return true;
};

void button_init() {
  button_config.Pin = BUTTON_PIN;
  button_config.Mode = GPIO_MODE_INPUT;
  button_config.Pull = GPIO_PULLDOWN;

  HAL_GPIO_Init(GPIOC, &button_config);

  last_raw = HAL_GPIO_ReadPin(GPIOC, BUTTON_PIN) == GPIO_PIN_SET;
  stable = last_raw;
  changed_at = HAL_GetTick();
  press_pending = false;
}