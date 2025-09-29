#!/bin/bash

# 湘江一号 - MQTT服务器启动脚本
# 用于为ESP32设备提供MQTT通信服务

echo "🚀 启动湘江一号MQTT服务器..."
echo "================================"

# 检查mosquitto是否已安装
if ! command -v mosquitto &> /dev/null; then
    echo "❌ Mosquitto MQTT broker 未安装"
    echo "请运行以下命令安装："
    echo "Ubuntu/Debian: sudo apt-get install mosquitto mosquitto-clients"
    echo "CentOS/RHEL: sudo yum install mosquitto mosquitto-clients"
    echo "macOS: brew install mosquitto"
    exit 1
fi

# 检查配置文件
if [ ! -f "mosquitto.conf" ]; then
    echo "❌ 配置文件 mosquitto.conf 不存在"
    exit 1
fi

# 创建数据目录
mkdir -p mosquitto_data

echo "📡 MQTT服务器配置信息："
echo "  监听端口: 1883 (MQTT), 9001 (WebSocket)"
echo "  允许匿名连接: 是"
echo "  最大连接数: 100"
echo "  数据目录: ./mosquitto_data"
echo ""

echo "💡 ESP32设备连接信息："
echo "  MQTT服务器地址: $(hostname -I | awk '{print $1}') 或 localhost"
echo "  端口: 1883"
echo "  客户端ID: xj1core-student-01"
echo ""

echo "🎯 主要通信主题："
echo "  学生->老师: xj1core/student/message"
echo "  老师->学生: xj1cloud/teacher/message"
echo "  心跳消息: xj1core/heartbeat"
echo "  状态消息: xj1core/status"
echo ""

echo "🔧 测试命令："
echo "  订阅所有消息: mosquitto_sub -h localhost -t '#'"
echo "  发送测试消息: mosquitto_pub -h localhost -t 'test' -m 'Hello ESP32'"
echo ""

# 启动MQTT服务器
echo "✅ 启动MQTT服务器..."
echo "按 Ctrl+C 停止服务器"
echo "================================"

# 使用当前目录的配置文件启动mosquitto
mosquitto -c mosquitto.conf -v
