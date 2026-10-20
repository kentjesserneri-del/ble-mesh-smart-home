#include "app_blecommon.h"
#include <stdio.h>
#include <string.h>
#include <sdkconfig.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_ble_mesh_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"

#define TAG "app.ble"

uint16_t g_net_idx = 0x0000;
uint16_t g_app_idx = 0x0000;

void appble_get_dev_uid(uint8_t *dev_uuid)
{
    if (dev_uuid == NULL) return;
    memcpy(dev_uuid + 2, esp_bt_dev_get_address(), BD_ADDR_LEN);
}

esp_err_t appble_bt_init(void)
{
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) return ret;
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) return ret;
    ret = esp_bluedroid_init();
    if (ret) return ret;
    ret = esp_bluedroid_enable();
    return ret;
}

static void reboot_after_ble_reset(void *arg)
{
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    esp_restart();
}

void appble_provisioning_handler(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        g_net_idx = param->node_prov_complete.net_idx;
        ESP_LOGI(TAG, "provisioned. addr: 0x%04x net_idx: 0x%04x", param->node_prov_complete.addr, g_net_idx);
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        ESP_LOGI(TAG, "node reset");
        esp_ble_mesh_node_local_reset();
        xTaskCreate(reboot_after_ble_reset, "reboot", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
        break;
    default: break;
    }
}

void appble_config_handler(esp_ble_mesh_cfg_server_cb_event_t event, esp_ble_mesh_cfg_server_cb_param_t *param)
{
    if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT) {
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD) {
            g_net_idx = param->value.state_change.appkey_add.net_idx;
            g_app_idx = param->value.state_change.appkey_add.app_idx;
            ESP_LOGI(TAG, "config: app key added net_idx=0x%04x app_idx=0x%04x", g_net_idx, g_app_idx);
        }
    }
}

static appble_attn_cbs_t attn_cbs = {NULL, NULL};

void appble_health_evt_handler(esp_ble_mesh_health_server_cb_event_t event, esp_ble_mesh_health_server_cb_param_t *param) {
    if (event == ESP_BLE_MESH_HEALTH_SERVER_ATTENTION_ON_EVT && attn_cbs.attn_on) attn_cbs.attn_on();
    if (event == ESP_BLE_MESH_HEALTH_SERVER_ATTENTION_OFF_EVT && attn_cbs.attn_off) attn_cbs.attn_off();
}

void appble_set_attn_cbs(appble_attn_cbs_t cbs) { attn_cbs = cbs; }