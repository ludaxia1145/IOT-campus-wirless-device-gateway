#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QMap>

class NetworkClient : public QObject
{
    Q_OBJECT
public:
    explicit NetworkClient(QObject *parent = nullptr);
    ~NetworkClient();

    void connectToServer(const QString &host, quint16 port);
    void sendMessage(const QString &type, const QJsonObject &data);
    bool isConnected() const;

signals:
    void courseInfoReceived(const QJsonObject &info);
    void noticeListReceived(const QJsonObject &data);
    void noticeDetailReceived(const QJsonObject &data);
    void librarySeatsReceived(const QJsonObject &data);
    void attendanceListReceived(const QJsonObject &list);
    void responseReceived(const QString &type, const QJsonObject &data);
    void connectionStatusChanged(bool connected);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
    void onHeartbeatTimeout();
    void onReconnectTimeout();
    void onRetryTimeout();

private:
    void startHeartbeat();
    void stopHeartbeat();
    void startReconnect();
    void sendMessageWithRetry(const QString &type, const QJsonObject &data);
    void saveToCache(const QString &type, const QJsonObject &data);
    void retryFailedMessage();

    struct PendingMessage {
        QString type;
        QJsonObject data;
        int retryCount;
    };

    QTcpSocket *socket;
    QString buffer;

    QTimer *heartbeatTimer;
    QTimer *reconnectTimer;
    QTimer *retryTimer;
    int reconnectCount;
    int currentReconnectInterval;
    const int initialInterval = 3000;
    const int maxInterval = 30000;
    const int maxReconnectAttempts = 50;

    QMap<int, PendingMessage> pendingMessages;
    int messageId;
    const int retryInterval = 3000;
    const int maxRetries = 3;
};

#endif
