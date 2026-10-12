#ifndef APP_SENSOR_H
#define APP_SENSOR_H

#include <stdbool.h>

#define LIGHT_DO_PIN     34
#define LIGHT_SAMPLE_MS  1000

typedef void (*light_changed_f)(bool);

void init_hw(light_changed_f cb);
bool is_light_low(void);

#endif