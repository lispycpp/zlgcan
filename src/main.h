#ifndef MAIN_H
#define MAIN_H

#include <QCanBus>
#include <QCanBusDevice>
#if(QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QCanBusFactory>
#else
#include <QCanBusFactoryV2>
#define QCanBusFactory QCanBusFactoryV2
#endif
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

class ZLGCanBusPlugin: public QObject, public QCanBusFactory
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QCanBusFactory" FILE "plugin.json")

    Q_INTERFACES(QCanBusFactory)

public:
    QList<QCanBusDeviceInfo> availableDevices(QString* errorMessage) const override;

    QCanBusDevice* createDevice(const QString& interfaceName, QString* errorMessage) const override;
};

QT_END_NAMESPACE

#endif // MAIN_H
