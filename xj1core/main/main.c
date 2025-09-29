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

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"

// 项目模块头文件
#include "config_manager.h"
#include "auth.h"
#include "web_server.h"
#include "wifi_manager.h"
#include "ethernet_manager.h"
#include "bluetooth_manager.h"
#include "mqtt_manager.h"

// 确保包含WiFi扫描相关的定义
#include "esp_wifi.h"

static const char *TAG = "main";

// 任务句柄
static TaskHandle_t status_update_task_handle = NULL;
// static TaskHandle_t wifi_scan_task_handle = NULL; // 已禁用WiFi扫描任务

/**
 * @brief 状态更新任务
 */
static void status_update_task(void *pvParameters) {
    network_status_t status;
    
    while (1) {
        // 获取各模块状态
        memset(&status, 0, sizeof(network_status_t));
        
        // WiFi状态（安全获取）
        wifi_status_t wifi_status;
        memset(&wifi_status, 0, sizeof(wifi_status));
        if (wifi_manager_get_status(&wifi_status) == ESP_OK) {
            status.wifi_ap_enabled = wifi_status.ap_enabled;
            status.wifi_sta_connected = wifi_status.sta_connected;
            strncpy(status.wifi_sta_ip, wifi_status.sta_ip, sizeof(status.wifi_sta_ip) - 1);
        } else {
            status.wifi_ap_enabled = false;
            status.wifi_sta_connected = false;
            strcpy(status.wifi_sta_ip, "");
        }
        
        // 以太网状态（安全获取）
        ethernet_status_t eth_status;
        memset(&eth_status, 0, sizeof(eth_status));
        if (ethernet_manager_get_status(&eth_status) == ESP_OK) {
            status.ethernet_connected = eth_status.connected;
            strncpy(status.ethernet_ip, eth_status.ip, sizeof(status.ethernet_ip) - 1);
        } else {
            status.ethernet_connected = false;
            strcpy(status.ethernet_ip, "");
        }
        
        // 蓝牙状态（安全获取）
        bluetooth_status_t bt_status;
        memset(&bt_status, 0, sizeof(bt_status));
        if (bluetooth_manager_get_status(&bt_status) == ESP_OK) {
            status.bluetooth_enabled = bt_status.enabled;
            status.bluetooth_clients = bt_status.client_count;
        } else {
            status.bluetooth_enabled = false;
            status.bluetooth_clients = 0;
        }
        
        // MQTT状态（安全获取）
        mqtt_status_t mqtt_status;
        memset(&mqtt_status, 0, sizeof(mqtt_status));
        if (mqtt_client_get_status(&mqtt_status) == ESP_OK) {
            status.mqtt_connected = mqtt_status.connected;
        } else {
            status.mqtt_connected = false;
        }
        
        // 更新Web服务器状态
        web_server_update_network_status(&status);
        
        // 清理过期的认证会话
        auth_cleanup_expired_sessions();
        
        // 获取状态更新间隔配置
        interval_config_t interval_config;
        int update_interval = 5000; // 默认值
        if (config_manager_get_intervals(&interval_config) == ESP_OK) {
            update_interval = interval_config.status_update_interval;
        }
        
        vTaskDelay(pdMS_TO_TICKS(update_interval));
    }
}

// 定期WiFi扫描任务已被移除，WiFi扫描只在Web界面手动触发时执行

// 手动WiFi扫描演示函数已被移除，WiFi扫描只在Web界面手动触发时执行

/**
 * @brief 系统初始化
 */
