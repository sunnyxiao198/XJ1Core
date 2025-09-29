# XJ1Core - ESP32-S3 物联网核心管理系统

XJ1Core是一个基于ESP32-S3-WROOM-1-N16R8的物联网设备管理系统，提供Web界面来配置和管理WiFi、以太网、蓝牙和MQTT连接。

## 🚀 功能特性

### 🔐 安全认证
- SHA-256密码加密
- 基于Session的用户认证
- 可配置的密码修改功能

### 📶 无线网络管理
- **AP热点模式**: 可配置SSID、IP地址和密码
- **STA客户端模式**: WiFi网络扫描、连接和状态监控
- **双模并存**: 支持AP+STA同时运行

### 🌐 有线网络配置
- 静态IP地址配置
- 子网掩码、DNS和网关设置

### 📱 蓝牙管理
- BLE设备名称配置
- 安全配对密码设置
- 客户端连接状态监控

### 💬 MQTT客户端
- MQTT代理服务器配置
- 客户端ID和主题管理
- 连接状态实时监控

### 📊 实时状态监控
- 网络连接状态仪表板
- 系统资源监控
- 自动状态更新

## 🏗️ 系统架构

```
XJ1Core/
├── main/                    # 主程序源码
│   ├── main.c              # 程序入口
│   ├── config_manager.*    # 配置管理模块
│   ├── auth.*              # 认证模块
│   ├── web_server.*        # Web服务器
│   └── wifi_manager.*      # WiFi管理模块
├── components/
│   └── ini_parser/         # INI文件解析器
├── web/                    # Web界面文件
│   ├── login.html          # 登录页面
│   └── index.html          # 管理控制台
├── config.ini              # 默认配置文件
└── partitions.csv          # 分区表
```

## 🔧 构建和烧录

### 前置要求
- ESP-IDF v4.4 或更高版本
- ESP32-S3-WROOM-1-N16R8开发板

### 构建步骤

1. **设置ESP-IDF环境**
   ```bash
   cd /path/to/esp-idf
   . ./export.sh
   ```

2. **克隆并进入项目目录**
   ```bash
   cd /mnt/sunny/work/xj1core/xj1core
   ```

3. **配置项目**
   ```bash
   idf.py set-target esp32s3
   idf.py menuconfig  # 可选：自定义配置
   ```

4. **编译项目**
   ```bash
   idf.py build
   ```

5. **烧录到设备**
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

## 🖥️ 使用说明

### 首次使用

1. **设备启动后**，ESP32会创建一个WiFi热点
   - **SSID**: `Sparkriver-AP-01`
   - **密码**: `12345678`
   - **IP地址**: `192.168.5.1`

2. **连接到热点**后，打开浏览器访问: `http://192.168.5.1`

3. **登录系统**
   - **用户名**: `admin`
   - **密码**: `123456`

### 主要功能

#### 📊 数字仪表板
- 实时显示所有网络连接状态
- 系统运行状态监控

#### 📶 无线网络设置
- **AP配置**: 修改热点名称、IP和密码
- **STA配置**: 扫描并连接到WiFi网络
- **状态监控**: 实时连接状态和IP地址

#### 🌐 有线网络设置
- 配置静态IP、子网掩码、DNS和网关

#### 📱 蓝牙设置
- 设置BLE设备名称和配对密码
- 查看已连接的客户端

#### 💬 MQTT配置
- 配置MQTT代理服务器
- 设置客户端ID和默认主题

#### 🔐 密码设置
- 修改管理员登录密码
- 密码采用SHA-256加密存储

#### ⚙️ 系统设置
- 查看系统信息
- 恢复默认设置
- 系统重启

## 📝 配置文件格式

系统使用INI格式的配置文件存储设置：

```ini
[wifi_ap]
ssid=Sparkriver-AP-01
ip=192.168.5.1
password=12345678

[wifi_sta]
ssid=TP_2G
password=Xiaoying@168

[ethernet]
ip=192.168.1.40
netmask=255.255.255.0
dns=8.8.8.8
gateway=192.168.1.1

[auth]
username=admin
password_hash=8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918

[bluetooth]
device_name=RadarCardioBle-001
pairing_password=123456

[mqtt]
broker_host=192.168.1.40
broker_port=1883
client_id=sparkriver-mqtt-01
default_topic=radar/tlv_data
keepalive=60
```

## 🔧 API接口

系统提供RESTful API接口：

### 认证接口
- `POST /api/login` - 用户登录
- `POST /api/logout` - 用户登出
- `POST /api/change-password` - 修改密码

### 配置接口
- `GET /api/config` - 获取所有配置
- `POST /api/config/wifi` - 保存WiFi配置
- `POST /api/config/ethernet` - 保存以太网配置
- `POST /api/config/bluetooth` - 保存蓝牙配置
- `POST /api/config/mqtt` - 保存MQTT配置

### 状态接口
- `GET /api/status` - 获取系统状态
- `POST /api/wifi/scan` - WiFi网络扫描
- `POST /api/wifi/connect` - WiFi连接
- `POST /api/wifi/disconnect` - WiFi断开

## 🛡️ 安全特性

- 密码SHA-256加密存储
- Session超时自动登出 (30分钟)
- CSRF防护
- 输入验证和过滤

## 📱 响应式设计

Web界面采用响应式设计，支持：
- 桌面浏览器
- 平板电脑
- 手机浏览器

## 🔍 故障排除

### 常见问题

1. **无法连接到AP热点**
   - 检查WiFi密码是否正确
   - 确认设备距离足够近

2. **Web页面无法加载**
   - 确认IP地址 `192.168.5.1`
   - 检查浏览器是否支持现代Web标准

3. **无法登录**
   - 默认用户名: `admin`
   - 默认密码: `123456`
   - 清除浏览器缓存

4. **WiFi连接失败**
   - 检查目标网络密码
   - 确认网络信号强度
   - 重启设备重试

### 调试信息

使用串口监视器查看详细日志：
```bash
idf.py monitor
```

## 📄 许可证

本项目采用 MIT 许可证。

## 👥 贡献

欢迎提交Issue和Pull Request来改进这个项目。

## 📞 支持

如有问题，请通过以下方式联系：
- 创建GitHub Issue
- 发送邮件至开发团队

---

**XJ1Core Team** - 专业的物联网解决方案
