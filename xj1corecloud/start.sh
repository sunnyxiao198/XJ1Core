#!/bin/bash
# 湘江一号云端通信系统启动脚本

echo "=== 湘江一号云端通信系统 ==="
echo "正在启动MQTT Broker..."

# 启动mosquitto MQTT Broker（后台运行）
mosquitto -c mosquitto.conf -d

# 等待MQTT Broker启动
sleep 2

echo "正在启动Python应用程序..."

# 激活虚拟环境（如果存在）
if [ -d "venv" ]; then
    source venv/bin/activate
    echo "已激活虚拟环境"
fi

# 启动Python应用程序
python3 start.py
