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

#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_MAX_CLIENTS 5

/**
 * @brief 蓝牙客户端信息结构体
 */
typedef struct {
    char device_name[32];
    char address[18];  // MAC地址格式: XX:XX:XX:XX:XX:XX
    bool connected;
} bt_client_info_t;

/**
 * @brief 蓝牙状态结构体
 */
typedef struct {
    bool enabled;
    int client_count;
    bt_client_info_t clients[BT_MAX_CLIENTS];
    char device_name[32];
} bluetooth_status_t;

/**
 * @brief 初始化蓝牙管理器
 * @return ESP_OK成功，其他值失败
 */
esp_err_t bluetooth_manager_init(void);

/**
 * @brief 启动蓝牙服务
 * @return ESP_OK成功，其他值失败
 */
esp_err_t bluetooth_manager_start(void);

/**
 * @brief 停止蓝牙服务
 * @return ESP_OK成功，其他值失败
 */
esp_err_t bluetooth_manager_stop(void);

/**
 * @brief 获取蓝牙状态
 * @param status 蓝牙状态结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t bluetooth_manager_get_status(bluetooth_status_t* status);

/**
 * @brief 检查蓝牙是否启用
 * @return true已启用，false未启用
 */
bool bluetooth_manager_is_enabled(void);

/**
 * @brief 获取连接的客户端数量
 * @return 客户端数量
 */
int bluetooth_manager_get_client_count(void);

#ifdef __cplusplus
}
#endif

#endif // BLUETOOTH_MANAGER_H
