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

#include "web_server.h"
#include "auth.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "bluetooth_manager.h"
#include "ethernet_manager.h"
#include "mqtt_manager.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "web_server";
static httpd_handle_t g_server = NULL;
static network_status_t g_network_status = {0};

// 外部引用的HTML页面内容
extern const char login_html_start[] asm("_binary_login_html_start");
extern const char login_html_end[] asm("_binary_login_html_end");
extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

/**
 * @brief 从请求头获取会话ID
 */
static esp_err_t get_session_id_from_header(httpd_req_t *req, char* session_id) {
    size_t buf_len = httpd_req_get_hdr_value_len(req, "Cookie");
    if (buf_len == 0) {
        return ESP_ERR_NOT_FOUND;
    }
    
    char* buf = malloc(buf_len + 1);
    if (!buf) {
        return ESP_ERR_NO_MEM;
    }
    
    if (httpd_req_get_hdr_value_str(req, "Cookie", buf, buf_len + 1) != ESP_OK) {
        free(buf);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 查找session_id cookie
    char* session_start = strstr(buf, "session_id=");
    if (!session_start) {
        free(buf);
        return ESP_ERR_NOT_FOUND;
    }
    
    session_start += strlen("session_id=");
    char* session_end = strchr(session_start, ';');
    
    int session_len = session_end ? (session_end - session_start) : strlen(session_start);
    if (session_len > AUTH_SESSION_ID_LENGTH) {
        session_len = AUTH_SESSION_ID_LENGTH;
    }
    
    strncpy(session_id, session_start, session_len);
    session_id[session_len] = '\0';
    
    free(buf);
    return ESP_OK;
}

/**
 * @brief 检查用户是否已认证
 */
static bool is_authenticated(httpd_req_t *req) {
    char session_id[AUTH_SESSION_ID_LENGTH + 1];
    
    if (get_session_id_from_header(req, session_id) != ESP_OK) {
        return false;
    }
    
    return auth_validate_session(session_id);
}

/**
 * @brief 发送JSON响应
 */
static esp_err_t send_json_response(httpd_req_t *req, int status_code, const char* json_str) {
    if (status_code == 200) {
        httpd_resp_set_status(req, "200 OK");
    } else if (status_code == 401) {
        httpd_resp_set_status(req, "401 Unauthorized");
    } else {
        httpd_resp_set_status(req, "500 Internal Server Error");
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    return httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);
}

/**
 * @brief 根页面处理器 - 重定向到登录页面
 */
static esp_err_t root_handler(httpd_req_t *req) {
    if (is_authenticated(req)) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/index.html");
        httpd_resp_send(req, NULL, 0);
    } else {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/login.html");
        httpd_resp_send(req, NULL, 0);
    }
    return ESP_OK;
}

/**
 * @brief 登录页面处理器
 */
static esp_err_t login_page_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "utf-8");
    
    const size_t login_html_size = (login_html_end - login_html_start);
    httpd_resp_send(req, login_html_start, login_html_size);
    
    return ESP_OK;
}

/**
 * @brief 主页面处理器
 */
static esp_err_t index_page_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "/login.html");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "utf-8");
    
    const size_t index_html_size = (index_html_end - index_html_start);
    httpd_resp_send(req, index_html_start, index_html_size);
    
    return ESP_OK;
}

/**
 * @brief 登录API处理器
 */
static esp_err_t login_api_handler(httpd_req_t *req) {
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"无效的请求数据\"}");
        return ESP_OK;
    }
    content[ret] = '\0';
    
    // 解析JSON
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"JSON解析失败\"}");
        return ESP_OK;
    }
    
    cJSON *username_json = cJSON_GetObjectItem(json, "username");
    cJSON *password_json = cJSON_GetObjectItem(json, "password");
    
    if (!username_json || !password_json) {
        cJSON_Delete(json);
        send_json_response(req, 400, "{\"success\":false,\"message\":\"缺少用户名或密码\"}");
        return ESP_OK;
    }
    
    const char* username = cJSON_GetStringValue(username_json);
    const char* password = cJSON_GetStringValue(password_json);
    
    // 添加调试信息
    auth_config_t auth_config;
    config_manager_get_auth(&auth_config);
    char test_hash[65];
    auth_calculate_sha256(password, test_hash);
    
    // 验证登录
    char session_id[AUTH_SESSION_ID_LENGTH + 1];
    esp_err_t auth_result = auth_login(username, password, session_id);
    
    cJSON_Delete(json);
    
    if (auth_result == ESP_OK) {
        // 获取会话超时配置
        timeout_config_t timeout_config;
        int session_max_age = 1800; // 默认30分钟
        if (config_manager_get_timeouts(&timeout_config) == ESP_OK) {
            session_max_age = timeout_config.session_max_age;
        }
        
        // 设置cookie并返回成功响应
        char cookie_header[128];
        snprintf(cookie_header, sizeof(cookie_header), 
                "session_id=%s; Path=/; Max-Age=%d", session_id, session_max_age);
        httpd_resp_set_hdr(req, "Set-Cookie", cookie_header);
        
        send_json_response(req, 200, "{\"success\":true,\"message\":\"登录成功\"}");
        ESP_LOGI(TAG, "User %s logged in successfully", username);
    } else {
        // 使用cJSON构建安全的JSON响应
        cJSON *response = cJSON_CreateObject();
        cJSON *debug = cJSON_CreateObject();
        
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "message", "用户名或密码错误");
        
        cJSON_AddStringToObject(debug, "input_username", username ? username : "null");
        cJSON_AddStringToObject(debug, "input_password", password ? "***" : "null"); // 隐藏密码
        cJSON_AddStringToObject(debug, "config_username", auth_config.username);
        cJSON_AddStringToObject(debug, "config_hash", auth_config.password_hash);
        cJSON_AddStringToObject(debug, "calculated_hash", test_hash);
        cJSON_AddBoolToObject(debug, "hash_match", strcmp(test_hash, auth_config.password_hash) == 0);
        
        cJSON_AddItemToObject(response, "debug", debug);
        
        char *json_string = cJSON_Print(response);
        send_json_response(req, 401, json_string);
        
        free(json_string);
        cJSON_Delete(response);
        ESP_LOGW(TAG, "Login failed for user %s", username);
    }
    
    return ESP_OK;
}

/**
 * @brief 登出API处理器
 */
static esp_err_t logout_api_handler(httpd_req_t *req) {
    char session_id[AUTH_SESSION_ID_LENGTH + 1];
    
    if (get_session_id_from_header(req, session_id) == ESP_OK) {
        auth_logout(session_id);
    }
    
    // 清除cookie
    httpd_resp_set_hdr(req, "Set-Cookie", "session_id=; Path=/; Max-Age=0");
    send_json_response(req, 200, "{\"success\":true,\"message\":\"登出成功\"}");
    
    return ESP_OK;
}

