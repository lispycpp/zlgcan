#ifndef ZLGCANBACKEND_P_H
#define ZLGCANBACKEND_P_H

#include "zlgcan/zlgcan.h"
#include "zlgcanbackend.h"

#include <windows.h>
#undef SendMessage
#undef ERROR

#include <QHash>
#include <QMutex>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <QTimerEvent>
#include <QVariant>
#include <zlgcan/zlgcan.h>

QT_BEGIN_NAMESPACE

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

    class Loader
    {
        Loader(const Loader&) = delete;
        Loader& operator=(const Loader&) = delete;
        Loader(Loader&&) = delete;
        Loader& operator=(Loader&&) = delete;

    public:
        static const Loader* instance();

    private:
        explicit Loader();
        ~Loader();

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

    const zlg::Device* device{};
    const zlg::Loader* dll{};
};

QT_END_NAMESPACE

#endif // ZLGCANBACKEND_P_H
