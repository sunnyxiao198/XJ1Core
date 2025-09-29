# 湘江一号云端Web通信系统 (XJ1CoreCloud Web)

## 🌟 项目概述

这是湘江一号云端Web通信系统，基于Flask和WebSocket技术实现的MQTT消息可视化界面。用户可以通过Web浏览器实时查看MQTT消息，并发送消息到MQTT发布端。

## ✨ 主要功能

### 🎯 核心功能
- **📡 MQTT订阅端** - 接收并显示来自发布端的消息
- **📤 消息发送** - 通过Web界面发送消息到MQTT发布端
- **⚡ 实时通信** - 基于WebSocket的实时消息推送
- **📊 状态监控** - 实时显示MQTT连接状态和系统信息

### 🎨 界面特性
- **🎨 现代化UI** - 基于Bootstrap 5的响应式设计
- **📱 移动适配** - 支持手机、平板等移动设备
- **🌙 实时更新** - 消息自动刷新，无需手动刷新页面
- **🎯 快捷操作** - 预设快捷消息，一键发送常用内容

### 🔧 配置管理
- **📁 INI配置** - 通过`config.ini`文件管理所有配置
- **🔄 热加载** - 支持配置文件动态加载
- **🛠️ 灵活配置** - MQTT、Web服务器、消息等独立配置

## 🏗️ 技术架构

```
┌─────────────────────┐    ┌─────────────────────┐    ┌─────────────────────┐
│     Web前端         │    │    Flask后端        │    │   MQTT Broker       │
│                     │    │                     │    │                     │
│ • HTML5/CSS3/JS     │◄──►│ • Flask Web框架     │◄──►│ • Mosquitto         │
│ • Bootstrap 5       │    │ • Flask-SocketIO    │    │ • 主题路由          │
│ • WebSocket客户端   │    │ • MQTT客户端        │    │ • 消息分发          │
│ • 实时消息显示      │    │ • 配置管理          │    │                     │
│ • 交互式发送界面    │    │ • 日志记录          │    │                     │
└─────────────────────┘    └─────────────────────┘    └─────────────────────┘
```

## 📦 文件结构

```
xj1corecloud/
├── web_app.py              # Flask Web应用主程序
├── start_web.py            # Python启动脚本
├── start_web.sh            # Shell启动脚本
├── config.ini              # 系统配置文件（包含Web配置）
├── config_manager.py       # 配置管理器（已扩展Web配置）
├── requirements.txt        # Python依赖（已添加Web框架）
├── templates/              # HTML模板目录
│   └── index.html          # 主页面模板
├── static/                 # 静态资源目录
│   ├── css/
│   │   └── style.css       # 自定义样式文件
│   └── js/
│       └── app.js          # 前端JavaScript应用
└── README_WEB.md           # Web系统说明文档
```

## 🚀 快速开始

### 1. 环境准备

```bash
cd xj1corecloud

# 激活虚拟环境
source venv/bin/activate

# 安装新增的Web依赖
pip install -r requirements.txt
```

### 2. 配置文件

`config.ini`文件已自动更新，新增了Web服务器配置：

```ini
[web]
# Web服务器配置
host = 0.0.0.0
port = 5000
debug = false
secret_key = xj1core_web_secret_key_2025
```

### 3. 启动系统

#### 方法1: 使用Shell脚本（推荐）
```bash
./start_web.sh
```

#### 方法2: 使用Python脚本
```bash
python3 start_web.py
```

#### 方法3: 直接运行
```bash
python3 web_app.py
```

### 4. 访问Web界面

启动成功后，打开浏览器访问：
- **本地访问**: http://localhost:5000
- **局域网访问**: http://你的IP地址:5000

## 🎮 使用说明

### 📊 主界面布局

1. **左侧控制面板**
   - 系统状态显示（连接状态、消息计数、运行时间）
   - MQTT配置信息（Broker地址、主题配置）
   - 消息发送区域（文本输入框、字符计数）
   - 快捷消息按钮（预设常用消息）

2. **右侧消息区域**
   - 实时消息列表（发送/接收消息）
   - 消息操作按钮（清空、自动滚动切换）
   - 消息详情显示（时间戳、主题、内容）

### 💬 消息操作

#### 发送消息
1. 在左侧输入框中输入消息内容
2. 点击"发送消息"按钮或按Enter键
3. 消息将通过HTTP API调用Web服务器的MQTT客户端发布到配置的主题

#### 消息发送流程
```
Web界面 → HTTP POST → Flask服务器 → MQTT客户端 → MQTT Broker → 订阅端
```

**注意**: 消息发送不再使用WebSocket，而是通过HTTP API直接调用服务器的MQTT发布功能。

#### 快捷消息
- 点击快捷消息按钮，自动填入预设内容
- 支持动态内容（如测试消息会自动添加时间戳）

#### 消息历史
- 自动保存最近100条消息记录
- 页面刷新后会加载最近20条历史消息
- 支持手动清空消息记录

