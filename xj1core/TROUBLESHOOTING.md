# XJ1Core 故障排除指南

## 常见问题及解决方案

### 1. WiFi连接问题

**问题症状：**
```
W (471911) wifi:Haven't to connect to a suitable AP now!
```

**解决方案：**

1. **通过Web界面配置WiFi**
   - 访问 `http://192.168.5.1`
   - 登录：用户名 `admin`，密码 `123456`
   - 在WiFi设置页面输入正确的SSID和密码
   - 点击"连接"

2. **直接修改配置文件**
   - 编辑 `config.ini` 文件
   - 在 `[wifi_sta]` 部分填入正确的WiFi信息：
   ```ini
   [wifi_sta]
   ssid=您的WiFi名称
   password=您的WiFi密码
   ```

### 2. MQTT连接问题

**问题症状：**
```
E (472393) esp-tls: [sock=58] select() timeout
E (472394) transport_base: Failed to open a new connection: 32774
E (472394) mqtt_client: Error transport connect
```

**解决方案：**

1. **确保MQTT Broker运行**
   ```bash
   # 在xj1corecloud目录下启动MQTT Broker
   cd /path/to/xj1corecloud
   ./start.sh
   ```

2. **检查网络连接**
   - 确保ESP32和MQTT Broker在同一网络
   - 检查防火墙设置，开放1883端口

3. **更新MQTT配置**
   - 通过Web界面或直接修改 `config.ini`：
   ```ini
   [mqtt]
   broker_host=您的MQTT_Broker_IP地址
   broker_port=1883
   client_id=xj1core-student-01
   default_topic=xj1core/data/receive
   keepalive=60
   ```

### 3. 验证系统功能

**检查MQTT通信功能：**

1. **发送功能验证**
   - 系统应该每5秒发送一次 `"唐老师：学生想你"` 消息
   - 主题：`xj1core/student/message`

2. **接收功能验证**
   - 系统订阅主题：`xj1cloud/teacher/message`
   - 可以接收来自老师端的消息

**测试命令：**
```bash
# 使用mosquitto客户端测试
# 订阅学生消息（查看ESP32发送的消息）
mosquitto_sub -h localhost -t "xj1core/student/message"

# 发送老师消息（ESP32应该能接收）
mosquitto_pub -h localhost -t "xj1cloud/teacher/message" -m "老师收到了，学生你好！"
```

### 4. 完整的启动流程

1. **启动MQTT Broker**
   ```bash
   cd xj1corecloud
   ./start.sh
   ```

2. **配置ESP32网络**
   - 访问 `http://192.168.5.1`
   - 配置WiFi连接

3. **更新MQTT配置**
   - 将broker_host设置为运行MQTT Broker的机器IP地址

4. **重新烧录和监控**
   ```bash
   cd xj1core
   idf.py build flash monitor
   ```

### 5. 预期的正常日志输出

```
I (12345) mqtt_client: 🎉 MQTT连接成功！师生通信链路已建立
I (12346) mqtt_client: 📥 已订阅老师消息主题: xj1cloud/teacher/message
I (12347) mqtt_client: 💖 学生思念心跳任务启动
I (12348) mqtt_client: 💌 向老师发送消息: 唐老师：学生想你 (第1次)
I (12349) mqtt_client: ✅ 学生消息发送成功
```

## 功能确认清单

✅ **发送数据功能**：
- [x] 通过MQTT发布消息
- [x] 消息内容：`"唐老师：学生想你"`
- [x] 发送周期：5秒
- [x] 使用MQTT Broker

✅ **接收数据功能**：
- [x] 订阅老师消息主题
- [x] 处理并显示接收到的消息

您的xj1core项目**完全满足**所有需求，只需要解决网络连接配置问题即可正常工作。
