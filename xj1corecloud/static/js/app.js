/**
 * 湘江一号云端通信系统前端JavaScript
 * 实现WebSocket通信和界面交互
 */

class XJ1CloudApp {
    constructor() {
        this.socket = null;
        this.isConnected = false;
        this.autoScroll = true;
        this.messageCount = 0;
        this.connectionStartTime = null;
        
        this.init();
    }
    
    init() {
        this.initializeSocket();
        this.bindEvents();
        this.startConnectionTimer();
    }
    
    initializeSocket() {
        // 初始化Socket.IO连接
        this.socket = io();
        
        // 连接事件
        this.socket.on('connect', () => {
            console.log('WebSocket连接已建立');
            this.updateConnectionStatus(true);
            this.showNotification('WebSocket连接成功', 'success');
            this.connectionStartTime = new Date();
        });
        
        // 断开连接事件
        this.socket.on('disconnect', () => {
            console.log('WebSocket连接已断开');
            this.updateConnectionStatus(false);
            this.showNotification('WebSocket连接断开', 'warning');
            this.connectionStartTime = null;
        });
        
        // 接收状态更新
        this.socket.on('status_update', (data) => {
            this.updateMQTTStatus(data);
        });
        
        // 接收新消息
        this.socket.on('new_message', (message) => {
            this.addMessage(message);
            this.messageCount++;
            this.updateMessageCount();
        });
        
        // 接收消息历史
        this.socket.on('message_history', (messages) => {
            this.loadMessageHistory(messages);
        });
        
        // WebSocket发送结果监听已移除，现在使用HTTP API
        
        // 连接错误
        this.socket.on('connect_error', (error) => {
            console.error('WebSocket连接错误:', error);
            this.showNotification('WebSocket连接错误', 'danger');
        });
    }
    
    bindEvents() {
        // 消息发送表单
        const messageForm = document.getElementById('message-form');
        const messageInput = document.getElementById('message-input');
        const sendBtn = document.getElementById('send-btn');
        
        messageForm.addEventListener('submit', (e) => {
            e.preventDefault();
            this.sendMessage();
        });
        
        // 字符计数
        messageInput.addEventListener('input', () => {
            this.updateCharCount();
        });
        
        // 快捷消息按钮
        document.querySelectorAll('.quick-message').forEach(btn => {
            btn.addEventListener('click', async (e) => {
                let message = e.target.dataset.message;
                
                // 测试消息特殊处理
                if (e.target.id === 'test-message') {
                    message += new Date().toLocaleTimeString();
                }
                
                // 直接发送快捷消息
                if (message) {
                    messageInput.value = message;
                    this.updateCharCount();
                    
                    // 自动发送消息
                    await this.sendMessage();
                } else {
                    messageInput.value = message;
                    this.updateCharCount();
                    messageInput.focus();
                }
            });
        });
        
        // 清空消息
        document.getElementById('clear-messages').addEventListener('click', () => {
            this.clearMessages();
        });
        
        // 自动滚动切换
        document.getElementById('auto-scroll-toggle').addEventListener('click', () => {
            this.toggleAutoScroll();
        });
        
        // 消息容器滚动事件
        const messageContainer = document.getElementById('message-container');
        messageContainer.addEventListener('scroll', () => {
            this.handleScroll();
        });
        
        // Enter键发送消息（Ctrl+Enter换行）
        messageInput.addEventListener('keydown', (e) => {
            if (e.key === 'Enter' && !e.ctrlKey) {
                e.preventDefault();
                this.sendMessage();
            }
        });
    }
    
    updateConnectionStatus(connected) {
        const statusElement = document.getElementById('connection-status');
        const statusIcon = statusElement.querySelector('i');
        
        if (connected) {
            statusElement.className = 'badge bg-success status-connected';
            statusElement.innerHTML = '<i class="bi bi-circle-fill"></i> 已连接';
        } else {
            statusElement.className = 'badge bg-danger status-disconnected';
            statusElement.innerHTML = '<i class="bi bi-circle-fill"></i> 未连接';
        }
        
        this.isConnected = connected;
    }
    
