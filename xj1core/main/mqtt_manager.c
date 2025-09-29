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

#include "mqtt_manager.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "mqtt_client";

static esp_mqtt_client_handle_t g_mqtt_client = NULL;
static bool g_mqtt_initialized = false;
static bool g_mqtt_connected = false;
static int g_message_count = 0;
static TaskHandle_t g_heartbeat_task_handle = NULL;
static TaskHandle_t g_mqtt_monitor_task_handle = NULL;

// 师生通信主题变量（从配置文件读取）
static char g_topic_student_to_teacher[64] = "xj1core/student/message";
static char g_topic_teacher_to_student[64] = "xj1cloud/teacher/message";
static char g_topic_student_heartbeat[64] = "xj1core/heartbeat";
static char g_topic_student_status[64] = "xj1core/status";


/**
 * @brief 从配置文件加载MQTT主题
 */
static void load_mqtt_topics_from_config(void) {
    mqtt_config_t mqtt_config;
    esp_err_t ret = config_manager_get_mqtt(&mqtt_config);
    if (ret == ESP_OK) {
        strcpy(g_topic_student_to_teacher, mqtt_config.topic_student_to_teacher);
        strcpy(g_topic_teacher_to_student, mqtt_config.topic_teacher_to_student);
        strcpy(g_topic_student_heartbeat, mqtt_config.topic_student_heartbeat);
        strcpy(g_topic_student_status, mqtt_config.topic_student_status);
        
        ESP_LOGI(TAG, "MQTT topics loaded from config:");
        ESP_LOGI(TAG, "  Student->Teacher: %s", g_topic_student_to_teacher);
        ESP_LOGI(TAG, "  Teacher->Student: %s", g_topic_teacher_to_student);
        ESP_LOGI(TAG, "  Heartbeat: %s", g_topic_student_heartbeat);
        ESP_LOGI(TAG, "  Status: %s", g_topic_student_status);
    } else {
        ESP_LOGW(TAG, "Failed to load MQTT config, using default topics");
    }
}

// MQTT事件处理
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "🎉 MQTT连接成功！师生通信链路已建立");
            ESP_LOGI(TAG, "⏰ 连接时间: %lld毫秒", esp_timer_get_time() / 1000);
            g_mqtt_connected = true;
            
            // 订阅老师的消息主题
            int msg_id = esp_mqtt_client_subscribe(g_mqtt_client, g_topic_teacher_to_student, 1);
            ESP_LOGI(TAG, "📥 已订阅老师消息主题: %s, msg_id=%d", g_topic_teacher_to_student, msg_id);
            
            // 自动订阅默认主题
            mqtt_config_t mqtt_config;
            if (config_manager_get_mqtt(&mqtt_config) == ESP_OK) {
                msg_id = esp_mqtt_client_subscribe(g_mqtt_client, mqtt_config.default_topic, 0);
                ESP_LOGI(TAG, "📥 已订阅默认主题: %s, msg_id=%d", mqtt_config.default_topic, msg_id);
            }
            
            // 发送上线消息
            mqtt_client_publish_status("online", "学生设备已上线，等待与老师通信");
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "❌ MQTT连接断开");
            g_mqtt_connected = false;
            // 这里可以添加自动重连逻辑
            ESP_LOGI(TAG, "将由ESP-IDF自动重连...");
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            g_message_count++;
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "📨 收到MQTT消息");
            
            // 提取主题和数据
            char topic[128] = {0};
            char data[256] = {0};
            
            if (event->topic_len < sizeof(topic)) {
                strncpy(topic, event->topic, event->topic_len);
            }
            if (event->data_len < sizeof(data)) {
                strncpy(data, event->data, event->data_len);
            }
            
            ESP_LOGI(TAG, "📍 主题: %s", topic);
            ESP_LOGI(TAG, "💬 内容: %s", data);
            
            // 特殊处理老师的消息
            if (strstr(topic, "teacher") != NULL || strstr(topic, g_topic_teacher_to_student) != NULL) {
                ESP_LOGI(TAG, "💖 收到老师的消息！");
                ESP_LOGI(TAG, "🎓 老师说: %s", data);
                
                // 可以在这里添加LED闪烁、蜂鸣器响等提示
                // 或者通过Web界面显示老师的消息
            }
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "❌ MQTT连接错误");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "TCP传输错误: %s", strerror(event->error_handle->esp_transport_sock_errno));
                ESP_LOGE(TAG, "错误代码: %d", event->error_handle->esp_transport_sock_errno);
                ESP_LOGE(TAG, "请检查MQTT服务器地址和端口是否正确");
                
                // 根据具体错误给出建议
                if (event->error_handle->esp_transport_sock_errno == 104) { // Connection reset by peer
                    ESP_LOGE(TAG, "💡 连接被服务器重置，可能原因:");
                    ESP_LOGE(TAG, "   1. MQTT协议版本不匹配");
                    ESP_LOGE(TAG, "   2. 客户端ID冲突");
                    ESP_LOGE(TAG, "   3. 服务器配置问题");
                    ESP_LOGE(TAG, "   4. 网络路由问题");
                } else if (event->error_handle->esp_transport_sock_errno == 111) { // Connection refused
                    ESP_LOGE(TAG, "💡 连接被拒绝，请检查MQTT服务器是否运行");
                }
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGE(TAG, "MQTT服务器拒绝连接，请检查服务器状态");
            }
            g_mqtt_connected = false;
            break;
            
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

