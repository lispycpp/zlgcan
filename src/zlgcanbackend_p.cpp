#include "zlgcanbackend_p.h"

#include "zlgcanbackend.h"

#include <windows.h>
#undef SendMessage
#undef ERROR
#include <QFile>
#include <QLoggingCategory>
#include <QXmlStreamReader>
// #include <stdexcept>
#include <zlgcan/zlgcan.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_CANBUS_PLUGINS_ZLGCAN)

namespace zlg
{
    enum class ConfigureFunction
    {
        ZCAN_INITCAN,
        ZCAN_SETVALUE,
        IPROPERTY_SETVALUE,
    };
    enum class ConfigureOrder
    {
        BEFORE_INIT_CAN,
        BEFORE_START_CAN,
        AFTER_START_CAN,
    };

    struct Configuration
    {
        QCanBusDevice::ConfigurationKey key{QCanBusDevice::UserKey};
        bool configurable{false};
        ConfigureFunction function{ConfigureFunction::ZCAN_SETVALUE};
        ConfigureOrder order{ConfigureOrder::BEFORE_INIT_CAN};
    };

    struct Device
    {
        QString name{};
        unsigned int type{0};
        bool fd{false};
        unsigned int channels{0};
        QHash<QCanBusDevice::ConfigurationKey, Configuration> configurations{};
        QSet<unsigned int> bitrate{};
        QSet<unsigned int> data_field_bitrate{};
    };

    const QHash<unsigned int, Device>& get_devices()
    {
        static QHash<unsigned int, Device> devices{};

        if(devices.empty())
        {
            QFile file(":devices.xml");
            if(file.open(QFile::ReadOnly))
            {
                QXmlStreamReader devices_xml_reader(&file);

                auto read_bitrate = [](QXmlStreamReader& xml, QSet<unsigned int>& bitRate) {
                    while(xml.readNextStartElement())
                    {
                        if("VALUE" == xml.name().toString().toUpper())
                        {
                            auto value{xml.readElementText().toUInt()};
                            if(value)
                            {
                                bitRate << value;
                            }
                        }
                    }
                };

                auto read_key = [](QXmlStreamReader& xml, Configuration& configuration) {
                    configuration.configurable = ("TRUE" == xml.attributes().value("configurable").toString().toUpper());

                    if("ZCAN_INITCAN" == xml.attributes().value("method").toString().toUpper())
                    {
                        configuration.function = ConfigureFunction::ZCAN_INITCAN;
                    }
                    else if("ZCAN_SETVALUE" == xml.attributes().value("method").toString().toUpper())
                    {
                        configuration.function = ConfigureFunction::ZCAN_SETVALUE;
                    }
                    else if("IPROPERTY_SETVALUE" == xml.attributes().value("method").toString().toUpper())
                    {
                        configuration.function = ConfigureFunction::IPROPERTY_SETVALUE;
                    }

                    if("BEFORE_INIT_CAN" == xml.attributes().value("sequence").toString().toUpper())
                    {
                        configuration.order = ConfigureOrder::BEFORE_INIT_CAN;
                    }
                    else if("BEFORE_START_CAN" == xml.attributes().value("sequence").toString().toUpper())
                    {
                        configuration.order = ConfigureOrder::BEFORE_START_CAN;
                    }
                    else if("AFTER_START_CAN" == xml.attributes().value("sequence").toString().toUpper())
                    {
                        configuration.order = ConfigureOrder::AFTER_START_CAN;
                    }
                };

                auto read_configuration = [&](Device& device) {
                    while(devices_xml_reader.readNextStartElement())
                    {
                        if(devices_xml_reader.attributes().hasAttribute("configurable") && ("FALSE" == devices_xml_reader.attributes().value("configurable").toString().toUpper()))
                        {
                            devices_xml_reader.skipCurrentElement();
                            continue;
                        }

                        auto keyName{devices_xml_reader.name().toString().toUpper()};
                        if("RAWFILTE" == keyName)
                        {
                            auto& configuration = device.configurations[QCanBusDevice::RawFilterKey];
                            configuration.key = QCanBusDevice::RawFilterKey;
                            read_key(devices_xml_reader, configuration);
                            devices_xml_reader.skipCurrentElement();
                        }
                        else if("ERRORFILTER" == keyName)
                        {
                            auto& configuration = device.configurations[QCanBusDevice::ErrorFilterKey];
                            configuration.key = QCanBusDevice::ErrorFilterKey;
                            read_key(devices_xml_reader, configuration);
                            devices_xml_reader.skipCurrentElement();
                        }
                        else if("LOOPBACK" == keyName)
                        {
                            auto& configuration = device.configurations[QCanBusDevice::LoopbackKey];
                            configuration.key = QCanBusDevice::LoopbackKey;
                            read_key(devices_xml_reader, configuration);
                            devices_xml_reader.skipCurrentElement();
                        }
                        else if("RECEIVEOWN" == keyName)
                        {
                            auto& configuration = device.configurations[QCanBusDevice::ReceiveOwnKey];
                            configuration.key = QCanBusDevice::ReceiveOwnKey;
                            read_key(devices_xml_reader, configuration);
                            devices_xml_reader.skipCurrentElement();
                        }
                        else if("BITRATE" == keyName)
                        {
                            auto& configuration = device.configurations[QCanBusDevice::BitRateKey];
                            configuration.key = QCanBusDevice::BitRateKey;
                            read_key(devices_xml_reader, configuration);
                            read_bitrate(devices_xml_reader, device.bitrate);
                            // devicesXml.skipCurrentElement();
                        }
                        else if("CANFD" == keyName)
                        {
                            auto& configuration = device.configurations[QCanBusDevice::CanFdKey];
                            configuration.key = QCanBusDevice::CanFdKey;
                            read_key(devices_xml_reader, configuration);
                            devices_xml_reader.skipCurrentElement();
                        }
                        else if("DATABITRATE" == keyName)
                        {
                            auto& configuration = device.configurations[QCanBusDevice::DataBitRateKey];
                            configuration.key = QCanBusDevice::DataBitRateKey;
                            read_key(devices_xml_reader, configuration);
                            read_bitrate(devices_xml_reader, device.data_field_bitrate);
                            // devicesXml.skipCurrentElement();
                        }
                    }
                };

                auto read_configurations = [&](Device& device) {
                    while(devices_xml_reader.readNextStartElement())
                    {
                        if("CONFIGURATIONS" == devices_xml_reader.name().toString().toUpper())
                        {
                            read_configuration(device);
                        }
                        else
                        {
                            devices_xml_reader.skipCurrentElement();
                        }
                    }
                };

                auto read_device = [&](Device& device) {
                    device.name = devices_xml_reader.attributes().value("name").toString().toUpper();
                    device.type = devices_xml_reader.attributes().value("type").toUInt();
                    device.fd = ("TRUE" == devices_xml_reader.attributes().value("fd").toLatin1().toUpper());
                    device.channels = devices_xml_reader.attributes().value("channels").toUInt();
                    read_configurations(device);
                };

                auto readDevices = [&](QHash<unsigned int, Device>& devices) {
                    while(devices_xml_reader.readNextStartElement())
                    {
                        if("DEVICES" == devices_xml_reader.name().toString().toUpper())
                        {
                            while(devices_xml_reader.readNextStartElement())
                            {
                                if("DEVICE" == devices_xml_reader.name().toString().toUpper())
                                {
                                    Device device{};
                                    read_device(device);
                                    devices[device.type] = device;
                                }
                                else
                                {
                                    devices_xml_reader.skipCurrentElement();
                                }
                            }
                        }
                        else
                        {
                            devices_xml_reader.skipCurrentElement();
                            // return;
                        }
                    }
                };

                readDevices(devices);
            }
        }
        return devices;
    }