    updateMQTTStatus(data) {
        // 更新MQTT连接状态
        const statusElement = document.getElementById('connection-status');
        
        if (data.mqtt_connected) {
            statusElement.className = 'badge bg-success status-connected';
            statusElement.innerHTML = '<i class="bi bi-circle-fill"></i> MQTT已连接';
        } else {
            statusElement.className = 'badge bg-warning status-connecting';
            statusElement.innerHTML = '<i class="bi bi-circle-fill"></i> MQTT断开';
        }
        
        // 更新Broker信息
        document.getElementById('broker-info').textContent = 
            `${data.broker_host}:${data.broker_port}`;
        document.getElementById('publish-topic').textContent = data.publish_topic || '--';
        document.getElementById('subscribe-topic').textContent = data.subscribe_topic || '--';
        
        // 更新消息计数
        if (data.message_count !== undefined) {
            this.messageCount = data.message_count;
            this.updateMessageCount();
        }
    }
    
    async sendMessage() {
        const messageInput = document.getElementById('message-input');
        const sendBtn = document.getElementById('send-btn');
        const message = messageInput.value.trim();
        
        if (!message) {
            this.showNotification('请输入消息内容', 'warning');
            return;
        }
        
        // 禁用发送按钮
        sendBtn.disabled = true;
        sendBtn.innerHTML = '<span class="loading-spinner"></span> 发送中...';
        
        try {
            // 通过HTTP API发送MQTT消息
            const response = await fetch('/api/mqtt/publish', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Accept': 'application/json'
                },
                body: JSON.stringify({
                    message: message,
                    source: 'web_interface'
                })
            });
            
            const result = await response.json();
            
