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

#include "ini_parser.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "ini_parser";

#define MAX_INI_ITEMS 100

/**
 * @brief INI配置文件句柄结构体
 */
struct ini_config_s {
    ini_item_t items[MAX_INI_ITEMS];
    int item_count;
};

/**
 * @brief 去除字符串首尾空白字符
 */
static char* trim_whitespace(char* str) {
    char* end;
    
    // 去除前导空白
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }
    
    if (*str == 0) {
        return str;
    }
    
    // 去除尾部空白
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    
    *(end + 1) = '\0';
    return str;
}

/**
 * @brief 检查是否为段名行
 */
static bool is_section_line(const char* line, char* section_name) {
    const char* trimmed = trim_whitespace((char*)line);
    int len = strlen(trimmed);
    
    if (len < 3 || trimmed[0] != '[' || trimmed[len-1] != ']') {
        return false;
    }
    
    strncpy(section_name, trimmed + 1, len - 2);
    section_name[len - 2] = '\0';
    return true;
}

/**
 * @brief 解析键值对
 */
static bool parse_key_value(const char* line, char* key, char* value) {
    char* equal_pos = strchr(line, '=');
    if (!equal_pos) {
        return false;
    }
    
    // 提取键名
    int key_len = equal_pos - line;
    strncpy(key, line, key_len);
    key[key_len] = '\0';
    trim_whitespace(key);
    
    // 提取值
    strcpy(value, equal_pos + 1);
    trim_whitespace(value);
    
    return true;
}

ini_config_t* ini_config_create(void) {
    ini_config_t* config = (ini_config_t*)heap_caps_malloc(sizeof(ini_config_t), MALLOC_CAP_8BIT);
    if (!config) {
        ESP_LOGE(TAG, "Failed to allocate memory for ini_config");
        return NULL;
    }
    
    config->item_count = 0;
    memset(config->items, 0, sizeof(config->items));
    
    ESP_LOGI(TAG, "INI config created successfully");
    return config;
}

void ini_config_destroy(ini_config_t* config) {
    if (config) {
        free(config);
        ESP_LOGI(TAG, "INI config destroyed");
    }
}

esp_err_t ini_config_load_from_file(ini_config_t* config, const char* filename) {
    if (!config || !filename) {
        return ESP_ERR_INVALID_ARG;
    }
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", filename);
        return ESP_ERR_NOT_FOUND;
    }
    
    char line[INI_MAX_LINE_LENGTH];
    char current_section[INI_MAX_SECTION_NAME] = "";
    char key[INI_MAX_KEY_NAME];
    char value[INI_MAX_VALUE_LENGTH];
    
    config->item_count = 0;
    
    while (fgets(line, sizeof(line), file) && config->item_count < MAX_INI_ITEMS) {
        char* trimmed_line = trim_whitespace(line);
        
        // 跳过空行和注释行
        if (strlen(trimmed_line) == 0 || trimmed_line[0] == '#' || trimmed_line[0] == ';') {
            continue;
        }
        
        // 检查是否为段名
        if (is_section_line(trimmed_line, current_section)) {
            ESP_LOGD(TAG, "Found section: [%s]", current_section);
            continue;
        }
        
        // 解析键值对
        if (parse_key_value(trimmed_line, key, value)) {
            strcpy(config->items[config->item_count].section, current_section);
            strcpy(config->items[config->item_count].key, key);
            strcpy(config->items[config->item_count].value, value);
            config->item_count++;
            
            ESP_LOGD(TAG, "Loaded: [%s] %s = %s", current_section, key, value);
        }
    }
    
    fclose(file);
    ESP_LOGI(TAG, "Loaded %d items from %s", config->item_count, filename);
    return ESP_OK;
}

esp_err_t ini_config_load_from_string(ini_config_t* config, const char* ini_string) {
    if (!config || !ini_string) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char* str_copy = strdup(ini_string);
    if (!str_copy) {
        return ESP_ERR_NO_MEM;
    }
    
    char current_section[INI_MAX_SECTION_NAME] = "";
    char key[INI_MAX_KEY_NAME];
    char value[INI_MAX_VALUE_LENGTH];
    
    config->item_count = 0;
    
    char* line = strtok(str_copy, "\n\r");
    while (line && config->item_count < MAX_INI_ITEMS) {
        char* trimmed_line = trim_whitespace(line);
        
        // 跳过空行和注释行
        if (strlen(trimmed_line) == 0 || trimmed_line[0] == '#' || trimmed_line[0] == ';') {
            line = strtok(NULL, "\n\r");
            continue;
        }
        
        // 检查是否为段名
        if (is_section_line(trimmed_line, current_section)) {
            line = strtok(NULL, "\n\r");
            continue;
        }
        
        // 解析键值对
        if (parse_key_value(trimmed_line, key, value)) {
            strcpy(config->items[config->item_count].section, current_section);
            strcpy(config->items[config->item_count].key, key);
            strcpy(config->items[config->item_count].value, value);
            config->item_count++;
        }
        
        line = strtok(NULL, "\n\r");
    }
    
    free(str_copy);
    ESP_LOGI(TAG, "Loaded %d items from string", config->item_count);
    return ESP_OK;
}