    unsigned int get_device_type(QString device_name)
    {
        static QHash<QString, unsigned int> dict{};
        if(dict.empty())
        {
            auto& devices{get_devices()};
            for(auto& item: devices)
            {
                dict[item.name] = item.type;
            }
        }
        device_name = device_name.toUpper();
        if(dict.contains(device_name))
        {
            return dict[device_name];
        }
        return 0;
    }

    class Loader
    {
        Loader(const Loader&) = delete;
        Loader& operator=(const Loader&) = delete;
        Loader(Loader&&) = delete;
        Loader& operator=(Loader&&) = delete;

    public:
        static Loader& instance()
        {
            static Loader s_instance{};
            return s_instance;
        }

    private:
        explicit Loader()
        {
            if(!(_handle = LoadLibraryA("zlgcan.dll")))
            {
                qCCritical(QT_CANBUS_PLUGINS_ZLGCAN, "Cannot load library: %ls", qUtf16Printable("zlgcan.dll"));
                // throw std::runtime_error("Load library \"zlgcan.dll\" failed.");
            }

            ZCAN_OpenDevice = (pf_ZCAN_OpenDevice)GetProcAddress(_handle, "ZCAN_OpenDevice");
            ZCAN_CloseDevice = (pf_ZCAN_CloseDevice)GetProcAddress(_handle, "ZCAN_CloseDevice");
            ZCAN_GetDeviceInf = (pf_ZCAN_GetDeviceInf)GetProcAddress(_handle, "ZCAN_GetDeviceInf");
            ZCAN_IsDeviceOnLine = (pf_ZCAN_IsDeviceOnLine)GetProcAddress(_handle, "ZCAN_IsDeviceOnLine");
            ZCAN_InitCAN = (pf_ZCAN_InitCAN)GetProcAddress(_handle, "ZCAN_InitCAN");
            ZCAN_StartCAN = (pf_ZCAN_StartCAN)GetProcAddress(_handle, "ZCAN_StartCAN");
            ZCAN_ResetCAN = (pf_ZCAN_ResetCAN)GetProcAddress(_handle, "ZCAN_ResetCAN");
            ZCAN_ClearBuffer = (pf_ZCAN_ClearBuffer)GetProcAddress(_handle, "ZCAN_ClearBuffer");
            ZCAN_ReadChannelErrInfo = (pf_ZCAN_ReadChannelErrInfo)GetProcAddress(_handle, "ZCAN_ReadChannelErrInfo");
            ZCAN_ReadChannelStatus = (pf_ZCAN_ReadChannelStatus)GetProcAddress(_handle, "ZCAN_ReadChannelStatus");
            ZCAN_GetReceiveNum = (pf_ZCAN_GetReceiveNum)GetProcAddress(_handle, "ZCAN_GetReceiveNum");
            ZCAN_Transmit = (pf_ZCAN_Transmit)GetProcAddress(_handle, "ZCAN_Transmit");
            ZCAN_Receive = (pf_ZCAN_Receive)GetProcAddress(_handle, "ZCAN_Receive");
            ZCAN_TransmitFD = (pf_ZCAN_TransmitFD)GetProcAddress(_handle, "ZCAN_TransmitFD");
            ZCAN_ReceiveFD = (pf_ZCAN_ReceiveFD)GetProcAddress(_handle, "ZCAN_ReceiveFD");
            ZCAN_TransmitData = (pf_ZCAN_TransmitData)GetProcAddress(_handle, "ZCAN_TransmitData");
            ZCAN_ReceiveData = (pf_ZCAN_ReceiveData)GetProcAddress(_handle, "ZCAN_ReceiveData");
            ZCAN_SetValue = (pf_ZCAN_SetValue)GetProcAddress(_handle, "ZCAN_SetValue");
            ZCAN_GetValue = (pf_ZCAN_GetValue)GetProcAddress(_handle, "ZCAN_GetValue");
            GetIProperty = (pf_GetIProperty)GetProcAddress(_handle, "GetIProperty");
            ReleaseIProperty = (pf_ReleaseIProperty)GetProcAddress(_handle, "ReleaseIProperty");
            ZCLOUD_SetServerInfo = (pf_ZCLOUD_SetServerInfo)GetProcAddress(_handle, "ZCLOUD_SetServerInfo");
            ZCLOUD_ConnectServer = (pf_ZCLOUD_ConnectServer)GetProcAddress(_handle, "ZCLOUD_ConnectServer");
            ZCLOUD_IsConnected = (pf_ZCLOUD_IsConnected)GetProcAddress(_handle, "ZCLOUD_IsConnected");
            ZCLOUD_DisconnectServer = (pf_ZCLOUD_DisconnectServer)GetProcAddress(_handle, "ZCLOUD_DisconnectServer");
            ZCLOUD_GetUserData = (pf_ZCLOUD_GetUserData)GetProcAddress(_handle, "ZCLOUD_GetUserData");
            ZCLOUD_ReceiveGPS = (pf_ZCLOUD_ReceiveGPS)GetProcAddress(_handle, "ZCLOUD_ReceiveGPS");
            ZCAN_InitLIN = (pf_ZCAN_InitLIN)GetProcAddress(_handle, "ZCAN_InitLIN");
            ZCAN_StartLIN = (pf_ZCAN_StartLIN)GetProcAddress(_handle, "ZCAN_StartLIN");
            ZCAN_ResetLIN = (pf_ZCAN_ResetLIN)GetProcAddress(_handle, "ZCAN_ResetLIN");
            ZCAN_TransmitLIN = (pf_ZCAN_TransmitLIN)GetProcAddress(_handle, "ZCAN_TransmitLIN");
            ZCAN_GetLINReceiveNum = (pf_ZCAN_GetLINReceiveNum)GetProcAddress(_handle, "ZCAN_GetLINReceiveNum");
            ZCAN_ReceiveLIN = (pf_ZCAN_ReceiveLIN)GetProcAddress(_handle, "ZCAN_ReceiveLIN");
            ZCAN_SetLINSlaveMsg = (pf_ZCAN_SetLINSlaveMsg)GetProcAddress(_handle, "ZCAN_SetLINSlaveMsg");
            ZCAN_ClearLINSlaveMsg = (pf_ZCAN_ClearLINSlaveMsg)GetProcAddress(_handle, "ZCAN_ClearLINSlaveMsg");
        }

