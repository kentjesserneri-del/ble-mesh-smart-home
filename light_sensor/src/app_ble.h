#ifndef APP_BLE_H
#define APP_BLE_H

#include <stdlib.h>
#include "app_blecommon.h"

void init_ble(appble_attn_cbs_t cbs);
void appble_update_state(bool onoff);

#endif