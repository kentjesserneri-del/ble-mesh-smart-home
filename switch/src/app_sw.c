#include "app_sw.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define TAG "app.sw"

static bool s_relay_state = false;

void init_hw(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << SWITCH_RELAY_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    gpio_set_level(SWITCH_RELAY_PIN, 0);
    s_relay_state = false;

    ESP_LOGI(TAG, "relay on GPIO%d", SWITCH_RELAY_PIN);
}

void switch_set(bool on)
{
    s_relay_state = on;
    gpio_set_level(SWITCH_RELAY_PIN, on ? 1 : 0);
    ESP_LOGI(TAG, "relay -> %s", on ? "ON" : "OFF");
}

bool switch_get(void)
{
    return s_relay_state;
}