        ~Loader()
        {
            if(_handle)
            {
                FreeLibrary(_handle);
            }
        }

    public:
        typedef DEVICE_HANDLE (*pf_ZCAN_OpenDevice)(UINT, UINT, UINT);
        pf_ZCAN_OpenDevice ZCAN_OpenDevice{};

        typedef UINT (*pf_ZCAN_CloseDevice)(DEVICE_HANDLE);
        pf_ZCAN_CloseDevice ZCAN_CloseDevice{};

        typedef UINT (*pf_ZCAN_GetDeviceInf)(DEVICE_HANDLE, ZCAN_DEVICE_INFO*);
        pf_ZCAN_GetDeviceInf ZCAN_GetDeviceInf{};

        typedef UINT (*pf_ZCAN_IsDeviceOnLine)(DEVICE_HANDLE);
        pf_ZCAN_IsDeviceOnLine ZCAN_IsDeviceOnLine{};

        typedef CHANNEL_HANDLE (*pf_ZCAN_InitCAN)(DEVICE_HANDLE, UINT, ZCAN_CHANNEL_INIT_CONFIG*);
        pf_ZCAN_InitCAN ZCAN_InitCAN{};

        typedef UINT (*pf_ZCAN_StartCAN)(CHANNEL_HANDLE);
        pf_ZCAN_StartCAN ZCAN_StartCAN{};

        typedef UINT (*pf_ZCAN_ResetCAN)(CHANNEL_HANDLE);
        pf_ZCAN_ResetCAN ZCAN_ResetCAN{};

        typedef UINT (*pf_ZCAN_ClearBuffer)(CHANNEL_HANDLE);
        pf_ZCAN_ClearBuffer ZCAN_ClearBuffer{};

        typedef UINT (*pf_ZCAN_ReadChannelErrInfo)(CHANNEL_HANDLE, ZCAN_CHANNEL_ERR_INFO*);
        pf_ZCAN_ReadChannelErrInfo ZCAN_ReadChannelErrInfo{};

        typedef UINT (*pf_ZCAN_ReadChannelStatus)(CHANNEL_HANDLE, ZCAN_CHANNEL_STATUS*);
        pf_ZCAN_ReadChannelStatus ZCAN_ReadChannelStatus{};