/**
 * @brief 获取配置API处理器
 */
static esp_err_t get_config_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    system_config_t config;
    esp_err_t ret = config_manager_load(&config);
    if (ret != ESP_OK) {
        send_json_response(req, 500, "{\"success\":false,\"message\":\"读取配置失败\"}");
        return ESP_OK;
    }
    
    // 构建JSON响应
    cJSON *json = cJSON_CreateObject();
    cJSON *wifi_ap = cJSON_CreateObject();
    cJSON *wifi_sta = cJSON_CreateObject();
    cJSON *ethernet = cJSON_CreateObject();
    cJSON *bluetooth = cJSON_CreateObject();
    cJSON *mqtt = cJSON_CreateObject();
    
    // WiFi AP配置
    cJSON_AddStringToObject(wifi_ap, "ssid", config.wifi_ap.ssid);
    cJSON_AddStringToObject(wifi_ap, "ip", config.wifi_ap.ip);
    cJSON_AddStringToObject(wifi_ap, "password", config.wifi_ap.password);
    cJSON_AddItemToObject(json, "wifi_ap", wifi_ap);
    
    // WiFi STA配置
    cJSON_AddStringToObject(wifi_sta, "ssid", config.wifi_sta.ssid);
    cJSON_AddStringToObject(wifi_sta, "password", config.wifi_sta.password);
    cJSON_AddItemToObject(json, "wifi_sta", wifi_sta);
    
    // 以太网配置
    cJSON_AddStringToObject(ethernet, "ip", config.ethernet.ip);
    cJSON_AddStringToObject(ethernet, "netmask", config.ethernet.netmask);
    cJSON_AddStringToObject(ethernet, "dns", config.ethernet.dns);
    cJSON_AddStringToObject(ethernet, "gateway", config.ethernet.gateway);
    cJSON_AddItemToObject(json, "ethernet", ethernet);
    
    // 蓝牙配置
    cJSON_AddStringToObject(bluetooth, "device_name", config.bluetooth.device_name);
    cJSON_AddStringToObject(bluetooth, "pairing_password", config.bluetooth.pairing_password);
    cJSON_AddItemToObject(json, "bluetooth", bluetooth);
    
    // MQTT配置
    cJSON_AddStringToObject(mqtt, "broker_host", config.mqtt.broker_host);
    cJSON_AddNumberToObject(mqtt, "broker_port", config.mqtt.broker_port);
    cJSON_AddStringToObject(mqtt, "client_id", config.mqtt.client_id);
    cJSON_AddStringToObject(mqtt, "default_topic", config.mqtt.default_topic);
    cJSON_AddNumberToObject(mqtt, "keepalive", config.mqtt.keepalive);
    cJSON_AddItemToObject(json, "mqtt", mqtt);
    
    cJSON_AddBoolToObject(json, "success", true);
    
    char *json_string = cJSON_Print(json);
    send_json_response(req, 200, json_string);
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}

/**
 * @brief 获取网络状态API处理器
 */
static esp_err_t get_status_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    // 添加CORS头部以防止跨域问题
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    // 构建状态JSON
    cJSON *json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "success", true);
    
    // 添加时间戳用于调试
    cJSON_AddNumberToObject(json, "timestamp", esp_timer_get_time() / 1000000);
    
    // 网络状态
    cJSON_AddBoolToObject(json, "wifi_ap_enabled", g_network_status.wifi_ap_enabled);
    cJSON_AddBoolToObject(json, "wifi_sta_connected", g_network_status.wifi_sta_connected);
    cJSON_AddStringToObject(json, "wifi_sta_ip", g_network_status.wifi_sta_ip);
    cJSON_AddBoolToObject(json, "ethernet_connected", g_network_status.ethernet_connected);
    cJSON_AddStringToObject(json, "ethernet_ip", g_network_status.ethernet_ip);
    cJSON_AddBoolToObject(json, "bluetooth_enabled", g_network_status.bluetooth_enabled);
    cJSON_AddNumberToObject(json, "bluetooth_clients", g_network_status.bluetooth_clients);
    cJSON_AddBoolToObject(json, "mqtt_connected", g_network_status.mqtt_connected);
    
    // 添加系统信息
    cJSON_AddNumberToObject(json, "free_heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(json, "uptime_seconds", esp_timer_get_time() / 1000000);
    
    char *json_string = cJSON_Print(json);
    send_json_response(req, 200, json_string);
    
    free(json_string);
    cJSON_Delete(json);
    
    // 调试日志（限制频率）
    static int status_request_count = 0;
    if (++status_request_count % 50 == 0) {
        ESP_LOGI("web_server", "状态API调用 #%d - WiFi STA: %s, 以太网: %s", 
                 status_request_count,
                 g_network_status.wifi_sta_connected ? "已连接" : "未连接",
                 g_network_status.ethernet_connected ? "已连接" : "未连接");
    }
    
    return ESP_OK;
}

/**
 * @brief 获取WiFi加密类型字符串
 */
static const char* get_auth_mode_name(wifi_auth_mode_t authmode) {
    switch (authmode) {
        case WIFI_AUTH_OPEN: return "开放";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2企业版";
        case WIFI_AUTH_WPA3_ENTERPRISE: return "WPA3企业版";
        case WIFI_AUTH_WPA2_WPA3_ENTERPRISE: return "WPA2/WPA3企业版";
        case WIFI_AUTH_WAPI_PSK: return "WAPI";
        case WIFI_AUTH_OWE: return "OWE";
        case WIFI_AUTH_WPA3_ENT_192: return "WPA3企业版192";
        case WIFI_AUTH_WPA3_EXT_PSK: return "WPA3扩展PSK";
        case WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE: return "WPA3扩展PSK混合";
        case WIFI_AUTH_DPP: return "DPP";
        case WIFI_AUTH_MAX:
        default: return "未知";
    }
}

/**
 * @brief 获取WiFi信号强度描述
 */
static const char* get_signal_strength(int rssi) {
    if (rssi >= -30) return "极强";
    else if (rssi >= -50) return "强";
    else if (rssi >= -70) return "中等";
    else if (rssi >= -85) return "弱";
    else return "极弱";
}

/**
 * @brief WiFi扫描API处理器
 */
