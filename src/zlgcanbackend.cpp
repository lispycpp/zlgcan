#include "zlgcanbackend.h"

#include "zlgcanbackend_p.h"

#include <QFile>
#include <QLoggingCategory>
#include <QXmlStreamReader>

QT_BEGIN_NAMESPACE

ZlgCanBackend::ZlgCanBackend(const QString& interfaceName, QObject* parent): QCanBusDevice(parent), d_ptr(new ZlgCanBackendPrivate(this))
{
    Q_D(ZlgCanBackend);

    connect(&d->_read_timer, &QTimer::timeout, this, [=]() {
        d->startRead();
    });

    connect(&d->_write_timer, &QTimer::timeout, this, [=]() {
        d->startWrite();
    });

    d->setInterfaceName(interfaceName);

#if(QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    std::function<void()> f = std::bind(&ZlgCanBackend::resetController, this);
    setResetControllerFunction(f);

    std::function<CanBusStatus()> g = std::bind(&ZlgCanBackend::busStatus, this);
    setCanBusStatusGetter(g);
#endif

}

ZlgCanBackend::~ZlgCanBackend()
{
    Q_D(ZlgCanBackend);

    ZlgCanBackend::close();

    delete d;
}

bool ZlgCanBackend::open()
{
    Q_D(ZlgCanBackend);

    if(d->open())
    {
        setState(QCanBusDevice::ConnectedState);
        return true;
    }
    return false;
}

void ZlgCanBackend::close()
{
    Q_D(ZlgCanBackend);

    d->close();

    setState(QCanBusDevice::UnconnectedState);
}

#if(QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
void ZlgCanBackend::setConfigurationParameter(int key, const QVariant& value)
{
    Q_D(ZlgCanBackend);

    if(d->setConfigurationParameter(key, value))
    {
        QCanBusDevice::setConfigurationParameter(key, value);
    }
}
#else
void ZlgCanBackend::setConfigurationParameter(ConfigurationKey key, const QVariant& value)
{
    Q_D(ZlgCanBackend);

    if(d->setConfigurationParameter(key, value))
    {
        QCanBusDevice::setConfigurationParameter(key, value);
    }
}
#endif

bool ZlgCanBackend::writeFrame(const QCanBusFrame& frame)
{
    Q_D(ZlgCanBackend);

    if(Q_UNLIKELY(QCanBusDevice::ConnectedState != state()))
    {
        return false;
    }

    if(Q_UNLIKELY(!frame.isValid()))
    {
        setError(tr("Cannot write invalid QCanBusFrame"), QCanBusDevice::WriteError);
        return false;
    }

    if(Q_UNLIKELY(QCanBusFrame::DataFrame != frame.frameType() && QCanBusFrame::RemoteRequestFrame != frame.frameType() && QCanBusFrame::ErrorFrame != frame.frameType()))
    {
        setError(tr("Unable to write a frame with unacceptable type"), QCanBusDevice::WriteError);
        return false;
    }

    // if(Q_UNLIKELY(frame.hasFlexibleDataRateFormat()))
    // {
    //     setError(tr("CAN FD frame format not supported."), QCanBusDevice::WriteError);
    //     return false;
    // }

    enqueueOutgoingFrame(frame);

    if(!d->_write_timer.isActive())
    {
        d->_write_timer.start();
    }

    return true;
}

QString ZlgCanBackend::interpretErrorFrame(const QCanBusFrame& frame)
{
    Q_D(ZlgCanBackend);
    Q_UNUSED(d);
    Q_UNUSED(frame);

    return QString();
}

QList<QCanBusDeviceInfo> ZlgCanBackend::interfaces()
{
    static QList<QCanBusDeviceInfo> devices_info{};

    if(devices_info.empty())
    {
        QFile file(":devices.xml");
        if(file.open(QFile::ReadOnly))
        {
            QXmlStreamReader devices_xml_reader(&file);
            while(devices_xml_reader.readNextStartElement())
            {
                if(Q_UNLIKELY("Devices" != devices_xml_reader.name().toString()))
                {
                    break;
                }
                while(devices_xml_reader.readNextStartElement())
                {
                    if("Device" == devices_xml_reader.name().toString() && devices_xml_reader.attributes().hasAttribute("name"))
                    {
                        auto name = devices_xml_reader.attributes().value("name").toString();
                        auto fd = ("TRUE" == devices_xml_reader.attributes().value("fd").toString().toUpper());

#if(QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                        devices_info.append(QCanBusDevice::createDeviceInfo("zlgcan", name, false, fd));
#else
                        devices_info.append(std::move(QCanBusDevice::createDeviceInfo(name, false, fd)));
#endif
                    }
                    devices_xml_reader.skipCurrentElement();
                }
            }
        }
    }

    return devices_info;
}

void ZlgCanBackend::resetController()
{
    Q_D(ZlgCanBackend);

    d->resetController();
}

bool ZlgCanBackend::hasBusStatus() const
{
    return true;
}

QCanBusDevice::CanBusStatus ZlgCanBackend::busStatus()
{
    Q_D(ZlgCanBackend);

    return d->busStatus();
}

QT_END_NAMESPACE
