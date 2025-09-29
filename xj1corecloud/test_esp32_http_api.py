#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ESP32 HTTP API测试脚本
模拟ESP32通过HTTP请求发送MQTT消息
"""

import requests
import json
import time
from datetime import datetime


class ESP32HTTPClient:
    """ESP32 HTTP客户端模拟器"""
    
    def __init__(self, server_url="http://192.168.1.40:5000"):
        """
        初始化HTTP客户端
        
        Args:
            server_url (str): Web服务器地址
        """
        self.server_url = server_url
        self.api_url = f"{server_url}/api/mqtt/publish"
        self.session = requests.Session()
        
        # 设置请求头
        self.session.headers.update({
            'User-Agent': 'XJ1Core-ESP32/1.0',
            'Content-Type': 'application/json'
        })
        
    def send_message(self, message, source="xj1core_esp32", topic=None):
        """
        发送消息到Web服务器
        
        Args:
            message (str): 要发送的消息内容
            source (str): 消息来源标识
            topic (str): 自定义MQTT主题（可选）
            
        Returns:
            dict: 服务器响应结果
        """
        try:
            # 构造请求数据
            data = {
                'message': message,
                'source': source
            }
            
            if topic:
                data['topic'] = topic
                
            print(f"[{datetime.now().strftime('%H:%M:%S')}] 发送HTTP请求...")
            print(f"URL: {self.api_url}")
            print(f"数据: {json.dumps(data, ensure_ascii=False)}")
            
            # 发送POST请求
            response = self.session.post(
                self.api_url, 
                json=data,
                timeout=10
            )
            
            # 解析响应
            result = response.json()
            
            print(f"[{datetime.now().strftime('%H:%M:%S')}] HTTP响应:")
            print(f"状态码: {response.status_code}")
            print(f"响应: {json.dumps(result, ensure_ascii=False, indent=2)}")
            
            return {
                'success': response.status_code == 200,
                'status_code': response.status_code,
                'data': result
            }
            
        except requests.exceptions.ConnectionError:
            error_msg = f"连接失败: 无法连接到 {self.server_url}"
            print(f"❌ {error_msg}")
            return {'success': False, 'error': error_msg}
            
        except requests.exceptions.Timeout:
            error_msg = "请求超时"
            print(f"❌ {error_msg}")
            return {'success': False, 'error': error_msg}
            
        except Exception as e:
            error_msg = f"请求失败: {str(e)}"
            print(f"❌ {error_msg}")
            return {'success': False, 'error': error_msg}
            
    def send_form_data(self, message, source="xj1core_esp32"):
        """
        使用表单数据格式发送消息（适合ESP32的简单HTTP库）
        
        Args:
            message (str): 要发送的消息内容
            source (str): 消息来源标识
            
        Returns:
            dict: 服务器响应结果
        """
        try:
            # 构造表单数据
            data = {
                'message': message,
                'source': source
            }
            
            print(f"[{datetime.now().strftime('%H:%M:%S')}] 发送表单HTTP请求...")
            print(f"URL: {self.api_url}")
            print(f"表单数据: {data}")
            
            # 发送POST请求（表单格式）
            response = self.session.post(
                self.api_url,
                data=data,  # 使用data而不是json
                headers={'Content-Type': 'application/x-www-form-urlencoded'},
                timeout=10
            )
            
            # 解析响应
            result = response.json()
            
            print(f"[{datetime.now().strftime('%H:%M:%S')}] HTTP响应:")
            print(f"状态码: {response.status_code}")
            print(f"响应: {json.dumps(result, ensure_ascii=False, indent=2)}")
            
            return {
                'success': response.status_code == 200,
                'status_code': response.status_code,
                'data': result
            }
            
        except Exception as e:
            error_msg = f"表单请求失败: {str(e)}"
            print(f"❌ {error_msg}")
            return {'success': False, 'error': error_msg}
            
    def test_server_status(self):
        """测试服务器状态"""
        try:
            status_url = f"{self.server_url}/api/status"
            response = self.session.get(status_url, timeout=5)
            
            if response.status_code == 200:
                status = response.json()
                print("✅ 服务器状态正常:")
                print(f"   MQTT连接: {'✅' if status.get('mqtt_connected') else '❌'}")
                print(f"   Broker: {status.get('broker_host')}:{status.get('broker_port')}")
                print(f"   发布主题: {status.get('publish_topic')}")
                print(f"   订阅主题: {status.get('subscribe_topic')}")
                print(f"   消息数量: {status.get('message_count')}")
                return True
            else:
                print(f"❌ 服务器状态异常: {response.status_code}")
                return False
                
        except Exception as e:
            print(f"❌ 无法连接到服务器: {e}")
            return False


def main():
    """主测试函数"""
    print("=== ESP32 HTTP API 测试工具 ===")
    print()
    
    # 创建HTTP客户端
    client = ESP32HTTPClient("http://192.168.1.40:5000")
    
    # 测试服务器连接
    print("1. 测试服务器连接...")
    if not client.test_server_status():
        print("服务器连接失败，请检查Web服务是否启动")
        return
    
    print("\n" + "="*50)
    
    # 测试JSON格式消息发送
    print("2. 测试JSON格式消息发送...")
    result1 = client.send_message(
        message="唐老师：学生想你了！", 
        source="xj1core_esp32"
    )
    
    if result1['success']:
        print("✅ JSON消息发送成功")
    else:
        print("❌ JSON消息发送失败")
    
    time.sleep(2)
    
    print("\n" + "="*50)
    
    # 测试表单格式消息发送
    print("3. 测试表单格式消息发送...")
    result2 = client.send_form_data(
        message=f"ESP32测试消息 - {datetime.now().strftime('%H:%M:%S')}",
        source="xj1core_esp32_form"
    )
    
    if result2['success']:
        print("✅ 表单消息发送成功")
    else:
        print("❌ 表单消息发送失败")
    
    time.sleep(2)
    
    print("\n" + "="*50)
    
    # 测试自定义主题
    print("4. 测试自定义主题消息...")
    result3 = client.send_message(
        message="自定义主题测试消息",
        source="xj1core_esp32",
        topic="xj1core/test/custom"
    )
    
    if result3['success']:
        print("✅ 自定义主题消息发送成功")
    else:
        print("❌ 自定义主题消息发送失败")
    
    print("\n" + "="*50)
    print("测试完成！")
    print("\n💡 ESP32代码示例:")
    print("""
// ESP32 Arduino代码示例
#include <HTTPClient.h>
#include <ArduinoJson.h>

void sendMQTTMessage(String message) {
    HTTPClient http;
    http.begin("http://192.168.1.40:5000/api/mqtt/publish");
    http.addHeader("Content-Type", "application/json");
    
    // 构造JSON数据
    DynamicJsonDocument doc(1024);
    doc["message"] = message;
    doc["source"] = "xj1core_esp32";
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // 发送POST请求
    int httpCode = http.POST(jsonString);
    
    if(httpCode == 200) {
        String response = http.getString();
        Serial.println("消息发送成功: " + response);
    } else {
        Serial.println("消息发送失败: " + String(httpCode));
    }
    
    http.end();
}
    """)


if __name__ == "__main__":
    main()
