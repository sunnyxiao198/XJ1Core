#!/bin/bash
# 湘江一号云端Web通信系统启动脚本

echo "=== 湘江一号云端Web通信系统 ==="
echo "启动时间: $(date)"

# 检查虚拟环境
if [ -d "venv" ]; then
    echo "激活Python虚拟环境..."
    source venv/bin/activate
else
    echo "警告: 未找到虚拟环境目录 'venv'"
    echo "请先创建并安装依赖:"
    echo "  python3 -m venv venv"
    echo "  source venv/bin/activate"
    echo "  pip install -r requirements.txt"
    exit 1
fi

# 检查依赖
echo "检查Python依赖..."
python3 -c "import flask, flask_socketio, paho.mqtt.client" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "安装缺失的依赖..."
    pip install -r requirements.txt
fi

# 启动MQTT Broker（如果需要）
echo "检查MQTT Broker状态..."
if ! pgrep -f "mosquitto" > /dev/null; then
    echo "启动MQTT Broker..."
    if [ -f "mosquitto.conf" ]; then
        mosquitto -c mosquitto.conf -d
        sleep 2
    else
        echo "警告: 未找到mosquitto.conf文件，使用默认配置启动"
        mosquitto -d
        sleep 2
    fi
fi

# 启动Web应用
echo "启动Web应用服务器..."
echo "Web界面地址: http://localhost:5000"
echo "按 Ctrl+C 停止服务"
echo "=========================="

python3 start_web.py
