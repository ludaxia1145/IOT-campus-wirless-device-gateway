#include "audioplayer.h"
#include <QDebug>

AudioPlayer* AudioPlayer::m_instance = nullptr;

AudioPlayer::AudioPlayer(QObject *parent) : QObject(parent)
{
    // 使用Qt资源系统或绝对路径
    // 这里使用/usr/share目录存放音频文件
    successSound = new QSound("/usr/share/smart_schedule/success.wav", this);
    newMessageSound = new QSound("/usr/share/smart_schedule/new_message.wav", this);

    qDebug() << "[v0] AudioPlayer initialized";
}

AudioPlayer* AudioPlayer::instance()
{
    if (!m_instance) {
        m_instance = new AudioPlayer();
    }
    return m_instance;
}

void AudioPlayer::playSuccess()
{
    qDebug() << "[v0] Playing success sound";
    successSound->play();
}

void AudioPlayer::playNewMessage()
{
    qDebug() << "[v0] Playing new message sound";
    newMessageSound->play();
}