        typedef UINT (*pf_ZCAN_GetReceiveNum)(CHANNEL_HANDLE, BYTE);
        pf_ZCAN_GetReceiveNum ZCAN_GetReceiveNum{};

        typedef UINT (*pf_ZCAN_Transmit)(CHANNEL_HANDLE, ZCAN_Transmit_Data*, UINT);
        pf_ZCAN_Transmit ZCAN_Transmit{};

        typedef UINT (*pf_ZCAN_Receive)(CHANNEL_HANDLE, ZCAN_Receive_Data*, UINT, int);
        pf_ZCAN_Receive ZCAN_Receive{};

        typedef UINT (*pf_ZCAN_TransmitFD)(CHANNEL_HANDLE, ZCAN_TransmitFD_Data*, UINT);
        pf_ZCAN_TransmitFD ZCAN_TransmitFD{};

        typedef UINT (*pf_ZCAN_ReceiveFD)(CHANNEL_HANDLE, ZCAN_ReceiveFD_Data*, UINT, int);
        pf_ZCAN_ReceiveFD ZCAN_ReceiveFD{};

        typedef UINT (*pf_ZCAN_TransmitData)(DEVICE_HANDLE, ZCANDataObj*, UINT);
        pf_ZCAN_TransmitData ZCAN_TransmitData{};

        typedef UINT (*pf_ZCAN_ReceiveData)(DEVICE_HANDLE, ZCANDataObj*, UINT, int);
        pf_ZCAN_ReceiveData ZCAN_ReceiveData{};

        typedef UINT (*pf_ZCAN_SetValue)(DEVICE_HANDLE, const char*, const void*);
        pf_ZCAN_SetValue ZCAN_SetValue{};

        typedef const void* (*pf_ZCAN_GetValue)(DEVICE_HANDLE, const char*);
        pf_ZCAN_GetValue ZCAN_GetValue{};

        typedef IProperty* (*pf_GetIProperty)(DEVICE_HANDLE);
        pf_GetIProperty GetIProperty{};

        typedef UINT (*pf_ReleaseIProperty)(IProperty*);
        pf_ReleaseIProperty ReleaseIProperty{};

        typedef void (*pf_ZCLOUD_SetServerInfo)(const char*, unsigned short, const char*, unsigned short);
        pf_ZCLOUD_SetServerInfo ZCLOUD_SetServerInfo{};

        typedef UINT (*pf_ZCLOUD_ConnectServer)(const char*, const char*);
        pf_ZCLOUD_ConnectServer ZCLOUD_ConnectServer{};

        typedef bool (*pf_ZCLOUD_IsConnected)();
        pf_ZCLOUD_IsConnected ZCLOUD_IsConnected{};

        typedef UINT (*pf_ZCLOUD_DisconnectServer)();
        pf_ZCLOUD_DisconnectServer ZCLOUD_DisconnectServer{};

        typedef const ZCLOUD_USER_DATA* (*pf_ZCLOUD_GetUserData)(int);
        pf_ZCLOUD_GetUserData ZCLOUD_GetUserData{};

        typedef UINT (*pf_ZCLOUD_ReceiveGPS)(DEVICE_HANDLE, ZCLOUD_GPS_FRAME*, UINT, int);
        pf_ZCLOUD_ReceiveGPS ZCLOUD_ReceiveGPS{};

        typedef CHANNEL_HANDLE (*pf_ZCAN_InitLIN)(DEVICE_HANDLE, UINT, PZCAN_LIN_INIT_CONFIG);
        pf_ZCAN_InitLIN ZCAN_InitLIN{};

        typedef UINT (*pf_ZCAN_StartLIN)(CHANNEL_HANDLE);
        pf_ZCAN_StartLIN ZCAN_StartLIN{};

        typedef UINT (*pf_ZCAN_ResetLIN)(CHANNEL_HANDLE);
        pf_ZCAN_ResetLIN ZCAN_ResetLIN{};

        typedef UINT (*pf_ZCAN_TransmitLIN)(CHANNEL_HANDLE, PZCAN_LIN_MSG, UINT);
        pf_ZCAN_TransmitLIN ZCAN_TransmitLIN{};

        typedef UINT (*pf_ZCAN_GetLINReceiveNum)(CHANNEL_HANDLE);
        pf_ZCAN_GetLINReceiveNum ZCAN_GetLINReceiveNum{};

        typedef UINT (*pf_ZCAN_ReceiveLIN)(CHANNEL_HANDLE, PZCAN_LIN_MSG, UINT, int);
        pf_ZCAN_ReceiveLIN ZCAN_ReceiveLIN{};

        typedef UINT (*pf_ZCAN_SetLINSlaveMsg)(CHANNEL_HANDLE, PZCAN_LIN_MSG, UINT);
        pf_ZCAN_SetLINSlaveMsg ZCAN_SetLINSlaveMsg{};

        typedef UINT (*pf_ZCAN_ClearLINSlaveMsg)(CHANNEL_HANDLE, BYTE*, UINT);
        pf_ZCAN_ClearLINSlaveMsg ZCAN_ClearLINSlaveMsg{};

    private:
        HINSTANCE _handle{NULL};
    };
} //namespace zlg

ZlgCanBackendPrivate::ZlgCanBackendPrivate(ZlgCanBackend* q): q_ptr(q) {}

ZlgCanBackendPrivate::~ZlgCanBackendPrivate()
{
    close();
}

