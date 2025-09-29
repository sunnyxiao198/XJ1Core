#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
湘江一号云端Web通信系统启动脚本
"""

import sys
import os

# 添加当前目录到Python路径
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from web_app import main

if __name__ == "__main__":
    print("=== 湘江一号云端Web通信系统 ===")
    print("启动中...")
    try:
        main()
    except Exception as e:
        print(f"系统启动失败: {e}")
        sys.exit(1)
