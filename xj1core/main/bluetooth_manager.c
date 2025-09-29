/**
* 项目：湘江一号 - 物联网核心教学套件  
* 作者：ironxiao
* 描述：ESP32物联网开发核心模板，实现MQTT双向通信
*       献给所有怀揣物联梦想的学子及开发者
*       愿此代码成为你们探索世界的起点
* 
* 特别致谢：谨以此项目感谢我的恩师唐家乾老师
*       唐老师，您的教诲是我前行路上的光。学生很想您 * 
* 开源协议：MIT License 
* 物联网开发核心：从零构建一个双向通信系统
* 许多初学者认为物联网（IoT）高深莫测，但其实它的核心逻辑可以非常直观。想象一下，我们要构建一个完整的'神经'系统：
*    '感官'（传感器） 负责采集数据。
*     '脊髓'（ESP32） 负责汇集信息并传递指令。
*    '大脑'（云端） 负责处理信息并做出决策。
*而贯穿全程的'神经信号'，就是MQTT协议。
*本项目将带您亲手实现这个系统：首先，为ESP32设计一个Web配置管理系统；接着，实现MQTT双向通信，将传感器数据上报云端；最后，完成从云端下发指令控制传感器的闭环。 打通这个流程，您就掌握了物联网开发最精髓的骨架。
 */

#include "bluetooth_manager.h"
#include "config_manager.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include <string.h>

static const char *TAG = "bluetooth_manager";

static bool g_bt_initialized = false;
static bool g_bt_enabled = false;
static int g_client_count = 0;
static bt_client_info_t g_clients[BT_MAX_CLIENTS];

// BLE事件处理
__attribute__((unused)) static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "Advertising data set complete");
            break;
            
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising start failed");
            } else {
                ESP_LOGI(TAG, "Advertising started successfully");
            }
            break;
            
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising stop failed");
            } else {
                ESP_LOGI(TAG, "Advertising stopped successfully");
            }
            break;
            
        default:
            break;
    }
}

// GATTS事件处理
__attribute__((unused)) static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(TAG, "GATTS register event, app_id: %d", param->reg.app_id);
            break;
            
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "Client connected, conn_id: %d", param->connect.conn_id);
            if (g_client_count < BT_MAX_CLIENTS) {
                // 添加客户端信息（简化实现）
                snprintf(g_clients[g_client_count].device_name, sizeof(g_clients[g_client_count].device_name), 
                        "Client-%d", g_client_count + 1);
                snprintf(g_clients[g_client_count].address, sizeof(g_clients[g_client_count].address), 
                        "%02x:%02x:%02x:%02x:%02x:%02x",
                        param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                        param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
                g_clients[g_client_count].connected = true;
                g_client_count++;
            }
            break;
            
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "Client disconnected, conn_id: %d", param->disconnect.conn_id);
            // 简化实现：减少客户端数量
            if (g_client_count > 0) {
                g_client_count--;
                if (g_client_count >= 0 && g_client_count < BT_MAX_CLIENTS) {
                    g_clients[g_client_count].connected = false;
                }
            }
            break;
            
        default:
            break;
    }
}

esp_err_t bluetooth_manager_init(void) {
    if (g_bt_initialized) {
        return ESP_OK;
    }
    
    // 简化实现：仅初始化基本状态，不进行复杂的蓝牙初始化
    // 避免链接错误，在后续版本中完善
    ESP_LOGI(TAG, "Bluetooth manager initialized (simplified implementation)");
    ESP_LOGW(TAG, "Full Bluetooth functionality will be implemented in future updates");
    
    g_bt_initialized = true;
    return ESP_OK;
}

esp_err_t bluetooth_manager_start(void) {
    if (!g_bt_initialized) {
        ESP_LOGE(TAG, "Bluetooth not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 简化实现：仅标记为启用状态
    // 完整的BLE广播功能需要更复杂的GATT服务配置
    g_bt_enabled = true;
    ESP_LOGI(TAG, "Bluetooth service marked as enabled");
    ESP_LOGW(TAG, "Full BLE advertising features will be implemented in future updates");
    return ESP_OK;
}

esp_err_t bluetooth_manager_stop(void) {
    if (!g_bt_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    g_bt_enabled = false;
    g_client_count = 0;
    memset(g_clients, 0, sizeof(g_clients));
    
    ESP_LOGI(TAG, "Bluetooth stopped");
    return ESP_OK;
}

esp_err_t bluetooth_manager_get_status(bluetooth_status_t* status) {
    if (!status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 获取蓝牙配置
    bluetooth_config_t bt_config;
    esp_err_t ret = config_manager_get_bluetooth(&bt_config);
    if (ret != ESP_OK) {
        return ret;
    }
    
    status->enabled = g_bt_enabled;
    status->client_count = g_client_count;
    strncpy(status->device_name, bt_config.device_name, sizeof(status->device_name) - 1);
    
    // 复制客户端信息
    for (int i = 0; i < g_client_count && i < BT_MAX_CLIENTS; i++) {
        memcpy(&status->clients[i], &g_clients[i], sizeof(bt_client_info_t));
    }
    
    return ESP_OK;
}

bool bluetooth_manager_is_enabled(void) {
    return g_bt_enabled;
}

int bluetooth_manager_get_client_count(void) {
    return g_client_count;
}
