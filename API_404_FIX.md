# ESP32 Web API 404错误修复说明

## 🐛 问题描述

用户在使用ESP32 Web界面时遇到以下错误：
- `POST http://192.168.5.1/api/config/mqtt 404 (Not Found)`
- `POST http://192.168.5.1/api/config/bluetooth 404 (Not Found)`

## 🔍 问题分析

通过分析代码发现问题根源：

### 1. 主要原因：URI处理器数量超限
- ESP32 HTTP服务器配置中 `max_uri_handlers = 20`
- 但实际注册的URI处理器有 **22个**
- 导致最后两个处理器（蓝牙和MQTT配置API）注册失败

### 2. URI处理器列表
```
1.  GET  / - 根页面
2.  GET  /login.html - 登录页面  
3.  GET  /index.html - 主页面
4.  POST /api/login - 登录API
5.  POST /api/logout - 登出API
6.  GET  /api/config - 获取配置API
7.  GET  /api/status - 获取状态API
8.  POST /api/wifi/scan - WiFi扫描API
9.  POST /api/wifi/scan_custom - 自定义WiFi扫描API
10. GET  /api/wifi/scan_custom - 获取扫描结果API
11. POST /api/wifi/connect - WiFi连接API
12. POST /api/wifi/disconnect - WiFi断开API
13. GET  /api/wifi/status - WiFi状态API
14. GET  /api/wifi/wait_connection - 等待连接API
15. POST /api/change_password - 修改密码API
16. GET  /api/debug - 调试API
17. POST /api/reset_config - 重置配置API
18. POST /api/config/wifi - WiFi配置API
19. POST /api/config/ethernet - 以太网配置API
20. GET  /api/ethernet/restart_status - 以太网重启状态API
21. POST /api/config/bluetooth - 蓝牙配置API ❌ (未注册)
22. POST /api/config/mqtt - MQTT配置API ❌ (未注册)
```

## ✅ 解决方案

### 1. 增加URI处理器数量限制
```c
// 修改前
config.max_uri_handlers = 20;

// 修改后  
config.max_uri_handlers = 30;  // 增加到30以容纳所有处理器
```

### 2. 添加注册状态日志
```c
esp_err_t bt_reg_result = httpd_register_uri_handler(g_server, &save_bluetooth_config_uri);
ESP_LOGI(TAG, "Bluetooth config URI registration: %s", bt_reg_result == ESP_OK ? "SUCCESS" : "FAILED");

esp_err_t mqtt_reg_result = httpd_register_uri_handler(g_server, &save_mqtt_config_uri);
ESP_LOGI(TAG, "MQTT config URI registration: %s", mqtt_reg_result == ESP_OK ? "SUCCESS" : "FAILED");
```

### 3. 添加API调试日志
在处理函数开头添加调试信息：
```c
static esp_err_t save_mqtt_config_api_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "MQTT config API handler called");
    // ... 其余代码
}

static esp_err_t save_bluetooth_config_api_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Bluetooth config API handler called");
    // ... 其余代码
}
```

## 🔧 修复文件

修改的文件：
- `/mnt/sunny/work/xj1core/xj1core/main/web_server.c`

主要修改：
1. 第1433行：`config.max_uri_handlers = 30;`
2. 第1617-1618行：添加蓝牙配置注册日志
3. 第1626-1627行：添加MQTT配置注册日志
4. 第1172行：添加蓝牙配置API调试日志
5. 第1221行：添加MQTT配置API调试日志

## 🚀 测试验证

### 1. 编译验证
```bash
cd xj1core
idf.py build
```
✅ 编译成功，无错误

### 2. 功能测试
使用测试工具验证API功能：
```bash
python3 test_esp32_api.py 192.168.5.1
```

### 3. 日志验证
启动ESP32后，在串口监视器中应该看到：
```
I (xxxx) web_server: Bluetooth config URI registration: SUCCESS
I (xxxx) web_server: MQTT config URI registration: SUCCESS
```

## 📝 注意事项

1. **内存使用**：增加URI处理器数量会增加内存使用，但30个处理器对ESP32来说是合理的
2. **调试日志**：生产环境可以考虑移除详细的调试日志以节省Flash空间
3. **扩展性**：如果将来需要添加更多API，记得相应增加`max_uri_handlers`的值

## 🎯 预期结果

修复后，用户应该能够：
- ✅ 成功保存MQTT配置
- ✅ 成功保存蓝牙配置  
- ✅ 在串口日志中看到API处理器被调用
- ✅ 收到正确的成功/失败响应

## 🔍 进一步调试

如果问题仍然存在，可以：
1. 检查ESP32串口日志中的URI注册状态
2. 使用`test_esp32_api.py`工具测试API连通性
3. 验证Web界面的JavaScript请求格式是否正确
4. 检查网络连接和认证状态

---
**修复时间**: 2025-09-29  
**修复版本**: ESP-IDF v5.4.1  
**测试状态**: ✅ 编译通过，待设备测试验证