esp_err_t mqtt_client_init(void) {
    if (g_mqtt_initialized) {
        return ESP_OK;
    }
    
    // 从配置文件加载MQTT主题
    load_mqtt_topics_from_config();
    
    // 获取MQTT配置
    mqtt_config_t mqtt_config;
    esp_err_t ret = config_manager_get_mqtt(&mqtt_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get MQTT config");
        return ret;
    }
    
    // 验证配置完整性
    if (strlen(mqtt_config.broker_host) == 0) {
        ESP_LOGE(TAG, "MQTT broker_host为空，请检查config.ini配置");
        return ESP_ERR_INVALID_ARG;
    }
    if (mqtt_config.broker_port <= 0 || mqtt_config.broker_port > 65535) {
        ESP_LOGE(TAG, "MQTT broker_port无效: %d，请检查config.ini配置", mqtt_config.broker_port);
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(mqtt_config.client_id) == 0) {
        ESP_LOGE(TAG, "MQTT client_id为空，请检查config.ini配置");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "🔧 MQTT配置验证通过:");
    ESP_LOGI(TAG, "  服务器: %s:%d", mqtt_config.broker_host, mqtt_config.broker_port);
    ESP_LOGI(TAG, "  客户端ID: %s", mqtt_config.client_id);
    ESP_LOGI(TAG, "  保活时间: %d秒", mqtt_config.keepalive);
    
    // 构建MQTT URI
    char mqtt_uri[128];
    snprintf(mqtt_uri, sizeof(mqtt_uri), "mqtt://%s:%d", mqtt_config.broker_host, mqtt_config.broker_port);
    
    // 生成唯一的客户端ID（避免冲突）
    char unique_client_id[64];
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(unique_client_id, sizeof(unique_client_id), "%s-%02x%02x%02x", 
             mqtt_config.client_id, mac[3], mac[4], mac[5]);
    
    ESP_LOGI(TAG, "🆔 使用唯一客户端ID: %s", unique_client_id);
    ESP_LOGI(TAG, "🔗 MQTT URI: %s", mqtt_uri);
    ESP_LOGI(TAG, "⏱️  保活时间: %d秒", mqtt_config.keepalive);
    
    // 获取超时配置
    timeout_config_t timeout_config;
    esp_err_t timeout_ret = config_manager_get_timeouts(&timeout_config);
    if (timeout_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get timeout config, using defaults");
        timeout_config.mqtt_reconnect_timeout = 10000;
        timeout_config.mqtt_connect_timeout = 15000;
        timeout_config.mqtt_refresh_connection = 30000;
    }
    
    // 配置MQTT客户端（从配置文件读取超时参数）
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt_uri,
        .credentials.client_id = unique_client_id,
        .session.keepalive = mqtt_config.keepalive,
        .session.disable_clean_session = true,  // 使用clean session避免状态冲突
        .network.reconnect_timeout_ms = timeout_config.mqtt_reconnect_timeout,
        .network.timeout_ms = timeout_config.mqtt_connect_timeout,
        .network.refresh_connection_after_ms = timeout_config.mqtt_refresh_connection,
        .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,  // 使用MQTT 3.1.1协议
        .network.disable_auto_reconnect = false,
        .session.last_will.topic = "xj1core/status",
        .session.last_will.msg = "offline",
        .session.last_will.msg_len = 7,
        .session.last_will.qos = 0,
        .session.last_will.retain = false,
    };
    
    g_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!g_mqtt_client) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }
    
    // 注册事件处理器
    ret = esp_mqtt_client_register_event(g_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register MQTT event handler");
        esp_mqtt_client_destroy(g_mqtt_client);
        g_mqtt_client = NULL;
        return ret;
    }
    
    g_mqtt_initialized = true;
    ESP_LOGI(TAG, "MQTT client initialized successfully");
    ESP_LOGI(TAG, "Broker: %s:%d, Client ID: %s", mqtt_config.broker_host, mqtt_config.broker_port, mqtt_config.client_id);
    
    // 提示用户关于MQTT服务器的信息
    if (strcmp(mqtt_config.broker_host, "localhost") == 0 || strcmp(mqtt_config.broker_host, "127.0.0.1") == 0) {
        ESP_LOGW(TAG, "⚠️  MQTT服务器配置为localhost，ESP32无法连接");
        ESP_LOGW(TAG, "请在config.ini中修改broker_host为实际的服务器IP地址");
    } else if (strcmp(mqtt_config.broker_host, "192.168.5.1") == 0) {
        ESP_LOGI(TAG, "💡 MQTT服务器配置为AP地址，请确保在AP网络中运行MQTT服务器");
    } else if (strcmp(mqtt_config.broker_host, "192.168.1.40") == 0) {
        ESP_LOGI(TAG, "💡 MQTT服务器配置为: %s，请确保该地址可访问", mqtt_config.broker_host);
        ESP_LOGI(TAG, "💡 提示：如果WiFi STA未连接，可能无法访问外部网络服务器");
    } else {
        ESP_LOGI(TAG, "💡 MQTT服务器地址: %s，请确保网络可达", mqtt_config.broker_host);
    }
    return ESP_OK;
}

