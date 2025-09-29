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

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "esp_err.h"
#include "ini_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_FILE_PATH "/spiffs/config.ini"

/**
 * @brief WiFi AP配置结构体
 */
typedef struct {
    char ssid[32];
    char ip[16];
    char password[64];
} xj1_wifi_ap_config_t;

/**
 * @brief WiFi STA配置结构体
 */
typedef struct {
    char ssid[32];
    char password[64];
} xj1_wifi_sta_config_t;

/**
 * @brief 以太网配置结构体
 */
typedef struct {
    char ip[16];
    char netmask[16];
    char dns[16];
    char gateway[16];
} ethernet_config_t;

/**
 * @brief 认证配置结构体
 */
typedef struct {
    char username[32];
    char password_hash[65];  // SHA-256哈希值(64字符) + '\0'
} auth_config_t;

/**
 * @brief 蓝牙配置结构体
 */
typedef struct {
    char device_name[32];
    char pairing_password[32];
} bluetooth_config_t;

/**
 * @brief MQTT配置结构体
 */
typedef struct {
    char broker_host[64];
    int broker_port;
    char client_id[32];
    char default_topic[64];
    int keepalive;
    char topic_student_to_teacher[64];
    char topic_teacher_to_student[64];
    char topic_student_heartbeat[64];
    char topic_student_status[64];
} mqtt_config_t;

/**
 * @brief Web服务器配置结构体
 */
typedef struct {
    int port;
} web_server_config_t;

/**
 * @brief 超时配置结构体
 */
typedef struct {
    int mqtt_reconnect_timeout;
    int mqtt_connect_timeout;
    int mqtt_refresh_connection;
    int wifi_scan_timeout;
    int wifi_scan_advanced_timeout;
    int session_max_age;
} timeout_config_t;

/**
 * @brief 时间间隔配置结构体
 */
typedef struct {
    int status_update_interval;
    int heartbeat_interval;
    int monitor_check_interval;
} interval_config_t;

/**
 * @brief 系统配置结构体
 */
typedef struct {
    xj1_wifi_ap_config_t wifi_ap;
    xj1_wifi_sta_config_t wifi_sta;
    ethernet_config_t ethernet;
    auth_config_t auth;
    bluetooth_config_t bluetooth;
    mqtt_config_t mqtt;
    web_server_config_t web_server;
    timeout_config_t timeouts;
    interval_config_t intervals;
} system_config_t;

/**
 * @brief 初始化配置管理器
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_init(void);

/**
 * @brief 加载系统配置
 * @param config 系统配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_load(system_config_t* config);

/**
 * @brief 保存系统配置
 * @param config 系统配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_save(const system_config_t* config);

/**
 * @brief 获取WiFi AP配置
 * @param config WiFi AP配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_get_wifi_ap(xj1_wifi_ap_config_t* config);

/**
 * @brief 获取Web服务器配置
 * @param config Web服务器配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_get_web_server(web_server_config_t* config);

/**
 * @brief 设置WiFi AP配置
 * @param config WiFi AP配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_set_wifi_ap(const xj1_wifi_ap_config_t* config);

/**
 * @brief 获取WiFi STA配置
 * @param config WiFi STA配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_get_wifi_sta(xj1_wifi_sta_config_t* config);

/**
 * @brief 设置WiFi STA配置
 * @param config WiFi STA配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_set_wifi_sta(const xj1_wifi_sta_config_t* config);

/**
 * @brief 获取以太网配置
 * @param config 以太网配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_get_ethernet(ethernet_config_t* config);

/**
 * @brief 设置以太网配置
 * @param config 以太网配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_set_ethernet(const ethernet_config_t* config);

/**
 * @brief 获取认证配置
 * @param config 认证配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_get_auth(auth_config_t* config);

/**
 * @brief 设置认证配置
 * @param config 认证配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_set_auth(const auth_config_t* config);

/**
 * @brief 获取蓝牙配置
 * @param config 蓝牙配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_get_bluetooth(bluetooth_config_t* config);

/**
 * @brief 设置蓝牙配置
 * @param config 蓝牙配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_set_bluetooth(const bluetooth_config_t* config);

/**
 * @brief 获取MQTT配置
 * @param config MQTT配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_get_mqtt(mqtt_config_t* config);

/**
 * @brief 设置MQTT配置
 * @param config MQTT配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_set_mqtt(const mqtt_config_t* config);

/**
 * @brief 重置配置为默认值
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_reset_to_default(void);

/**
 * @brief 获取超时配置
 * @param config 超时配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_get_timeouts(timeout_config_t* config);

/**
 * @brief 获取时间间隔配置
 * @param config 时间间隔配置结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t config_manager_get_intervals(interval_config_t* config);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_MANAGER_H
