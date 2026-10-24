#include "app_sensor.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "sensor"

static light_changed_f s_cb = NULL;
static bool s_last_low = false;

static void sensor_task(void *arg)
{
    int stable = -1;
    int agree_count = 0;

    while (1) {
        int level = gpio_get_level(LIGHT_DO_PIN);

        if (level == stable) {
            agree_count = 0;
        } else {
            agree_count++;
            if (agree_count >= 3) {
                stable = level;
                agree_count = 0;

                bool low = (stable == LIGHT_DARK_LEVEL);
                s_last_low = low;
                ESP_LOGI(TAG, "light change: DO=%d low=%d", stable, low);

                if (s_cb) s_cb(low);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(LIGHT_SAMPLE_MS));
    }
}

void init_hw(light_changed_f cb)
{
    s_cb = cb;

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << LIGHT_DO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "DO photoresistor on GPIO%d (dark level=%d -> ON)",
             LIGHT_DO_PIN, LIGHT_DARK_LEVEL);
}

bool is_light_low(void)
{
    return s_last_low;
}