bool ZlgCanBackendPrivate::open()
{
    Q_Q(ZlgCanBackend);

    static auto& ZCAN_OpenDevice{zlg::Loader::instance().ZCAN_OpenDevice};
    static auto& ZCAN_InitCAN{zlg::Loader::instance().ZCAN_InitCAN};
    static auto& ZCAN_StartCAN{zlg::Loader::instance().ZCAN_StartCAN};
    static auto& ZCAN_CloseDevice{zlg::Loader::instance().ZCAN_CloseDevice};

    if(!_device_handle && !_channel_handle && _device_type)
    {
        const QMutexLocker locker{&_mutex};

        _device_handle = ZCAN_OpenDevice(_device_type, _device_index, 0);
        if(_device_handle)
        {
            setConfigurations(static_cast<int>(zlg::ConfigureOrder::BEFORE_INIT_CAN));
            ZCAN_CHANNEL_INIT_CONFIG config{};
            ::memset(&config, 0, sizeof(config));
            config.can_type = _fd_enabled ? 1 : 0;
            if(config.can_type)
            {
                config.canfd.acc_code = 0;
                config.canfd.acc_mask = 0xFFFFFFFF;
                // config.canfd.filter = 1;
                // config.canfd.mode = 0;
            }
            else
            {
                config.can.acc_code = 0;
                config.can.acc_mask = 0xFFFFFFFF;
                // config.can.filter = 1;
                // config.can.mode = 0;
            }
            _channel_handle = ZCAN_InitCAN(_device_handle, _channel_index, &config);
            if(_channel_handle)
            {
                setConfigurations(static_cast<int>(zlg::ConfigureOrder::BEFORE_START_CAN));
                if(STATUS_OK == ZCAN_StartCAN(_channel_handle))
                {
                    setConfigurations(static_cast<int>(zlg::ConfigureOrder::AFTER_START_CAN));
                    _read_timer.start();
                    return true;
                }
            }
        }

        if(_device_handle)
        {
            ZCAN_CloseDevice(_device_handle);
            _device_handle = INVALID_DEVICE_HANDLE;
            _channel_handle = INVALID_CHANNEL_HANDLE;
        }
        auto& error_string{systemErrorString()};
        q->setError(error_string, QCanBusDevice::CanBusError::ConnectionError);
    }
    return _channel_handle;
}

void ZlgCanBackendPrivate::close()
{
    static auto& ZCAN_CloseDevice{zlg::Loader::instance().ZCAN_CloseDevice};

    _read_timer.stop();
    _write_timer.stop();

    if(_device_handle)
    {
        const QMutexLocker channel_locker{&_mutex};

        ZCAN_CloseDevice(_device_handle);
        _device_handle = INVALID_DEVICE_HANDLE;
        _channel_handle = INVALID_CHANNEL_HANDLE;
    }
}

void ZlgCanBackendPrivate::setInterfaceName(const QString& interfaceName)
{
    _device_index = 0;
    _channel_index = 0;
    QString device_name{interfaceName.toUpper()};

    /*!
     * parsing interfaceName, like this
     * <?xml version="1.0" encoding="utf-8"?>
     * <Device type="ZCAN_USBCAN_E_U" index="0" channel="0" />
     */
    QXmlStreamReader interface_name_xml_reader{interfaceName};
    while(interface_name_xml_reader.readNextStartElement())
    {
        if("DEVICE" == interface_name_xml_reader.name().toString().toUpper())
        {
            device_name = interface_name_xml_reader.attributes().value("type").toString().toUpper();
            _device_index = interface_name_xml_reader.attributes().value("index").toUInt();
            _channel_index = interface_name_xml_reader.attributes().value("channel").toUInt();
            break;
        }
    }
    _device_type = zlg::get_device_type(device_name);
    _fd_enabled = _device_type ? zlg::get_devices()[_device_type].fd : false;
}

bool ZlgCanBackendPrivate::setConfigurationParameter(int key, const QVariant& value)
{
    auto configuration_key{static_cast<QCanBusDevice::ConfigurationKey>(key)};
    if(QCanBusDevice::ConfigurationKey::CanFdKey == configuration_key)
    {
        _fd_enabled = _fd_enabled && value.isValid() && value.toBool();
    }
    else
    {
        _configurations[configuration_key] = value;
    }
    return true;
}

