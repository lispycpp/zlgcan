#include "main.h"

#include "ZLGCanBackend.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_CANBUS_PLUGINS_ZLGCAN, "qt.canbus.plugins.zlgcan")

QList<QCanBusDeviceInfo> ZLGCanBusPlugin::availableDevices(QString* errorMessage) const
{
    Q_UNUSED(errorMessage)

    return ZlgCanBackend::interfaces();
}

QCanBusDevice* ZLGCanBusPlugin::createDevice(const QString& interfaceName, QString* errorMessage) const
{
    Q_UNUSED(errorMessage)

    auto device = new ZlgCanBackend(interfaceName);
    return device;
}

QT_END_NAMESPACE
