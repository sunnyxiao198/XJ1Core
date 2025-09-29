#!/bin/bash

# 湘江一号 - MQTT服务器启动脚本 (AP网络模式)
# 在192.168.5.1上运行MQTT服务器，供ESP32 AP网络访问

echo "🚀 启动湘江一号MQTT服务器 (AP网络模式)..."
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

# 检查是否有192.168.5.x网络接口
if ! ip addr show | grep -q "192.168.5."; then
    echo "⚠️  警告: 系统未配置192.168.5.x网段IP地址"
    echo "请确保连接到ESP32的AP网络(Sparkriver-AP-01)，或配置相应网络接口"
    echo ""
    echo "当前网络接口IP地址："
    ip addr show | grep "inet " | grep -v "127.0.0.1"
    echo ""
    echo "如果您已连接到ESP32 AP，继续启动MQTT服务器..."
    echo ""
fi

# 创建AP网络专用配置文件
cat > mosquitto_ap_network.conf << EOF
# 湘江一号 - MQTT Broker 配置 (AP网络模式)
# 为ESP32 AP网络内的设备提供MQTT服务

# 监听所有接口（这样ESP32 AP网络内的设备都能连接）
listener 1883
listener 9001
protocol websockets

# 允许匿名连接
allow_anonymous true

# 日志配置
log_dest stdout
log_type all

# 持久化配置
persistence true
persistence_location ./mosquitto_data_ap/

# 连接配置
max_connections 100
keepalive_interval 60

# 消息配置
max_queued_messages 1000
message_size_limit 1048576

# 允许所有客户端连接
allow_anonymous true
EOF

# 创建数据目录
mkdir -p mosquitto_data_ap

echo "📡 MQTT服务器配置信息："
echo "  监听模式: 所有接口"
echo "  监听端口: 1883 (MQTT), 9001 (WebSocket)"
echo "  允许匿名连接: 是"
echo "  数据目录: ./mosquitto_data_ap"
echo ""

echo "💡 ESP32连接信息："
echo "  连接方式1: 通过AP网络连接"
echo "    - 连接ESP32 AP: Sparkriver-AP-01"
echo "    - MQTT服务器: 192.168.5.1:1883"
echo "  连接方式2: 通过STA网络连接"
echo "    - 确保PC和ESP32在同一网络"
echo "    - MQTT服务器: <PC的IP>:1883"
echo ""

echo "🎯 主要通信主题："
echo "  学生->老师: xj1core/student/message"
echo "  老师->学生: xj1cloud/teacher/message"
echo "  心跳消息: xj1core/heartbeat"
echo "  状态消息: xj1core/status"
echo ""

echo "🔧 测试命令（请根据网络情况选择IP）:"
echo "  本机测试: mosquitto_sub -h localhost -t '#'"
echo "  AP网络测试: mosquitto_sub -h 192.168.5.1 -t '#'"
echo "  发送测试消息: mosquitto_pub -h localhost -t 'xj1cloud/teacher/message' -m '老师的测试消息'"
echo ""

# 启动MQTT服务器
echo "✅ 启动MQTT服务器（监听所有接口）..."
echo "按 Ctrl+C 停止服务器"
echo "================================"

# 使用AP网络配置启动mosquitto
mosquitto -c mosquitto_ap_network.conf -v