esp_err_t ini_config_save_to_file(ini_config_t* config, const char* filename) {
    if (!config || !filename) {
        return ESP_ERR_INVALID_ARG;
    }
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", filename);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 写入UTF-8 BOM和头注释
    fprintf(file, "# XJ1Core 配置文件\n");
    fprintf(file, "# 编码: UTF-8\n\n");
    
    char current_section[INI_MAX_SECTION_NAME] = "";
    
    for (int i = 0; i < config->item_count; i++) {
        // 如果段名发生变化，写入新段名
        if (strcmp(current_section, config->items[i].section) != 0) {
            if (strlen(current_section) > 0) {
                fprintf(file, "\n");  // 段之间空行
            }
            strcpy(current_section, config->items[i].section);
            fprintf(file, "[%s]\n", current_section);
        }
        
        // 写入键值对
        fprintf(file, "%s=%s\n", config->items[i].key, config->items[i].value);
    }
    
    fclose(file);
    ESP_LOGI(TAG, "Saved %d items to %s", config->item_count, filename);
    return ESP_OK;
}

const char* ini_config_get_string(ini_config_t* config, const char* section, const char* key, const char* default_value) {
    if (!config || !section || !key) {
        return default_value;
    }
    
    for (int i = 0; i < config->item_count; i++) {
        if (strcmp(config->items[i].section, section) == 0 && 
            strcmp(config->items[i].key, key) == 0) {
            return config->items[i].value;
        }
    }
    
    return default_value;
}

int ini_config_get_int(ini_config_t* config, const char* section, const char* key, int default_value) {
    const char* str_value = ini_config_get_string(config, section, key, NULL);
    if (!str_value) {
        return default_value;
    }
    
    return atoi(str_value);
}

esp_err_t ini_config_set_string(ini_config_t* config, const char* section, const char* key, const char* value) {
    if (!config || !section || !key || !value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 检查是否已存在，如果存在则更新
    for (int i = 0; i < config->item_count; i++) {
        if (strcmp(config->items[i].section, section) == 0 && 
            strcmp(config->items[i].key, key) == 0) {
            strncpy(config->items[i].value, value, INI_MAX_VALUE_LENGTH - 1);
            config->items[i].value[INI_MAX_VALUE_LENGTH - 1] = '\0';
            return ESP_OK;
        }
    }
    
    // 如果不存在且还有空间，则添加新项
    if (config->item_count < MAX_INI_ITEMS) {
        strncpy(config->items[config->item_count].section, section, INI_MAX_SECTION_NAME - 1);
        config->items[config->item_count].section[INI_MAX_SECTION_NAME - 1] = '\0';
        
        strncpy(config->items[config->item_count].key, key, INI_MAX_KEY_NAME - 1);
        config->items[config->item_count].key[INI_MAX_KEY_NAME - 1] = '\0';
        
        strncpy(config->items[config->item_count].value, value, INI_MAX_VALUE_LENGTH - 1);
        config->items[config->item_count].value[INI_MAX_VALUE_LENGTH - 1] = '\0';
        
        config->item_count++;
        return ESP_OK;
    }
    
    ESP_LOGE(TAG, "INI config is full, cannot add more items");
    return ESP_ERR_NO_MEM;
}

esp_err_t ini_config_set_int(ini_config_t* config, const char* section, const char* key, int value) {
    char str_value[32];
    snprintf(str_value, sizeof(str_value), "%d", value);
    return ini_config_set_string(config, section, key, str_value);
}

bool ini_config_has_key(ini_config_t* config, const char* section, const char* key) {
    if (!config || !section || !key) {
        return false;
    }
    
    for (int i = 0; i < config->item_count; i++) {
        if (strcmp(config->items[i].section, section) == 0 && 
            strcmp(config->items[i].key, key) == 0) {
            return true;
        }
    }
    
    return false;
}
