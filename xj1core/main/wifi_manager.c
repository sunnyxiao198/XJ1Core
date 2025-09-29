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

#include "wifi_manager.h"
#include "config_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "lwip/ip4_addr.h"
#include <string.h>

static const char *TAG = "wifi_manager";

static esp_netif_t *g_wifi_ap_netif = NULL;
static esp_netif_t *g_wifi_sta_netif = NULL;
static bool g_wifi_initialized = false;
static bool g_ap_started = false;
static bool g_sta_connected = false;
static char g_sta_ip[16] = {0};
static int g_sta_rssi = 0;

// WiFi事件处理
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "WiFi AP started");
                g_ap_started = true;
                break;
                
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "WiFi AP stopped");
                g_ap_started = false;
                break;
                
            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
                        MAC2STR(event->mac), event->aid);
                break;
            }
            
            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d",
                        MAC2STR(event->mac), event->aid);
                break;
            }
            
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi STA started");
                break;
                
            case WIFI_EVENT_STA_STOP:
                ESP_LOGI(TAG, "WiFi STA stopped");
                g_sta_connected = false;
                memset(g_sta_ip, 0, sizeof(g_sta_ip));
                break;
                
            case WIFI_EVENT_STA_CONNECTED: {
                wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*) event_data;
                ESP_LOGI(TAG, "Connected to AP SSID:%s, channel:%d, authmode:%d", 
                         event->ssid, event->channel, event->authmode);
                // 注意：这里不设置g_sta_connected=true，等待IP_EVENT_STA_GOT_IP事件
                ESP_LOGI(TAG, "等待获取IP地址...");
                break;
            }
            
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
                ESP_LOGW(TAG, "❌ WiFi连接断开 SSID:%s, 原因:%d", event->ssid, event->reason);
                g_sta_connected = false;
                memset(g_sta_ip, 0, sizeof(g_sta_ip));
                
                // 根据断开原因给出提示
                switch(event->reason) {
                    case WIFI_REASON_NO_AP_FOUND:
                        ESP_LOGE(TAG, "找不到WiFi热点，请检查SSID是否正确");
                        break;
                    case WIFI_REASON_AUTH_FAIL:
                        ESP_LOGE(TAG, "WiFi认证失败，请检查密码是否正确");
                        break;
                    case WIFI_REASON_ASSOC_LEAVE:
                        ESP_LOGW(TAG, "WiFi主动断开连接");
                        break;
                    default:
                        ESP_LOGW(TAG, "WiFi断开，原因代码: %d", event->reason);
                        break;
                }
                
                // 简单的重连机制（ESP-IDF会自动重连，这里只是提示）
                ESP_LOGI(TAG, "将自动尝试重新连接...");
                break;
            }
            
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(TAG, "🎉 WiFi连接成功! IP地址: " IPSTR, IP2STR(&event->ip_info.ip));
                ESP_LOGI(TAG, "网关: " IPSTR ", 子网掩码: " IPSTR, 
                         IP2STR(&event->ip_info.gw), IP2STR(&event->ip_info.netmask));
                
                // 更新状态变量
                snprintf(g_sta_ip, sizeof(g_sta_ip), IPSTR, IP2STR(&event->ip_info.ip));
                g_sta_connected = true;
                
                // 获取AP信息和RSSI
                wifi_ap_record_t ap_info;
                if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                    g_sta_rssi = ap_info.rssi;
                    ESP_LOGI(TAG, "连接到AP: %s, 信号强度: %d dBm, 信道: %d", 
                             ap_info.ssid, ap_info.rssi, ap_info.primary);
                } else {
                    ESP_LOGW(TAG, "无法获取AP信息");
                    g_sta_rssi = 0;
                }
                
                ESP_LOGI(TAG, "WiFi状态已更新: connected=%s, ip=%s", 
                         g_sta_connected ? "true" : "false", g_sta_ip);
                break;
            }
            
            case IP_EVENT_STA_LOST_IP:
                ESP_LOGI(TAG, "Lost IP address");
                g_sta_connected = false;
                memset(g_sta_ip, 0, sizeof(g_sta_ip));
                break;
                
            default:
                break;
        }
    }
}

esp_err_t wifi_manager_init(void) {
    if (g_wifi_initialized) {
        return ESP_OK;
    }
    
    // 创建网络接口
    g_wifi_ap_netif = esp_netif_create_default_wifi_ap();
    g_wifi_sta_netif = esp_netif_create_default_wifi_sta();
    
    // 初始化WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 注册事件处理器
    ret = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                            &wifi_event_handler, NULL, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                            &wifi_event_handler, NULL, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_LOST_IP, 
                                            &wifi_event_handler, NULL, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP lost event handler: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 设置WiFi模式为AP+STA
    ret = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 启动WiFi
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return ret;
    }
    
    g_wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi manager initialized successfully");
    return ESP_OK;
}

