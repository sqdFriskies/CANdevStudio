#pragma once

#include "plugin_type.h"
#include "customdevicemodel.h"

using DevicePlugin = PluginBase<typestring_is("Device Layer"), 0xf7aa1b, 43>;

struct CustomDevicePlugin {
    using Model = CustomDeviceModel;
    static constexpr const char* name = "CustomDevice";
    using PluginType = DevicePlugin;
};

