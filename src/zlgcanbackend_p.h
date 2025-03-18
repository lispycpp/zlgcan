#ifndef ZLGCANBACKEND_P_H
#define ZLGCANBACKEND_P_H

#include "zlgcan/zlgcan.h"
#include "zlgcanbackend.h"

#include <QHash>
#include <QMutex>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <QTimerEvent>
#include <QVariant>

QT_BEGIN_NAMESPACE

class ZlgCanBackendPrivate
{
    Q_DECLARE_PUBLIC(ZlgCanBackend)

    Q_DISABLE_COPY(ZlgCanBackendPrivate)

public:
    explicit ZlgCanBackendPrivate(ZlgCanBackend* q);
    ~ZlgCanBackendPrivate();

public:
    bool open();
    void close();

    void setInterfaceName(const QString& interfaceName);
    bool setConfigurationParameter(int key, const QVariant& value);

    void startWrite();
    void startRead();

    void resetController();

    QCanBusDevice::CanBusStatus busStatus();

private:
    bool setConfigurations(int order);
    const QString& systemErrorString(int* errorCode = nullptr);

private:
    ZlgCanBackend* const q_ptr;

    unsigned int _device_type{};
    unsigned int _device_index{};
    unsigned int _channel_index{};
    DEVICE_HANDLE _device_handle{INVALID_DEVICE_HANDLE};
    CHANNEL_HANDLE _channel_handle{INVALID_CHANNEL_HANDLE};

    bool _fd_enabled{false};
    QHash<QCanBusDevice::ConfigurationKey, QVariant> _configurations{};

    QTimer _read_timer{};
    QTimer _write_timer{};
    QMutex _mutex{};
};

QT_END_NAMESPACE

#endif // ZLGCANBACKEND_P_H
