#include "customdevice.h"
#include "confighelpers.h"
#include "customdevice_p.h"
#include <QVariant>
#include <QtCore/QQueue>
#include <iostream>

CustomDevice::CustomDevice()
    : d_ptr(new CustomDevicePrivate(this))
{
}

CustomDevice::CustomDevice(CustomDeviceCtx&& ctx)
    : d_ptr(new CustomDevicePrivate(this, std::move(ctx)))
{
}

CustomDevice::~CustomDevice() {}

bool CustomDevice::init()
{
    Q_D(CustomDevice);

    const auto& props = d->_props;
    const auto& backend = d->_backendProperty;
    const auto& iface = d->_interfaceProperty;

    // check if required properties are set. They always exists as are initialized in constructor
    if ((props.at(backend).toString().length() == 0) || (props.at(iface).toString().length() == 0)) {
        return d->_initialized;
    }

    QString errorString;

    if (d->_initialized) {
        d->_canDevice.clearCallbacks();
    }

    d->_initialized = false;

    QString b = props.at(backend).toString();
    QString i = props.at(iface).toString();
    if (d->_canDevice.init(b, i)) {
        d->_canDevice.setFramesWrittenCbk(std::bind(&CustomDevice::framesWritten, this, std::placeholders::_1));
        d->_canDevice.setFramesReceivedCbk(std::bind(&CustomDevice::framesReceived, this));
        d->_canDevice.setErrorOccurredCbk(std::bind(&CustomDevice::errorOccurred, this, std::placeholders::_1));

        for (auto& item : d->getDevConfig()) {
            d->_canDevice.setConfigurationParameter(item.first, item.second);
        }

        d->_initialized = true;
    }

    return d->_initialized;
}

void CustomDevice::sendFrame(const QCanBusFrame& frame)
{
    Q_D(CustomDevice);

    if (!d->_initialized) {
        return;
    }

    // Success will be reported in framesWritten signal.
    // Sending may be buffered. Keep correlation between sending results and frame/context
    d->_sendQueue.push_back(frame);

    d->_canDevice.writeFrame(frame);

    // writeFrame status will be handled by framesWritten and errorOccured slots
}

ComponentInterface::ComponentProperties CustomDevice::getSupportedProperties() const
{
    return d_ptr->_supportedProps;
}

void CustomDevice::framesReceived()
{
    Q_D(CustomDevice);

    if (!d->_initialized) {
        return;
    }

    while (static_cast<bool>(d->_canDevice.framesAvailable())) {
        const QCanBusFrame frame = d->_canDevice.readFrame();
        emit frameReceived(frame);
    }
}

void CustomDevice::framesWritten(qint64 cnt)
{
    Q_D(CustomDevice);

    while (cnt--) {
        if (!d->_sendQueue.isEmpty()) {
            auto sendItem = d->_sendQueue.takeFirst();
            emit frameSent(true, sendItem);
        } else {
            cds_warn("Send queue is empty!");
        }
    }
}

void CustomDevice::errorOccurred(int error)
{
    Q_D(CustomDevice);

    cds_warn("Error occurred. Send queue size {}", d->_sendQueue.count());

    if (error == QCanBusDevice::WriteError && !d->_sendQueue.isEmpty()) {
        auto sendItem = d->_sendQueue.takeFirst();
        emit frameSent(false, sendItem);
    }
}

void CustomDevice::setConfig(const QJsonObject& json)
{
    assert(d_ptr != nullptr);
    d_ptr->restoreConfiguration(json);
}

QJsonObject CustomDevice::getConfig() const
{
    QJsonObject config;

    d_ptr->saveSettings(config);

    return config;
}

void CustomDevice::setConfig(const QWidget& qobject)
{
    Q_D(CustomDevice);

    configHelpers::setQConfig(qobject, getSupportedProperties(), d->_props);
}

std::shared_ptr<QWidget> CustomDevice::getQConfig() const
{
    const Q_D(CustomDevice);

    return configHelpers::getQConfig(getSupportedProperties(), d->_props);
}

void CustomDevice::startSimulation()
{
    Q_D(CustomDevice);

    d->_simStarted = true;

    if (!d->_initialized) {
        cds_info("CustomDevice not initialized");
        return;
    }
    else{
        cds_info("CustomDevice initialazed");
    }

    if (!d->_canDevice.connectDevice()) {
        cds_error("Failed to connect device. Trying to init the device again...");

        // Cannelloni plugin fails to reconnect after initial disconnection.
        // This workaround reinit the device before reconnection
        // TODO: Findout why cannelloni plugin fails.
        if (init()) {
            cds_info("Re-init successful");

            if (!d->_canDevice.connectDevice()) {
                cds_error("Failed to re-connect device");
            } else {
                cds_info("Re-connection successful");
            }
        }
    }

    d->_sendQueue.clear();
}

void CustomDevice::stopSimulation()
{
    Q_D(CustomDevice);

    d->_simStarted = false;

    if (!d->_initialized) {
        cds_info("CustomDevice not initialized");
        return;
    }

    d->_canDevice.disconnectDevice();
}

QWidget* CustomDevice::mainWidget()
{
    // Component does not have main widget
    return nullptr;
}

bool CustomDevice::mainWidgetDocked() const
{
    // Widget does not exist. Return always true
    return true;
}

void CustomDevice::configChanged()
{
    Q_D(CustomDevice);

    init();

    if (d->_simStarted) {
        startSimulation();
    }
}

void CustomDevice::simBcastRcv(const QJsonObject& msg, const QVariant& param)
{
    Q_UNUSED(msg);
    Q_UNUSED(param);
}
