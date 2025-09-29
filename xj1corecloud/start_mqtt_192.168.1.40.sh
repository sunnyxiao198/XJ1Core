#!/bin/bash

# 湘江一号 - MQTT服务器启动脚本 (绑定到192.168.1.40)
# 用于为ESP32设备提供MQTT通信服务

echo "🚀 启动湘江一号MQTT服务器 (192.168.1.40:1883)..."
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

# 检查IP地址是否配置
if ! ip addr show | grep -q "192.168.1.40"; then
    echo "⚠️  警告: 系统未配置IP地址 192.168.1.40"
    echo "请确保网络接口配置了此IP地址，或修改config.ini中的broker_host"
    echo ""
    echo "当前网络接口IP地址："
    ip addr show | grep "inet " | grep -v "127.0.0.1"
    echo ""
    echo "继续启动MQTT服务器，但ESP32可能无法连接..."
    echo ""
fi

# 创建专用配置文件
cat > mosquitto_192.168.1.40.conf << EOF
# 湘江一号 - MQTT Broker 配置 (192.168.1.40)
# 为师生通信系统提供可靠的消息传递服务

# 监听指定IP和端口
listener 1883 192.168.1.40
listener 9001 192.168.1.40
protocol websockets

# 允许匿名连接（简化配置，生产环境建议启用认证）
allow_anonymous true

# 日志配置
log_dest stdout
log_type all

# 持久化配置
persistence true
persistence_location ./mosquitto_data/

# 连接配置
max_connections 100
keepalive_interval 60

# 消息配置
max_queued_messages 1000
message_size_limit 1048576
EOF

# 创建数据目录
mkdir -p mosquitto_data

echo "📡 MQTT服务器配置信息："
echo "  监听地址: 192.168.1.40"
echo "  监听端口: 1883 (MQTT), 9001 (WebSocket)"
echo "  允许匿名连接: 是"
echo "  最大连接数: 100"
echo "  数据目录: ./mosquitto_data"
echo ""

echo "💡 ESP32设备连接信息："
echo "  MQTT服务器地址: 192.168.1.40"
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
echo "  订阅所有消息: mosquitto_sub -h 192.168.1.40 -t '#'"
echo "  发送测试消息: mosquitto_pub -h 192.168.1.40 -t 'xj1cloud/teacher/message' -m '老师的测试消息'"
echo ""

# 启动MQTT服务器
echo "✅ 启动MQTT服务器..."
echo "按 Ctrl+C 停止服务器"
echo "================================"

# 使用专用配置文件启动mosquitto
mosquitto -c mosquitto_192.168.1.40.conf -v
