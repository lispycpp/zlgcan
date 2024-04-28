#ifndef ZLGCANBACKEND_H
#define ZLGCANBACKEND_H

#include <QCanBusDevice>
#include <QCanBusDeviceInfo>
#include <QCanBusFrame>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE

class ZLGCanBackendPrivate;

class ZLGCanBackend: public QCanBusDevice
{
    Q_OBJECT

    Q_DECLARE_PRIVATE(ZLGCanBackend)

    Q_DISABLE_COPY(ZLGCanBackend)

public:
    explicit ZLGCanBackend(const QString& interfaceName, QObject* parent = nullptr);
    ~ZLGCanBackend();

    // static bool canCreate(QString* errorReason);
    static QList<QCanBusDeviceInfo> interfaces();

    virtual QString interpretErrorFrame(const QCanBusFrame& frame) override;

#if(QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#define KEY_TYPE QCanBusDevice::ConfigurationKey
    virtual void resetController() override;
#else
#define KEY_TYPE int
    void resetController();
#endif

    virtual void setConfigurationParameter(KEY_TYPE key, const QVariant& value) override;
    virtual bool writeFrame(const QCanBusFrame& frame) override;
    // virtual CanBusStatus busStatus() override;

protected:
    virtual void close() override;
    virtual bool open() override;

private:
    ZLGCanBackendPrivate* const d_ptr{nullptr};
};

QT_END_NAMESPACE

#endif // ZLGCANBACKEND_H