void ZlgCanBackendPrivate::startWrite()
{
    Q_Q(ZlgCanBackend);

    static auto& ZCAN_Transmit{zlg::Loader::instance().ZCAN_Transmit};
    static auto& ZCAN_TransmitFD{zlg::Loader::instance().ZCAN_TransmitFD};

    if(_channel_handle && q->hasOutgoingFrames())
    {
        ZCAN_Transmit_Data data[64]{};
        ZCAN_TransmitFD_Data fd_data[64]{};

        auto write_frame{[&]() {
            auto len{0U};
            constexpr auto data_size{sizeof(data) / sizeof(data[0])};
            while(q->hasOutgoingFrames() && len < data_size)
            {
                const auto frame{q->dequeueOutgoingFrame()};
                if(Q_UNLIKELY(!frame.isValid()))
                {
                    QString error_string{"Invalid frame."};
                    qCWarning(QT_CANBUS_PLUGINS_ZLGCAN(), error_string.toLatin1());
                    // q->setError(error_string, QCanBusDevice::WriteError);
                    continue;
                }
                if(Q_UNLIKELY(frame.hasFlexibleDataRateFormat()))
                {
                    QString error_string{"Cannot send CAN FD frame format as CAN FD is not enabled."};
                    qCWarning(QT_CANBUS_PLUGINS_ZLGCAN(), error_string.toLatin1());
                    q->setError(error_string, QCanBusDevice::WriteError);
                    continue;
                }
                const auto payload{frame.payload()};
                if(Q_UNLIKELY(payload.size() > sizeof(ZCAN_Transmit_Data::frame.data) / sizeof(ZCAN_Transmit_Data::frame.data[0])))
                {
                    QString error_string{"Cannot write frame with payload size %1."};
                    qCWarning(QT_CANBUS_PLUGINS_ZLGCAN(), error_string.arg(payload.size()).toLatin1());
                    // q->setError(error_string, QCanBusDevice::WriteError);
                    continue;
                }

                ::memset(&data[len], 0, sizeof(ZCAN_Transmit_Data));
                data[len].frame.can_id = MAKE_CAN_ID(frame.frameId(), frame.hasExtendedFrameFormat(), frame.frameType() == QCanBusFrame::RemoteRequestFrame, frame.frameType() == QCanBusFrame::ErrorFrame);
                data[len].frame.can_dlc = payload.size();
                ::memcpy_s(data[len].frame.data, sizeof(ZCAN_Transmit_Data::frame.data), payload, payload.size());
                ++len;
            }

            auto result{0U};
            {
                const QMutexLocker locker{&_mutex};
                result = ZCAN_Transmit(_channel_handle, data, len);
            }
            return result;
        }};

        auto write_frame_fd{[&]() {
            auto len{0U};
            constexpr auto fd_data_size{sizeof(fd_data) / sizeof(fd_data[0])};
            while(q->hasOutgoingFrames() && len < fd_data_size)
            {
                const QCanBusFrame frame{q->dequeueOutgoingFrame()};
                if(Q_UNLIKELY(!frame.isValid()))
                {
                    QString error_string{"Invalid frame."};
                    qCWarning(QT_CANBUS_PLUGINS_ZLGCAN(), error_string.toLatin1());
                    // q->setError(error_string, QCanBusDevice::WriteError);
                    continue;
                }
                const QByteArray payload{frame.payload()};
                if(Q_UNLIKELY(payload.size() > sizeof(ZCAN_TransmitFD_Data::frame.data) / sizeof(ZCAN_TransmitFD_Data::frame.data[0])))
                {
                    QString error_string{"Cannot write frame with payload size %1."};
                    qCWarning(QT_CANBUS_PLUGINS_ZLGCAN(), error_string.arg(payload.size()).toLatin1());
                    // q->setError(error_string, QCanBusDevice::WriteError);
                    continue;
                }

                ::memset(&fd_data[len], 0, sizeof(ZCAN_TransmitFD_Data));
                fd_data[len].frame.can_id = MAKE_CAN_ID(frame.frameId(), frame.hasExtendedFrameFormat(), frame.frameType() == QCanBusFrame::RemoteRequestFrame, frame.frameType() == QCanBusFrame::ErrorFrame);
                fd_data[len].frame.flags |= frame.hasBitrateSwitch() ? CANFD_BRS : 0;
                fd_data[len].frame.len = payload.size();
                ::memcpy_s(fd_data[len].frame.data, sizeof(ZCAN_TransmitFD_Data::frame.data), payload, payload.size());
                ++len;
            }

            auto result{0U};
            {
                const QMutexLocker locker{&_mutex};
                result = ZCAN_TransmitFD(_channel_handle, fd_data, len);
            }
            return result;
        }};

        auto count{0U};
        while(q->hasOutgoingFrames())
        {
            auto result = _fd_enabled ? write_frame_fd() : write_frame();
            if(result > 0)
            {
                count += result;
            }
            else
            {
                auto& error_string{systemErrorString()};
                qCWarning(QT_CANBUS_PLUGINS_ZLGCAN(), error_string.toLatin1());
                q->setError(error_string, QCanBusDevice::CanBusError::WriteError);
            }
        }
        if(count)
        {
            emit q->framesWritten(count);
        }
    }
    else
    {
        _write_timer.stop();
    }
}

