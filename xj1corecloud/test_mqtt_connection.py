#!/usr/bin/env python3
"""
MQTT连接测试脚本
模拟ESP32连接MQTT服务器，用于诊断连接问题
"""

import paho.mqtt.client as mqtt
import time
import sys

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ MQTT连接成功!")
        print(f"连接标志: {flags}")
        # 订阅测试主题
        client.subscribe("xj1cloud/teacher/message")
        client.subscribe("xj1core/+")
        print("📥 已订阅测试主题")
        
        # 发送测试消息
        client.publish("xj1core/student/message", "测试消息：学生设备连接成功")
        client.publish("xj1core/status", "online")
        print("📤 已发送测试消息")
    else:
        print(f"❌ MQTT连接失败，错误代码: {rc}")
        error_messages = {
            1: "协议版本不正确",
            2: "客户端ID无效", 
            3: "服务器不可用",
            4: "用户名或密码错误",
            5: "未授权"
        }
        if rc in error_messages:
            print(f"错误原因: {error_messages[rc]}")

def on_disconnect(client, userdata, rc):
    if rc != 0:
        print(f"❌ MQTT意外断开连接，错误代码: {rc}")
    else:
        print("✅ MQTT正常断开连接")

def on_message(client, userdata, msg):
    print(f"📨 收到消息 - 主题: {msg.topic}, 内容: {msg.payload.decode()}")

def on_publish(client, userdata, mid):
    print(f"📤 消息发布成功，消息ID: {mid}")

def on_subscribe(client, userdata, mid, granted_qos):
    print(f"📥 订阅成功，消息ID: {mid}, QoS: {granted_qos}")

def test_mqtt_connection(broker_host, broker_port, client_id):
    print("🚀 开始MQTT连接测试...")
    print(f"服务器: {broker_host}:{broker_port}")
    print(f"客户端ID: {client_id}")
    print("-" * 50)
    
    # 创建MQTT客户端
    client = mqtt.Client(client_id)
    
    # 设置回调函数
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    client.on_publish = on_publish
    client.on_subscribe = on_subscribe
    
    # 设置keepalive
    client.keepalive = 60
    
    try:
        print(f"🔌 尝试连接到 {broker_host}:{broker_port}...")
        client.connect(broker_host, broker_port, 60)
        
        # 启动网络循环
        client.loop_start()
        
        # 保持连接30秒进行测试
        print("⏱️  保持连接30秒进行测试...")
        for i in range(30):
            time.sleep(1)
            if i % 5 == 0:
                # 每5秒发送一次心跳消息
                client.publish("xj1core/heartbeat", f'{{"timestamp": {time.time()}, "test_count": {i//5}}}')
        
        print("✅ 测试完成，断开连接...")
        client.loop_stop()
        client.disconnect()
        
    except Exception as e:
        print(f"❌ 连接异常: {e}")
        return False
    
    return True

if __name__ == "__main__":
    # 使用与ESP32相同的配置
    BROKER_HOST = "192.168.1.40"
    BROKER_PORT = 1883
    CLIENT_ID = "xj1core-test-client"
    
    print("=" * 60)
    print("🔧 湘江一号 MQTT连接测试工具")
    print("=" * 60)
    
    success = test_mqtt_connection(BROKER_HOST, BROKER_PORT, CLIENT_ID)
    
    if success:
        print("\n✅ MQTT连接测试成功！ESP32应该能够正常连接")
    else:
        print("\n❌ MQTT连接测试失败，请检查:")
        print("1. MQTT服务器是否在192.168.1.40:1883运行")
        print("2. 防火墙是否阻止了连接")
        print("3. 网络配置是否正确")
    
    print("=" * 60)