esp_err_t wifi_manager_start_ap(void) {
    if (!g_wifi_initialized) {
        ESP_LOGE(TAG, "WiFi not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 获取AP配置
    xj1_wifi_ap_config_t ap_config;
    esp_err_t ret = config_manager_get_wifi_ap(&ap_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get AP config");
        return ret;
    }
    
    // 配置AP参数
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(ap_config.ssid),
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    
    strcpy((char*)wifi_config.ap.ssid, ap_config.ssid);
    strcpy((char*)wifi_config.ap.password, ap_config.password);
    
    if (strlen(ap_config.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    // 设置AP配置
    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AP config: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 配置AP IP地址
    esp_netif_ip_info_t ip_info;
    esp_netif_dhcps_stop(g_wifi_ap_netif);
    
    memset(&ip_info, 0, sizeof(ip_info));
    esp_netif_str_to_ip4(ap_config.ip, &ip_info.ip);
    IP4_ADDR(&ip_info.gw, 192, 168, 5, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    
    ret = esp_netif_set_ip_info(g_wifi_ap_netif, &ip_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set AP IP info: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_netif_dhcps_start(g_wifi_ap_netif);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start DHCP server: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "WiFi AP started - SSID: %s, IP: %s", ap_config.ssid, ap_config.ip);
    return ESP_OK;
}

esp_err_t wifi_manager_stop_ap(void) {
    if (!g_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_netif_dhcps_stop(g_wifi_ap_netif);
    g_ap_started = false;
    
    ESP_LOGI(TAG, "WiFi AP stopped");
    return ESP_OK;
}

esp_err_t wifi_manager_connect_sta(const char* ssid, const char* password) {
    if (!g_wifi_initialized) {
        ESP_LOGE(TAG, "WiFi not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!ssid) {
        ESP_LOGE(TAG, "SSID cannot be null");
        return ESP_ERR_INVALID_ARG;
    }
    
    // 先断开现有连接
    esp_wifi_disconnect();
    
    // 配置STA参数
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    
    // 设置STA配置
    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set STA config: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 连接WiFi
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect WiFi: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", ssid);
    return ESP_OK;
}

esp_err_t wifi_manager_disconnect_sta(void) {
    if (!g_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = esp_wifi_disconnect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disconnect WiFi: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "WiFi STA disconnected");
    return ESP_OK;
}

esp_err_t wifi_manager_scan(wifi_scan_result_t* results, int max_results, int* actual_results) {
    if (!g_wifi_initialized || !results || !actual_results) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *actual_results = 0;
    
    // 启动扫描
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = 300,
            },
        },
    };
    
    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi scan: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 获取扫描结果
    uint16_t ap_count = 0;
    ret = esp_wifi_scan_get_ap_num(&ap_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get scan result count: %s", esp_err_to_name(ret));
        return ret;
    }
    
    if (ap_count == 0) {
        ESP_LOGW(TAG, "No WiFi networks found");
        return ESP_OK;
    }
    
    // 限制结果数量
    uint16_t scan_count = (ap_count > max_results) ? max_results : ap_count;
    
    wifi_ap_record_t* ap_records = malloc(sizeof(wifi_ap_record_t) * scan_count);
    if (!ap_records) {
        ESP_LOGE(TAG, "Failed to allocate memory for scan results");
        return ESP_ERR_NO_MEM;
    }
    
    ret = esp_wifi_scan_get_ap_records(&scan_count, ap_records);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get scan results: %s", esp_err_to_name(ret));
        free(ap_records);
        return ret;
    }
    
    // 转换结果格式
    for (int i = 0; i < scan_count; i++) {
        strncpy(results[i].ssid, (char*)ap_records[i].ssid, sizeof(results[i].ssid) - 1);
        results[i].ssid[sizeof(results[i].ssid) - 1] = '\0';
        results[i].rssi = ap_records[i].rssi;
        results[i].authmode = ap_records[i].authmode;
    }
    
    *actual_results = scan_count;
    free(ap_records);
    
    ESP_LOGI(TAG, "WiFi scan completed, found %d networks", scan_count);
    return ESP_OK;
}

/**
 * @brief 比较函数，用于按RSSI排序
 */
static int compare_rssi(const void *a, const void *b) {
    wifi_scan_result_t *ap_a = (wifi_scan_result_t *)a;
    wifi_scan_result_t *ap_b = (wifi_scan_result_t *)b;
    // 降序排列（信号强度从强到弱）
    return ap_b->rssi - ap_a->rssi;
}

/**
 * @brief 增强版WiFi扫描函数
 */
esp_err_t wifi_manager_scan_advanced(wifi_scan_result_t* results, int max_results, int* actual_results, const wifi_scan_options_t* options) {
    if (!g_wifi_initialized || !results || !actual_results) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *actual_results = 0;
    
    // 设置默认选项
    wifi_scan_options_t default_options = {
        .show_hidden = false,
        .sort_by_rssi = true,
        .scan_timeout = 5000
    };
    
    const wifi_scan_options_t* scan_opts = options ? options : &default_options;
    
    ESP_LOGI(TAG, "开始增强WiFi扫描 - 显示隐藏网络:%s, 按信号排序:%s, 超时:%lu毫秒", 
             scan_opts->show_hidden ? "是" : "否",
             scan_opts->sort_by_rssi ? "是" : "否",
             scan_opts->scan_timeout);
    
    // 启动扫描
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = scan_opts->show_hidden,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = scan_opts->scan_timeout > 1000 ? scan_opts->scan_timeout / 10 : 300,
            },
        },
    };
    
    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start advanced WiFi scan: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 获取扫描结果
    uint16_t ap_count = 0;
    ret = esp_wifi_scan_get_ap_num(&ap_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get scan result count: %s", esp_err_to_name(ret));
        return ret;
    }
    
    if (ap_count == 0) {
        ESP_LOGW(TAG, "No WiFi networks found");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "扫描发现 %d 个WiFi网络", ap_count);
    
    // 限制结果数量
    uint16_t scan_count = (ap_count > max_results) ? max_results : ap_count;
    
    wifi_ap_record_t* ap_records = malloc(sizeof(wifi_ap_record_t) * scan_count);
    if (!ap_records) {
        ESP_LOGE(TAG, "Failed to allocate memory for scan results");
        return ESP_ERR_NO_MEM;
    }
    
    ret = esp_wifi_scan_get_ap_records(&scan_count, ap_records);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get scan results: %s", esp_err_to_name(ret));
        free(ap_records);
        return ret;
    }
    
    // 转换结果格式
    for (int i = 0; i < scan_count; i++) {
        strncpy(results[i].ssid, (char*)ap_records[i].ssid, sizeof(results[i].ssid) - 1);
        results[i].ssid[sizeof(results[i].ssid) - 1] = '\0';
        results[i].rssi = ap_records[i].rssi;
        results[i].authmode = ap_records[i].authmode;
    }
    
    // 按信号强度排序（如果需要）
    if (scan_opts->sort_by_rssi) {
        qsort(results, scan_count, sizeof(wifi_scan_result_t), compare_rssi);
        ESP_LOGI(TAG, "WiFi网络已按信号强度排序");
    }
    
    *actual_results = scan_count;
    free(ap_records);
    
    ESP_LOGI(TAG, "增强WiFi扫描完成，返回 %d 个网络", scan_count);
    
    // 打印扫描结果详情
    for (int i = 0; i < scan_count; i++) {
        const char* auth_name = "未知";
        switch (results[i].authmode) {
            case WIFI_AUTH_OPEN: auth_name = "开放"; break;
            case WIFI_AUTH_WEP: auth_name = "WEP"; break;
            case WIFI_AUTH_WPA_PSK: auth_name = "WPA"; break;
            case WIFI_AUTH_WPA2_PSK: auth_name = "WPA2"; break;
            case WIFI_AUTH_WPA_WPA2_PSK: auth_name = "WPA/WPA2"; break;
            case WIFI_AUTH_WPA3_PSK: auth_name = "WPA3"; break;
            case WIFI_AUTH_WPA2_WPA3_PSK: auth_name = "WPA2/WPA3"; break;
            case WIFI_AUTH_WPA2_ENTERPRISE: auth_name = "WPA2企业版"; break;
            case WIFI_AUTH_WPA3_ENTERPRISE: auth_name = "WPA3企业版"; break;
            case WIFI_AUTH_WPA2_WPA3_ENTERPRISE: auth_name = "WPA2/WPA3企业版"; break;
            case WIFI_AUTH_WAPI_PSK: auth_name = "WAPI"; break;
            case WIFI_AUTH_OWE: auth_name = "OWE"; break;
            case WIFI_AUTH_WPA3_ENT_192: auth_name = "WPA3企业版192"; break;
            case WIFI_AUTH_WPA3_EXT_PSK: auth_name = "WPA3扩展PSK"; break;
            case WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE: auth_name = "WPA3扩展PSK混合"; break;
            case WIFI_AUTH_DPP: auth_name = "DPP"; break;
            case WIFI_AUTH_MAX:
            default: auth_name = "未知"; break;
        }
        
        ESP_LOGI(TAG, "  %d. SSID: '%s', RSSI: %d dBm, 加密: %s", 
                 i + 1, 
                 strlen(results[i].ssid) > 0 ? results[i].ssid : "[隐藏网络]", 
                 results[i].rssi, 
                 auth_name);
    }
    
    return ESP_OK;
}