static esp_err_t system_init(void) {
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "XJ1Core System Starting...");
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "Device: ESP32-S3-WROOM-1-N16R8");
    ESP_LOGI(TAG, "Firmware: XJ1Core v1.0.0");
    ESP_LOGI(TAG, "=================================");
    
    // 初始化NVS
    ESP_LOGI(TAG, "Initializing NVS...");
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized successfully");
    
    // 初始化网络接口（关键模块，失败则退出）
    ESP_LOGI(TAG, "Initializing network interface...");
    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize network interface: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Network interface initialized");
    
    // 初始化配置管理器（关键模块，失败则退出）
    ESP_LOGI(TAG, "Initializing configuration manager...");
    ret = config_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "CRITICAL: Failed to initialize configuration manager: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "System cannot continue without configuration. Restarting in 5 seconds...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
        return ret;
    }
    ESP_LOGI(TAG, "Configuration manager initialized");
    
    // 初始化认证模块（关键模块，失败则退出）
    ESP_LOGI(TAG, "Initializing authentication module...");
    ret = auth_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "CRITICAL: Failed to initialize authentication module: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "System cannot continue without authentication. Restarting in 5 seconds...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
        return ret;
    }
    ESP_LOGI(TAG, "Authentication module initialized");
    
    // 初始化WiFi管理器（关键模块，但允许失败）
    ESP_LOGI(TAG, "Initializing WiFi manager...");
    ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi manager: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "WiFi functionality will be disabled");
    } else {
        ESP_LOGI(TAG, "WiFi manager initialized");
    }
    
    // 启动WiFi AP模式（如果WiFi管理器初始化成功）
    ESP_LOGI(TAG, "Starting WiFi AP...");
    ret = wifi_manager_start_ap();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to start WiFi AP: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "WiFi AP functionality disabled");
    } else {
        ESP_LOGI(TAG, "WiFi AP started successfully");
    }
    
    // 自动连接WiFi STA（如果配置了SSID）
    ESP_LOGI(TAG, "Attempting to connect WiFi STA...");
    xj1_wifi_sta_config_t sta_config;
    ret = config_manager_get_wifi_sta(&sta_config);
    if (ret == ESP_OK && strlen(sta_config.ssid) > 0) {
        ESP_LOGI(TAG, "Connecting to WiFi STA: %s", sta_config.ssid);
        ret = wifi_manager_connect_sta(sta_config.ssid, sta_config.password);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to initiate WiFi STA connection: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "WiFi STA connection initiated");
        }
    } else {
        ESP_LOGW(TAG, "No WiFi STA SSID configured, skipping connection");
    }
    
    // 初始化以太网管理器（非关键模块，允许失败）
    ESP_LOGI(TAG, "Initializing ethernet manager...");
    ret = ethernet_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Ethernet manager initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "Ethernet functionality disabled");
    } else {
        ESP_LOGI(TAG, "Ethernet manager initialized");
        ESP_LOGW(TAG, "注意：ESP32-S3-WROOM-1没有内置以太网，需要外接模块");
        ret = ethernet_manager_start();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to start ethernet: %s", esp_err_to_name(ret));
        }
    }
    
    // 初始化蓝牙管理器（非关键模块，允许失败）
    ESP_LOGI(TAG, "Initializing bluetooth manager...");
    ret = bluetooth_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Bluetooth manager initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "Bluetooth functionality disabled");
    } else {
        ESP_LOGI(TAG, "Bluetooth manager initialized");
        ret = bluetooth_manager_start();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to start bluetooth: %s", esp_err_to_name(ret));
        }
    }
    
    // 等待一段时间让WiFi连接建立
    ESP_LOGI(TAG, "等待网络连接建立...");
    vTaskDelay(pdMS_TO_TICKS(3000));  // 等待3秒
    
    // 初始化MQTT客户端（非关键模块，允许失败）
    ESP_LOGI(TAG, "Initializing MQTT client...");
    ret = mqtt_client_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "MQTT client initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "MQTT functionality disabled");
    } else {
        ESP_LOGI(TAG, "MQTT client initialized");
        
        // 检查网络连接状态
        wifi_status_t wifi_status;
        bool network_ready = false;
        if (wifi_manager_get_status(&wifi_status) == ESP_OK && wifi_status.sta_connected) {
            ESP_LOGI(TAG, "WiFi STA已连接，IP: %s", wifi_status.sta_ip);
            network_ready = true;
        } else {
            ESP_LOGW(TAG, "WiFi STA未连接，MQTT可能无法连接到外部服务器");
            ESP_LOGI(TAG, "如果MQTT服务器在AP网络内(192.168.5.x)，仍可尝试连接");
            network_ready = true; // 仍然尝试连接，可能是AP内的服务器
        }
        
        if (network_ready) {
            ret = mqtt_client_start();
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to start MQTT client: %s", esp_err_to_name(ret));
            } else {
                ESP_LOGI(TAG, "MQTT client started successfully");
                
                // 启动MQTT监控任务
                ret = mqtt_client_start_monitor();
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "🔍 MQTT连接监控任务已启动");
                }
                
                // 启动学生思念心跳任务
                ESP_LOGI(TAG, "💖 启动师生通信功能...");
                ret = mqtt_client_start_student_heartbeat();
                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "🎓 学生思念心跳任务已启动，每5秒向老师发送思念");
                } else {
                    ESP_LOGW(TAG, "💔 学生心跳任务启动失败");
                }
            }
        } else {
            ESP_LOGW(TAG, "网络未就绪，延迟启动MQTT客户端");
        }
    }
    
    // 初始化Web服务器（关键模块，失败则重试）
    ESP_LOGI(TAG, "Initializing web server...");
    ret = web_server_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize web server: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "Retrying web server initialization in 3 seconds...");
        vTaskDelay(pdMS_TO_TICKS(3000));
        ret = web_server_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Web server initialization failed again, continuing without web interface");
        }
    }
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Web server initialized");
        
        // 启动Web服务器
        ESP_LOGI(TAG, "Starting web server...");
        ret = web_server_start();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
            ESP_LOGW(TAG, "Retrying web server start in 3 seconds...");
            vTaskDelay(pdMS_TO_TICKS(3000));
            ret = web_server_start();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Web server start failed again, system will continue without web interface");
            } else {
                ESP_LOGI(TAG, "Web server started on port 80 (retry successful)");
            }
        } else {
            // 获取Web服务器配置显示端口
            web_server_config_t web_config;
            if (config_manager_get_web_server(&web_config) == ESP_OK) {
                ESP_LOGI(TAG, "Web server started on port %d", web_config.port);
            } else {
                ESP_LOGI(TAG, "Web server started");
            }
        }
    }
    
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "XJ1Core System Started Successfully!");
    ESP_LOGI(TAG, "Access web interface at: http://192.168.5.1");
    ESP_LOGI(TAG, "Default login: admin / 123456");
    ESP_LOGI(TAG, "=================================");
    
    return ESP_OK;
}

