#include "atelier/atelier_vk_completed.h"
#include "atelier/atelier_vk_mutable.h"
using namespace Atelier;

result VkCompletedState::pre_surface_default_init()
{
    // Reserve an instance for us to add content into
    auto& out_instance = m_instances.emplace_back();

    // First we need to try and initialize this current default instance
    VkMutableInstanceCreateInfo instance_create;
    if (VkMutableInstanceCreateInfo::create_default(instance_create) != k_success) {
        Log::error("Failed to generate default instance create info");
        return -1;
    }
    if (out_instance.init_from_mutable_instance(instance_create) != k_success) {
        Log::error("Failed to create a instance from default create info");
        return -2;
    }

    // Now for each of the physical devices we found, create a default logical device
    m_devices.reserve(m_devices.size() + out_instance.m_physical_devices.size());
    for (auto& p_dev : out_instance.m_physical_devices) {
        // TODO: Save some time by deriving the physical device info in the mutable create info by using an
        // additional method for creating default from
        auto dev_info = VkMutableDeviceCreateInfo();
        if (VkMutableDeviceCreateInfo::create_default(dev_info, out_instance.m_handle, p_dev.m_handle) !=
            k_success) {
            Log::error("Failed to create default device info");
            return -6;
        }

        // create one logical device for this physical device
        auto& out_logical = m_devices.emplace_back();
        if (out_logical.init_from_mutable_device(dev_info) != k_success) {
            Log::error("Failed to get the logical device created");
            return -7;
        }
        out_logical.m_parent = &out_instance;
        out_logical.m_physical = &p_dev;
    }

    return k_success;
}

result VkCompletedState::shutdown()
{
    for (auto& inst : m_instances) {
        inst.shutdown(*this);
    }
    m_devices.clear();
    m_instances.clear();

    return k_success;
}