esp_err_t wifi_manager_get_status(wifi_status_t* status) {
    if (!status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 实时检查WiFi连接状态
    wifi_ap_record_t ap_info;
    bool real_connected = false;
    
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        // 如果能获取到AP信息，说明确实连接了
        real_connected = true;
        g_sta_rssi = ap_info.rssi;  // 更新RSSI
        
        // 如果全局状态不一致，更新它
        if (!g_sta_connected) {
            ESP_LOGW(TAG, "检测到状态不一致，正在更新...");
            g_sta_connected = true;
            
            // 尝试获取当前IP地址
            esp_netif_ip_info_t ip_info;
            if (esp_netif_get_ip_info(g_wifi_sta_netif, &ip_info) == ESP_OK) {
                snprintf(g_sta_ip, sizeof(g_sta_ip), IPSTR, IP2STR(&ip_info.ip));
                ESP_LOGI(TAG, "更新IP地址: %s", g_sta_ip);
            }
        }
    } else {
        // 无法获取AP信息，可能真的断开了
        if (g_sta_connected) {
            ESP_LOGW(TAG, "检测到连接可能已断开");
            real_connected = false;
        }
    }
    
    status->ap_enabled = g_ap_started;
    status->sta_connected = real_connected ? real_connected : g_sta_connected;
    strncpy(status->sta_ip, g_sta_ip, sizeof(status->sta_ip) - 1);
    status->sta_ip[sizeof(status->sta_ip) - 1] = '\0';
    status->sta_rssi = g_sta_rssi;
    
    // 调试信息
    static int debug_count = 0;
    if (++debug_count % 20 == 0) {  // 每20次调用打印一次调试信息
        ESP_LOGI(TAG, "WiFi状态查询: AP=%s, STA=%s, IP=%s, RSSI=%d", 
                 status->ap_enabled ? "ON" : "OFF",
                 status->sta_connected ? "CONNECTED" : "DISCONNECTED",
                 status->sta_ip,
                 status->sta_rssi);
    }
    
    return ESP_OK;
}

