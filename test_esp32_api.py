#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ESP32 API测试工具
用于测试ESP32的Web API接口
"""

import requests
import json
import sys
from requests.auth import HTTPBasicAuth

class ESP32APITester:
    def __init__(self, esp32_ip="192.168.5.1"):
        self.base_url = f"http://{esp32_ip}"
        self.session = requests.Session()
        self.session.timeout = 10
        
    def test_connection(self):
        """测试基本连接"""
        print(f"🔗 测试连接到 {self.base_url}")
        try:
            response = self.session.get(f"{self.base_url}/")
            print(f"✅ 连接成功 - 状态码: {response.status_code}")
            return True
        except Exception as e:
            print(f"❌ 连接失败: {e}")
            return False
            
    def login(self, username="admin", password="123456"):
        """登录获取会话"""
        print(f"🔑 尝试登录 - 用户名: {username}")
        try:
            login_data = {
                "username": username,
                "password": password
            }
            
            response = self.session.post(
                f"{self.base_url}/api/login",
                json=login_data,
                headers={'Content-Type': 'application/json'}
            )
            
            print(f"登录响应状态码: {response.status_code}")
            if response.status_code == 200:
                result = response.json()
                print(f"✅ 登录成功: {result}")
                return True
            else:
                print(f"❌ 登录失败: {response.text}")
                return False
                
        except Exception as e:
            print(f"❌ 登录请求失败: {e}")
            return False
            
    def test_status_api(self):
        """测试状态API"""
        print("📊 测试状态API")
        try:
            response = self.session.get(f"{self.base_url}/api/status")
            print(f"状态API响应码: {response.status_code}")
            if response.status_code == 200:
                result = response.json()
                print(f"✅ 状态API成功: {json.dumps(result, indent=2, ensure_ascii=False)}")
                return True
            else:
                print(f"❌ 状态API失败: {response.text}")
                return False
        except Exception as e:
            print(f"❌ 状态API请求失败: {e}")
            return False
            
    def test_mqtt_config_api(self):
        """测试MQTT配置API"""
        print("📡 测试MQTT配置API")
        try:
            mqtt_config = {
                "broker_host": "192.168.1.40",
                "broker_port": 1883,
                "client_id": "xj1core_test",
                "default_topic": "test/topic",
                "keepalive": 60
            }
            
            print(f"发送MQTT配置: {json.dumps(mqtt_config, indent=2)}")
            
            response = self.session.post(
                f"{self.base_url}/api/config/mqtt",
                json=mqtt_config,
                headers={'Content-Type': 'application/json'}
            )
            
            print(f"MQTT配置API响应码: {response.status_code}")
            print(f"响应内容: {response.text}")
            
            if response.status_code == 200:
                result = response.json()
                print(f"✅ MQTT配置成功: {result}")
                return True
            elif response.status_code == 401:
                print("❌ MQTT配置失败: 需要认证")
                return False
            elif response.status_code == 404:
                print("❌ MQTT配置失败: API接口不存在")
                return False
            else:
                print(f"❌ MQTT配置失败: {response.text}")
                return False
                
        except Exception as e:
            print(f"❌ MQTT配置请求失败: {e}")
            return False
            
    def test_all_apis(self):
        """测试所有API"""
        print("🚀 开始API测试")
        print("=" * 50)
        
        # 1. 测试基本连接
        if not self.test_connection():
            print("❌ 基本连接失败，无法继续测试")
            return False
            
        print("\n" + "=" * 50)
        
        # 2. 测试登录
        if not self.login():
            print("❌ 登录失败，尝试测试无需认证的API")
            
        print("\n" + "=" * 50)
        
        # 3. 测试状态API
        self.test_status_api()
        
        print("\n" + "=" * 50)
        
        # 4. 测试MQTT配置API
        self.test_mqtt_config_api()
        
        print("\n" + "=" * 50)
        print("🎯 测试完成")
        
    def debug_request(self, url, method="GET", data=None):
        """调试特定请求"""
        print(f"🔍 调试请求: {method} {url}")
        try:
            if method.upper() == "GET":
                response = self.session.get(url)
            elif method.upper() == "POST":
                response = self.session.post(url, json=data, headers={'Content-Type': 'application/json'})
            
            print(f"状态码: {response.status_code}")
            print(f"响应头: {dict(response.headers)}")
            print(f"响应内容: {response.text}")
            
        except Exception as e:
            print(f"请求失败: {e}")

def main():
    if len(sys.argv) > 1:
        esp32_ip = sys.argv[1]
    else:
        esp32_ip = "192.168.5.1"  # 默认AP模式IP
        
    print(f"ESP32 API 测试工具")
    print(f"目标设备: {esp32_ip}")
    print("=" * 50)
    
    tester = ESP32APITester(esp32_ip)
    
    # 运行所有测试
    tester.test_all_apis()
    
    # 额外的调试测试
    print("\n" + "=" * 50)
    print("🔍 额外调试测试")
    
    # 直接测试MQTT API（无认证）
    tester.debug_request(f"http://{esp32_ip}/api/config/mqtt", "POST", {
        "broker_host": "test.mosquitto.org",
        "broker_port": 1883,
        "client_id": "debug_test",
        "default_topic": "debug/topic",
        "keepalive": 60
    })

if __name__ == "__main__":
    main()
