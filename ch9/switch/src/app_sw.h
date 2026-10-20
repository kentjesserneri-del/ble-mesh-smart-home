#ifndef APP_SW_H
#define APP_SW_H

#include <stdbool.h>

#define SWITCH_RELAY_PIN 4

void init_hw(void);
void switch_set(bool on);
bool switch_get(void);

#endif