bool wifi_manager_is_sta_connected(void) {
    return g_sta_connected;
}

esp_err_t wifi_manager_get_sta_ip(char* ip_str) {
    if (!ip_str) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (g_sta_connected) {
        strcpy(ip_str, g_sta_ip);
        return ESP_OK;
    } else {
        ip_str[0] = '\0';
        return ESP_ERR_INVALID_STATE;
    }
}

/**
 * @brief WiFi扫描测试函数
 * @note 这是一个测试函数，演示如何使用不同的扫描选项
 */
esp_err_t wifi_manager_test_scan_functions(void) {
    ESP_LOGI(TAG, "=== WiFi扫描功能测试开始 ===");
    
    // 测试1：基础扫描
    ESP_LOGI(TAG, "测试1：基础WiFi扫描");
    wifi_scan_result_t basic_results[WIFI_SCAN_MAX_AP];
    int basic_count = 0;
    esp_err_t ret = wifi_manager_scan(basic_results, WIFI_SCAN_MAX_AP, &basic_count);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "基础扫描成功，发现 %d 个网络", basic_count);
    } else {
        ESP_LOGE(TAG, "基础扫描失败: %s", esp_err_to_name(ret));
    }
    
    // 测试2：增强扫描 - 显示隐藏网络，按信号强度排序
    ESP_LOGI(TAG, "测试2：增强WiFi扫描（显示隐藏网络+信号排序）");
    wifi_scan_options_t advanced_options = {
        .show_hidden = true,
        .sort_by_rssi = true,
        .scan_timeout = 10000  // 10秒超时
    };
    
    wifi_scan_result_t advanced_results[WIFI_SCAN_MAX_AP];
    int advanced_count = 0;
    ret = wifi_manager_scan_advanced(advanced_results, WIFI_SCAN_MAX_AP, &advanced_count, &advanced_options);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "增强扫描成功，发现 %d 个网络", advanced_count);
    } else {
        ESP_LOGE(TAG, "增强扫描失败: %s", esp_err_to_name(ret));
    }
    
    // 测试3：快速扫描 - 不显示隐藏网络，不排序
    ESP_LOGI(TAG, "测试3：快速WiFi扫描（不显示隐藏网络+不排序）");
    wifi_scan_options_t quick_options = {
        .show_hidden = false,
        .sort_by_rssi = false,
        .scan_timeout = 3000  // 3秒超时
    };
    
    wifi_scan_result_t quick_results[WIFI_SCAN_MAX_AP];
    int quick_count = 0;
    ret = wifi_manager_scan_advanced(quick_results, WIFI_SCAN_MAX_AP, &quick_count, &quick_options);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "快速扫描成功，发现 %d 个网络", quick_count);
    } else {
        ESP_LOGE(TAG, "快速扫描失败: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "=== WiFi扫描功能测试完成 ===");
    return ESP_OK;
}
