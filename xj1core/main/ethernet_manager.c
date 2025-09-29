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

#include "ethernet_manager.h"
#include "config_manager.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_eth.h"
#include <string.h>

static const char *TAG = "ethernet_manager";

__attribute__((unused)) static esp_netif_t *g_eth_netif = NULL;
static bool g_eth_initialized = false;
static bool g_eth_connected = false;
static char g_eth_ip[16] = {0};

// 以太网事件处理
__attribute__((unused)) static void ethernet_event_handler(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data) {
    if (event_base == ETH_EVENT) {
        switch (event_id) {
            case ETHERNET_EVENT_CONNECTED:
                ESP_LOGI(TAG, "Ethernet Link Up");
                break;
                
            case ETHERNET_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "Ethernet Link Down");
                g_eth_connected = false;
                memset(g_eth_ip, 0, sizeof(g_eth_ip));
                break;
                
            case ETHERNET_EVENT_START:
                ESP_LOGI(TAG, "Ethernet Started");
                break;
                
            case ETHERNET_EVENT_STOP:
                ESP_LOGI(TAG, "Ethernet Stopped");
                g_eth_connected = false;
                memset(g_eth_ip, 0, sizeof(g_eth_ip));
                break;
                
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_ETH_GOT_IP: {
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(TAG, "Ethernet Got IP Address");
                ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
                ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
                ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
                
                snprintf(g_eth_ip, sizeof(g_eth_ip), IPSTR, IP2STR(&event->ip_info.ip));
                g_eth_connected = true;
                break;
            }
            
            case IP_EVENT_ETH_LOST_IP:
                ESP_LOGI(TAG, "Ethernet Lost IP");
                g_eth_connected = false;
                memset(g_eth_ip, 0, sizeof(g_eth_ip));
                break;
                
            default:
                break;
        }
    }
}

esp_err_t ethernet_manager_init(void) {
    if (g_eth_initialized) {
        return ESP_OK;
    }
    
    // 注意：ESP32-S3-WROOM-1-N16R8 通常没有内置以太网PHY
    // 这里提供一个基本的框架，实际使用需要外接以太网模块
    ESP_LOGW(TAG, "Ethernet manager initialized (no physical ethernet on ESP32-S3-WROOM-1)");
    ESP_LOGI(TAG, "以太网功能需要外接以太网模块才能使用");
    
    g_eth_initialized = true;
    return ESP_OK;
}

esp_err_t ethernet_manager_start(void) {
    if (!g_eth_initialized) {
        ESP_LOGE(TAG, "Ethernet not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // ESP32-S3-WROOM-1-N16R8 没有内置以太网，这里模拟状态
    ESP_LOGW(TAG, "Ethernet start requested (no physical ethernet available)");
    return ESP_OK;
}

esp_err_t ethernet_manager_stop(void) {
    if (!g_eth_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    g_eth_connected = false;
    memset(g_eth_ip, 0, sizeof(g_eth_ip));
    
    ESP_LOGI(TAG, "Ethernet stopped");
    return ESP_OK;
}

esp_err_t ethernet_manager_get_status(ethernet_status_t* status) {
    if (!status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 获取以太网配置
    ethernet_config_t eth_config;
    esp_err_t ret = config_manager_get_ethernet(&eth_config);
    if (ret != ESP_OK) {
        return ret;
    }
    
    status->connected = g_eth_connected;
    strncpy(status->ip, g_eth_connected ? g_eth_ip : eth_config.ip, sizeof(status->ip) - 1);
    strncpy(status->netmask, eth_config.netmask, sizeof(status->netmask) - 1);
    strncpy(status->gateway, eth_config.gateway, sizeof(status->gateway) - 1);
    strncpy(status->dns, eth_config.dns, sizeof(status->dns) - 1);
    
    return ESP_OK;
}

bool ethernet_manager_is_connected(void) {
    return g_eth_connected;
}

esp_err_t ethernet_manager_get_ip(char* ip_str) {
    if (!ip_str) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (g_eth_connected) {
        strcpy(ip_str, g_eth_ip);
        return ESP_OK;
    } else {
        ip_str[0] = '\0';
        return ESP_ERR_INVALID_STATE;
    }
}
