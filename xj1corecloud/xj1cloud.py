#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
湘江一号云端通信系统
实现MQTT数据接收和发送功能
"""

import time
import threading
import logging
from datetime import datetime
import paho.mqtt.client as mqtt
import json
from config_manager import ConfigManager


class XJ1CloudMQTT:
    """湘江一号MQTT通信客户端"""
    
    def __init__(self, config_file="config.ini"):
        """
        初始化MQTT客户端
        
        Args:
            config_file (str): 配置文件路径
        """
        # 加载配置
        self.config_manager = ConfigManager(config_file)
        config = self.config_manager.get_all_config()
        
        # MQTT配置
        mqtt_config = config['mqtt']
        self.broker_host = mqtt_config['broker_host']
        self.broker_port = mqtt_config['broker_port']
        self.keepalive = mqtt_config['keepalive']
        self.publish_topic = mqtt_config['publish_topic']
        self.subscribe_topic = mqtt_config['subscribe_topic']
        
        # 消息配置
        message_config = config['message']
        self.send_interval = message_config['send_interval']
        self.message_content = message_config['default_message']
        
        # 初始化MQTT客户端
        self.client = mqtt.Client()
        self.is_connected = False
        
        # 设置日志
        logging_config = config['logging']
        logging.basicConfig(
            level=getattr(logging, logging_config['log_level']),
            format=logging_config['log_format']
        )
        self.logger = logging.getLogger(__name__)
        
        # 设置MQTT回调函数
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message
        self.client.on_publish = self.on_publish
        
    def on_connect(self, client, userdata, flags, rc):
        """MQTT连接回调"""
        if rc == 0:
            self.is_connected = True
            self.logger.info(f"成功连接到MQTT Broker: {self.broker_host}:{self.broker_port}")
            
            # 订阅接收主题
            client.subscribe(self.subscribe_topic)
            self.logger.info(f"已订阅主题: {self.subscribe_topic}")
        else:
            self.logger.error(f"连接MQTT Broker失败，错误代码: {rc}")
            
    def on_disconnect(self, client, userdata, rc):
        """MQTT断开连接回调"""
        self.is_connected = False
        self.logger.info("与MQTT Broker断开连接")
        
    def on_message(self, client, userdata, msg):
        """接收消息回调"""
        try:
            message = msg.payload.decode('utf-8')
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            self.logger.info(f"[{timestamp}] 接收到消息 - 主题: {msg.topic}, 内容: {message}")
        except Exception as e:
            self.logger.error(f"处理接收消息时出错: {e}")
            
    def on_publish(self, client, userdata, mid):
        """发布消息回调"""
        self.logger.debug(f"消息发布成功，消息ID: {mid}")
        
    def connect_broker(self):
        """连接到MQTT Broker"""
        try:
            self.logger.info(f"正在连接到MQTT Broker: {self.broker_host}:{self.broker_port}")
            self.client.connect(self.broker_host, self.broker_port, self.keepalive)
            self.client.loop_start()
            
            # 等待连接建立
            timeout = 10
            start_time = time.time()
            while not self.is_connected and (time.time() - start_time) < timeout:
                time.sleep(0.1)
                
            if not self.is_connected:
                raise Exception("连接超时")
                
        except Exception as e:
            self.logger.error(f"连接MQTT Broker失败: {e}")
            return False
        return True
        
    def disconnect_broker(self):
        """断开MQTT Broker连接"""
        if self.client:
            self.client.loop_stop()
            self.client.disconnect()
            self.logger.info("已断开MQTT Broker连接")
            
    def send_data(self, message=None):
        """发送数据到MQTT"""
        if not self.is_connected:
            self.logger.error("未连接到MQTT Broker，无法发送数据")
            return False
            
        try:
            send_message = message if message else self.message_content
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            
            # 构造发送的数据包
            data_packet = {
                "timestamp": timestamp,
                "message": send_message,
                "source": "xj1cloud"
            }
            
            # 发送JSON格式数据
            payload = json.dumps(data_packet, ensure_ascii=False)
            
            result = self.client.publish(self.publish_topic, payload)
            
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                self.logger.info(f"[{timestamp}] 发送消息成功 - 主题: {self.publish_topic}, 内容: {send_message}")
                return True
            else:
                self.logger.error(f"发送消息失败，错误代码: {result.rc}")
                return False
                
        except Exception as e:
            self.logger.error(f"发送数据时出错: {e}")
            return False
            
    def start_periodic_send(self):
        """启动定期发送线程"""
        def send_loop():
            self.logger.info(f"启动定期发送，周期: {self.send_interval}秒")
            while self.is_connected:
                self.send_data()
                time.sleep(self.send_interval)
                
        send_thread = threading.Thread(target=send_loop, daemon=True)
        send_thread.start()
        return send_thread
        
    def run(self):
        """运行主程序"""
        self.logger.info("=== 湘江一号云端通信系统启动 ===")
        
        # 连接MQTT Broker
        if not self.connect_broker():
            self.logger.error("无法连接到MQTT Broker，程序退出")
            return
            
        # 启动定期发送
        send_thread = self.start_periodic_send()
        
        try:
            self.logger.info("系统运行中，按Ctrl+C停止...")
            while True:
                time.sleep(1)
                
        except KeyboardInterrupt:
            self.logger.info("接收到停止信号，正在关闭系统...")
            
        finally:
            self.disconnect_broker()
            self.logger.info("=== 湘江一号云端通信系统已停止 ===")


def main():
    """主函数"""
    # 创建MQTT客户端实例
    # 配置参数从config.ini文件中读取
    mqtt_client = XJ1CloudMQTT()
    
    # 运行系统
    mqtt_client.run()


if __name__ == "__main__":
    main()