esp_err_t mqtt_client_start(void) {
    if (!g_mqtt_initialized) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!g_mqtt_client) {
        ESP_LOGE(TAG, "MQTT client handle is NULL");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 启动前检查网络状态
    wifi_status_t wifi_status;
    if (wifi_manager_get_status(&wifi_status) == ESP_OK) {
        if (wifi_status.sta_connected) {
            ESP_LOGI(TAG, "🌐 WiFi STA已连接，IP: %s", wifi_status.sta_ip);
            
            // 检查IP网段
            if (strncmp(wifi_status.sta_ip, "192.168.1.", 10) == 0) {
                ESP_LOGI(TAG, "✅ ESP32在192.168.1.x网段，能访问MQTT服务器");
            } else {
                ESP_LOGW(TAG, "⚠️  ESP32在%s，可能无法访问192.168.1.40", wifi_status.sta_ip);
                ESP_LOGW(TAG, "💡 需要确保ESP32和MQTT服务器在同一网络或可路由");
            }
        } else {
            ESP_LOGW(TAG, "⚠️  WiFi STA未连接，检查AP模式网络...");
            ESP_LOGI(TAG, "💡 即使STA未连接，AP模式仍可能提供网络访问");
        }
    }
    
    // 获取并显示MQTT配置
    mqtt_config_t mqtt_config;
    if (config_manager_get_mqtt(&mqtt_config) == ESP_OK) {
        ESP_LOGI(TAG, "🔧 即将连接MQTT服务器: %s:%d", mqtt_config.broker_host, mqtt_config.broker_port);
        
        // 等待网络稳定
        ESP_LOGI(TAG, "⏳ 等待网络稳定...");
        vTaskDelay(pdMS_TO_TICKS(2000));  // 等待2秒确保网络稳定
    }
    
    esp_err_t ret = esp_mqtt_client_start(g_mqtt_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "MQTT client started, waiting for connection...");
    return ESP_OK;
}