### ⚙️ 界面设置

#### 自动滚动
- 默认开启自动滚动到最新消息
- 点击"自动滚动"按钮可切换开关状态
- 手动滚动时会临时暂停自动滚动

#### 连接状态
- 实时显示WebSocket和MQTT连接状态
- 连接正常：绿色"已连接"
- 连接断开：红色"未连接"
- 连接中：黄色"连接中"

## 🔧 配置详解

### Web服务器配置 `[web]`
```ini
host = 0.0.0.0          # 服务器绑定地址（0.0.0.0允许外部访问）
port = 5000             # Web服务器端口
debug = false           # 调试模式（生产环境建议false）
secret_key = xxx        # Flask会话密钥（安全相关）
```

### MQTT配置 `[mqtt]`
```ini
broker_host = 192.168.1.40              # MQTT Broker地址
broker_port = 1883                      # MQTT Broker端口
publish_topic = xj1cloud/teacher/message   # 发送消息主题
subscribe_topic = xj1core/student/message  # 接收消息主题
```

## 📱 移动端适配

Web界面已完全适配移动设备：

- **响应式布局** - 自动适应不同屏幕尺寸
- **触摸优化** - 按钮和输入框适合触摸操作
- **移动端菜单** - 紧凑的移动端界面布局
- **手势支持** - 支持滑动和触摸手势

## 🎨 界面特性

### 消息类型区分
- **发送消息** - 蓝色边框，向上箭头图标
- **接收消息** - 紫色边框，向下箭头图标
- **消息高亮** - 新消息3秒高亮显示

### 状态指示
- **连接指示器** - 彩色圆点显示连接状态
- **实时计时器** - 显示连接持续时间
- **消息计数器** - 实时更新消息总数

### 交互反馈
- **Toast通知** - 操作结果弹窗提示
- **按钮动画** - 悬停和点击动画效果
- **加载状态** - 发送时显示加载动画

## 🔍 故障排除

### 常见问题

1. **无法访问Web界面**
   - 检查防火墙是否开放5000端口
   - 确认config.ini中host配置正确
   - 查看启动日志是否有错误信息

2. **MQTT连接失败**
   - 检查MQTT Broker是否运行
   - 确认broker_host和broker_port配置
   - 查看网络连接是否正常

3. **WebSocket连接断开**
   - 检查网络稳定性
   - 确认浏览器支持WebSocket
   - 查看控制台是否有JavaScript错误

### 日志查看

启动时的日志信息会显示在终端中，包括：
- MQTT连接状态
- Web服务器启动信息
- 消息发送/接收日志
- 错误和警告信息

## 🚀 扩展功能

### 已实现的高级功能
- ✅ **实时WebSocket通信**
- ✅ **消息历史记录**
- ✅ **响应式Web界面**
- ✅ **移动端适配**
- ✅ **状态监控面板**
- ✅ **快捷消息功能**

## 🔗 HTTP API 接口

### 消息发布接口
```
POST /api/mqtt/publish
Content-Type: application/json

{
    "message": "消息内容",
    "source": "消息来源标识",
    "topic": "自定义主题(可选)"
}
```

**响应示例:**
```json
{
    "status": "success",
    "message": "消息发布成功",
    "data": {
        "content": "消息内容",
        "topic": "xj1cloud/teacher/message",
        "timestamp": "2025-09-29 15:30:00",
        "source": "web_interface"
    },
    "code": 200
}
```

### ESP32使用示例
```cpp
// ESP32 Arduino代码
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
        String response = http.getString();
        Serial.println("MQTT消息发送成功: " + response);
    } else {
        Serial.println("MQTT消息发送失败: " + String(httpCode));
    }
    
    http.end();
}
```

### 可扩展功能
- 🔄 **用户认证系统**
- 📊 **消息统计图表**
- 🔔 **消息通知提醒**
- 💾 **消息持久化存储**
- 🎨 **主题切换功能**
- 📁 **文件传输支持**

## 💡 技术亮点

1. **前后端分离** - Flask后端API + JavaScript前端
2. **实时通信** - WebSocket双向通信
3. **配置驱动** - INI文件统一配置管理
4. **模块化设计** - 清晰的代码结构和职责分离
5. **现代化UI** - Bootstrap 5 + 自定义CSS样式
6. **移动优先** - 响应式设计，移动端体验优秀

## 🎉 总结

湘江一号云端Web通信系统成功实现了：

✅ **完整的MQTT Web界面** - 可视化消息收发  
✅ **实时双向通信** - WebSocket + MQTT集成  
✅ **现代化用户体验** - 响应式设计，移动适配  
✅ **灵活的配置管理** - INI文件统一配置  
✅ **完善的功能特性** - 消息历史、状态监控、快捷操作  

系统已完全满足需求，可以通过Web界面实时接收和发送MQTT消息，配置完全来自`config.ini`文件，实现了真正的可视化MQTT通信管理。
