/**
* 项目：湘江一号 - 物联网核心教学套件  
* 作者：ironxiao
* 描述：ESP32物联网开发核心模板，实现MQTT双向通信
*       献给所有怀揣物联梦想的学子及开发者
*       愿此代码成为你们探索世界的起点
* 
* 特别致谢：谨以此项目感谢我的恩师唐家乾老师
*       唐老师，您的教诲是我前行路上的光。学生很想您 * 
* 物联网开发核心：从零构建一个双向通信系统
* 许多初学者认为物联网（IoT）高深莫测，但其实它的核心逻辑可以非常直观。想象一下，我们要构建一个完整的'神经'系统：
*    '感官'（传感器） 负责采集数据。
*     '脊髓'（ESP32） 负责汇集信息并传递指令。
*    '大脑'（云端） 负责处理信息并做出决策。
*而贯穿全程的'神经信号'，就是MQTT协议。
*本项目将带您亲手实现这个系统：首先，为ESP32设计一个Web配置管理系统；接着，实现MQTT双向通信，将传感器数据上报云端；最后，完成从云端下发指令控制传感器的闭环。 打通这个流程，您就掌握了物联网开发最精髓的骨架。
* 开源协议：MIT License 
*/

# 湘江一号 - 物联网核心教学套件 🌟

