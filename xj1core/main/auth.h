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

#ifndef AUTH_H
#define AUTH_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUTH_SESSION_TIMEOUT_MS (30 * 60 * 1000)  // 30分钟会话超时
#define AUTH_MAX_SESSIONS 5                        // 最大并发会话数
#define AUTH_SESSION_ID_LENGTH 32                  // 会话ID长度

/**
 * @brief 会话信息结构体
 */
typedef struct {
    char session_id[AUTH_SESSION_ID_LENGTH + 1];
    char username[32];
    uint64_t created_time;
    uint64_t last_access_time;
    bool is_valid;
} auth_session_t;

/**
 * @brief 初始化认证模块
 * @return ESP_OK成功，其他值失败
 */
esp_err_t auth_init(void);

/**
 * @brief 计算字符串的SHA-256哈希值
 * @param input 输入字符串
 * @param output 输出哈希值(64字符的十六进制字符串)
 * @return ESP_OK成功，其他值失败
 */
esp_err_t auth_calculate_sha256(const char* input, char* output);

/**
 * @brief 验证密码
 * @param username 用户名
 * @param password 明文密码
 * @return true验证成功，false验证失败
 */
bool auth_verify_password(const char* username, const char* password);

/**
 * @brief 修改密码
 * @param username 用户名
 * @param old_password 旧密码(明文)
 * @param new_password 新密码(明文)
 * @return ESP_OK成功，其他值失败
 */
esp_err_t auth_change_password(const char* username, const char* old_password, const char* new_password);

/**
 * @brief 用户登录
 * @param username 用户名
 * @param password 密码(明文)
 * @param session_id 输出会话ID
 * @return ESP_OK成功，其他值失败
 */
esp_err_t auth_login(const char* username, const char* password, char* session_id);

/**
 * @brief 用户登出
 * @param session_id 会话ID
 * @return ESP_OK成功，其他值失败
 */
esp_err_t auth_logout(const char* session_id);

/**
 * @brief 验证会话
 * @param session_id 会话ID
 * @return true会话有效，false会话无效
 */
bool auth_validate_session(const char* session_id);

/**
 * @brief 获取会话信息
 * @param session_id 会话ID
 * @param session 输出会话信息
 * @return ESP_OK成功，其他值失败
 */
esp_err_t auth_get_session_info(const char* session_id, auth_session_t* session);

/**
 * @brief 清理过期会话
 * @return 清理的会话数量
 */
int auth_cleanup_expired_sessions(void);

/**
 * @brief 生成随机会话ID
 * @param session_id 输出会话ID
 * @return ESP_OK成功，其他值失败
 */
esp_err_t auth_generate_session_id(char* session_id);

#ifdef __cplusplus
}
#endif

#endif // AUTH_H