static esp_err_t wifi_scan_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    ESP_LOGI("web_server", "开始WiFi网络扫描...");
    
    // 获取WiFi扫描超时配置
    timeout_config_t timeout_config;
    int wifi_scan_timeout = 5000; // 默认值
    if (config_manager_get_timeouts(&timeout_config) == ESP_OK) {
        wifi_scan_timeout = timeout_config.wifi_scan_timeout;
    }
    
    // 进行WiFi扫描（使用增强版，默认按信号强度排序）
    wifi_scan_result_t scan_results[WIFI_SCAN_MAX_AP];
    int actual_results = 0;
    
    wifi_scan_options_t scan_options = {
        .show_hidden = true,    // 显示隐藏网络
        .sort_by_rssi = true,   // 按信号强度排序
        .scan_timeout = wifi_scan_timeout
    };
    
    esp_err_t ret = wifi_manager_scan_advanced(scan_results, WIFI_SCAN_MAX_AP, &actual_results, &scan_options);
    
    // 构建JSON响应
    cJSON *json = cJSON_CreateObject();
    
    if (ret != ESP_OK) {
        ESP_LOGE("web_server", "WiFi扫描失败: %s", esp_err_to_name(ret));
        cJSON_AddBoolToObject(json, "success", false);
        cJSON_AddStringToObject(json, "message", "WiFi扫描失败");
        cJSON_AddNumberToObject(json, "error_code", ret);
    } else {
        ESP_LOGI("web_server", "WiFi扫描完成，发现 %d 个网络", actual_results);
        cJSON_AddBoolToObject(json, "success", true);
        cJSON_AddNumberToObject(json, "count", actual_results);
        
        // 创建网络数组
        cJSON *networks = cJSON_CreateArray();
        
        for (int i = 0; i < actual_results; i++) {
            cJSON *network = cJSON_CreateObject();
            
            // 基本信息
            cJSON_AddStringToObject(network, "ssid", scan_results[i].ssid);
            cJSON_AddNumberToObject(network, "rssi", scan_results[i].rssi);
            cJSON_AddNumberToObject(network, "auth", scan_results[i].authmode);
            
            // 增强信息
            cJSON_AddStringToObject(network, "auth_name", get_auth_mode_name(scan_results[i].authmode));
            cJSON_AddStringToObject(network, "signal_strength", get_signal_strength(scan_results[i].rssi));
            cJSON_AddBoolToObject(network, "secure", scan_results[i].authmode != WIFI_AUTH_OPEN);
            
            // 信号强度百分比 (粗略估算)
            int signal_percent = 0;
            if (scan_results[i].rssi >= -30) signal_percent = 100;
            else if (scan_results[i].rssi >= -50) signal_percent = 75;
            else if (scan_results[i].rssi >= -70) signal_percent = 50;
            else if (scan_results[i].rssi >= -85) signal_percent = 25;
            else signal_percent = 10;
            cJSON_AddNumberToObject(network, "signal_percent", signal_percent);
            
            cJSON_AddItemToArray(networks, network);
        }
        
        cJSON_AddItemToObject(json, "networks", networks);
    }
    
    char *json_string = cJSON_Print(json);
    send_json_response(req, 200, json_string);
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}

/**
 * @brief 自定义WiFi扫描API处理器
 */
static esp_err_t wifi_scan_custom_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    // 获取WiFi扫描超时配置
    timeout_config_t timeout_config;
    int wifi_scan_timeout = 5000; // 默认值
    if (config_manager_get_timeouts(&timeout_config) == ESP_OK) {
        wifi_scan_timeout = timeout_config.wifi_scan_timeout;
    }
    
    // 设置默认扫描选项
    wifi_scan_options_t scan_options = {
        .show_hidden = false,
        .sort_by_rssi = true,
        .scan_timeout = wifi_scan_timeout
    };
    
    // 如果是POST请求，解析自定义选项
    if (req->method == HTTP_POST) {
        char content[256];
        int ret = httpd_req_recv(req, content, sizeof(content) - 1);
        if (ret > 0) {
            content[ret] = '\0';
            
            cJSON *json = cJSON_Parse(content);
            if (json) {
                cJSON *show_hidden = cJSON_GetObjectItem(json, "show_hidden");
                if (cJSON_IsBool(show_hidden)) {
                    scan_options.show_hidden = cJSON_IsTrue(show_hidden);
                }
                
                cJSON *sort_by_rssi = cJSON_GetObjectItem(json, "sort_by_rssi");
                if (cJSON_IsBool(sort_by_rssi)) {
                    scan_options.sort_by_rssi = cJSON_IsTrue(sort_by_rssi);
                }
                
                cJSON *scan_timeout = cJSON_GetObjectItem(json, "scan_timeout");
                if (cJSON_IsNumber(scan_timeout)) {
                    scan_options.scan_timeout = (uint32_t)cJSON_GetNumberValue(scan_timeout);
                    // 限制超时时间范围
                    if (scan_options.scan_timeout < 1000) scan_options.scan_timeout = 1000;
                    if (scan_options.scan_timeout > 30000) scan_options.scan_timeout = 30000;
                }
                
                cJSON_Delete(json);
            }
        }
    }
    
    ESP_LOGI("web_server", "开始自定义WiFi扫描 - 隐藏网络:%s, 信号排序:%s, 超时:%lums", 
             scan_options.show_hidden ? "显示" : "隐藏",
             scan_options.sort_by_rssi ? "启用" : "禁用",
             scan_options.scan_timeout);
    
    // 执行扫描
    wifi_scan_result_t scan_results[WIFI_SCAN_MAX_AP];
    int actual_results = 0;
    
    esp_err_t scan_ret = wifi_manager_scan_advanced(scan_results, WIFI_SCAN_MAX_AP, &actual_results, &scan_options);
    
    // 构建JSON响应
    cJSON *json = cJSON_CreateObject();
    
    if (scan_ret != ESP_OK) {
        ESP_LOGE("web_server", "自定义WiFi扫描失败: %s", esp_err_to_name(scan_ret));
        cJSON_AddBoolToObject(json, "success", false);
        cJSON_AddStringToObject(json, "message", "WiFi扫描失败");
        cJSON_AddNumberToObject(json, "error_code", scan_ret);
    } else {
        ESP_LOGI("web_server", "自定义WiFi扫描完成，发现 %d 个网络", actual_results);
        cJSON_AddBoolToObject(json, "success", true);
        cJSON_AddNumberToObject(json, "count", actual_results);
        
        // 添加扫描选项到响应
        cJSON *options = cJSON_CreateObject();
        cJSON_AddBoolToObject(options, "show_hidden", scan_options.show_hidden);
        cJSON_AddBoolToObject(options, "sort_by_rssi", scan_options.sort_by_rssi);
        cJSON_AddNumberToObject(options, "scan_timeout", scan_options.scan_timeout);
        cJSON_AddItemToObject(json, "scan_options", options);
        
        // 创建网络数组
        cJSON *networks = cJSON_CreateArray();
        
        for (int i = 0; i < actual_results; i++) {
            cJSON *network = cJSON_CreateObject();
            
            // 基本信息
            const char* ssid_display = strlen(scan_results[i].ssid) > 0 ? scan_results[i].ssid : "[隐藏网络]";
            cJSON_AddStringToObject(network, "ssid", ssid_display);
            cJSON_AddNumberToObject(network, "rssi", scan_results[i].rssi);
            cJSON_AddNumberToObject(network, "auth", scan_results[i].authmode);
            cJSON_AddBoolToObject(network, "hidden", strlen(scan_results[i].ssid) == 0);
            
            // 增强信息
            cJSON_AddStringToObject(network, "auth_name", get_auth_mode_name(scan_results[i].authmode));
            cJSON_AddStringToObject(network, "signal_strength", get_signal_strength(scan_results[i].rssi));
            cJSON_AddBoolToObject(network, "secure", scan_results[i].authmode != WIFI_AUTH_OPEN);
            
            // 信号强度百分比
            int signal_percent = 0;
            if (scan_results[i].rssi >= -30) signal_percent = 100;
            else if (scan_results[i].rssi >= -50) signal_percent = 75;
            else if (scan_results[i].rssi >= -70) signal_percent = 50;
            else if (scan_results[i].rssi >= -85) signal_percent = 25;
            else signal_percent = 10;
            cJSON_AddNumberToObject(network, "signal_percent", signal_percent);
            
            cJSON_AddItemToArray(networks, network);
        }
        
        cJSON_AddItemToObject(json, "networks", networks);
    }
    
    char *json_string = cJSON_Print(json);
    send_json_response(req, 200, json_string);
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}

