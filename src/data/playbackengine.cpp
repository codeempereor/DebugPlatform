#include "playbackengine.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDateTime>
#include <QDebug>
#include <algorithm>

/**
 * @file playbackengine.cpp
 * @brief 历史报文回放引擎实现
 */

PlaybackEngine::PlaybackEngine(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &PlaybackEngine::onTick);
}

PlaybackEngine::~PlaybackEngine() = default;

bool PlaybackEngine::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "PlaybackEngine: Failed to open file" << filePath
                   << file.errorString();
        return false;
    }

    QTextStream in(&file);
    QVector<PlaybackPacket> packets;

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }

        const QStringList parts = line.split(QLatin1Char('\t'));
        if (parts.size() < 3) {
            continue;
        }

        PlaybackPacket pkt;
        pkt.timestamp = QDateTime::fromString(parts.at(0),
                                              QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
        if (!pkt.timestamp.isValid()) {
            continue;
        }

        const QString dir = parts.at(1).trimmed().toUpper();
        pkt.isTx = (dir == QLatin1String("TX"));

        // 解析十六进制数据
        const QString hex = parts.at(2).trimmed().simplified().remove(QLatin1Char(' '));
        pkt.data = QByteArray::fromHex(hex.toUtf8());

        packets.append(pkt);
    }

    file.close();

    // 按时间戳升序排序
    std::sort(packets.begin(), packets.end(),
              [](const PlaybackPacket &a, const PlaybackPacket &b) {
                  return a.timestamp < b.timestamp;
              });

    m_packets = packets;
    m_position = -1;
    m_state = PlaybackState::Idle;
    emitPositionChanged();

    qDebug() << "PlaybackEngine: Loaded" << m_packets.size() << "packets from" << filePath;
    return true;
}

void PlaybackEngine::start()
{
    if (m_packets.isEmpty()) {
        return;
    }
    if (m_state == PlaybackState::Playing) {
        return;
    }

    // 从头开始
    if (m_position >= m_packets.size() - 1) {
        m_position = -1;
        emitPositionChanged();
    }

    m_state = PlaybackState::Playing;
    onTick();
}

void PlaybackEngine::pause()
{
    if (m_state != PlaybackState::Playing) {
        return;
    }
    m_state = PlaybackState::Paused;
    m_timer->stop();
}

void PlaybackEngine::resume()
{
    if (m_state != PlaybackState::Paused) {
        return;
    }
    m_state = PlaybackState::Playing;
    onTick();
}

void PlaybackEngine::stop()
{
    m_timer->stop();
    m_state = PlaybackState::Idle;
    m_position = -1;
    emitPositionChanged();
}

void PlaybackEngine::setSpeed(double speed)
{
    if (speed < 0.5) {
        speed = 0.5;
    } else if (speed > 4.0) {
        speed = 4.0;
    }
    m_speed = speed;

    // 若正在播放，按新速度调整定时器间隔
    if (m_state == PlaybackState::Playing) {
        onTick();
    }
}

void PlaybackEngine::stepForward()
{
    if (m_packets.isEmpty()) {
        return;
    }

    m_timer->stop();
    m_state = PlaybackState::Idle;

    const int next = m_position + 1;
    if (next >= m_packets.size()) {
        return;
    }

    m_position = next;
    emitPositionChanged();
    emit packetPlayed(m_packets.at(m_position));
}

void PlaybackEngine::stepBackward()
{
    if (m_packets.isEmpty()) {
        return;
    }

    m_timer->stop();
    m_state = PlaybackState::Idle;

    if (m_position <= 0) {
        m_position = -1;
    } else {
        --m_position;
    }
    emitPositionChanged();
}

int PlaybackEngine::totalPackets() const
{
    return m_packets.size();
}

int PlaybackEngine::currentPosition() const
{
    return m_position;
}

PlaybackEngine::PlaybackState PlaybackEngine::state() const
{
    return m_state;
}

void PlaybackEngine::onTick()
{
    if (m_state != PlaybackState::Playing || m_packets.isEmpty()) {
        return;
    }

    const int next = m_position + 1;
    if (next >= m_packets.size()) {
        m_state = PlaybackState::Idle;
        m_timer->stop();
        emit playbackFinished();
        return;
    }

    m_position = next;
    emitPositionChanged();
    emit packetPlayed(m_packets.at(m_position));

    // 若已是最后一个，发射完成信号
    if (m_position >= m_packets.size() - 1) {
        m_state = PlaybackState::Idle;
        emit playbackFinished();
        return;
    }

    // 计算下一次间隔
    const int interval = intervalForIndex(m_position + 1);
    const int scaled = qMax(1, static_cast<int>(interval / m_speed));
    m_timer->start(scaled);
}

int PlaybackEngine::intervalForIndex(int index) const
{
    if (index <= 0 || index >= m_packets.size()) {
        return 100; // 默认 100ms
    }
    const qint64 deltaMs = m_packets.at(index).timestamp
                               .msecsTo(m_packets.at(index - 1).timestamp);
    if (deltaMs <= 0) {
        return 100;
    }
    return static_cast<int>(deltaMs);
}

void PlaybackEngine::emitPositionChanged()
{
    emit positionChanged(m_position);
}
