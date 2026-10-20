#include "app_ble.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_gap_ble_api.h"
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"
#include "esp_ble_mesh_health_model_api.h"

#define TAG "app.ble"
#define CID_ESP 0x02E5

static uint8_t dev_uuid[16] = {0xdd, 0xdd};

static esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
    .default_ttl = 7,
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_cli_pub, 2 + 1, ROLE_NODE);
static esp_ble_mesh_client_t onoff_client;

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&onoff_cli_pub, &onoff_client),
};

static uint8_t test_ids[] = {0, 1, 2};
ESP_BLE_MESH_MODEL_PUB_DEFINE(health_pub_0, 10, ROLE_NODE);
static esp_ble_mesh_health_srv_t health_server = {
    .health_test = {
        .id_count = ARRAY_SIZE(test_ids),
        .test_ids = test_ids,
        .company_id = CID_ESP,
    },
};

static esp_ble_mesh_model_t root_models_health[] = {
    ESP_BLE_MESH_MODEL_HEALTH_SRV(&health_server, &health_pub_0),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, root_models_health),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
    .output_size = 0,
    .output_actions = 0,
};

void appble_set_switch(bool onoff)
{
    if (!esp_ble_mesh_node_is_provisioned()) {
        ESP_LOGE(TAG, "Node not provisioned");
        return;
    }

    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_generic_client_set_state_t set = {0};

    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK;
    common.model = onoff_client.model;
    common.ctx.net_idx = g_net_idx;
    common.ctx.app_idx = g_app_idx;

    if (common.model->pub == NULL || common.model->pub->publish_addr == 0x0000) {
        ESP_LOGE(TAG, "No publication address set in the app!");
        return;
    }
    common.ctx.addr = common.model->pub->publish_addr;
    common.ctx.send_ttl = 7;
    common.ctx.send_rel = false;
    common.msg_timeout = 0;
    common.msg_role = ROLE_NODE;

    set.onoff_set.op_en = false;
    set.onoff_set.onoff = onoff ? 1 : 0;
    set.onoff_set.tid = (uint8_t)(xTaskGetTickCount() & 0xFF);

    esp_err_t err = esp_ble_mesh_generic_client_set_state(&common, &set);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Send failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Sent ONOFF to 0x%04x (from app pub)", common.ctx.addr);
}

void init_ble(appble_attn_cbs_t cbs)
{
    appble_set_attn_cbs(cbs);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    if (appble_bt_init() != ESP_OK) {
        ESP_LOGE(TAG, "bluetooth_init failed");
        return;
    }

    appble_get_dev_uid(dev_uuid);

    esp_ble_mesh_register_prov_callback(appble_provisioning_handler);
    esp_ble_mesh_register_config_server_callback(appble_config_handler);
    esp_ble_mesh_register_health_server_callback(appble_health_evt_handler);

    if (esp_ble_mesh_init(&provision, &composition) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize mesh stack");
        return;
    }
    if (esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable mesh node");
        return;
    }

    ESP_LOGI(TAG, "BLE Mesh node initialized");
}