/**
 * @brief 应用程序入口点
 */
void app_main(void) {
    // 系统初始化
    esp_err_t ret = system_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "System initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "System will restart in 5 seconds...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
        return;
    }
    
    // 创建状态更新任务
    xTaskCreate(
        status_update_task,
        "status_update",
        4096,
        NULL,
        5,
        &status_update_task_handle
    );
    
    // 等待WiFi系统稳定后进行扫描演示
    ESP_LOGI(TAG, "等待WiFi系统稳定...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // 演示WiFi扫描功能
    ESP_LOGI(TAG, "WiFi扫描功能已禁用，只在Web界面手动触发时才执行");
    
    // 主循环 - 监控系统状态
    static int main_loop_count = 0;
    
    while (1) {
        main_loop_count++;
        
        // 打印系统运行信息
        ESP_LOGI(TAG, "System running... Free heap: %" PRIu32 " bytes", esp_get_free_heap_size());
        
        // 检查系统健康状态
        if (esp_get_free_heap_size() < 10240) {  // 低于10KB内存时警告
            ESP_LOGW(TAG, "Low memory warning! Free heap: %" PRIu32 " bytes", esp_get_free_heap_size());
        }
        
        // WiFi扫描已禁用，只在Web界面手动触发时才执行
        if (main_loop_count % 10 == 0) {
            ESP_LOGI(TAG, "💡 提示：WiFi扫描已禁用，请通过Web界面手动触发扫描");
        }
        
        // 每30秒打印一次状态
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