/**
 * @brief WiFi连接API处理器
 */
static esp_err_t wifi_connect_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"无效的请求数据\"}");
        return ESP_OK;
    }
    content[ret] = '\0';
    
    ESP_LOGI("web_server", "收到WiFi连接请求: %s", content);
    
    // 解析JSON
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"JSON解析失败\"}");
        return ESP_OK;
    }
    
    cJSON *ssid_json = cJSON_GetObjectItem(json, "ssid");
    cJSON *password_json = cJSON_GetObjectItem(json, "password");
    
    if (!ssid_json || !cJSON_IsString(ssid_json)) {
        cJSON_Delete(json);
        send_json_response(req, 400, "{\"success\":false,\"message\":\"缺少SSID参数\"}");
        return ESP_OK;
    }
    
    const char* ssid = cJSON_GetStringValue(ssid_json);
    const char* password = password_json && cJSON_IsString(password_json) ? 
                          cJSON_GetStringValue(password_json) : NULL;
    
    ESP_LOGI("web_server", "尝试连接WiFi - SSID: '%s', 密码: %s", 
             ssid, password ? "[已提供]" : "[无密码]");
    
    // 调用WiFi管理器连接函数
    esp_err_t connect_ret = wifi_manager_connect_sta(ssid, password);
    
    cJSON_Delete(json);
    
    // 构建响应
    cJSON *response = cJSON_CreateObject();
    
    if (connect_ret == ESP_OK) {
        ESP_LOGI("web_server", "WiFi连接请求已发送，SSID: %s", ssid);
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddStringToObject(response, "message", "WiFi连接请求已发送，正在连接...");
        cJSON_AddStringToObject(response, "ssid", ssid);
        cJSON_AddStringToObject(response, "status", "connecting");
        
        // 提示用户如何检查连接状态
        cJSON_AddStringToObject(response, "hint", "请等待几秒后调用 /api/wifi/status 查询连接状态");
        cJSON_AddStringToObject(response, "status_api", "/api/wifi/status");
    } else {
        ESP_LOGE("web_server", "WiFi连接失败: %s", esp_err_to_name(connect_ret));
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "message", "WiFi连接失败");
        cJSON_AddNumberToObject(response, "error_code", connect_ret);
        cJSON_AddStringToObject(response, "error_name", esp_err_to_name(connect_ret));
    }
    
    char *json_string = cJSON_Print(response);
    send_json_response(req, connect_ret == ESP_OK ? 200 : 500, json_string);
    
    free(json_string);
    cJSON_Delete(response);
    
    return ESP_OK;
}

/**
 * @brief WiFi断开连接API处理器
 */
static esp_err_t wifi_disconnect_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    ESP_LOGI("web_server", "收到WiFi断开连接请求");
    
    // 调用WiFi管理器断开函数
    esp_err_t disconnect_ret = wifi_manager_disconnect_sta();
    
    // 构建响应
    cJSON *response = cJSON_CreateObject();
    
    if (disconnect_ret == ESP_OK) {
        ESP_LOGI("web_server", "WiFi断开连接成功");
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddStringToObject(response, "message", "WiFi连接已断开");
        cJSON_AddStringToObject(response, "status", "disconnected");
    } else {
        ESP_LOGE("web_server", "WiFi断开连接失败: %s", esp_err_to_name(disconnect_ret));
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "message", "WiFi断开连接失败");
        cJSON_AddNumberToObject(response, "error_code", disconnect_ret);
        cJSON_AddStringToObject(response, "error_name", esp_err_to_name(disconnect_ret));
    }
    
    char *json_string = cJSON_Print(response);
    send_json_response(req, disconnect_ret == ESP_OK ? 200 : 500, json_string);
    
    free(json_string);
    cJSON_Delete(response);
    
    return ESP_OK;
}

/**
 * @brief WiFi状态查询API处理器
 */
