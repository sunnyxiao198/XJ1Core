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

#ifndef ETHERNET_MANAGER_H
#define ETHERNET_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 以太网状态结构体
 */
typedef struct {
    bool connected;
    char ip[16];
    char netmask[16];
    char gateway[16];
    char dns[16];
} ethernet_status_t;

/**
 * @brief 初始化以太网管理器
 * @return ESP_OK成功，其他值失败
 */
esp_err_t ethernet_manager_init(void);

/**
 * @brief 启动以太网
 * @return ESP_OK成功，其他值失败
 */
esp_err_t ethernet_manager_start(void);

/**
 * @brief 停止以太网
 * @return ESP_OK成功，其他值失败
 */
esp_err_t ethernet_manager_stop(void);

/**
 * @brief 获取以太网状态
 * @param status 以太网状态结构体指针
 * @return ESP_OK成功，其他值失败
 */
esp_err_t ethernet_manager_get_status(ethernet_status_t* status);

/**
 * @brief 检查以太网连接状态
 * @return true已连接，false未连接
 */
bool ethernet_manager_is_connected(void);

/**
 * @brief 获取以太网IP地址
 * @param ip_str IP地址字符串(至少16字节)
 * @return ESP_OK成功，其他值失败
 */
esp_err_t ethernet_manager_get_ip(char* ip_str);

#ifdef __cplusplus
}
#endif

#endif // ETHERNET_MANAGER_H
