#pragma once

#include <QtCore/QtGlobal>
#include <QtSerialBus/QCanBusFrame>
#include <functional>

struct CustomDeviceInterface {
    virtual ~CustomDeviceInterface() {}

    typedef std::function<void(qint64)> framesWritten_t;
    typedef std::function<void()> framesReceived_t;
    typedef std::function<void(int)> errorOccurred_t;

    virtual void setFramesWrittenCbk(const framesWritten_t& cb) = 0;
    virtual void setFramesReceivedCbk(const framesReceived_t& cb) = 0;
    virtual void setErrorOccurredCbk(const errorOccurred_t& cb) = 0;

    virtual bool init(const QString& backend, const QString& iface) = 0;
    virtual bool writeFrame(const QCanBusFrame& frame) = 0;
    virtual bool connectDevice() = 0;
    virtual void disconnectDevice() = 0;
    virtual qint64 framesAvailable() = 0;
    virtual void clearCallbacks() = 0;
    virtual void setConfigurationParameter(int key, const QVariant& value) = 0;

    virtual QCanBusFrame readFrame() = 0;
    virtual void setParent(QObject* parent) = 0;
};

