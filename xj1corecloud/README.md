# 湘江一号云端通信系统 (XJ1CoreCloud)

## 💖 项目简介

这是湘江一号云端通信系统，使用Python和MQTT协议实现数据的接收和发送功能。系统每5秒定期发送"唐老师：您好吗？"消息，同时监听接收来自其他设备的消息。

**特别致谢**：谨以此项目感谢恩师唐家乾老师 🎓

## 🏗️ 系统架构

```
ESP32 (学生端)          MQTT Broker (Docker)          Python (老师端)
     |                        |                           |
     |-- 每5秒发送 ----------->|<---------- 每5秒发送 -----|
     |   "唐老师：学生想你"      |           "唐老师：您好吗？"  |
     |                        |                           |
     |<-- 接收老师消息 ---------|------------ 接收学生消息 -->|
```

## 📡 MQTT主题设计

| 主题 | 发送方 | 接收方 | 内容 |
|------|--------|--------|------|
| `xj1core/student/message` | ESP32 | Python | 学生思念消息 |
| `xj1cloud/teacher/message` | Python | ESP32 | 老师问候消息 |
| `xj1core/heartbeat` | ESP32 | Python | 设备心跳状态 |
| `xj1core/status` | ESP32 | Python | 设备状态信息 |

## 🚀 快速启动

### 1. 安装依赖

```bash
cd xj1corecloud

# 安装Python依赖
pip install -r requirements.txt

# 安装mosquitto MQTT Broker (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install mosquitto mosquitto-clients

# 或者使用提供的安装脚本
./install_mosquitto.sh
```

### 2. 启动系统

```bash
# 方法1: 使用启动脚本（推荐）
./start.sh

# 方法2: 手动启动
# 先启动MQTT Broker
mosquitto -c mosquitto.conf -d
# 再启动Python应用
python3 xj1cloud.py
```

### 3. 烧录并运行ESP32学生端

```bash
cd xj1core

# 配置MQTT Broker地址（通过Web界面 http://192.168.5.1）
# 或者修改 config.ini 文件

# 烧录固件
idf.py flash monitor
```

## 📝 消息格式

### 发送消息格式：
```json
{
  "timestamp": "2025-09-28 19:00:00",
  "message": "唐老师：您好吗？",
  "source": "xj1cloud"
}
```

### 接收消息格式：
系统会接收并显示任何发送到订阅主题的消息。

## 🔧 配置说明

系统配置通过 `config.ini` 文件进行管理：

```ini
[mqtt]
# MQTT Broker配置
broker_host = localhost
broker_port = 1883
keepalive = 60

# MQTT主题配置
publish_topic = xj1core/data/send
subscribe_topic = xj1core/data/receive

[message]
# 消息内容配置
default_message = 唐老师：您好吗？
send_interval = 5

[logging]
# 日志配置
log_level = INFO
log_format = %(asctime)s - %(levelname)s - %(message)s
```

您可以修改配置文件来自定义：
- MQTT Broker地址和端口
- 发送和接收主题
- 消息内容和发送周期
- 日志级别和格式

## 📊 运行效果

### 系统启动日志：
```
=== 湘江一号云端通信系统启动 ===
2025-09-28 19:00:00 - INFO - 正在连接到MQTT Broker: localhost:1883
2025-09-28 19:00:00 - INFO - 成功连接到MQTT Broker: localhost:1883
2025-09-28 19:00:00 - INFO - 已订阅主题: xj1core/data/receive
2025-09-28 19:00:00 - INFO - 启动定期发送，周期: 5秒
2025-09-28 19:00:00 - INFO - 系统运行中，按Ctrl+C停止...
```

### 消息发送日志：
```
2025-09-28 19:00:05 - INFO - [2025-09-28 19:00:05] 发送消息成功 - 主题: xj1core/data/send, 内容: 唐老师：您好吗？
2025-09-28 19:00:10 - INFO - [2025-09-28 19:00:10] 发送消息成功 - 主题: xj1core/data/send, 内容: 唐老师：您好吗？
```

### 消息接收日志：
```
2025-09-28 19:00:15 - INFO - [2025-09-28 19:00:15] 接收到消息 - 主题: xj1core/data/receive, 内容: 来自其他设备的消息
```

## 🎯 功能特点

- ✅ **MQTT通信**: 基于MQTT协议的可靠消息传递
- ✅ **定时发送**: 每5秒自动发送"唐老师：您好吗？"消息
- ✅ **实时接收**: 监听并显示接收到的消息
- ✅ **JSON格式**: 结构化消息，包含时间戳和来源信息
- ✅ **配置管理**: 通过INI文件灵活配置系统参数
- ✅ **日志记录**: 完整的通信和系统运行日志
- ✅ **断线重连**: MQTT客户端自动重连机制
- ✅ **多线程**: 发送和接收功能独立运行

## 📁 项目文件结构

```
xj1corecloud/
├── xj1cloud.py          # 主应用程序
├── config_manager.py    # 配置管理模块
├── config.ini          # 系统配置文件
├── start.py            # Python启动脚本
├── start.sh            # Shell启动脚本
├── install_mosquitto.sh # Mosquitto安装脚本
├── mosquitto.conf      # MQTT Broker配置
├── requirements.txt    # Python依赖包
└── README.md           # 项目说明文档
```

## 💡 扩展功能

- 📱 Web界面显示实时消息
- 📊 消息统计和可视化
- 🔔 消息提醒和通知
- 💾 消息持久化存储
- 🔐 认证和加密通信
- 🌐 多设备支持和管理

这个系统展示了MQTT通信技术的实际应用，传递了温暖的师生情谊 ❤️
