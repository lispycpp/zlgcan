#ifndef ZLGCANBACKEND_H
#define ZLGCANBACKEND_H

#include <QCanBusDevice>
#include <QCanBusDeviceInfo>
#include <QCanBusFrame>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE

class ZlgCanBackendPrivate;

class ZlgCanBackend: public QCanBusDevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ZlgCanBackend)
    Q_DISABLE_COPY(ZlgCanBackend)

public:
    explicit ZlgCanBackend(const QString& interfaceName, QObject* parent = nullptr);
    ~ZlgCanBackend();

    virtual bool open() override;
    virtual void close() override;

#if(QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    virtual void setConfigurationParameter(int key, const QVariant& value) override;

    virtual bool writeFrame(const QCanBusFrame& newData) override;

    virtual QString interpretErrorFrame(const QCanBusFrame& errorFrame) override;

    static bool canCreate(QString* errorReason);
    static QList<QCanBusDeviceInfo> interfaces();

private:
    void resetController();
    bool hasBusStatus() const;
    CanBusStatus busStatus();
#else
    virtual void setConfigurationParameter(ConfigurationKey key, const QVariant& value) override;

    virtual bool writeFrame(const QCanBusFrame& newData) override;

    virtual QString interpretErrorFrame(const QCanBusFrame& errorFrame) override;

    static QList<QCanBusDeviceInfo> interfaces();

    virtual void resetController() override;
    virtual bool hasBusStatus() const override;
    virtual CanBusStatus busStatus() override;
    // virtual QCanBusDeviceInfo deviceInfo() const override;
#endif

private:
    ZlgCanBackendPrivate* const d_ptr{nullptr};
};

QT_END_NAMESPACE

#endif // ZLGCANBACKEND_H