static esp_err_t wifi_status_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    ESP_LOGI("web_server", "收到WiFi状态查询请求");
    
    // 获取WiFi状态
    wifi_status_t wifi_status;
    esp_err_t ret = wifi_manager_get_status(&wifi_status);
    
    ESP_LOGI("web_server", "WiFi状态获取结果: ret=%d, AP=%s, STA=%s, IP=%s", 
             ret, 
             wifi_status.ap_enabled ? "ON" : "OFF",
             wifi_status.sta_connected ? "CONNECTED" : "DISCONNECTED",
             wifi_status.sta_ip);
    
    // 构建响应
    cJSON *response = cJSON_CreateObject();
    
    if (ret == ESP_OK) {
        cJSON_AddBoolToObject(response, "success", true);
        
        // WiFi AP状态
        cJSON *ap_info = cJSON_CreateObject();
        cJSON_AddBoolToObject(ap_info, "enabled", wifi_status.ap_enabled);
        cJSON_AddItemToObject(response, "ap", ap_info);
        
        // WiFi STA状态
        cJSON *sta_info = cJSON_CreateObject();
        cJSON_AddBoolToObject(sta_info, "connected", wifi_status.sta_connected);
        cJSON_AddStringToObject(sta_info, "ip", wifi_status.sta_ip);
        cJSON_AddNumberToObject(sta_info, "rssi", wifi_status.sta_rssi);
        
        // 添加连接状态描述
        if (wifi_status.sta_connected) {
            cJSON_AddStringToObject(sta_info, "status", "connected");
            cJSON_AddStringToObject(sta_info, "status_text", "已连接");
            
            // 信号强度描述
            const char* signal_desc = "未知";
            if (wifi_status.sta_rssi >= -30) signal_desc = "极强";
            else if (wifi_status.sta_rssi >= -50) signal_desc = "强";
            else if (wifi_status.sta_rssi >= -70) signal_desc = "中等";
            else if (wifi_status.sta_rssi >= -85) signal_desc = "弱";
            else signal_desc = "极弱";
            cJSON_AddStringToObject(sta_info, "signal_strength", signal_desc);
            
            // 信号强度百分比
            int signal_percent = 0;
            if (wifi_status.sta_rssi >= -30) signal_percent = 100;
            else if (wifi_status.sta_rssi >= -50) signal_percent = 75;
            else if (wifi_status.sta_rssi >= -70) signal_percent = 50;
            else if (wifi_status.sta_rssi >= -85) signal_percent = 25;
            else signal_percent = 10;
            cJSON_AddNumberToObject(sta_info, "signal_percent", signal_percent);
        } else {
            cJSON_AddStringToObject(sta_info, "status", "disconnected");
            cJSON_AddStringToObject(sta_info, "status_text", "未连接");
        }
        
        cJSON_AddItemToObject(response, "sta", sta_info);
        
        ESP_LOGI("web_server", "WiFi状态查询 - AP:%s, STA:%s(%s)", 
                 wifi_status.ap_enabled ? "启用" : "禁用",
                 wifi_status.sta_connected ? "已连接" : "未连接",
                 wifi_status.sta_ip);
    } else {
        ESP_LOGE("web_server", "获取WiFi状态失败: %s", esp_err_to_name(ret));
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "message", "获取WiFi状态失败");
        cJSON_AddNumberToObject(response, "error_code", ret);
    }
    
    char *json_string = cJSON_Print(response);
    send_json_response(req, ret == ESP_OK ? 200 : 500, json_string);
    
    free(json_string);
    cJSON_Delete(response);
    
    return ESP_OK;
}

// 全局变量跟踪以太网重启状态
static volatile bool g_ethernet_restarting = false;
static volatile bool g_ethernet_restart_success = false;

/**
 * @brief 重启以太网任务
 * @param pvParameters 任务参数
 */
static void restart_ethernet_task(void *pvParameters) {
    ESP_LOGI("web_server", "开始重启以太网...");
    g_ethernet_restarting = true;
    g_ethernet_restart_success = false;
    
    // 等待一下确保HTTP响应已发送
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // 停止以太网
    esp_err_t ret = ethernet_manager_stop();
    if (ret != ESP_OK) {
        ESP_LOGW("web_server", "停止以太网失败: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI("web_server", "以太网已停止");
    }
    
    // 等待2秒
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // 重新启动以太网
    ret = ethernet_manager_start();
    if (ret == ESP_OK) {
        ESP_LOGI("web_server", "以太网重启成功，新配置已应用");
        g_ethernet_restart_success = true;
    } else {
        ESP_LOGE("web_server", "以太网重启失败: %s", esp_err_to_name(ret));
        g_ethernet_restart_success = false;
    }
    
    g_ethernet_restarting = false;
    
    // 任务完成，自动删除
    vTaskDelete(NULL);
}

/**
 * @brief 以太网重启状态查询API处理器
 */
static esp_err_t ethernet_restart_status_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    // 添加CORS头部
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddBoolToObject(response, "restarting", g_ethernet_restarting);
    cJSON_AddBoolToObject(response, "restart_success", g_ethernet_restart_success);
    
    if (g_ethernet_restarting) {
        cJSON_AddStringToObject(response, "status", "restarting");
        cJSON_AddStringToObject(response, "message", "以太网正在重启中...");
    } else if (g_ethernet_restart_success) {
        cJSON_AddStringToObject(response, "status", "completed");
        cJSON_AddStringToObject(response, "message", "以太网重启成功");
    } else {
        cJSON_AddStringToObject(response, "status", "idle");
        cJSON_AddStringToObject(response, "message", "以太网未在重启");
    }
    
    char *json_string = cJSON_Print(response);
    send_json_response(req, 200, json_string);
    
    free(json_string);
    cJSON_Delete(response);
    
    return ESP_OK;
}

/**
 * @brief WiFi连接状态等待API处理器
 * @note 等待WiFi连接完成并返回最终状态
 */
static esp_err_t wifi_wait_connection_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    ESP_LOGI("web_server", "开始等待WiFi连接状态...");
    
    // 等待最多15秒检查连接状态
    int max_wait_seconds = 15;
    int check_interval_ms = 1000;  // 每秒检查一次
    
    for (int i = 0; i < max_wait_seconds; i++) {
        // 获取当前WiFi状态
        wifi_status_t wifi_status;
        esp_err_t ret = wifi_manager_get_status(&wifi_status);
        
        if (ret == ESP_OK) {
            ESP_LOGI("web_server", "等待中... 第%d秒, STA状态: %s, IP: %s", 
                     i + 1,
                     wifi_status.sta_connected ? "已连接" : "未连接",
                     wifi_status.sta_ip);
            
            if (wifi_status.sta_connected && strlen(wifi_status.sta_ip) > 0) {
                // 连接成功
                cJSON *response = cJSON_CreateObject();
                cJSON_AddBoolToObject(response, "success", true);
                cJSON_AddStringToObject(response, "message", "WiFi连接成功");
                cJSON_AddStringToObject(response, "status", "connected");
                cJSON_AddStringToObject(response, "ip", wifi_status.sta_ip);
                cJSON_AddNumberToObject(response, "rssi", wifi_status.sta_rssi);
                cJSON_AddNumberToObject(response, "wait_time", i + 1);
                
                char *json_string = cJSON_Print(response);
                send_json_response(req, 200, json_string);
                
                free(json_string);
                cJSON_Delete(response);
                
                ESP_LOGI("web_server", "WiFi连接成功确认，用时 %d 秒", i + 1);
                return ESP_OK;
            }
        }
        
        // 等待1秒后再次检查
        vTaskDelay(pdMS_TO_TICKS(check_interval_ms));
    }
    
    // 超时，连接失败
    ESP_LOGW("web_server", "WiFi连接等待超时");
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", false);
    cJSON_AddStringToObject(response, "message", "WiFi连接超时");
    cJSON_AddStringToObject(response, "status", "timeout");
    cJSON_AddNumberToObject(response, "wait_time", max_wait_seconds);
    
    char *json_string = cJSON_Print(response);
    send_json_response(req, 408, json_string);  // 408 Request Timeout
    
    free(json_string);
    cJSON_Delete(response);
    
    return ESP_OK;
}

/**
 * @brief WiFi配置保存API处理器
 */