esp_err_t mqtt_client_stop(void) {
    if (!g_mqtt_initialized || !g_mqtt_client) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = esp_mqtt_client_stop(g_mqtt_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop MQTT client: %s", esp_err_to_name(ret));
        return ret;
    }
    
    g_mqtt_connected = false;
    ESP_LOGI(TAG, "MQTT client stopped");
    return ESP_OK;
}

esp_err_t mqtt_client_get_status(mqtt_status_t* status) {
    if (!status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 获取MQTT配置
    mqtt_config_t mqtt_config;
    esp_err_t ret = config_manager_get_mqtt(&mqtt_config);
    if (ret != ESP_OK) {
        return ret;
    }
    
    status->connected = g_mqtt_connected;
    strncpy(status->broker_host, mqtt_config.broker_host, sizeof(status->broker_host) - 1);
    status->broker_port = mqtt_config.broker_port;
    strncpy(status->client_id, mqtt_config.client_id, sizeof(status->client_id) - 1);
    strncpy(status->default_topic, mqtt_config.default_topic, sizeof(status->default_topic) - 1);
    status->keepalive = mqtt_config.keepalive;
    status->message_count = g_message_count;
    
    return ESP_OK;
}

bool mqtt_client_is_connected(void) {
    return g_mqtt_connected;
}

esp_err_t mqtt_client_publish(const char* topic, const char* data, int len) {
    if (!g_mqtt_initialized || !g_mqtt_client) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!topic || !data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int msg_id = esp_mqtt_client_publish(g_mqtt_client, topic, data, len, 0, 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish message");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Published message to topic %s, msg_id=%d", topic, msg_id);
    return ESP_OK;
}

esp_err_t mqtt_client_subscribe(const char* topic, int qos) {
    if (!g_mqtt_initialized || !g_mqtt_client) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!topic) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int msg_id = esp_mqtt_client_subscribe(g_mqtt_client, topic, qos);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to subscribe to topic %s", topic);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Subscribed to topic %s, msg_id=%d", topic, msg_id);
    return ESP_OK;
}

esp_err_t mqtt_client_unsubscribe(const char* topic) {
    if (!g_mqtt_initialized || !g_mqtt_client) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!topic) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int msg_id = esp_mqtt_client_unsubscribe(g_mqtt_client, topic);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to unsubscribe from topic %s", topic);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Unsubscribed from topic %s, msg_id=%d", topic, msg_id);
    return ESP_OK;
}

/**
 * @brief 发送学生思念消息到老师
 */
esp_err_t mqtt_client_send_student_message(const char* message) {
    if (!g_mqtt_connected) {
        ESP_LOGW(TAG, "MQTT未连接，无法发送学生消息");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 构建JSON消息
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "from", "学生");
    cJSON_AddStringToObject(json, "to", "唐老师");
    cJSON_AddStringToObject(json, "message", message);
    cJSON_AddNumberToObject(json, "timestamp", esp_timer_get_time() / 1000000);
    cJSON_AddStringToObject(json, "device", "XJ1Core-ESP32");
    cJSON_AddNumberToObject(json, "sequence", g_message_count + 1);
    
    char *json_string = cJSON_Print(json);
    
    ESP_LOGI(TAG, "💌 向老师发送消息: %s", message);
    
    esp_err_t ret = mqtt_client_publish(g_topic_student_to_teacher, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(json);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✅ 学生消息发送成功");
    } else {
        ESP_LOGE(TAG, "❌ 学生消息发送失败");
    }
    
    return ret;
}

