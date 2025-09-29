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

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_wifi.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_SCAN_MAX_AP 20

/**
 * @brief WiFi扫描结果结构体
 */
typedef struct {
    char ssid[33];
    int8_t rssi;
    wifi_auth_mode_t authmode;
} wifi_scan_result_t;

/**
 * @brief WiFi状态结构体
 */
typedef struct {
    bool ap_enabled;
    bool sta_connected;
    char sta_ip[16];
    int sta_rssi;
} wifi_status_t;

/**
 * @brief 初始化WiFi管理器
 * @return ESP_OK成功，其他值失败
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief 启动WiFi AP模式
 * @return ESP_OK成功，其他值失败
 */
esp_err_t wifi_manager_start_ap(void);

/**
 * @brief 停止WiFi AP模式
 * @return ESP_OK成功，其他值失败
 */
esp_err_t wifi_manager_stop_ap(void);

/**
 * @brief 连接WiFi STA
 * @param ssid WiFi网络名称
 * @param password WiFi密码
 * @return ESP_OK成功，其他值失败
 */
esp_err_t wifi_manager_connect_sta(const char* ssid, const char* password);

/**
 * @brief 断开WiFi STA连接
 * @return ESP_OK成功，其他值失败
 */
esp_err_t wifi_manager_disconnect_sta(void);

/**
 * @brief WiFi扫描选项结构体
 */
typedef struct {
    bool show_hidden;       // 是否显示隐藏网络
    bool sort_by_rssi;      // 是否按信号强度排序
    uint32_t scan_timeout;  // 扫描超时时间(毫秒)
} wifi_scan_options_t;

/**
 * @brief 扫描WiFi网络(基础版本)
 * @param results 扫描结果数组
 * @param max_results 最大结果数量
 * @param actual_results 实际扫描到的网络数量
 * @return ESP_OK成功，其他值失败
 */
esp_err_t wifi_manager_scan(wifi_scan_result_t* results, int max_results, int* actual_results);

/**
 * @brief 扫描WiFi网络(增强版本)
 * @param results 扫描结果数组
 * @param max_results 最大结果数量
 * @param actual_results 实际扫描到的网络数量
 * @param options 扫描选项
 * @return ESP_OK成功，其他值失败
 */
esp_err_t wifi_manager_scan_advanced(wifi_scan_result_t* results, int max_results, int* actual_results, const wifi_scan_options_t* options);

/**
 * @brief 获取WiFi状态
 * @param status WiFi状态结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t wifi_manager_get_status(wifi_status_t* status);

/**
 * @brief 检查STA连接状态
 * @return true已连接，false未连接
 */
bool wifi_manager_is_sta_connected(void);

/**
 * @brief 获取STA IP地址
 * @param ip_str IP地址字符串(至少16字节)
 * @return ESP_OK成功，其他值失败
 */
esp_err_t wifi_manager_get_sta_ip(char* ip_str);

/**
 * @brief WiFi扫描测试函数
 * @note 演示不同扫描选项的使用方法
 * @return ESP_OK成功，其他值失败
 */
esp_err_t wifi_manager_test_scan_functions(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
