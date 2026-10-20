#ifndef app_blecommon_h_
#define app_blecommon_h_

#include <stdint.h>
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_health_model_api.h"

extern uint16_t g_net_idx;
extern uint16_t g_app_idx;

void appble_get_dev_uid(uint8_t *dev_uuid);
esp_err_t appble_bt_init(void);
void appble_provisioning_handler(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *param);
void appble_config_handler(esp_ble_mesh_cfg_server_cb_event_t event, esp_ble_mesh_cfg_server_cb_param_t *param);
void appble_health_evt_handler(esp_ble_mesh_health_server_cb_event_t event, esp_ble_mesh_health_server_cb_param_t *param);

typedef void (*appble_attn_on_f)(void);
typedef void (*appble_attn_off_f)(void);

typedef struct {
   appble_attn_on_f attn_on;
   appble_attn_off_f attn_off;
} appble_attn_cbs_t;

void appble_set_attn_cbs(appble_attn_cbs_t cbs);

#endif