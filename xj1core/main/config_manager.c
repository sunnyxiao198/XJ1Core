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

#include "config_manager.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include <string.h>
#include <stdio.h>

// 外部引用的嵌入config.ini文件内容
extern const char config_ini_start[] asm("_binary_config_ini_start");
extern const char config_ini_end[] asm("_binary_config_ini_end");

static const char *TAG = "config_manager";
static ini_config_t *g_ini_config = NULL;
static system_config_t g_system_config = {0};
static bool g_config_loaded = false;

/**
 * @brief 将嵌入的config.ini复制到SPIFFS
 */
static esp_err_t copy_embedded_config_to_spiffs(void) {
    // 计算嵌入文件的大小
    size_t config_size = config_ini_end - config_ini_start;
    
    ESP_LOGI(TAG, "Copying embedded config.ini to SPIFFS (%d bytes)", config_size);
    ESP_LOGI(TAG, "Embedded config start: %p, end: %p", config_ini_start, config_ini_end);
    
    // 检查嵌入文件是否有效
    if (config_size == 0 || config_size > 10000) {
        ESP_LOGE(TAG, "Invalid embedded config size: %d bytes", config_size);
        return ESP_FAIL;
    }
    
    // 创建文件并写入内容
    FILE* file = fopen(CONFIG_FILE_PATH, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to create config file in SPIFFS");
        return ESP_FAIL;
    }
    
    size_t written = fwrite(config_ini_start, 1, config_size, file);
    fclose(file);
    
    if (written != config_size) {
        ESP_LOGE(TAG, "Failed to write complete config file (wrote %d of %d bytes)", written, config_size);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Successfully copied embedded config.ini to SPIFFS");
    return ESP_OK;
}

/**
 * @brief 加载默认配置
 */
static void load_default_config(system_config_t* config) {
    // WiFi AP默认配置
    strcpy(config->wifi_ap.ssid, "Sparkriver-AP-01");
    strcpy(config->wifi_ap.ip, "192.168.5.1");
    strcpy(config->wifi_ap.password, "12345678");
    
    // WiFi STA默认配置
    strcpy(config->wifi_sta.ssid, "fengqi-2G");
    strcpy(config->wifi_sta.password, "Xiaoying168");
    
    // 以太网默认配置
    strcpy(config->ethernet.ip, "192.168.1.40");
    strcpy(config->ethernet.netmask, "255.255.255.0");
    strcpy(config->ethernet.dns, "8.8.8.8");
    strcpy(config->ethernet.gateway, "192.168.1.1");
    
    // 认证默认配置
    strcpy(config->auth.username, "admin");
    // 默认密码123456的SHA-256哈希值
    strcpy(config->auth.password_hash, "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92");
    
    // 蓝牙默认配置
    strcpy(config->bluetooth.device_name, "Sparkriver-Ble-01");
    strcpy(config->bluetooth.pairing_password, "123456");
    
    // MQTT默认配置
    strcpy(config->mqtt.broker_host, "localhost");
    config->mqtt.broker_port = 1883;
    strcpy(config->mqtt.client_id, "xj1core-student-01");
    strcpy(config->mqtt.default_topic, "xj1core/data/receive");
    config->mqtt.keepalive = 60;
    strcpy(config->mqtt.topic_student_to_teacher, "xj1core/student/message");
    strcpy(config->mqtt.topic_teacher_to_student, "xj1cloud/teacher/message");
    strcpy(config->mqtt.topic_student_heartbeat, "xj1core/heartbeat");
    strcpy(config->mqtt.topic_student_status, "xj1core/status");
    
    // Web服务器默认配置
    config->web_server.port = 80;
    
    // 超时配置默认值
    config->timeouts.mqtt_reconnect_timeout = 10000;
    config->timeouts.mqtt_connect_timeout = 15000;
    config->timeouts.mqtt_refresh_connection = 30000;
    config->timeouts.wifi_scan_timeout = 5000;
    config->timeouts.wifi_scan_advanced_timeout = 10000;
    config->timeouts.session_max_age = 1800;
    
    // 时间间隔配置默认值
    config->intervals.status_update_interval = 5000;
    config->intervals.heartbeat_interval = 5000;
    config->intervals.monitor_check_interval = 10000;
    
    ESP_LOGI(TAG, "Default configuration loaded");
}

/**
 * @brief 从INI配置加载到系统配置结构体
 */
static void load_from_ini(system_config_t* config) {
    if (!g_ini_config) {
        ESP_LOGE(TAG, "INI config not initialized");
        return;
    }
    
    // WiFi AP配置
    strcpy(config->wifi_ap.ssid, ini_config_get_string(g_ini_config, "wifi_ap", "ssid", "Sparkriver-AP-01"));
    strcpy(config->wifi_ap.ip, ini_config_get_string(g_ini_config, "wifi_ap", "ip", "192.168.5.1"));
    strcpy(config->wifi_ap.password, ini_config_get_string(g_ini_config, "wifi_ap", "password", "12345678"));
    
    // WiFi STA配置
    strcpy(config->wifi_sta.ssid, ini_config_get_string(g_ini_config, "wifi_sta", "ssid", "fengqi-2G"));
    strcpy(config->wifi_sta.password, ini_config_get_string(g_ini_config, "wifi_sta", "password", "Xiaoying168"));
    
    // 以太网配置
    strcpy(config->ethernet.ip, ini_config_get_string(g_ini_config, "ethernet", "ip", "192.168.1.40"));
    strcpy(config->ethernet.netmask, ini_config_get_string(g_ini_config, "ethernet", "netmask", "255.255.255.0"));
    strcpy(config->ethernet.dns, ini_config_get_string(g_ini_config, "ethernet", "dns", "8.8.8.8"));
    strcpy(config->ethernet.gateway, ini_config_get_string(g_ini_config, "ethernet", "gateway", "192.168.1.1"));
    
    // 认证配置
    strcpy(config->auth.username, ini_config_get_string(g_ini_config, "auth", "username", "admin"));
    strcpy(config->auth.password_hash, ini_config_get_string(g_ini_config, "auth", "password_hash", 
           "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92"));
    
    // 蓝牙配置
    strcpy(config->bluetooth.device_name, ini_config_get_string(g_ini_config, "bluetooth", "device_name", "Sparkriver-Ble-01"));
    strcpy(config->bluetooth.pairing_password, ini_config_get_string(g_ini_config, "bluetooth", "pairing_password", "123456"));
    
    // MQTT配置
    strcpy(config->mqtt.broker_host, ini_config_get_string(g_ini_config, "mqtt", "broker_host", "localhost"));
    config->mqtt.broker_port = ini_config_get_int(g_ini_config, "mqtt", "broker_port", 1883);
    strcpy(config->mqtt.client_id, ini_config_get_string(g_ini_config, "mqtt", "client_id", "xj1core-student-01"));
    strcpy(config->mqtt.default_topic, ini_config_get_string(g_ini_config, "mqtt", "default_topic", "xj1core/data/receive"));
    config->mqtt.keepalive = ini_config_get_int(g_ini_config, "mqtt", "keepalive", 60);
    strcpy(config->mqtt.topic_student_to_teacher, ini_config_get_string(g_ini_config, "mqtt", "topic_student_to_teacher", "xj1core/student/message"));
    strcpy(config->mqtt.topic_teacher_to_student, ini_config_get_string(g_ini_config, "mqtt", "topic_teacher_to_student", "xj1cloud/teacher/message"));
    strcpy(config->mqtt.topic_student_heartbeat, ini_config_get_string(g_ini_config, "mqtt", "topic_student_heartbeat", "xj1core/heartbeat"));
    strcpy(config->mqtt.topic_student_status, ini_config_get_string(g_ini_config, "mqtt", "topic_student_status", "xj1core/status"));
    
    // Web服务器配置
    config->web_server.port = ini_config_get_int(g_ini_config, "web_server", "port", 80);
    
    // 超时配置
    config->timeouts.mqtt_reconnect_timeout = ini_config_get_int(g_ini_config, "timeouts", "mqtt_reconnect_timeout", 10000);
    config->timeouts.mqtt_connect_timeout = ini_config_get_int(g_ini_config, "timeouts", "mqtt_connect_timeout", 15000);
    config->timeouts.mqtt_refresh_connection = ini_config_get_int(g_ini_config, "timeouts", "mqtt_refresh_connection", 30000);
    config->timeouts.wifi_scan_timeout = ini_config_get_int(g_ini_config, "timeouts", "wifi_scan_timeout", 5000);
    config->timeouts.wifi_scan_advanced_timeout = ini_config_get_int(g_ini_config, "timeouts", "wifi_scan_advanced_timeout", 10000);
    config->timeouts.session_max_age = ini_config_get_int(g_ini_config, "timeouts", "session_max_age", 1800);
    
    // 时间间隔配置
    config->intervals.status_update_interval = ini_config_get_int(g_ini_config, "intervals", "status_update_interval", 5000);
    config->intervals.heartbeat_interval = ini_config_get_int(g_ini_config, "intervals", "heartbeat_interval", 5000);
    config->intervals.monitor_check_interval = ini_config_get_int(g_ini_config, "intervals", "monitor_check_interval", 10000);
}

/**
 * @brief 从系统配置结构体保存到INI配置
 */
static esp_err_t save_to_ini(const system_config_t* config) {
    if (!g_ini_config) {
        ESP_LOGE(TAG, "INI config not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // WiFi AP配置
    ini_config_set_string(g_ini_config, "wifi_ap", "ssid", config->wifi_ap.ssid);
    ini_config_set_string(g_ini_config, "wifi_ap", "ip", config->wifi_ap.ip);
    ini_config_set_string(g_ini_config, "wifi_ap", "password", config->wifi_ap.password);
    
    // WiFi STA配置
    ini_config_set_string(g_ini_config, "wifi_sta", "ssid", config->wifi_sta.ssid);
    ini_config_set_string(g_ini_config, "wifi_sta", "password", config->wifi_sta.password);
    
    // 以太网配置
    ini_config_set_string(g_ini_config, "ethernet", "ip", config->ethernet.ip);
    ini_config_set_string(g_ini_config, "ethernet", "netmask", config->ethernet.netmask);
    ini_config_set_string(g_ini_config, "ethernet", "dns", config->ethernet.dns);
    ini_config_set_string(g_ini_config, "ethernet", "gateway", config->ethernet.gateway);
    
    // 认证配置
    ini_config_set_string(g_ini_config, "auth", "username", config->auth.username);
    ini_config_set_string(g_ini_config, "auth", "password_hash", config->auth.password_hash);
    
    // 蓝牙配置
    ini_config_set_string(g_ini_config, "bluetooth", "device_name", config->bluetooth.device_name);
    ini_config_set_string(g_ini_config, "bluetooth", "pairing_password", config->bluetooth.pairing_password);
    
    // MQTT配置
    ini_config_set_string(g_ini_config, "mqtt", "broker_host", config->mqtt.broker_host);
    ini_config_set_int(g_ini_config, "mqtt", "broker_port", config->mqtt.broker_port);
    ini_config_set_string(g_ini_config, "mqtt", "client_id", config->mqtt.client_id);
    ini_config_set_string(g_ini_config, "mqtt", "default_topic", config->mqtt.default_topic);
    ini_config_set_int(g_ini_config, "mqtt", "keepalive", config->mqtt.keepalive);
    ini_config_set_string(g_ini_config, "mqtt", "topic_student_to_teacher", config->mqtt.topic_student_to_teacher);
    ini_config_set_string(g_ini_config, "mqtt", "topic_teacher_to_student", config->mqtt.topic_teacher_to_student);
    ini_config_set_string(g_ini_config, "mqtt", "topic_student_heartbeat", config->mqtt.topic_student_heartbeat);
    ini_config_set_string(g_ini_config, "mqtt", "topic_student_status", config->mqtt.topic_student_status);
    
    // Web服务器配置
    ini_config_set_int(g_ini_config, "web_server", "port", config->web_server.port);
    
    return ESP_OK;
}

esp_err_t config_manager_init(void) {
    esp_err_t ret = ESP_OK;
    
    // 初始化SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }
    
    // 创建INI配置句柄
    g_ini_config = ini_config_create();
    if (!g_ini_config) {
        ESP_LOGE(TAG, "Failed to create INI config");
        return ESP_ERR_NO_MEM;
    }
    
    // 尝试从文件加载配置
    ESP_LOGI(TAG, "Attempting to load config from: %s", CONFIG_FILE_PATH);
    ret = ini_config_load_from_file(g_ini_config, CONFIG_FILE_PATH);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load config file from SPIFFS (error: %s), copying embedded config", esp_err_to_name(ret));
        // 复制嵌入的配置文件到SPIFFS
        ret = copy_embedded_config_to_spiffs();
        if (ret == ESP_OK) {
            // 重新尝试加载配置文件
            ret = ini_config_load_from_file(g_ini_config, CONFIG_FILE_PATH);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Config file loaded from embedded copy");
                load_from_ini(&g_system_config);
            } else {
                ESP_LOGE(TAG, "Failed to load config even after copying embedded file, using defaults");
                load_default_config(&g_system_config);
                save_to_ini(&g_system_config);
                ini_config_save_to_file(g_ini_config, CONFIG_FILE_PATH);
            }
        } else {
            ESP_LOGE(TAG, "Failed to copy embedded config, using defaults");
            load_default_config(&g_system_config);
            save_to_ini(&g_system_config);
            ini_config_save_to_file(g_ini_config, CONFIG_FILE_PATH);
        }
    } else {
        ESP_LOGI(TAG, "Config file loaded successfully from SPIFFS");
        // 从INI文件加载配置到系统配置结构体
        load_from_ini(&g_system_config);
    }
    
    // 调试：打印加载的MQTT配置
    ESP_LOGI(TAG, "Loaded MQTT config - broker_host: '%s', broker_port: %d", 
             g_system_config.mqtt.broker_host, g_system_config.mqtt.broker_port);
    
    // 调试：打印加载的认证配置
    ESP_LOGI(TAG, "Loaded auth config - Username: '%s', Password hash: '%s'", 
             g_system_config.auth.username, g_system_config.auth.password_hash);
    
    g_config_loaded = true;
    ESP_LOGI(TAG, "Configuration manager initialized successfully");
    return ESP_OK;
}

esp_err_t config_manager_load(system_config_t* config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!g_config_loaded) {
        ESP_LOGE(TAG, "Configuration not loaded");
        return ESP_ERR_INVALID_STATE;
    }
    
    memcpy(config, &g_system_config, sizeof(system_config_t));
    return ESP_OK;
}