/**
 * @brief 发送心跳消息
 */
esp_err_t mqtt_client_send_heartbeat(void) {
    if (!g_mqtt_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 构建心跳JSON消息
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "device", "XJ1Core-ESP32");
    cJSON_AddStringToObject(json, "status", "alive");
    cJSON_AddNumberToObject(json, "timestamp", esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(json, "uptime", esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(json, "free_heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(json, "message_count", g_message_count);
    
    char *json_string = cJSON_Print(json);
    
    esp_err_t ret = mqtt_client_publish(g_topic_student_heartbeat, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(json);
    
    return ret;
}

/**
 * @brief 发送状态消息
 */
esp_err_t mqtt_client_publish_status(const char* status, const char* message) {
    if (!g_mqtt_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "device", "XJ1Core-ESP32");
    cJSON_AddStringToObject(json, "status", status);
    cJSON_AddStringToObject(json, "message", message);
    cJSON_AddNumberToObject(json, "timestamp", esp_timer_get_time() / 1000000);
    
    char *json_string = cJSON_Print(json);
    esp_err_t ret = mqtt_client_publish(g_topic_student_status, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(json);
    
    return ret;
}

/**
 * @brief 学生思念心跳任务
 * @param pvParameters 任务参数
 */
static void student_heartbeat_task(void *pvParameters) {
    static int heartbeat_count = 0;
    
    ESP_LOGI(TAG, "💖 学生思念心跳任务启动");
    ESP_LOGI(TAG, "🎓 将每5秒向老师发送思念消息...");
    
    // 等待MQTT连接建立
    while (!g_mqtt_connected) {
        ESP_LOGI(TAG, "等待MQTT连接...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    ESP_LOGI(TAG, "✅ MQTT已连接，开始发送思念消息");
    
    while (1) {
        if (g_mqtt_connected) {
            heartbeat_count++;
            
            // 发送思念消息
            char message[128];
            snprintf(message, sizeof(message), "唐老师：学生想你 (第%d次)", heartbeat_count);
            
            esp_err_t ret = mqtt_client_send_student_message(message);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "💕 第%d次思念消息已发送", heartbeat_count);
            } else {
                ESP_LOGW(TAG, "💔 思念消息发送失败，第%d次", heartbeat_count);
            }
            
            // 同时发送心跳
            mqtt_client_send_heartbeat();
            
        } else {
            ESP_LOGW(TAG, "MQTT连接断开，暂停发送思念消息");
        }
        
        // 获取心跳间隔配置
        interval_config_t interval_config;
        int heartbeat_interval = 5000; // 默认值
        if (config_manager_get_intervals(&interval_config) == ESP_OK) {
            heartbeat_interval = interval_config.heartbeat_interval;
        }
        
        vTaskDelay(pdMS_TO_TICKS(heartbeat_interval));
    }
}

/**
 * @brief 启动学生思念心跳任务
 */
esp_err_t mqtt_client_start_student_heartbeat(void) {
    if (g_heartbeat_task_handle != NULL) {
        ESP_LOGW(TAG, "学生心跳任务已在运行");
        return ESP_OK;
    }
    
    BaseType_t ret = xTaskCreate(
        student_heartbeat_task,
        "student_heartbeat",
        8192,
        NULL,
        5,
        &g_heartbeat_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建学生心跳任务失败");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "🚀 学生思念心跳任务已启动");
    return ESP_OK;
}

/**
 * @brief MQTT连接监控任务
 */
static void mqtt_monitor_task(void *pvParameters) {
    static int connection_attempts = 0;
    static int last_connected_time = 0;
    
    ESP_LOGI(TAG, "🔍 MQTT连接监控任务启动");
    
    while (1) {
        if (!g_mqtt_connected && g_mqtt_initialized) {
            connection_attempts++;
            int current_time = esp_timer_get_time() / 1000000;
            
            // 每30秒检查一次连接状态
            if (current_time - last_connected_time > 30) {
                ESP_LOGW(TAG, "🔄 MQTT连接监控 - 尝试次数: %d", connection_attempts);
                
                // 检查网络状态
                wifi_status_t wifi_status;
                if (wifi_manager_get_status(&wifi_status) == ESP_OK) {
                    if (wifi_status.sta_connected) {
                        ESP_LOGI(TAG, "✅ WiFi STA已连接: %s", wifi_status.sta_ip);
                        
                        // 检查IP地址段是否匹配
                        if (strncmp(wifi_status.sta_ip, "192.168.1.", 10) == 0) {
                            ESP_LOGI(TAG, "✅ ESP32在192.168.1.x网段，应该能访问MQTT服务器");
                        } else if (strncmp(wifi_status.sta_ip, "192.168.5.", 10) == 0) {
                            ESP_LOGW(TAG, "⚠️  ESP32在192.168.5.x网段，可能无法访问192.168.1.40");
                            ESP_LOGW(TAG, "💡 建议修改config.ini中broker_host为192.168.5.1");
                        } else {
                            ESP_LOGW(TAG, "⚠️  ESP32在%s网段，请确认能访问192.168.1.40", wifi_status.sta_ip);
                        }
                    } else {
                        ESP_LOGW(TAG, "⚠️  WiFi STA未连接，这可能影响MQTT连接");
                        ESP_LOGI(TAG, "💡 ESP32可能只能通过AP模式(192.168.5.x)访问MQTT服务器");
                    }
                }
                
                // 获取MQTT配置并显示
                mqtt_config_t mqtt_config;
                if (config_manager_get_mqtt(&mqtt_config) == ESP_OK) {
                    ESP_LOGI(TAG, "🔧 MQTT配置 - 服务器: %s:%d, 客户端ID: %s", 
                             mqtt_config.broker_host, mqtt_config.broker_port, mqtt_config.client_id);
                }
                
                last_connected_time = current_time;
            }
        } else if (g_mqtt_connected) {
            // 连接成功，重置计数器
            if (connection_attempts > 0) {
                ESP_LOGI(TAG, "🎉 MQTT连接监控 - 连接成功！");
                connection_attempts = 0;
            }
        }
        
        // 获取监控检查间隔配置
        interval_config_t interval_config;
        int monitor_interval = 10000; // 默认值
        if (config_manager_get_intervals(&interval_config) == ESP_OK) {
            monitor_interval = interval_config.monitor_check_interval;
        }
        
        vTaskDelay(pdMS_TO_TICKS(monitor_interval));
    }
}

/**
 * @brief 启动MQTT监控任务
 */
esp_err_t mqtt_client_start_monitor(void) {
    if (g_mqtt_monitor_task_handle != NULL) {
        ESP_LOGW(TAG, "MQTT监控任务已在运行");
        return ESP_OK;
    }
    
    BaseType_t ret = xTaskCreate(
        mqtt_monitor_task,
        "mqtt_monitor",
        4096,
        NULL,
        3,
        &g_mqtt_monitor_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建MQTT监控任务失败");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "🚀 MQTT连接监控任务已启动");
    return ESP_OK;
}

/**
 * @brief 停止学生思念心跳任务
 */
esp_err_t mqtt_client_stop_student_heartbeat(void) {
    if (g_heartbeat_task_handle != NULL) {
        vTaskDelete(g_heartbeat_task_handle);
        g_heartbeat_task_handle = NULL;
        ESP_LOGI(TAG, "学生心跳任务已停止");
    }
    return ESP_OK;
}