<div align="center">

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Python](https://img.shields.io/badge/Python-3.8+-green.svg)](https://www.python.org/)
[![MQTT](https://img.shields.io/badge/Protocol-MQTT-orange.svg)](https://mqtt.org/)

**一个完整的ESP32物联网开发教学套件，从硬件到云端的全栈解决方案**

*献给所有怀揣物联梦想的学子及开发者*

</div>

## 💖 项目愿景

这是一个专为物联网学习和教学设计的完整套件。通过这个项目，你将学会构建一个真正的物联网系统：从ESP32硬件编程，到Web界面开发，再到云端数据处理，打通物联网开发的完整链路。

**特别致谢**：谨以此项目感谢恩师唐家乾老师 🎓  
*唐老师，您的教诲是我前行路上的光。学生很想您*

## 🏗️ 系统架构

```
┌─────────────────────┐    ┌─────────────────────┐    ┌─────────────────────┐
│    ESP32设备端      │    │    MQTT Broker      │    │     云端服务        │
│  (xj1core)         │    │   (Mosquitto)       │    │  (xj1corecloud)     │
│                     │    │                     │    │                     │
│ • WiFi/蓝牙/以太网  │◄──►│ • 消息路由          │◄──►│ • Web管理界面       │
│ • 传感器数据采集    │    │ • 主题管理          │    │ • MQTT客户端        │
│ • Web配置界面       │    │ • 消息分发          │    │ • HTTP API接口      │
│ • MQTT通信          │    │ • 持久化存储        │    │ • 实时数据显示      │
│ • 远程控制          │    │                     │    │ • 历史数据管理      │
└─────────────────────┘    └─────────────────────┘    └─────────────────────┘
```

## 🎯 核心特性

### 🔌 ESP32设备端 (xj1core)
- **多网络支持**: WiFi STA/AP模式、蓝牙、以太网
- **Web配置系统**: 通过浏览器配置网络和MQTT参数
- **MQTT双向通信**: 数据上报和指令接收
- **配置管理**: INI格式配置文件，支持热加载
- **认证系统**: Web界面登录保护
- **模块化设计**: 清晰的代码结构，易于扩展

### ☁️ 云端服务 (xj1corecloud)
- **Web管理界面**: 现代化响应式设计
- **实时消息显示**: WebSocket实时推送
- **MQTT客户端**: 完整的订阅和发布功能
- **HTTP API**: RESTful接口，支持ESP32调用
- **消息历史**: 自动保存和查询历史消息
- **状态监控**: 实时显示连接状态和系统信息

## 📦 项目结构

```
xj1core/
├── xj1core/                    # ESP32固件源码
│   ├── main/                   # 主程序目录
│   │   ├── main.c             # 主程序入口
│   │   ├── wifi_manager.c/h   # WiFi管理模块
│   │   ├── mqtt_manager.c/h   # MQTT通信模块
│   │   ├── web_server.c/h     # Web服务器模块
│   │   ├── bluetooth_manager.c/h # 蓝牙管理模块
│   │   ├── ethernet_manager.c/h  # 以太网管理模块
│   │   ├── config_manager.c/h    # 配置管理模块
│   │   └── auth.c/h           # 认证模块
│   ├── components/            # 自定义组件
│   │   └── ini_parser/        # INI解析器
│   ├── web/                   # Web界面资源
│   │   ├── index.html         # 主页面
│   │   ├── login.html         # 登录页面
│   │   ├── css/              # 样式文件
│   │   └── js/               # JavaScript文件
│   ├── config.ini             # 配置文件
│   ├── CMakeLists.txt         # 构建配置
│   └── sdkconfig             # ESP-IDF配置
│
├── xj1corecloud/              # 云端服务源码
│   ├── web_app.py            # Flask Web应用
│   ├── xj1cloud.py           # MQTT客户端(命令行版)
│   ├── config_manager.py     # 配置管理器
│   ├── config.ini            # 服务配置
│   ├── templates/            # HTML模板
│   │   └── index.html        # Web界面
│   ├── static/               # 静态资源
│   │   ├── css/style.css     # 样式文件
│   │   └── js/app.js         # 前端JavaScript
│   ├── requirements.txt      # Python依赖
│   ├── start_web.py         # Web服务启动脚本
│   ├── start_web.sh         # Shell启动脚本
│   ├── test_esp32_http_api.py # ESP32 API测试工具
│   ├── test_http_api.html    # HTTP API测试页面
│   └── README_WEB.md         # Web服务详细文档
│
├── README.md                  # 项目主文档
├── TROUBLESHOOTING.md        # 故障排除指南
└── WEB_DASHBOARD_UPDATE.md   # Web界面更新日志
```

## 🚀 快速开始

### 📋 环境要求

**ESP32开发环境:**
- ESP-IDF v4.4+
- Python 3.8+
- Git

**云端服务环境:**
- Python 3.8+
- MQTT Broker (Mosquitto推荐)

### 🔧 安装步骤

#### 1. 克隆项目
```bash
git clone https://github.com/your-repo/xj1core.git
cd xj1core
```

#### 2. 设置ESP32开发环境
```bash
# 安装ESP-IDF
curl -LO https://github.com/espressif/esp-idf/releases/download/v4.4.2/esp-idf-v4.4.2.zip
unzip esp-idf-v4.4.2.zip
cd esp-idf-v4.4.2
./install.sh
source export.sh

# 返回项目目录
cd ../xj1core
```

#### 3. 编译和烧录ESP32固件
```bash
cd xj1core
idf.py set-target esp32s3  # 或 esp32
idf.py menuconfig          # 可选：配置项目参数
idf.py build
idf.py flash monitor
```

#### 4. 设置云端服务
```bash
cd xj1corecloud

# 创建虚拟环境
python3 -m venv venv
source venv/bin/activate

# 安装依赖
pip install -r requirements.txt

# 启动服务
./start_web.sh
```

## 🎮 使用指南

### 🔌 ESP32设备配置

1. **首次启动**: ESP32会创建一个名为"XJ1Core-Setup"的WiFi热点
2. **连接配置**: 
   - 连接到热点
   - 打开浏览器访问 `http://192.168.5.1`
   - 输入默认密码: `xj1core2025`
3. **网络配置**: 在Web界面中配置WiFi、MQTT等参数
4. **保存重启**: 配置保存后ESP32会自动重启并连接到指定网络

### ☁️ 云端服务使用

1. **启动服务**: 运行 `./start_web.sh` 启动Web服务
2. **访问界面**: 打开浏览器访问 `http://localhost:5000`
3. **消息管理**: 
   - 实时查看ESP32上报的消息
   - 通过Web界面发送控制指令
   - 查看历史消息记录

### 📡 MQTT通信

**默认主题设计:**
- `xj1core/student/message` - ESP32发送消息
- `xj1cloud/teacher/message` - 云端发送消息
- `xj1core/heartbeat` - 设备心跳
- `xj1core/status` - 设备状态

## 🔗 HTTP API接口

### 消息发布接口
```http
POST http://192.168.1.40:5000/api/mqtt/publish
Content-Type: application/json

{
    "message": "消息内容",
    "source": "消息来源标识",
    "topic": "自定义主题(可选)"
}
```

### ESP32使用示例
```cpp
#include <HTTPClient.h>
#include <ArduinoJson.h>

void sendMQTTMessage(String message) {
    HTTPClient http;
    http.begin("http://192.168.1.40:5000/api/mqtt/publish");
    http.addHeader("Content-Type", "application/json");
    
    DynamicJsonDocument doc(1024);
    doc["message"] = message;
    doc["source"] = "xj1core_esp32";
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    int httpCode = http.POST(jsonString);
    
    if(httpCode == 200) {
        Serial.println("MQTT消息发送成功!");
    }
    
    http.end();
}
```

## ⚙️ 配置说明

### ESP32配置 (xj1core/config.ini)
```ini
[wifi]
ssid = YourWiFiName
password = YourWiFiPassword
ap_ssid = XJ1Core-Setup
ap_password = xj1core2025

[mqtt]
broker_host = 192.168.1.40
broker_port = 1883
client_id = xj1core_student
publish_topic = xj1core/student/message
subscribe_topic = xj1cloud/teacher/message

[web]
port = 80
username = admin
password = xj1core2025
```

### 云端配置 (xj1corecloud/config.ini)
```ini
[mqtt]
broker_host = 192.168.1.40
broker_port = 1883
publish_topic = xj1cloud/teacher/message
subscribe_topic = xj1core/student/message

[web]
host = 0.0.0.0
port = 5000
debug = false
secret_key = xj1core_web_secret_key_2025
```

## 🎨 Web界面特性

### 🖥️ ESP32 Web配置界面
- **现代化设计**: 响应式布局，支持移动设备
- **实时状态**: 显示网络连接、MQTT状态等
- **参数配置**: WiFi、MQTT、蓝牙、以太网配置
- **安全认证**: 登录保护，防止未授权访问

### 🌐 云端Web管理界面
- **实时消息**: WebSocket实时推送，无需刷新
- **消息发送**: 支持快捷消息和自定义消息
- **历史记录**: 自动保存消息历史，支持查询
- **状态监控**: 实时显示系统运行状态
- **移动适配**: 完美支持手机和平板访问

## 🛠️ 开发指南

### 📝 添加新功能

1. **ESP32端添加传感器**:
```c
// 在main/sensor_manager.c中添加
float read_temperature() {
    // 读取温度传感器
    return temperature_value;
}

// 在MQTT消息中包含传感器数据
void publish_sensor_data() {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "temperature", read_temperature());
    // ... 发送MQTT消息
}
```

2. **云端添加新API**:
```python
@app.route('/api/sensors/temperature')
def get_temperature():
    # 获取最新温度数据
    return jsonify({'temperature': latest_temperature})
```

### 🔧 自定义配置

修改 `config.ini` 文件来适配你的环境:
- 更改WiFi网络信息
- 配置MQTT Broker地址
- 调整Web服务端口
- 设置认证密码

## 🐛 故障排除

### 常见问题

1. **ESP32无法连接WiFi**
   - 检查SSID和密码是否正确
   - 确认WiFi信号强度足够
   - 尝试重启ESP32

2. **MQTT连接失败**
   - 检查Broker地址和端口
   - 确认网络连通性
   - 查看防火墙设置

3. **Web界面无法访问**
   - 检查ESP32是否正常启动
   - 确认IP地址是否正确
   - 尝试清除浏览器缓存

详细的故障排除指南请参考 [TROUBLESHOOTING.md](TROUBLESHOOTING.md)

## 📚 学习资源

### 🎓 教学内容
1. **物联网基础概念**
2. **ESP32开发入门**
3. **MQTT协议详解**
4. **Web前后端开发**
5. **系统集成与部署**

### 📖 推荐阅读
- [ESP-IDF编程指南](https://docs.espressif.com/projects/esp-idf/)
- [MQTT协议规范](https://mqtt.org/mqtt-specification/)
- [Flask Web开发](https://flask.palletsprojects.com/)

## 🤝 贡献指南

我们欢迎所有形式的贡献！

### 如何贡献
1. Fork 这个项目
2. 创建你的特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交你的更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开一个 Pull Request

### 贡献类型
- 🐛 Bug修复
- ✨ 新功能开发
- 📝 文档改进
- 🎨 界面优化
- 🔧 性能优化

## 📄 开源协议

本项目采用 MIT 协议开源。详情请见 [LICENSE](LICENSE) 文件。

## 🌟 致谢

### 特别感谢
- **唐家乾老师** - 项目指导和技术支持
- **ESP32社区** - 提供了丰富的开发资源
- **开源社区** - 为本项目提供了基础技术支持

### 技术栈
- **硬件平台**: ESP32/ESP32-S3
- **开发框架**: ESP-IDF
- **通信协议**: MQTT, HTTP, WebSocket
- **前端技术**: HTML5, CSS3, JavaScript, Bootstrap
- **后端技术**: Python, Flask, Flask-SocketIO
- **数据库**: SQLite (可扩展)
- **消息队列**: Mosquitto MQTT Broker

## 📞 联系我们

- **作者**: ironxiao
- **邮箱**: [your-email@example.com]
- **项目主页**: [GitHub仓库链接]
- **问题反馈**: [Issues页面链接]

---

<div align="center">

**🎯 愿此代码成为你们探索物联网世界的起点！**

*让我们一起构建更美好的智能世界* 🌍

</div>
