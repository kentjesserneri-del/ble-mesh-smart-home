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

    // Set HIGH on boot so the active-LOW relay stays OFF
    gpio_set_level(SWITCH_RELAY_PIN, 1);
    s_relay_state = false;

    ESP_LOGI(TAG, "relay on GPIO%d (active-LOW)", SWITCH_RELAY_PIN);
}

void switch_set(bool on)
{
    s_relay_state = on;
    // Invert the logic: ON = LOW (0), OFF = HIGH (1)
    gpio_set_level(SWITCH_RELAY_PIN, on ? 0 : 1);
    ESP_LOGI(TAG, "relay -> %s", on ? "ON" : "OFF");
}

bool switch_get(void)
{
    return s_relay_state;
}