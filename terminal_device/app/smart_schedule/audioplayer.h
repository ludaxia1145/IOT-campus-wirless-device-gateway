#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>
#include <QSound>

class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    static AudioPlayer* instance();

    void playSuccess();
    void playNewMessage();

private:
    // 私有构造函数，确保单例
    explicit AudioPlayer(QObject *parent = nullptr);
    Q_DISABLE_COPY(AudioPlayer)

    static AudioPlayer *m_instance;
    QSound *successSound;
    QSound *newMessageSound;
};

#endif // AUDIOPLAYER_H
