#include "networkclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QFile>

NetworkClient::NetworkClient(QObject *parent) : QObject(parent), reconnectCount(0), currentReconnectInterval(3000), messageId(0)
{
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, this, &NetworkClient::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &NetworkClient::onDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)));

    heartbeatTimer = new QTimer(this);
    connect(heartbeatTimer, &QTimer::timeout, this, &NetworkClient::onHeartbeatTimeout);

    reconnectTimer = new QTimer(this);
    connect(reconnectTimer, &QTimer::timeout, this, &NetworkClient::onReconnectTimeout);

    retryTimer = new QTimer(this);
    connect(retryTimer, &QTimer::timeout, this, &NetworkClient::onRetryTimeout);
}

NetworkClient::~NetworkClient()
{
    if (socket->isOpen()) {
        socket->close();
    }
}

void NetworkClient::connectToServer(const QString &host, quint16 port)
{
    qDebug() << "Connecting to" << host << ":" << port;
    socket->connectToHost(host, port);
}

void NetworkClient::sendMessage(const QString &type, const QJsonObject &data)
{
    sendMessageWithRetry(type, data);
}

void NetworkClient::sendMessageWithRetry(const QString &type, const QJsonObject &data)
{
    if (socket->isOpen()) {
        QJsonObject json;
        json["type"] = type;
        json["data"] = data;
        QJsonDocument doc(json);
        QString message = doc.toJson(QJsonDocument::Compact) + "\n";

        socket->write(message.toUtf8());
        socket->flush();
        qDebug() << "Sent:" << message;

        int msgId = ++messageId;
        PendingMessage pending;
        pending.type = type;
        pending.data = data;
        pending.retryCount = 0;
        pendingMessages[msgId] = pending;
    } else {
        qDebug() << "[v0] Socket not connected, caching message";
        saveToCache(type, data);

        if (!retryTimer->isActive()) {
            retryTimer->start(retryInterval);
        }
    }
}

bool NetworkClient::isConnected() const
{
    return socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::onConnected()
{
    qDebug() << "Connected to server";
    reconnectCount = 0;
    currentReconnectInterval = initialInterval;
    reconnectTimer->stop();
    emit connectionStatusChanged(true);

    startHeartbeat();
}

void NetworkClient::onDisconnected()
{
    qDebug() << "Disconnected from server";
    stopHeartbeat();
    emit connectionStatusChanged(false);

    if (reconnectCount < maxReconnectAttempts) {
        startReconnect();
    } else {
        qDebug() << "Max reconnection attempts reached";
    }
}

void NetworkClient::onReadyRead()
{
    buffer += QString::fromUtf8(socket->readAll());

    while (buffer.contains('\n')) {
        int pos = buffer.indexOf('\n');
        QString message = buffer.left(pos);
        buffer = buffer.mid(pos + 1);

        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject json = doc.object();
            QString type = json["type"].toString();
            QJsonObject data = json["data"].toObject();

            qDebug() << "Received type:" << type;

            if (type == "course_info") {
                emit courseInfoReceived(data);
            } else if (type == "notice_list") {
                emit noticeListReceived(data);
            } else if (type == "notice_detail") {
                emit noticeDetailReceived(data);
            } else if (type == "seat_status") {
                emit librarySeatsReceived(data);
            } else if (type == "attendance_list") {
                emit attendanceListReceived(data);
            } else if (type == "message_response" || type == "reservation_response" || type == "attendance_response") {
                emit responseReceived(type, data);
            } else {
                emit responseReceived(type, data);
            }
        }
    }
}

void NetworkClient::onError(QAbstractSocket::SocketError error)
{
    qDebug() << "Socket error:" << error << socket->errorString();
    stopHeartbeat();
    if (reconnectCount < maxReconnectAttempts) {
        startReconnect();
    }
}

void NetworkClient::startHeartbeat()
{
    if (!heartbeatTimer->isActive()) {
        heartbeatTimer->start(30000);
    }
}

void NetworkClient::stopHeartbeat()
{
    heartbeatTimer->stop();
}

void NetworkClient::onHeartbeatTimeout()
{
    if (socket->isOpen()) {
        QJsonObject data;
        data["type"] = "heartbeat";
        sendMessage("heartbeat", data);
        qDebug() << "[v0] Heartbeat sent";
    } else {
        stopHeartbeat();
        startReconnect();
    }
}

void NetworkClient::startReconnect()
{
    if (!reconnectTimer->isActive()) {
        reconnectCount++;
        currentReconnectInterval = qMin(currentReconnectInterval * 2, maxInterval);
        qDebug() << "[v0] Reconnect attempt" << reconnectCount << "in" << currentReconnectInterval << "ms";
        reconnectTimer->start(currentReconnectInterval);
    }
}

void NetworkClient::onReconnectTimeout()
{
    reconnectTimer->stop();
    if (socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[v0] Attempting to reconnect...";
        socket->connectToHost(socket->peerName(), socket->peerPort());
    }
}

void NetworkClient::onRetryTimeout()
{
    retryTimer->stop();
    retryFailedMessage();
}

void NetworkClient::retryFailedMessage()
{
    if (pendingMessages.isEmpty()) {
        return;
    }

    auto it = pendingMessages.begin();
    while (it != pendingMessages.end()) {
        PendingMessage &pending = it.value();
        pending.retryCount++;

        if (socket->isOpen()) {
            QJsonObject json;
            json["type"] = pending.type;
            json["data"] = pending.data;
            QJsonDocument doc(json);
            QString message = doc.toJson(QJsonDocument::Compact) + "\n";

            socket->write(message.toUtf8());
            socket->flush();
            qDebug() << "[v0] Retrying message" << pending.type << "attempt" << pending.retryCount;

            it = pendingMessages.erase(it);
        } else if (pending.retryCount >= maxRetries) {
            qDebug() << "[v0] Max retries reached, caching to disk:" << pending.type;
            saveToCache(pending.type, pending.data);
            it = pendingMessages.erase(it);
        } else {
            ++it;
        }
    }

    if (!pendingMessages.isEmpty()) {
        retryTimer->start(retryInterval);
    }
}

void NetworkClient::saveToCache(const QString &type, const QJsonObject &data)
{
    QJsonObject json;
    json["type"] = type;
    json["data"] = data;
    json["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    QJsonDocument doc(json);

    QDir cacheDir("/var/cache");
    if (!cacheDir.exists()) {
        cacheDir.mkpath("/var/cache");
    }

    QString cacheFile = QString("/var/cache/message_cache_%1.json").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz"));
    QFile file(cacheFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "[v0] Message cached to" << cacheFile;
    }
}
