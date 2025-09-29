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

#include "auth.h"
#include "config_manager.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "mbedtls/sha256.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "auth";
static auth_session_t g_sessions[AUTH_MAX_SESSIONS];
static bool g_auth_initialized = false;

/**
 * @brief 将字节数组转换为十六进制字符串
 */
static void bytes_to_hex_string(const uint8_t* bytes, size_t len, char* hex_string) {
    for (size_t i = 0; i < len; i++) {
        sprintf(hex_string + (i * 2), "%02x", bytes[i]);
    }
    hex_string[len * 2] = '\0';
}

/**
 * @brief 查找空闲会话槽位
 */
static int find_free_session_slot(void) {
    for (int i = 0; i < AUTH_MAX_SESSIONS; i++) {
        if (!g_sessions[i].is_valid) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 根据会话ID查找会话
 */
static int find_session_by_id(const char* session_id) {
    if (!session_id) {
        return -1;
    }
    
    for (int i = 0; i < AUTH_MAX_SESSIONS; i++) {
        if (g_sessions[i].is_valid && 
            strcmp(g_sessions[i].session_id, session_id) == 0) {
            return i;
        }
    }
    return -1;
}

esp_err_t auth_init(void) {
    if (g_auth_initialized) {
        return ESP_OK;
    }
    
    // 初始化会话数组
    memset(g_sessions, 0, sizeof(g_sessions));
    
    // 测试SHA-256计算
    char test_hash[65];
    if (auth_calculate_sha256("123456", test_hash) == ESP_OK) {
        ESP_LOGI(TAG, "SHA-256 test - Input: '123456', Hash: '%s'", test_hash);
        ESP_LOGI(TAG, "Expected hash: '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92'");
        if (strcmp(test_hash, "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92") == 0) {
            ESP_LOGI(TAG, "SHA-256 calculation is CORRECT");
        } else {
            ESP_LOGE(TAG, "SHA-256 calculation is WRONG!");
        }
    }
    
    g_auth_initialized = true;
    ESP_LOGI(TAG, "Authentication module initialized");
    return ESP_OK;
}

esp_err_t auth_calculate_sha256(const char* input, char* output) {
    if (!input || !output) {
        return ESP_ERR_INVALID_ARG;
    }
    
    mbedtls_sha256_context ctx;
    uint8_t hash[32];
    
    mbedtls_sha256_init(&ctx);
    
    int ret = mbedtls_sha256_starts(&ctx, 0);  // 0 for SHA-256
    if (ret != 0) {
        mbedtls_sha256_free(&ctx);
        ESP_LOGE(TAG, "Failed to start SHA-256: %d", ret);
        return ESP_FAIL;
    }
    
    ret = mbedtls_sha256_update(&ctx, (const unsigned char*)input, strlen(input));
    if (ret != 0) {
        mbedtls_sha256_free(&ctx);
        ESP_LOGE(TAG, "Failed to update SHA-256: %d", ret);
        return ESP_FAIL;
    }
    
    ret = mbedtls_sha256_finish(&ctx, hash);
    if (ret != 0) {
        mbedtls_sha256_free(&ctx);
        ESP_LOGE(TAG, "Failed to finish SHA-256: %d", ret);
        return ESP_FAIL;
    }
    
    mbedtls_sha256_free(&ctx);
    
    // 转换为十六进制字符串
    bytes_to_hex_string(hash, 32, output);
    
    return ESP_OK;
}

bool auth_verify_password(const char* username, const char* password) {
    if (!username || !password) {
        ESP_LOGE(TAG, "Invalid username or password");
        return false;
    }
    
    ESP_LOGI(TAG, "Verifying login - Username: '%s', Password: '%s'", username, password);
    
    // 获取认证配置
    auth_config_t auth_config;
    esp_err_t ret = config_manager_get_auth(&auth_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get auth config");
        return false;
    }
    
    ESP_LOGI(TAG, "Config loaded - Username: '%s', Password hash: '%s'", auth_config.username, auth_config.password_hash);
    
    // 检查用户名
    if (strcmp(username, auth_config.username) != 0) {
        ESP_LOGW(TAG, "Invalid username: '%s' (expected: '%s')", username, auth_config.username);
        return false;
    }
    
    // 计算输入密码的哈希值
    char password_hash[65];
    ret = auth_calculate_sha256(password, password_hash);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to calculate password hash");
        return false;
    }
    
    ESP_LOGI(TAG, "Calculated hash: '%s'", password_hash);
    ESP_LOGI(TAG, "Expected hash:   '%s'", auth_config.password_hash);
    
    // 比较哈希值
    if (strcmp(password_hash, auth_config.password_hash) == 0) {
        ESP_LOGI(TAG, "Password verification successful for user: %s", username);
        return true;
    } else {
        ESP_LOGW(TAG, "Password verification failed for user: %s", username);
        ESP_LOGW(TAG, "Hash mismatch - calculated vs expected");
        return false;
    }
}

esp_err_t auth_change_password(const char* username, const char* old_password, const char* new_password) {
    if (!username || !old_password || !new_password) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 验证旧密码
    if (!auth_verify_password(username, old_password)) {
        ESP_LOGW(TAG, "Old password verification failed");
        return ESP_ERR_INVALID_ARG;
    }
    
    // 计算新密码的哈希值
    char new_password_hash[65];
    esp_err_t ret = auth_calculate_sha256(new_password, new_password_hash);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to calculate new password hash");
        return ret;
    }
    
    // 更新配置
    auth_config_t auth_config;
    ret = config_manager_get_auth(&auth_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get auth config");
        return ret;
    }
    
    strcpy(auth_config.password_hash, new_password_hash);
    ret = config_manager_set_auth(&auth_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save new password");
        return ret;
    }
    
    ESP_LOGI(TAG, "Password changed successfully for user: %s", username);
    return ESP_OK;
}

