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

#ifndef INI_PARSER_H
#define INI_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INI_MAX_LINE_LENGTH 256
#define INI_MAX_SECTION_NAME 64
#define INI_MAX_KEY_NAME 64
#define INI_MAX_VALUE_LENGTH 128

/**
 * @brief INI配置项结构体
 */
typedef struct {
    char section[INI_MAX_SECTION_NAME];
    char key[INI_MAX_KEY_NAME];
    char value[INI_MAX_VALUE_LENGTH];
} ini_item_t;

/**
 * @brief INI配置文件句柄
 */
typedef struct ini_config_s ini_config_t;

/**
 * @brief 创建INI配置句柄
 * @return INI配置句柄，失败返回NULL
 */
ini_config_t* ini_config_create(void);

/**
 * @brief 销毁INI配置句柄
 * @param config INI配置句柄
 */
void ini_config_destroy(ini_config_t* config);

/**
 * @brief 从文件加载INI配置
 * @param config INI配置句柄
 * @param filename 文件名
 * @return ESP_OK成功，其他值失败
 */
esp_err_t ini_config_load_from_file(ini_config_t* config, const char* filename);

/**
 * @brief 从字符串加载INI配置
 * @param config INI配置句柄
 * @param ini_string INI格式字符串
 * @return ESP_OK成功，其他值失败
 */
esp_err_t ini_config_load_from_string(ini_config_t* config, const char* ini_string);

/**
 * @brief 保存INI配置到文件
 * @param config INI配置句柄
 * @param filename 文件名
 * @return ESP_OK成功，其他值失败
 */
esp_err_t ini_config_save_to_file(ini_config_t* config, const char* filename);

/**
 * @brief 获取字符串值
 * @param config INI配置句柄
 * @param section 段名
 * @param key 键名
 * @param default_value 默认值
 * @return 配置值，如果不存在返回默认值
 */
const char* ini_config_get_string(ini_config_t* config, const char* section, const char* key, const char* default_value);

/**
 * @brief 获取整数值
 * @param config INI配置句柄
 * @param section 段名
 * @param key 键名
 * @param default_value 默认值
 * @return 配置值，如果不存在返回默认值
 */
int ini_config_get_int(ini_config_t* config, const char* section, const char* key, int default_value);

/**
 * @brief 设置字符串值
 * @param config INI配置句柄
 * @param section 段名
 * @param key 键名
 * @param value 值
 * @return ESP_OK成功，其他值失败
 */
esp_err_t ini_config_set_string(ini_config_t* config, const char* section, const char* key, const char* value);

/**
 * @brief 设置整数值
 * @param config INI配置句柄
 * @param section 段名
 * @param key 键名
 * @param value 值
 * @return ESP_OK成功，其他值失败
 */
esp_err_t ini_config_set_int(ini_config_t* config, const char* section, const char* key, int value);

/**
 * @brief 检查是否存在指定的段和键
 * @param config INI配置句柄
 * @param section 段名
 * @param key 键名
 * @return true存在，false不存在
 */
bool ini_config_has_key(ini_config_t* config, const char* section, const char* key);

#ifdef __cplusplus
}
#endif

#endif // INI_PARSER_H