esp_err_t config_manager_save(const system_config_t* config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!g_config_loaded) {
        ESP_LOGE(TAG, "Configuration not loaded");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 更新内存中的配置
    memcpy(&g_system_config, config, sizeof(system_config_t));
    
    // 保存到INI配置并写入文件
    esp_err_t ret = save_to_ini(config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save to INI config");
        return ret;
    }
    
    ret = ini_config_save_to_file(g_ini_config, CONFIG_FILE_PATH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save config file");
        return ret;
    }
    
    ESP_LOGI(TAG, "Configuration saved successfully");
    return ESP_OK;
}

esp_err_t config_manager_get_wifi_ap(xj1_wifi_ap_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &g_system_config.wifi_ap, sizeof(xj1_wifi_ap_config_t));
    return ESP_OK;
}

esp_err_t config_manager_set_wifi_ap(const xj1_wifi_ap_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&g_system_config.wifi_ap, config, sizeof(xj1_wifi_ap_config_t));
    return config_manager_save(&g_system_config);
}

esp_err_t config_manager_get_wifi_sta(xj1_wifi_sta_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &g_system_config.wifi_sta, sizeof(xj1_wifi_sta_config_t));
    return ESP_OK;
}

esp_err_t config_manager_set_wifi_sta(const xj1_wifi_sta_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&g_system_config.wifi_sta, config, sizeof(xj1_wifi_sta_config_t));
    return config_manager_save(&g_system_config);
}

