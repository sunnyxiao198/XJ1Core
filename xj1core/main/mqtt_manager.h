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

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MQTT状态结构体
 */
typedef struct {
    bool connected;
    char broker_host[64];
    int broker_port;
    char client_id[32];
    char default_topic[64];
    int keepalive;
    int message_count;  // 已发送消息数量
} mqtt_status_t;

/**
 * @brief 初始化MQTT客户端
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_init(void);

/**
 * @brief 启动MQTT客户端
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_start(void);

/**
 * @brief 停止MQTT客户端
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_stop(void);

/**
 * @brief 获取MQTT状态
 * @param status MQTT状态结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_get_status(mqtt_status_t* status);

/**
 * @brief 检查MQTT连接状态
 * @return true已连接，false未连接
 */
bool mqtt_client_is_connected(void);

/**
 * @brief 发布消息
 * @param topic 主题
 * @param data 消息数据
 * @param len 数据长度
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_publish(const char* topic, const char* data, int len);

/**
 * @brief 订阅主题
 * @param topic 主题
 * @param qos QoS级别
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_subscribe(const char* topic, int qos);

/**
 * @brief 取消订阅主题
 * @param topic 主题
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_unsubscribe(const char* topic);

/**
 * @brief 发送学生思念消息到老师
 * @param message 消息内容
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_send_student_message(const char* message);

/**
 * @brief 发送心跳消息
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_send_heartbeat(void);

/**
 * @brief 发送状态消息
 * @param status 状态
 * @param message 消息
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_publish_status(const char* status, const char* message);

/**
 * @brief 启动学生思念心跳任务
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_start_student_heartbeat(void);

/**
 * @brief 停止学生思念心跳任务
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_stop_student_heartbeat(void);

/**
 * @brief 启动MQTT连接监控任务
 * @return ESP_OK成功，其他值失败
 */
esp_err_t mqtt_client_start_monitor(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_MANAGER_H