static esp_err_t save_wifi_config_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"无效的请求数据\"}");
        return ESP_OK;
    }
    content[ret] = '\0';
    
    // 解析JSON
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"JSON解析失败\"}");
        return ESP_OK;
    }
    
    // 解析WiFi AP配置
    cJSON *wifi_ap = cJSON_GetObjectItem(json, "wifi_ap");
    if (wifi_ap) {
        xj1_wifi_ap_config_t ap_config = {0};
        
        cJSON *ssid = cJSON_GetObjectItem(wifi_ap, "ssid");
        cJSON *ip = cJSON_GetObjectItem(wifi_ap, "ip");
        cJSON *password = cJSON_GetObjectItem(wifi_ap, "password");
        
        if (ssid && cJSON_IsString(ssid)) {
            strncpy(ap_config.ssid, cJSON_GetStringValue(ssid), sizeof(ap_config.ssid) - 1);
        }
        if (ip && cJSON_IsString(ip)) {
            strncpy(ap_config.ip, cJSON_GetStringValue(ip), sizeof(ap_config.ip) - 1);
        }
        if (password && cJSON_IsString(password)) {
            strncpy(ap_config.password, cJSON_GetStringValue(password), sizeof(ap_config.password) - 1);
        }
        
        config_manager_set_wifi_ap(&ap_config);
    }
    
    // 解析WiFi STA配置
    cJSON *wifi_sta = cJSON_GetObjectItem(json, "wifi_sta");
    if (wifi_sta) {
        xj1_wifi_sta_config_t sta_config = {0};
        
        cJSON *ssid = cJSON_GetObjectItem(wifi_sta, "ssid");
        cJSON *password = cJSON_GetObjectItem(wifi_sta, "password");
        
        if (ssid && cJSON_IsString(ssid)) {
            strncpy(sta_config.ssid, cJSON_GetStringValue(ssid), sizeof(sta_config.ssid) - 1);
        }
        if (password && cJSON_IsString(password)) {
            strncpy(sta_config.password, cJSON_GetStringValue(password), sizeof(sta_config.password) - 1);
        }
        
        config_manager_set_wifi_sta(&sta_config);
    }
    
    cJSON_Delete(json);
    send_json_response(req, 200, "{\"success\":true,\"message\":\"WiFi配置保存成功\"}");
    return ESP_OK;
}

/**
 * @brief 以太网配置保存API处理器
 */
static esp_err_t save_ethernet_config_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"无效的请求数据\"}");
        return ESP_OK;
    }
    content[ret] = '\0';
    
    // 解析JSON
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"JSON解析失败\"}");
        return ESP_OK;
    }
    
    ethernet_config_t eth_config = {0};
    
    cJSON *ip = cJSON_GetObjectItem(json, "ip");
    cJSON *netmask = cJSON_GetObjectItem(json, "netmask");
    cJSON *dns = cJSON_GetObjectItem(json, "dns");
    cJSON *gateway = cJSON_GetObjectItem(json, "gateway");
    
    if (ip && cJSON_IsString(ip)) {
        strncpy(eth_config.ip, cJSON_GetStringValue(ip), sizeof(eth_config.ip) - 1);
    }
    if (netmask && cJSON_IsString(netmask)) {
        strncpy(eth_config.netmask, cJSON_GetStringValue(netmask), sizeof(eth_config.netmask) - 1);
    }
    if (dns && cJSON_IsString(dns)) {
        strncpy(eth_config.dns, cJSON_GetStringValue(dns), sizeof(eth_config.dns) - 1);
    }
    if (gateway && cJSON_IsString(gateway)) {
        strncpy(eth_config.gateway, cJSON_GetStringValue(gateway), sizeof(eth_config.gateway) - 1);
    }
    
    ESP_LOGI("web_server", "保存以太网配置: IP=%s, 网关=%s, 子网掩码=%s, DNS=%s", 
             eth_config.ip, eth_config.gateway, eth_config.netmask, eth_config.dns);
    
    // 检查IP配置是否有效
    bool config_valid = true;
    if (strlen(eth_config.ip) > 0) {
        // 简单的IP地址格式验证
        int a, b, c, d;
        if (sscanf(eth_config.ip, "%d.%d.%d.%d", &a, &b, &c, &d) != 4 ||
            a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255) {
            config_valid = false;
        }
    }
    
    if (!config_valid) {
        cJSON_Delete(json);
        send_json_response(req, 400, "{\"success\":false,\"message\":\"以太网IP地址格式无效\"}");
        return ESP_OK;
    }
    
    // 保存配置到存储
    esp_err_t config_ret = config_manager_set_ethernet(&eth_config);
    
    cJSON_Delete(json);
    
    if (config_ret == ESP_OK) {
        ESP_LOGI("web_server", "以太网配置保存成功");
        
        // 构建详细的成功响应
        cJSON *response = cJSON_CreateObject();
        cJSON_AddBoolToObject(response, "success", true);
        cJSON_AddStringToObject(response, "message", "以太网配置保存成功");
        cJSON_AddStringToObject(response, "hint", "配置将在后台应用，网络可能会短暂中断");
        cJSON_AddStringToObject(response, "restart_info", "以太网将在2秒后重启以应用新配置");
        
        char *json_string = cJSON_Print(response);
        send_json_response(req, 200, json_string);
        free(json_string);
        cJSON_Delete(response);
        
        // 创建任务来重新启动以太网（避免阻塞HTTP响应）
        xTaskCreate(
            restart_ethernet_task,
            "restart_eth",
            4096,
            NULL,
            2,
            NULL
        );
    } else {
        ESP_LOGE("web_server", "以太网配置保存失败: %s", esp_err_to_name(config_ret));
        
        cJSON *error_response = cJSON_CreateObject();
        cJSON_AddBoolToObject(error_response, "success", false);
        cJSON_AddStringToObject(error_response, "message", "以太网配置保存失败");
        cJSON_AddNumberToObject(error_response, "error_code", config_ret);
        
        char *error_json = cJSON_Print(error_response);
        send_json_response(req, 500, error_json);
        free(error_json);
        cJSON_Delete(error_response);
    }
    
    return ESP_OK;
}

/**
 * @brief 蓝牙配置保存API处理器
 */
static esp_err_t save_bluetooth_config_api_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Bluetooth config API handler called");
    
    if (!is_authenticated(req)) {
        ESP_LOGW(TAG, "Bluetooth config API: Authentication failed");
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Bluetooth config API: Authentication passed");
    
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"无效的请求数据\"}");
        return ESP_OK;
    }
    content[ret] = '\0';
    
    // 解析JSON
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"JSON解析失败\"}");
        return ESP_OK;
    }
    
    bluetooth_config_t bt_config = {0};
    
    cJSON *device_name = cJSON_GetObjectItem(json, "device_name");
    cJSON *pairing_password = cJSON_GetObjectItem(json, "pairing_password");
    
    if (device_name && cJSON_IsString(device_name)) {
        strncpy(bt_config.device_name, cJSON_GetStringValue(device_name), sizeof(bt_config.device_name) - 1);
    }
    if (pairing_password && cJSON_IsString(pairing_password)) {
        strncpy(bt_config.pairing_password, cJSON_GetStringValue(pairing_password), sizeof(bt_config.pairing_password) - 1);
    }
    
    config_manager_set_bluetooth(&bt_config);
    
    cJSON_Delete(json);
    send_json_response(req, 200, "{\"success\":true,\"message\":\"蓝牙配置保存成功\"}");
    return ESP_OK;
}


