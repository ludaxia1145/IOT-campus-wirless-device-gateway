# 智慧教务终端  - 基于IMX6ULL的校园无线通信设备

# 一.交叉编译
## 1.确保有这个NXP IMX交叉编译环境
ls /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi

## 2.将当前终端会话切换成针对 ARM 架构的交叉编译环境
source /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi

## 3.编写smart_schedule.pro,给项目文件生成 Makefile
qmake smart_schedule.pro

## 4.执行make
make

# 二.网络功能实现  QTcpSocket
## 1.断网自动重连（心跳机制） 心跳周期：30秒 格式: { "type": "heartbeat", "data": {} }
void NetworkClient::startHeartbeat()
{
    if (!heartbeatTimer->isActive()) {
        heartbeatTimer->start(30000);  // 30秒周期
    }
}


## 2.超时重传: 一次发送，失败则3次重传，间隔3s
void NetworkClient::retryFailedMessage()
{
    auto it = pendingMessages.begin();
    while (it != pendingMessages.end()) {
        PendingMessage &pending = it.value();
        pending.retryCount++;

        if (socket->isOpen()) {
            // 连接正常，重新发送
            QJsonObject json;
            json["type"] = pending.type;
            json["data"] = pending.data;
            // ... 发送消息
            
            it = pendingMessages.erase(it);  // 发送成功，移除队列
        } else if (pending.retryCount >= maxRetries) {
            // 超过最大重试次数，保存到缓存
            saveToCache(pending.type, pending.data);
            it = pendingMessages.erase(it);
        } else {
            ++it;
        }
    }
}

## 3.发送失败，离线缓存  缓存到/var/cache