esp_err_t auth_generate_session_id(char* session_id) {
    if (!session_id) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 生成32字节随机数据
    uint8_t random_bytes[16];
    esp_fill_random(random_bytes, sizeof(random_bytes));
    
    // 转换为十六进制字符串
    bytes_to_hex_string(random_bytes, sizeof(random_bytes), session_id);
    
    return ESP_OK;
}

esp_err_t auth_login(const char* username, const char* password, char* session_id) {
    if (!username || !password || !session_id) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!g_auth_initialized) {
        ESP_LOGE(TAG, "Auth module not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 验证用户名和密码
    if (!auth_verify_password(username, password)) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 清理过期会话
    auth_cleanup_expired_sessions();
    
    // 查找空闲会话槽位
    int slot = find_free_session_slot();
    if (slot < 0) {
        ESP_LOGW(TAG, "No available session slots");
        return ESP_ERR_NO_MEM;
    }
    
    // 生成会话ID
    esp_err_t ret = auth_generate_session_id(session_id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to generate session ID");
        return ret;
    }
    
    // 创建新会话
    uint64_t now = esp_timer_get_time() / 1000;  // 转换为毫秒
    strcpy(g_sessions[slot].session_id, session_id);
    strcpy(g_sessions[slot].username, username);
    g_sessions[slot].created_time = now;
    g_sessions[slot].last_access_time = now;
    g_sessions[slot].is_valid = true;
    
    ESP_LOGI(TAG, "User %s logged in successfully, session: %s", username, session_id);
    return ESP_OK;
}

esp_err_t auth_logout(const char* session_id) {
    if (!session_id) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!g_auth_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    int slot = find_session_by_id(session_id);
    if (slot < 0) {
        ESP_LOGW(TAG, "Session not found: %s", session_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 清除会话
    memset(&g_sessions[slot], 0, sizeof(auth_session_t));
    
    ESP_LOGI(TAG, "Session logged out: %s", session_id);
    return ESP_OK;
}

bool auth_validate_session(const char* session_id) {
    if (!session_id || !g_auth_initialized) {
        return false;
    }
    
    int slot = find_session_by_id(session_id);
    if (slot < 0) {
        return false;
    }
    
    uint64_t now = esp_timer_get_time() / 1000;  // 转换为毫秒
    
    // 检查会话是否过期
    if (now - g_sessions[slot].last_access_time > AUTH_SESSION_TIMEOUT_MS) {
        ESP_LOGW(TAG, "Session expired: %s", session_id);
        memset(&g_sessions[slot], 0, sizeof(auth_session_t));
        return false;
    }
    
    // 更新最后访问时间
    g_sessions[slot].last_access_time = now;
    
    return true;
}

esp_err_t auth_get_session_info(const char* session_id, auth_session_t* session) {
    if (!session_id || !session) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!g_auth_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    int slot = find_session_by_id(session_id);
    if (slot < 0) {
        return ESP_ERR_NOT_FOUND;
    }
    
    memcpy(session, &g_sessions[slot], sizeof(auth_session_t));
    return ESP_OK;
}

int auth_cleanup_expired_sessions(void) {
    if (!g_auth_initialized) {
        return 0;
    }
    
    int cleaned_count = 0;
    uint64_t now = esp_timer_get_time() / 1000;  // 转换为毫秒
    
    for (int i = 0; i < AUTH_MAX_SESSIONS; i++) {
        if (g_sessions[i].is_valid && 
            now - g_sessions[i].last_access_time > AUTH_SESSION_TIMEOUT_MS) {
            ESP_LOGI(TAG, "Cleaning expired session: %s", g_sessions[i].session_id);
            memset(&g_sessions[i], 0, sizeof(auth_session_t));
            cleaned_count++;
        }
    }
    
    if (cleaned_count > 0) {
        ESP_LOGI(TAG, "Cleaned %d expired sessions", cleaned_count);
    }
    
    return cleaned_count;
}