/**
 * @brief MQTT配置保存API处理器
 */
static esp_err_t save_mqtt_config_api_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "MQTT config API handler called");
    
    if (!is_authenticated(req)) {
        ESP_LOGW(TAG, "MQTT config API: Authentication failed");
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "MQTT config API: Authentication passed");
    
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"无效的请求数据\"}");
        return ESP_OK;
    }
    content[ret] = '\0';
    
    // 解析JSON
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"JSON解析失败\"}");
        return ESP_OK;
    }
    
    mqtt_config_t mqtt_config = {0};
    
    cJSON *broker_host = cJSON_GetObjectItem(json, "broker_host");
    cJSON *broker_port = cJSON_GetObjectItem(json, "broker_port");
    cJSON *client_id = cJSON_GetObjectItem(json, "client_id");
    cJSON *default_topic = cJSON_GetObjectItem(json, "default_topic");
    cJSON *keepalive = cJSON_GetObjectItem(json, "keepalive");
    
    if (broker_host && cJSON_IsString(broker_host)) {
        strncpy(mqtt_config.broker_host, cJSON_GetStringValue(broker_host), sizeof(mqtt_config.broker_host) - 1);
    }
    if (broker_port && cJSON_IsNumber(broker_port)) {
        mqtt_config.broker_port = cJSON_GetNumberValue(broker_port);
    }
    if (client_id && cJSON_IsString(client_id)) {
        strncpy(mqtt_config.client_id, cJSON_GetStringValue(client_id), sizeof(mqtt_config.client_id) - 1);
    }
    if (default_topic && cJSON_IsString(default_topic)) {
        strncpy(mqtt_config.default_topic, cJSON_GetStringValue(default_topic), sizeof(mqtt_config.default_topic) - 1);
    }
    if (keepalive && cJSON_IsNumber(keepalive)) {
        mqtt_config.keepalive = cJSON_GetNumberValue(keepalive);
    }
    
    config_manager_set_mqtt(&mqtt_config);
    
    cJSON_Delete(json);
    send_json_response(req, 200, "{\"success\":true,\"message\":\"MQTT配置保存成功\"}");
    return ESP_OK;
}

/**
 * @brief 重置配置API处理器
 */
static esp_err_t reset_config_api_handler(httpd_req_t *req) {
    esp_err_t ret = config_manager_reset_to_default();
    if (ret == ESP_OK) {
        send_json_response(req, 200, "{\"success\":true,\"message\":\"配置已重置为默认值\"}");
    } else {
        send_json_response(req, 500, "{\"success\":false,\"message\":\"重置配置失败\"}");
    }
    return ESP_OK;
}

/**
 * @brief 调试API处理器 - 显示配置信息
 */
static esp_err_t debug_api_handler(httpd_req_t *req) {
    // 获取认证配置
    auth_config_t auth_config;
    esp_err_t ret = config_manager_get_auth(&auth_config);
    
    // 测试SHA-256计算
    char test_hash[65];
    auth_calculate_sha256("123456", test_hash);
    
    // 构建调试信息JSON
    char debug_info[1024];
    snprintf(debug_info, sizeof(debug_info),
        "{"
        "\"config_load_result\":\"%s\","
        "\"config_username\":\"%s\","
        "\"config_password_hash\":\"%s\","
        "\"test_input\":\"123456\","
        "\"test_calculated_hash\":\"%s\","
        "\"expected_hash\":\"8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92\","
        "\"hash_match\":%s"
        "}",
        ret == ESP_OK ? "success" : "failed",
        ret == ESP_OK ? auth_config.username : "N/A",
        ret == ESP_OK ? auth_config.password_hash : "N/A",
        test_hash,
        strcmp(test_hash, "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92") == 0 ? "true" : "false"
    );
    
    send_json_response(req, 200, debug_info);
    return ESP_OK;
}

/**
 * @brief 修改密码API处理器
 */
static esp_err_t change_password_api_handler(httpd_req_t *req) {
    if (!is_authenticated(req)) {
        send_json_response(req, 401, "{\"success\":false,\"message\":\"未认证\"}");
        return ESP_OK;
    }
    
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"无效的请求数据\"}");
        return ESP_OK;
    }
    content[ret] = '\0';
    
    // 解析JSON
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"JSON解析失败\"}");
        return ESP_OK;
    }
    
    cJSON *old_password_json = cJSON_GetObjectItem(json, "old_password");
    cJSON *new_password_json = cJSON_GetObjectItem(json, "new_password");
    cJSON *confirm_password_json = cJSON_GetObjectItem(json, "confirm_password");
    
    if (!old_password_json || !new_password_json || !confirm_password_json) {
        cJSON_Delete(json);
        send_json_response(req, 400, "{\"success\":false,\"message\":\"缺少必要字段\"}");
        return ESP_OK;
    }
    
    const char* old_password = cJSON_GetStringValue(old_password_json);
    const char* new_password = cJSON_GetStringValue(new_password_json);
    const char* confirm_password = cJSON_GetStringValue(confirm_password_json);
    
    // 检查新密码确认
    if (strcmp(new_password, confirm_password) != 0) {
        cJSON_Delete(json);
        send_json_response(req, 400, "{\"success\":false,\"message\":\"新密码确认不匹配\"}");
        return ESP_OK;
    }
    
    // 修改密码
    esp_err_t result = auth_change_password("admin", old_password, new_password);
    
    cJSON_Delete(json);
    
    if (result == ESP_OK) {
        send_json_response(req, 200, "{\"success\":true,\"message\":\"密码修改成功\"}");
    } else {
        send_json_response(req, 400, "{\"success\":false,\"message\":\"旧密码错误\"}");
    }
    
    return ESP_OK;
}

esp_err_t web_server_init(void) {
    // 初始化网络状态
    memset(&g_network_status, 0, sizeof(network_status_t));
    
    ESP_LOGI(TAG, "Web server initialized");
    return ESP_OK;
}

