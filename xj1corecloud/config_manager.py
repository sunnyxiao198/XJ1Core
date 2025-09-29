#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
配置管理模块
读取和管理系统配置参数
"""

import configparser
import os
from typing import Dict, Any


class ConfigManager:
    """配置管理器"""
    
    def __init__(self, config_file="config.ini"):
        """
        初始化配置管理器
        
        Args:
            config_file (str): 配置文件路径
        """
        self.config_file = config_file
        self.config = configparser.ConfigParser()
        self.load_config()
        
    def load_config(self):
        """加载配置文件"""
        if os.path.exists(self.config_file):
            self.config.read(self.config_file, encoding='utf-8')
        else:
            raise FileNotFoundError(f"配置文件不存在: {self.config_file}")
            
    def get_mqtt_config(self) -> Dict[str, Any]:
        """获取MQTT配置"""
        mqtt_config = {}
        if 'mqtt' in self.config:
            mqtt_config = {
                'broker_host': self.config.get('mqtt', 'broker_host', fallback='localhost'),
                'broker_port': self.config.getint('mqtt', 'broker_port', fallback=1883),
                'keepalive': self.config.getint('mqtt', 'keepalive', fallback=60),
                'publish_topic': self.config.get('mqtt', 'publish_topic', fallback='xj1core/data/send'),
                'subscribe_topic': self.config.get('mqtt', 'subscribe_topic', fallback='xj1core/data/receive')
            }
        return mqtt_config
        
    def get_message_config(self) -> Dict[str, Any]:
        """获取消息配置"""
        message_config = {}
        if 'message' in self.config:
            message_config = {
                'default_message': self.config.get('message', 'default_message', fallback='唐老师：您好吗？'),
                'send_interval': self.config.getint('message', 'send_interval', fallback=5)
            }
        return message_config
        
    def get_logging_config(self) -> Dict[str, Any]:
        """获取日志配置"""
        logging_config = {}
        if 'logging' in self.config:
            logging_config = {
                'log_level': self.config.get('logging', 'log_level', fallback='INFO'),
                'log_format': self.config.get('logging', 'log_format', 
                                           fallback='%(asctime)s - %(levelname)s - %(message)s')
            }
        return logging_config
        
    def get_web_config(self) -> Dict[str, Any]:
        """获取Web服务器配置"""
        web_config = {}
        if 'web' in self.config:
            web_config = {
                'host': self.config.get('web', 'host', fallback='0.0.0.0'),
                'port': self.config.getint('web', 'port', fallback=5000),
                'debug': self.config.getboolean('web', 'debug', fallback=False),
                'secret_key': self.config.get('web', 'secret_key', fallback='xj1core_web_secret_key_2025')
            }
        return web_config
        
    def get_all_config(self) -> Dict[str, Dict[str, Any]]:
        """获取所有配置"""
        return {
            'mqtt': self.get_mqtt_config(),
            'message': self.get_message_config(),
            'logging': self.get_logging_config(),
            'web': self.get_web_config()
        }