void ZlgCanBackendPrivate::startRead()
{
    Q_Q(ZlgCanBackend);

    static auto& ZCAN_GetReceiveNum{zlg::Loader::instance().ZCAN_GetReceiveNum};
    static auto& ZCAN_Receive{zlg::Loader::instance().ZCAN_Receive};
    static auto& ZCAN_ReceiveFD{zlg::Loader::instance().ZCAN_ReceiveFD};

    if(_channel_handle)
    {
        QCanBusFrame frame{};
        QVector<QCanBusFrame> frames{};
        frames.reserve(256);

        ZCAN_Receive_Data data[64]{};
        ZCAN_ReceiveFD_Data fd_data[64]{};

        auto receive_frame{[&](unsigned int size) {
            constexpr auto data_size{sizeof(data) / sizeof(data[0])};
            size = size > data_size ? data_size : size;
            ::memset(&data, 0, sizeof(data[0]) * size);
            auto result{0U};
            {
                const QMutexLocker locker{&_mutex};
                result = ZCAN_Receive(_channel_handle, data, size, 0);
            }
            for(unsigned int i = 0; i < result; ++i)
            {
                frame.setFrameId(GET_ID(data[i].frame.can_id));
                frame.setPayload(QByteArray(reinterpret_cast<char*>(data[i].frame.data), int(data[i].frame.can_dlc)));
                frame.setTimeStamp(QCanBusFrame::TimeStamp::fromMicroSeconds(data[i].timestamp));
                frame.setExtendedFrameFormat(IS_EFF(data[i].frame.can_id));
                if(IS_ERR(data[i].frame.can_id))
                {
                    frame.setFrameType(QCanBusFrame::ErrorFrame);
                }
                else if(IS_RTR(data[i].frame.can_id))
                {
                    frame.setFrameType(QCanBusFrame::RemoteRequestFrame);
                }
                else
                {
                    frame.setFrameType(QCanBusFrame::DataFrame);
                }
                frames.append(frame);
            }
        }};

        auto receive_frame_fd{[&](unsigned int size) {
            constexpr auto fd_data_size{sizeof(fd_data) / sizeof(fd_data[0])};
            size = size > fd_data_size ? fd_data_size : size;
            ::memset(&fd_data, 0, sizeof(fd_data[0]) * size);
            auto result{0U};
            {
                const QMutexLocker locker{&_mutex};
                result = ZCAN_ReceiveFD(_channel_handle, fd_data, size, 0);
            }
            for(auto i{0U}; i < result; ++i)
            {
                frame.setFrameId(GET_ID(fd_data[i].frame.can_id));
                frame.setPayload(QByteArray(reinterpret_cast<char*>(fd_data[i].frame.data), int(fd_data[i].frame.len)));
                frame.setFlexibleDataRateFormat(true);
                frame.setBitrateSwitch(fd_data[i].frame.flags & CANFD_BRS);
                frame.setTimeStamp(QCanBusFrame::TimeStamp::fromMicroSeconds(fd_data[i].timestamp));
                frame.setExtendedFrameFormat(IS_EFF(fd_data[i].frame.can_id));
                if(IS_ERR(fd_data[i].frame.can_id))
                {
                    frame.setFrameType(QCanBusFrame::ErrorFrame);
                }
                else
                {
                    frame.setFrameType(QCanBusFrame::DataFrame);
                }
                frames.append(frame);
            }
        }};

        auto size{0U};
        while((size = ZCAN_GetReceiveNum(_channel_handle, 0)))
        {
            receive_frame(size);
            if(frames.size() >= 256)
            {
                q->enqueueReceivedFrames(frames);
                frames.clear();
            }
        }
        while(_fd_enabled && (size = ZCAN_GetReceiveNum(_channel_handle, 1)))
        {
            receive_frame_fd(size);
            if(frames.size() >= 256)
            {
                q->enqueueReceivedFrames(frames);
                frames.clear();
            }
        }
        if(!frames.isEmpty())
        {
            q->enqueueReceivedFrames(frames);
            frames.clear();
        }
    }
    else
    {
        _read_timer.stop();
    }
}

void ZlgCanBackendPrivate::resetController()
{
    Q_Q(ZlgCanBackend);

    static auto& ZCAN_ResetCAN{zlg::Loader::instance().ZCAN_ResetCAN};
    static auto& ZCAN_StartCAN{zlg::Loader::instance().ZCAN_StartCAN};

    if(_channel_handle)
    {
        auto result{false};
        {
            const QMutexLocker locker{&_mutex};
            result = (STATUS_OK == ZCAN_ResetCAN(_channel_handle)) && (STATUS_OK == ZCAN_StartCAN(_channel_handle));
        }

        if(result)
        {
            _read_timer.start();
            _write_timer.start();
        }
        else
        {
            auto& error_string{systemErrorString()};
            qCWarning(QT_CANBUS_PLUGINS_ZLGCAN, "Cannot perform hardware reset: %ls", qUtf16Printable(error_string));
            q->setError(error_string, QCanBusDevice::CanBusError::ConfigurationError);
        }
    }
}

QCanBusDevice::CanBusStatus ZlgCanBackendPrivate::busStatus()
{
    static auto& ZCAN_ReadChannelErrInfo{zlg::Loader::instance().ZCAN_ReadChannelErrInfo};

    if(_channel_handle)
    {
        ZCAN_CHANNEL_ERR_INFO error_info{};
        auto result{false};
        {
            const QMutexLocker locker{&_mutex};
            result = STATUS_OK == ZCAN_ReadChannelErrInfo(_channel_handle, &error_info);
        }
        if(result)
        {
            if((ZCAN_ERROR_CAN_ERRALARM | ZCAN_ERROR_CAN_LOSE) & error_info.error_code)
            {
                return QCanBusDevice::CanBusStatus::Warning;
            }
            else if((ZCAN_ERROR_CAN_PASSIVE | ZCAN_ERROR_CAN_BUSERR) & error_info.error_code)
            {
                return QCanBusDevice::CanBusStatus::Error;
            }
            else if(ZCAN_ERROR_CAN_BUSOFF & error_info.error_code)
            {
                return QCanBusDevice::CanBusStatus::BusOff;
            }
            else
            {
                return QCanBusDevice::CanBusStatus::Good;
            }

            if(ZCAN_ERROR_DEVICENOTEXIST & error_info.error_code)
            {
                return QCanBusDevice::CanBusStatus::Unknown;
            }
        }
    }
    qCWarning(QT_CANBUS_PLUGINS_ZLGCAN, "Unknown CAN bus status.");
    return QCanBusDevice::CanBusStatus::Unknown;
}