            if (response.ok && result.status === 'success') {
                this.showNotification('消息发送成功', 'success');
                
                // 清空输入框
                messageInput.value = '';
                this.updateCharCount();
                
                // 添加发送的消息到界面（如果WebSocket未及时更新）
                const messageObj = {
                    timestamp: result.data.timestamp,
                    topic: result.data.topic,
                    message: result.data.content,
                    type: 'sent',
                    source: result.data.source
                };
                
                // 延迟一点添加，避免重复显示
                setTimeout(() => {
                    this.addMessage(messageObj);
                    this.messageCount++;
                    this.updateMessageCount();
                }, 100);
                
            } else {
                this.showNotification(`发送失败: ${result.message || '未知错误'}`, 'danger');
            }
            
        } catch (error) {
            console.error('发送消息错误:', error);
            this.showNotification(`发送失败: 网络错误`, 'danger');
        } finally {
            // 恢复发送按钮
            sendBtn.disabled = false;
            sendBtn.innerHTML = '<i class="bi bi-send"></i> 发送消息';
        }
    }
    
    // handleSendResult 方法已移除，现在使用HTTP API直接处理发送结果
    
    addMessage(message) {
        const container = document.getElementById('message-container');
        const emptyState = container.querySelector('.text-center');
        
        // 移除空状态提示
        if (emptyState) {
            emptyState.remove();
        }
        
        // 创建消息元素
        const messageElement = this.createMessageElement(message);
        container.appendChild(messageElement);
        
        // 自动滚动到底部
        if (this.autoScroll) {
            this.scrollToBottom();
        }
        
        // 高亮新消息
        messageElement.classList.add('highlight');
        setTimeout(() => {
            messageElement.classList.remove('highlight');
        }, 3000);
    }
    
    createMessageElement(message) {
        const messageDiv = document.createElement('div');
        messageDiv.className = `message-item ${message.type}`;
        
        const typeText = message.type === 'sent' ? '发送' : '接收';
        const typeIcon = message.type === 'sent' ? 'arrow-up-circle' : 'arrow-down-circle';
        
        messageDiv.innerHTML = `
            <div class="message-header">
                <span class="message-type ${message.type}">
                    <i class="bi bi-${typeIcon} message-icon ${message.type}"></i>
                    ${typeText}
                </span>
                <span class="message-timestamp">${message.timestamp}</span>
            </div>
            <div class="message-content">${this.escapeHtml(message.message)}</div>
            <div class="message-topic">${message.topic}</div>
        `;
        
        return messageDiv;
    }
    
    loadMessageHistory(messages) {
        const container = document.getElementById('message-container');
        
        // 清空容器
        container.innerHTML = '';
        
        if (messages.length === 0) {
            container.innerHTML = `
                <div class="text-center p-4 text-muted empty-state">
                    <i class="bi bi-chat-square-dots fs-1"></i>
                    <p class="mt-2">暂无消息记录</p>
                </div>
            `;
            return;
        }
        
        // 添加历史消息
        messages.forEach(message => {
            const messageElement = this.createMessageElement(message);
            container.appendChild(messageElement);
        });
        
        this.messageCount = messages.length;
        this.updateMessageCount();
        
        // 滚动到底部
        this.scrollToBottom();
    }
    
    clearMessages() {
        if (confirm('确定要清空所有消息记录吗？')) {
            const container = document.getElementById('message-container');
            container.innerHTML = `
                <div class="text-center p-4 text-muted empty-state">
                    <i class="bi bi-chat-square-dots fs-1"></i>
                    <p class="mt-2">消息记录已清空</p>
                </div>
            `;
            this.messageCount = 0;
            this.updateMessageCount();
            this.showNotification('消息记录已清空', 'info');
        }
    }
    
    toggleAutoScroll() {
        this.autoScroll = !this.autoScroll;
        const toggleBtn = document.getElementById('auto-scroll-toggle');
        
        if (this.autoScroll) {
            toggleBtn.innerHTML = '<i class="bi bi-arrow-down"></i> 自动滚动';
            toggleBtn.className = 'btn btn-outline-primary btn-sm';
            this.scrollToBottom();
        } else {
            toggleBtn.innerHTML = '<i class="bi bi-pause"></i> 手动滚动';
            toggleBtn.className = 'btn btn-outline-secondary btn-sm';
        }
        
        this.showNotification(
            this.autoScroll ? '已开启自动滚动' : '已关闭自动滚动', 
            'info'
        );
    }
    
    handleScroll() {
        const container = document.getElementById('message-container');
        const isAtBottom = container.scrollHeight - container.clientHeight <= container.scrollTop + 1;
        
        // 如果用户手动滚动到非底部位置，暂时关闭自动滚动
        if (!isAtBottom && this.autoScroll) {
            // 可以在这里添加一个临时提示，表示自动滚动已暂停
        }
    }
    
    scrollToBottom() {
        const container = document.getElementById('message-container');
        container.scrollTop = container.scrollHeight;
    }
    
    updateCharCount() {
        const messageInput = document.getElementById('message-input');
        const charCount = document.getElementById('char-count');
        const currentLength = messageInput.value.length;
        const maxLength = 500;
        
        charCount.textContent = currentLength;
        
        // 根据字符数量改变颜色
        charCount.className = '';
        if (currentLength > maxLength * 0.8) {
            charCount.className = 'char-warning';
        }
        if (currentLength > maxLength * 0.95) {
            charCount.className = 'char-danger';
        }
    }
    
    updateMessageCount() {
        document.getElementById('message-count').textContent = this.messageCount;
    }
    
    startConnectionTimer() {
        setInterval(() => {
            this.updateConnectionTime();
        }, 1000);
    }
    
    updateConnectionTime() {
        const timeElement = document.getElementById('connection-time');
        
        if (this.connectionStartTime) {
            const now = new Date();
            const diff = Math.floor((now - this.connectionStartTime) / 1000);
            const minutes = Math.floor(diff / 60);
            const seconds = diff % 60;
            timeElement.textContent = `${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
        } else {
            timeElement.textContent = '--:--';
        }
    }
    
    showNotification(message, type = 'info') {
        const toast = document.getElementById('notification-toast');
        const toastBody = toast.querySelector('.toast-body');
        const toastHeader = toast.querySelector('.toast-header');
        
        // 设置图标和颜色
        let icon = 'info-circle';
        let iconClass = 'text-info';
        
        switch (type) {
            case 'success':
                icon = 'check-circle';
                iconClass = 'text-success';
                break;
            case 'warning':
                icon = 'exclamation-triangle';
                iconClass = 'text-warning';
                break;
            case 'danger':
                icon = 'x-circle';
                iconClass = 'text-danger';
                break;
        }
        
        // 更新图标
        const iconElement = toastHeader.querySelector('i');
        iconElement.className = `bi bi-${icon} ${iconClass} me-2`;
        
        // 设置消息内容
        toastBody.textContent = message;
        
        // 显示Toast
        const bsToast = new bootstrap.Toast(toast);
        bsToast.show();
    }
    
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }
    
    // 请求最新状态
    requestStatus() {
        if (this.socket && this.isConnected) {
            this.socket.emit('request_status');
        }
    }
}

// 页面加载完成后初始化应用
document.addEventListener('DOMContentLoaded', () => {
    window.xj1App = new XJ1CloudApp();
    
    // 定期请求状态更新
    setInterval(() => {
        if (window.xj1App) {
            window.xj1App.requestStatus();
        }
    }, 30000); // 每30秒请求一次状态更新
    
    console.log('湘江一号云端通信系统前端已初始化');
});