esp_err_t web_server_start(void) {
    if (g_server != NULL) {
        ESP_LOGW(TAG, "Web server already started");
        return ESP_OK;
    }
    
    // 获取Web服务器配置
    web_server_config_t web_config;
    esp_err_t ret = config_manager_get_web_server(&web_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get web server config, using default port 80");
        web_config.port = 80;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = web_config.port;
    config.max_uri_handlers = 30;  // 增加到30以容纳所有处理器
    config.max_open_sockets = 7;
    config.stack_size = 8192;
    
    ret = httpd_start(&g_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 注册URI处理器
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &root_uri);
    
    httpd_uri_t login_page_uri = {
        .uri = "/login.html",
        .method = HTTP_GET,
        .handler = login_page_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &login_page_uri);
    
    httpd_uri_t index_page_uri = {
        .uri = "/index.html",
        .method = HTTP_GET,
        .handler = index_page_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &index_page_uri);
    
    httpd_uri_t login_api_uri = {
        .uri = "/api/login",
        .method = HTTP_POST,
        .handler = login_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &login_api_uri);
    
    httpd_uri_t logout_api_uri = {
        .uri = "/api/logout",
        .method = HTTP_POST,
        .handler = logout_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &logout_api_uri);
    
    httpd_uri_t get_config_api_uri = {
        .uri = "/api/config",
        .method = HTTP_GET,
        .handler = get_config_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &get_config_api_uri);
    
    httpd_uri_t get_status_api_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = get_status_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &get_status_api_uri);
    
    httpd_uri_t wifi_scan_api_uri = {
        .uri = "/api/wifi/scan",
        .method = HTTP_POST,
        .handler = wifi_scan_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &wifi_scan_api_uri);
    
    httpd_uri_t wifi_scan_custom_api_uri = {
        .uri = "/api/wifi/scan-advanced",
        .method = HTTP_POST,
        .handler = wifi_scan_custom_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &wifi_scan_custom_api_uri);
    
    // 也支持GET方法的自定义扫描（使用默认选项）
    httpd_uri_t wifi_scan_custom_get_uri = {
        .uri = "/api/wifi/scan-advanced",
        .method = HTTP_GET,
        .handler = wifi_scan_custom_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &wifi_scan_custom_get_uri);
    
    // WiFi连接API
    httpd_uri_t wifi_connect_api_uri = {
        .uri = "/api/wifi/connect",
        .method = HTTP_POST,
        .handler = wifi_connect_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &wifi_connect_api_uri);
    
    // WiFi断开连接API
    httpd_uri_t wifi_disconnect_api_uri = {
        .uri = "/api/wifi/disconnect",
        .method = HTTP_POST,
        .handler = wifi_disconnect_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &wifi_disconnect_api_uri);
    
    // WiFi状态查询API
    httpd_uri_t wifi_status_api_uri = {
        .uri = "/api/wifi/status",
        .method = HTTP_GET,
        .handler = wifi_status_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &wifi_status_api_uri);
    
    // WiFi连接状态等待API
    httpd_uri_t wifi_wait_connection_api_uri = {
        .uri = "/api/wifi/wait-connection",
        .method = HTTP_GET,
        .handler = wifi_wait_connection_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &wifi_wait_connection_api_uri);
    
    httpd_uri_t change_password_api_uri = {
        .uri = "/api/change-password",
        .method = HTTP_POST,
        .handler = change_password_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &change_password_api_uri);
    
    httpd_uri_t debug_api_uri = {
        .uri = "/api/debug",
        .method = HTTP_GET,
        .handler = debug_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &debug_api_uri);
    
    httpd_uri_t reset_config_api_uri = {
        .uri = "/api/reset-config",
        .method = HTTP_POST,
        .handler = reset_config_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &reset_config_api_uri);
    
    // 注册配置保存API
    httpd_uri_t save_wifi_config_uri = {
        .uri = "/api/config/wifi",
        .method = HTTP_POST,
        .handler = save_wifi_config_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &save_wifi_config_uri);
    
    httpd_uri_t save_ethernet_config_uri = {
        .uri = "/api/config/ethernet",
        .method = HTTP_POST,
        .handler = save_ethernet_config_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &save_ethernet_config_uri);
    
    // 以太网重启状态查询API
    httpd_uri_t ethernet_restart_status_uri = {
        .uri = "/api/ethernet/restart-status",
        .method = HTTP_GET,
        .handler = ethernet_restart_status_api_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(g_server, &ethernet_restart_status_uri);
    
    httpd_uri_t save_bluetooth_config_uri = {
        .uri = "/api/config/bluetooth",
        .method = HTTP_POST,
        .handler = save_bluetooth_config_api_handler,
        .user_ctx = NULL
    };
    esp_err_t bt_reg_result = httpd_register_uri_handler(g_server, &save_bluetooth_config_uri);
    ESP_LOGI(TAG, "Bluetooth config URI registration: %s", bt_reg_result == ESP_OK ? "SUCCESS" : "FAILED");
    
    httpd_uri_t save_mqtt_config_uri = {
        .uri = "/api/config/mqtt",
        .method = HTTP_POST,
        .handler = save_mqtt_config_api_handler,
        .user_ctx = NULL
    };
    esp_err_t mqtt_reg_result = httpd_register_uri_handler(g_server, &save_mqtt_config_uri);
    ESP_LOGI(TAG, "MQTT config URI registration: %s", mqtt_reg_result == ESP_OK ? "SUCCESS" : "FAILED");
    
    ESP_LOGI(TAG, "Web server started on port %d", WEB_SERVER_PORT);
    ESP_LOGI(TAG, "Registered URI handlers:");
    ESP_LOGI(TAG, "  GET  / - Login page");
    ESP_LOGI(TAG, "  GET  /dashboard - Main dashboard");
    ESP_LOGI(TAG, "  POST /api/login - Login API");
    ESP_LOGI(TAG, "  POST /api/logout - Logout API");
    ESP_LOGI(TAG, "  GET  /api/status - Status API");
    ESP_LOGI(TAG, "  GET  /api/config - Config API");
    ESP_LOGI(TAG, "  POST /api/config/wifi - WiFi config API");
    ESP_LOGI(TAG, "  POST /api/config/ethernet - Ethernet config API");
    ESP_LOGI(TAG, "  POST /api/config/bluetooth - Bluetooth config API");
    ESP_LOGI(TAG, "  POST /api/config/mqtt - MQTT config API");
    return ESP_OK;
}

esp_err_t web_server_stop(void) {
    if (g_server == NULL) {
        ESP_LOGW(TAG, "Web server not started");
        return ESP_OK;
    }
    
    esp_err_t ret = httpd_stop(g_server);
    if (ret == ESP_OK) {
        g_server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
    }
    
    return ret;
}

esp_err_t web_server_get_network_status(network_status_t* status) {
    if (!status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(status, &g_network_status, sizeof(network_status_t));
    return ESP_OK;
}

esp_err_t web_server_update_network_status(const network_status_t* status) {
    if (!status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(&g_network_status, status, sizeof(network_status_t));
    return ESP_OK;
}