esp_err_t config_manager_get_ethernet(ethernet_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &g_system_config.ethernet, sizeof(ethernet_config_t));
    return ESP_OK;
}

esp_err_t config_manager_set_ethernet(const ethernet_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&g_system_config.ethernet, config, sizeof(ethernet_config_t));
    return config_manager_save(&g_system_config);
}

esp_err_t config_manager_get_auth(auth_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &g_system_config.auth, sizeof(auth_config_t));
    return ESP_OK;
}

esp_err_t config_manager_set_auth(const auth_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&g_system_config.auth, config, sizeof(auth_config_t));
    return config_manager_save(&g_system_config);
}

esp_err_t config_manager_get_bluetooth(bluetooth_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &g_system_config.bluetooth, sizeof(bluetooth_config_t));
    return ESP_OK;
}

esp_err_t config_manager_set_bluetooth(const bluetooth_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&g_system_config.bluetooth, config, sizeof(bluetooth_config_t));
    return config_manager_save(&g_system_config);
}

esp_err_t config_manager_get_mqtt(mqtt_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &g_system_config.mqtt, sizeof(mqtt_config_t));
    return ESP_OK;
}

esp_err_t config_manager_set_mqtt(const mqtt_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&g_system_config.mqtt, config, sizeof(mqtt_config_t));
    return config_manager_save(&g_system_config);
}

esp_err_t config_manager_get_web_server(web_server_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &g_system_config.web_server, sizeof(web_server_config_t));
    return ESP_OK;
}

esp_err_t config_manager_set_web_server(const web_server_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&g_system_config.web_server, config, sizeof(web_server_config_t));
    return config_manager_save(&g_system_config);
}

esp_err_t config_manager_reset_to_default(void) {
    if (!g_config_loaded) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 加载默认配置
    load_default_config(&g_system_config);
    
    // 保存到文件
    esp_err_t ret = save_to_ini(&g_system_config);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = ini_config_save_to_file(g_ini_config, CONFIG_FILE_PATH);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ESP_LOGI(TAG, "Configuration reset to default values");
    return ESP_OK;
}

esp_err_t config_manager_get_timeouts(timeout_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &g_system_config.timeouts, sizeof(timeout_config_t));
    return ESP_OK;
}

esp_err_t config_manager_get_intervals(interval_config_t* config) {
    if (!config || !g_config_loaded) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &g_system_config.intervals, sizeof(interval_config_t));
    return ESP_OK;
}