bool ZlgCanBackendPrivate::setConfigurations(int order)
{
    Q_Q(ZlgCanBackend);
    Q_UNUSED(q)

    static auto& ZCAN_SetValue{zlg::Loader::instance().ZCAN_SetValue};

    class IPropertyManager
    {
        Q_DISABLE_COPY(IPropertyManager)

    public:
        explicit IPropertyManager(void* handle)
        {
            static auto& GetIProperty{zlg::Loader::instance().GetIProperty};

            _iproperty = GetIProperty(handle);
        }

        virtual ~IPropertyManager()
        {
            static auto& ReleaseIProperty{zlg::Loader::instance().ReleaseIProperty};

            if(_iproperty)
            {
                ReleaseIProperty(_iproperty);
            }
        }

        operator bool()
        {
            return _iproperty;
        }

        IProperty* operator->()
        {
            return _iproperty;
        }

    private:
        IProperty* _iproperty{};
    };

    IPropertyManager iproperty_manager{_device_handle};

    auto set_value{[&](const QByteArray& path, const QByteArray& value, zlg::ConfigureFunction method) -> bool {
        if(!path.isEmpty() && !value.isEmpty())
        {
            if(zlg::ConfigureFunction::ZCAN_SETVALUE == method)
            {
                return STATUS_OK == ZCAN_SetValue(_device_handle, path, value);
            }

            else if(zlg::ConfigureFunction::IPROPERTY_SETVALUE == method)
            {
                return iproperty_manager->SetValue(path, value);
            }
            return true;
        }
        return false;
    }};

    if(_device_handle && _device_type)
    {
        QByteArray path{};
        QByteArray value{};
        const auto& device{zlg::get_devices()[_device_type]};
        const auto configure_order{static_cast<zlg::ConfigureOrder>(order)};
        for(auto iter{_configurations.cbegin()}; iter != _configurations.cend(); ++iter)
        {
            if(!iter.value().isValid() || !device.configurations.contains(iter.key()))
            {
                continue;
            }

            auto& configuration{device.configurations[iter.key()]};
            if(!configuration.configurable || configuration.order != configure_order)
            {
                continue;
            }

            path.clear();
            value.clear();
            switch(static_cast<int>(iter.key()))
            {
                case QCanBusDevice::RawFilterKey: break;
                case QCanBusDevice::ErrorFilterKey: break;
                case QCanBusDevice::LoopbackKey: break;
                case QCanBusDevice::ReceiveOwnKey: break;
                case QCanBusDevice::BitRateKey:
                {
                    auto bitrate{iter.value().toUInt()};
                    QString str{QString("%1/baud_rate")};
                    if(!device.bitrate.contains(bitrate))
                    {
                        str = QString("%1/baud_rate_custom");
                    }
                    else if(_fd_enabled)
                    {
                        str = QString("%1/canfd_abit_baud_rate");
                    }
                    path = str.arg(_channel_index).toLatin1();
                    value = QByteArray::number(bitrate);
                    break;
                }
                case QCanBusDevice::CanFdKey:
                {
                    _fd_enabled = device.fd && iter.value().toBool();
                    break;
                }
                case QCanBusDevice::DataBitRateKey:
                {
                    auto data_bitrate{iter.value().toUInt()};
                    if(_fd_enabled && device.data_field_bitrate.contains(data_bitrate))
                    {
                        path = QString("%1/canfd_dbit_baud_rate").arg(_channel_index).toLatin1();
                        value = QByteArray::number(data_bitrate);
                    }
                    break;
                }
            }
            if(!path.isEmpty() && !value.isEmpty() && !set_value(path, value, configuration.function))
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

const QString& ZlgCanBackendPrivate::systemErrorString(int* error_code)
{
    static auto& ZCAN_ReadChannelErrInfo{zlg::Loader::instance().ZCAN_ReadChannelErrInfo};

    static QHash<unsigned int, QString> dict{
        {                             0,                "Unknown error"},
        {       ZCAN_ERROR_CAN_OVERFLOW, "CAN controller FIFO overflow"},
        {       ZCAN_ERROR_CAN_ERRALARM,         "CAN controller alarm"},
        {        ZCAN_ERROR_CAN_PASSIVE,       "CAN controller passive"},
        {           ZCAN_ERROR_CAN_LOSE,          "CAN controller lose"},
        {         ZCAN_ERROR_CAN_BUSERR,     "CAN controller bus error"},
        {         ZCAN_ERROR_CAN_BUSOFF,       "CAN controller bus off"},
        {ZCAN_ERROR_CAN_BUFFER_OVERFLOW,          "CAN buffer overflow"},
        {       ZCAN_ERROR_DEVICEOPENED,                "Device opened"},
        {         ZCAN_ERROR_DEVICEOPEN,            "Open device error"},
        {      ZCAN_ERROR_DEVICENOTOPEN,    "The buffer cannot be read"},
        {     ZCAN_ERROR_BUFFEROVERFLOW,              "Buffer overflow"},
        {     ZCAN_ERROR_DEVICENOTEXIST,             "Device not exist"},
        {      ZCAN_ERROR_LOADKERNELDLL,        "Load kernel dll error"},
        {          ZCAN_ERROR_CMDFAILED,            "Run command error"},
        {       ZCAN_ERROR_BUFFERCREATE,          "Create buffer error"},
    };

    ZCAN_CHANNEL_ERR_INFO info{};
    ::memset(&info, 0, sizeof(info));
    if(_channel_handle && STATUS_OK == ZCAN_ReadChannelErrInfo(_channel_handle, &info) && error_code)
    {
        *error_code = info.error_code;
    }
    if(dict.contains(info.error_code))
    {
        return dict[info.error_code];
    }
    return dict[0];
}

QT_END_NAMESPACE
