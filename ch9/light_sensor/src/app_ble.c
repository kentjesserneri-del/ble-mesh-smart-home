#include "app_ble.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_gap_ble_api.h"
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"
#include "esp_ble_mesh_health_model_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "app.ble"

#define CID_ESP 0x02E5

static uint8_t dev_uuid[16] = {0xdd, 0xdd};
static bool pending_state_valid;
static bool pending_state;
static TaskHandle_t publish_task_handle;

static esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
    .default_ttl = 7,
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub_0, 2 + 3, ROLE_NODE);
static esp_ble_mesh_gen_onoff_srv_t onoff_server_0 = {
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .rsp_ctrl.status_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
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

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_0, &onoff_server_0),
    ESP_BLE_MESH_MODEL_HEALTH_SRV(&health_server, &health_pub_0),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
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

static void generic_server_cb(esp_ble_mesh_generic_server_cb_event_t event,
                              esp_ble_mesh_generic_server_cb_param_t *param)
{
    esp_ble_mesh_model_t *model = &root_models[1];
    esp_ble_mesh_gen_onoff_srv_t *srv = &onoff_server_0;
    esp_ble_mesh_msg_ctx_t *ctx = &param->ctx;

    switch (event)
    {
    case ESP_BLE_MESH_GENERIC_SERVER_STATE_CHANGE_EVT:
    case ESP_BLE_MESH_GENERIC_SERVER_RECV_SET_MSG_EVT:
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ||
            param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK)
        {
            if (ctx->recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET)
            {
                esp_ble_mesh_server_model_send_msg(model, ctx,
                                                   ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS,
                                                   sizeof(srv->state.onoff), &srv->state.onoff);
            }
        }
        break;
    default:
        ESP_LOGW(TAG, "unexpected event (%d)", event);
        break;
    }
}

static esp_err_t publish_state(void)
{
    esp_ble_mesh_model_t *model = &root_models[1];
    esp_ble_mesh_gen_onoff_srv_t *srv = &onoff_server_0;

    if (!esp_ble_mesh_node_is_provisioned() ||
        model->pub == NULL || model->pub->publish_addr == 0x0000) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = esp_ble_mesh_model_publish(model,
                                               ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS,
                                               sizeof(srv->state.onoff),
                                               &srv->state.onoff, ROLE_NODE);
    if (err == ESP_OK) {
        pending_state_valid = false;
    } else {
        ESP_LOGE(TAG, "Failed to publish light state: %s", esp_err_to_name(err));
    }
    return err;
}

static void publish_pending_state_task(void *arg)
{
    while (pending_state_valid) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (!pending_state_valid) {
            break;
        }

        onoff_server_0.state.onoff = pending_state;
        if (publish_state() == ESP_OK) {
            ESP_LOGI(TAG, "Published pending light state: %d", pending_state);
        }
    }

    publish_task_handle = NULL;
    vTaskDelete(NULL);
}

void appble_update_state(bool onoff)
{
    onoff_server_0.state.onoff = onoff;
    pending_state = onoff;
    pending_state_valid = true;

    if (publish_state() == ESP_OK) {
        return;
    }

    ESP_LOGW(TAG, "Light state deferred until Mesh is provisioned and configured");
    if (publish_task_handle == NULL) {
        xTaskCreate(publish_pending_state_task, "mesh_publish",
                    3072, NULL, 4, &publish_task_handle);
    }
}

void init_ble(appble_attn_cbs_t cbs)
{
    appble_set_attn_cbs(cbs);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    if (appble_bt_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "bluetooth_init failed");
        return;
    }

    appble_get_dev_uid(dev_uuid);

    esp_ble_mesh_register_prov_callback(appble_provisioning_handler);
    esp_ble_mesh_register_config_server_callback(appble_config_handler);
    esp_ble_mesh_register_generic_server_callback(generic_server_cb);
    esp_ble_mesh_register_health_server_callback(appble_health_evt_handler);

    if (esp_ble_mesh_init(&provision, &composition) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize mesh stack");
        return;
    }
    if (esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable mesh node");
        return;
    }

    ESP_LOGI(TAG, "BLE Mesh node initialized");
}