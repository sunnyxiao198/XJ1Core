#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
湘江一号云端Web通信系统
基于Flask和WebSocket实现MQTT消息的Web界面显示和交互
"""

import time
import threading
import logging
import json
from datetime import datetime
from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO, emit
import paho.mqtt.client as mqtt
from config_manager import ConfigManager


class XJ1CloudWeb:
    """湘江一号Web MQTT通信系统"""
    
    def __init__(self, config_file="config.ini"):
        """
        初始化Web应用和MQTT客户端
        
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
        
        # Web配置
        web_config = config['web']
        self.web_host = web_config['host']
        self.web_port = web_config['port']
        self.web_debug = web_config['debug']
        self.secret_key = web_config['secret_key']
        
        # 消息配置
        message_config = config['message']
        self.send_interval = message_config['send_interval']
        self.default_message = message_config['default_message']
        
        # 设置日志
        logging_config = config['logging']
        logging.basicConfig(
            level=getattr(logging, logging_config['log_level']),
            format=logging_config['log_format']
        )
        self.logger = logging.getLogger(__name__)
        
        # 初始化Flask应用
        self.app = Flask(__name__)
        self.app.config['SECRET_KEY'] = self.secret_key
        
        # 初始化SocketIO
        self.socketio = SocketIO(self.app, cors_allowed_origins="*", async_mode='eventlet')
        
        # MQTT客户端
        self.mqtt_client = mqtt.Client()
        self.is_connected = False
        self.message_history = []  # 存储历史消息
        self.max_history = 100     # 最大历史消息数量
        
        # 设置路由和事件处理
        self.setup_routes()
        self.setup_socketio_events()
        self.setup_mqtt_callbacks()
        
    def setup_routes(self):
        """设置Flask路由"""
        
        @self.app.route('/')
        def index():
            """主页"""
            return render_template('index.html')
            
        @self.app.route('/api/status')
        def get_status():
            """获取系统状态"""
            return jsonify({
                'mqtt_connected': self.is_connected,
                'broker_host': self.broker_host,
                'broker_port': self.broker_port,
                'publish_topic': self.publish_topic,
                'subscribe_topic': self.subscribe_topic,
                'message_count': len(self.message_history)
            })
            
        @self.app.route('/api/config')
        def get_config():
            """获取配置信息"""
            config = self.config_manager.get_all_config()
            # 移除敏感信息
            if 'web' in config and 'secret_key' in config['web']:
                config['web']['secret_key'] = '***'
            return jsonify(config)
            
        @self.app.route('/api/messages')
        def get_messages():
            """获取历史消息"""
            return jsonify(self.message_history[-50:])  # 返回最近50条消息
            
        @self.app.route('/api/send', methods=['POST'])
        def send_message():
            """发送消息API - 支持JSON和表单数据"""
            try:
                # 支持JSON格式
                if request.is_json:
                    data = request.get_json()
                    message = data.get('message', '')
                    source = data.get('source', 'web_api')
                # 支持表单数据格式
                else:
                    message = request.form.get('message', '')
                    source = request.form.get('source', 'web_api')
                
                if not message:
                    return jsonify({'success': False, 'error': '消息内容不能为空'})
                    
                success = self.send_mqtt_message(message, source)
                
                response_data = {
                    'success': success,
                    'message': message,
                    'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                    'topic': self.publish_topic
                }
                
                return jsonify(response_data)
                
            except Exception as e:
                self.logger.error(f"发送消息API错误: {e}")
                return jsonify({'success': False, 'error': str(e)})
                
        @self.app.route('/api/mqtt/publish', methods=['POST'])
        def mqtt_publish():
            """MQTT发布API - 专门为ESP32设计的简化接口"""
            try:
                # 支持多种数据格式
                if request.is_json:
                    data = request.get_json()
                elif request.content_type == 'application/x-www-form-urlencoded':
                    data = request.form.to_dict()
                else:
                    # 尝试解析为文本
                    data = {'message': request.get_data(as_text=True)}
                
                message = data.get('message', '')
                source = data.get('source', 'xj1core_esp32')
                custom_topic = data.get('topic', '')  # 允许自定义主题
                
                if not message:
                    return jsonify({
                        'status': 'error', 
                        'message': '消息内容不能为空',
                        'code': 400
                    }), 400
                
                # 使用自定义主题或默认发布主题
                topic = custom_topic if custom_topic else self.publish_topic
                success = self.send_mqtt_message(message, source, topic)
                
                if success:
                    return jsonify({
                        'status': 'success',
                        'message': '消息发布成功',
                        'data': {
                            'content': message,
                            'topic': topic,
                            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                            'source': source
                        },
                        'code': 200
                    })
                else:
                    return jsonify({
                        'status': 'error',
                        'message': 'MQTT发布失败',
                        'code': 500
                    }), 500
                    
            except Exception as e:
                self.logger.error(f"MQTT发布API错误: {e}")
                return jsonify({
                    'status': 'error',
                    'message': f'服务器错误: {str(e)}',
                    'code': 500
                }), 500
                
    def setup_socketio_events(self):
        """设置SocketIO事件处理"""
        
        @self.socketio.on('connect')
        def handle_connect():
            """客户端连接事件"""
            self.logger.info(f"Web客户端已连接: {request.sid}")
            # 发送当前状态
            emit('status_update', {
                'mqtt_connected': self.is_connected,
                'broker_host': self.broker_host,
                'broker_port': self.broker_port
            })
            # 发送最近的消息历史
            emit('message_history', self.message_history[-20:])
            
        @self.socketio.on('disconnect')
        def handle_disconnect():
            """客户端断开连接事件"""
            self.logger.info(f"Web客户端已断开: {request.sid}")
            
        @self.socketio.on('send_message')
        def handle_send_message(data):
            """处理发送消息事件"""
            try:
                message = data.get('message', '')
                if message:
                    success = self.send_mqtt_message(message)
                    emit('send_result', {'success': success, 'message': message})
                else:
                    emit('send_result', {'success': False, 'error': '消息内容不能为空'})
            except Exception as e:
                self.logger.error(f"WebSocket发送消息错误: {e}")
                emit('send_result', {'success': False, 'error': str(e)})
                
        @self.socketio.on('request_status')
        def handle_request_status():
            """处理状态请求"""
            emit('status_update', {
                'mqtt_connected': self.is_connected,
                'broker_host': self.broker_host,
                'broker_port': self.broker_port,
                'message_count': len(self.message_history)
            })
            
    def setup_mqtt_callbacks(self):
        """设置MQTT回调函数"""
        
        def on_connect(client, userdata, flags, rc):
            """MQTT连接回调"""
            if rc == 0:
                self.is_connected = True
                self.logger.info(f"MQTT连接成功: {self.broker_host}:{self.broker_port}")
                
                # 订阅接收主题
                client.subscribe(self.subscribe_topic)
                self.logger.info(f"已订阅主题: {self.subscribe_topic}")
                
                # 通知Web客户端连接状态更新
                self.socketio.emit('status_update', {
                    'mqtt_connected': True,
                    'broker_host': self.broker_host,
                    'broker_port': self.broker_port
                })
            else:
                self.logger.error(f"MQTT连接失败，错误代码: {rc}")
                self.is_connected = False
                
        def on_disconnect(client, userdata, rc):
            """MQTT断开连接回调"""
            self.is_connected = False
            self.logger.info("MQTT连接已断开")
            
            # 通知Web客户端连接状态更新
            self.socketio.emit('status_update', {
                'mqtt_connected': False,
                'broker_host': self.broker_host,
                'broker_port': self.broker_port
            })
            
        def on_message(client, userdata, msg):
            """接收消息回调"""
            try:
                message = msg.payload.decode('utf-8')
                timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                
                # 构造消息对象
                message_obj = {
                    'timestamp': timestamp,
                    'topic': msg.topic,
                    'message': message,
                    'type': 'received'
                }
                
                # 添加到历史记录
                self.add_message_to_history(message_obj)
                
                self.logger.info(f"[{timestamp}] 接收消息 - 主题: {msg.topic}, 内容: {message}")
                
                # 实时推送到Web客户端
                self.socketio.emit('new_message', message_obj)
                
            except Exception as e:
                self.logger.error(f"处理接收消息时出错: {e}")
                
        def on_publish(client, userdata, mid):
            """发布消息回调"""
            self.logger.debug(f"消息发布成功，消息ID: {mid}")
            
        # 设置回调函数
        self.mqtt_client.on_connect = on_connect
        self.mqtt_client.on_disconnect = on_disconnect
        self.mqtt_client.on_message = on_message
        self.mqtt_client.on_publish = on_publish
        
    def add_message_to_history(self, message_obj):
        """添加消息到历史记录"""
        self.message_history.append(message_obj)
        
        # 保持历史记录在限制范围内
        if len(self.message_history) > self.max_history:
            self.message_history = self.message_history[-self.max_history:]
            
    def connect_mqtt(self):
        """连接MQTT Broker"""
        try:
            self.logger.info(f"正在连接MQTT Broker: {self.broker_host}:{self.broker_port}")
            self.mqtt_client.connect(self.broker_host, self.broker_port, self.keepalive)
            self.mqtt_client.loop_start()
            
            # 等待连接建立
            timeout = 10
            start_time = time.time()
            while not self.is_connected and (time.time() - start_time) < timeout:
                time.sleep(0.1)
                
            if not self.is_connected:
                raise Exception("MQTT连接超时")
                
            return True
            
        except Exception as e:
            self.logger.error(f"连接MQTT Broker失败: {e}")
            return False
            
    def disconnect_mqtt(self):
        """断开MQTT连接"""
        if self.mqtt_client:
            self.mqtt_client.loop_stop()
            self.mqtt_client.disconnect()
            self.logger.info("MQTT连接已断开")
            
    def send_mqtt_message(self, message, source="xj1cloud_web", topic=None):
        """发送MQTT消息"""
        if not self.is_connected:
            self.logger.error("MQTT未连接，无法发送消息")
            return False
            
        try:
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            
            # 使用指定主题或默认发布主题
            publish_topic = topic if topic else self.publish_topic
            
            # 构造发送的数据包
            data_packet = {
                "timestamp": timestamp,
                "message": message,
                "source": source
            }
            
            # 发送JSON格式数据
            payload = json.dumps(data_packet, ensure_ascii=False)
            result = self.mqtt_client.publish(publish_topic, payload)
            
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                # 添加到历史记录
                message_obj = {
                    'timestamp': timestamp,
                    'topic': publish_topic,
                    'message': message,
                    'type': 'sent',
                    'source': source
                }
                self.add_message_to_history(message_obj)
                
                self.logger.info(f"[{timestamp}] 发送消息成功 - 主题: {publish_topic}, 来源: {source}, 内容: {message}")
                
                # 通知Web客户端消息已发送
                self.socketio.emit('new_message', message_obj)
                
                return True
            else:
                self.logger.error(f"发送消息失败，错误代码: {result.rc}")
                return False
                
        except Exception as e:
            self.logger.error(f"发送MQTT消息时出错: {e}")
            return False
            
    def start_periodic_send(self):
        """启动定期发送线程（可选功能）"""
        def send_loop():
            self.logger.info(f"启动定期发送，周期: {self.send_interval}秒")
            while self.is_connected:
                # 只有在没有Web客户端连接时才自动发送
                if not hasattr(self.socketio, 'server') or len(self.socketio.server.manager.rooms.get('/', {})) == 0:
                    self.send_mqtt_message(self.default_message)
                time.sleep(self.send_interval)
                
        send_thread = threading.Thread(target=send_loop, daemon=True)
        send_thread.start()
        return send_thread
        
    def run(self):
        """运行Web应用"""
        self.logger.info("=== 湘江一号云端Web通信系统启动 ===")
        
        # 连接MQTT
        if not self.connect_mqtt():
            self.logger.error("无法连接到MQTT Broker")
            return False
            
        # 启动定期发送（可选）
        # self.start_periodic_send()
        
        try:
            self.logger.info(f"Web服务器启动中: http://{self.web_host}:{self.web_port}")
            self.socketio.run(
                self.app,
                host=self.web_host,
                port=self.web_port,
                debug=self.web_debug
            )
        except KeyboardInterrupt:
            self.logger.info("接收到停止信号，正在关闭系统...")
        finally:
            self.disconnect_mqtt()
            self.logger.info("=== 湘江一号云端Web通信系统已停止 ===")
            
        return True


def main():
    """主函数"""
    # 创建Web应用实例
    web_app = XJ1CloudWeb()
    
    # 运行Web应用
    web_app.run()


if __name__ == "__main__":
    main()