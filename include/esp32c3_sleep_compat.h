#pragma once

#if defined(ESP32)
#include <esp_sleep.h>

#if !SOC_PM_SUPPORT_EXT1_WAKEUP
typedef enum {
	ESP_EXT1_WAKEUP_ANY_LOW = 0,
	ESP_EXT1_WAKEUP_ANY_HIGH = 1
} esp_sleep_ext1_wakeup_mode_t;

static inline esp_err_t esp_sleep_enable_ext1_wakeup(uint64_t io_mask, esp_sleep_ext1_wakeup_mode_t level_mode) {
#if SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP
	return esp_deep_sleep_enable_gpio_wakeup(
			io_mask,
			(level_mode == ESP_EXT1_WAKEUP_ANY_HIGH) ? ESP_GPIO_WAKEUP_GPIO_HIGH : ESP_GPIO_WAKEUP_GPIO_LOW);
#else
	(void)io_mask;
	(void)level_mode;
	return ESP_ERR_NOT_SUPPORTED;
#endif
}
#endif

#if !SOC_PM_SUPPORT_EXT0_WAKEUP
static inline esp_err_t esp_sleep_enable_ext0_wakeup(gpio_num_t gpio_num, int level) {
	return esp_sleep_enable_ext1_wakeup(
			(1ULL << gpio_num),
			level ? ESP_EXT1_WAKEUP_ANY_HIGH : ESP_EXT1_WAKEUP_ANY_LOW);
}
#endif
#endif
